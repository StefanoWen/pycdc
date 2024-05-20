#include "BuildFromCode.h"

BuildFromCode::BuildFromCode(PycRef<PycCode> param_code, PycModule* param_mod) :
	code(param_code),
	mod(param_mod),
	source(code->code()->value(), code->code()->length()),
	stack((mod->majorVer() == 1) ? 20 : code->stackSize()),
	opcode(0),
	operand(0),
	curpos(0),
	pos(0),
	bc_i(0),
	bc_i_skipped(false),
	unpack(0),
	else_pop(false),
	variable_annotations(false),
	cleanBuild(false),
	comprehension_counter(0),
	previous_depth(false)
{
	this->defblock = new ASTBlock(ASTBlock::BLK_MAIN, -1);
	this->defblock->init();
	this->curblock = this->defblock;
	this->blocks.push(this->defblock);

	std::vector<ExceptTableEntry> exceptTableVector = code->exceptTableVector();
	for (auto it = exceptTableVector.rbegin(); it != exceptTableVector.rend(); ++it)
	{
		exceptTableStack.push(*it);
	}
	
	while (!source.atEof())
	{
		curpos = pos;
		::bc_next(source, mod, opcode, operand, pos);

		bc.push_back(Instruction{ opcode, operand, curpos, pos });
	}
	bc.push_back(Instruction{ Pyc::STOP_CODE, 0, 0, 0 });
	bc_size = bc.size();
	this->bc_update();
}

BuildFromCode::~BuildFromCode()
{
}

bool BuildFromCode::getCleanBuild() const
{
	return this->cleanBuild;
}

void BuildFromCode::debug_print()
{
#if defined(BLOCK_DEBUG) || defined(STACK_DEBUG)
	fprintf(stderr, "%-7d", curpos);
#ifdef STACK_DEBUG
	fprintf(stderr, "%-5d", (unsigned int)stack_hist.size() + 1);
#endif
#ifdef BLOCK_DEBUG
	for (unsigned int i = 0; i < blocks.size(); i++)
		fprintf(stderr, "    ");
	fprintf(stderr, "%s (%d)", curblock->type_str(), curblock->end());
#endif
	fprintf(stderr, "\n");
#endif
}

void BuildFromCode::bc_set(size_t new_bc_i)
{
	if (new_bc_i < bc_size)
	{
		bc_i = new_bc_i;
		this->bc_update();
	}
}

void BuildFromCode::bc_next()
{
	this->bc_set(bc_i + 1);
}

void BuildFromCode::bc_update()
{
	opcode = bc[bc_i].opcode;
	operand = bc[bc_i].operand;
	curpos = bc[bc_i].curpos;
	pos = bc[bc_i].pos;
}

void BuildFromCode::checker()
{
	while (curblock->end() == curpos) {
		pop_append_top_block();
	}
}

PycRef<ASTNode> BuildFromCode::build()
{
	while (bc_i < bc_size-1)
	{
		this->debug_print();
		this->checker();
		try {
			this->switchOpcode();
		}
		catch (UnsupportedOpcodeException&) {
			return new ASTNodeList(defblock->nodes());
		}
		this->bc_next();
#ifdef BLOCK_DEBUG
		if (bc_i_skipped)
		{
			bc_i_skipped = false;
			fprintf(stderr, "** Skipped **\n");
		}
#endif
	}

	if (stack_hist.size()) {
		fputs("Warning: Stack history is not empty!\n", stderr);

		while (stack_hist.size()) {
			stack_hist.pop();
		}
	}

	if (blocks.size() > 1) {
		fputs("Warning: block stack is not empty!\n", stderr);

		while (blocks.size() > 1) {
			PycRef<ASTBlock> tmp = blocks.top();
			blocks.pop();

			blocks.top()->append(tmp.cast<ASTNode>());
		}
	}

	cleanBuild = true;
	return new ASTNodeList(defblock->nodes());
}

void BuildFromCode::switchOpcode()
{
	switch (opcode) {
	case Pyc::LOAD_CONST_A:
	{
		PycRef<ASTObject> t_ob = new ASTObject(code->getConst(operand));

		if ((t_ob->object().type() == PycObject::TYPE_TUPLE ||
			t_ob->object().type() == PycObject::TYPE_SMALL_TUPLE)) {
			ASTTuple::value_t values;
			stack.push(new ASTTuple(values));
		}
		else if (t_ob->object().type() == PycObject::TYPE_NONE) {
			stack.push(NULL);
		}
		else {
			stack.push(t_ob.cast<ASTNode>());
		}
	}
	break;
	case Pyc::STORE_NAME_A:
	{
		PycRef<ASTNode> value = pop_top();

		PycRef<PycString> varname = code->getName(operand);
		PycRef<ASTNode> name = new ASTName(varname);

		curblock->append(new ASTStore(value, name));
	}
	break;
	case Pyc::LOAD_NAME_A:
	{
		stack.push(new ASTName(code->getName(operand)));
	}
	break;
	case Pyc::COMPARE_OP_A:
	{
		PycRef<ASTNode> right = pop_top();
		PycRef<ASTNode> left = pop_top();
		//         changed under GH-100923
		auto arg = (mod->verCompare(3, 12) >= 0) ? operand >> 4 : operand;
		stack.push(new ASTCompare(left, right, arg));
	}
	break;
	case Pyc::POP_JUMP_IF_FALSE_A:
	{
		PycRef<ASTNode> cond = stack.top();
		PycRef<ASTCondBlock> condBlk;

		/* "Jump if true" means "Jump if not false" */
		bool neg = opcode == Pyc::POP_JUMP_IF_TRUE_A;

		int offs = operand;
		if (mod->verCompare(3, 10) >= 0)
			offs *= sizeof(uint16_t); // // BPO-27129
		if (mod->verCompare(3, 12) >= 0) {
			/* Offset is relative in these cases */
			offs += pos;
		}

		if (curblock->blktype() == ASTBlock::BLK_IF
			&& (curblock->end() == offs || curblock->end() == pos))
		{
			PycRef<ASTCondBlock> prevCondBlk = curblock.cast<ASTCondBlock>();
			pop_top_block();
			PycRef<ASTNode> prevCond = prevCondBlk.cast<ASTCondBlock>()->cond();
			bool prevCondNeg = prevCondBlk.cast<ASTCondBlock>()->negative();
			ASTBinary::BinOp binType = (prevCondBlk->end() == offs) ? ASTBinary::BIN_LOG_AND : ASTBinary::BIN_LOG_OR;
			if ((prevCondBlk->end() == offs && prevCondNeg) ||
				(prevCondBlk->end() == pos && !prevCondNeg)) {
				// if not prev_cond and cond
				prevCond = new ASTUnary(prevCond, ASTUnary::UN_NOT);
			}
			cond = new ASTBinary(prevCond, cond, binType);
		}

		condBlk = new ASTCondBlock(ASTBlock::BLK_IF, offs, cond, neg);
		push_block(condBlk.cast<ASTBlock>());
	}
	break;
	case Pyc::JUMP_FORWARD_A:
	{
		int offs = operand;
		if (mod->verCompare(3, 10) >= 0)
			offs *= sizeof(uint16_t); // // BPO-27129
		int target = offs + pos;

		if (curblock->blktype() == ASTBlock::BLK_IF && curblock->end() == pos) {
			pop_append_top_block();
			PycRef<ASTBlock> elseBlk = new ASTBlock(ASTBlock::BLK_ELSE, target);
			push_block(elseBlk);
		}
	}
	break;
	case Pyc::NOP:
	case Pyc::RESUME_A:
	case Pyc::CACHE:
	case Pyc::PUSH_NULL:
	{
		// no-operation opcode
	}
	break;
	default:
		fprintf(stderr, "Unsupported opcode: %s\n", Pyc::OpcodeName(opcode & 0xFF));
		cleanBuild = false;
		throw UnsupportedOpcodeException();
	}
}

PycRef<ASTNode> BuildFromCode::pop_top()
{
	PycRef<ASTNode> tos = stack.top();
	stack.pop();
	return tos;
}

void BuildFromCode::pop_append_top_block()
{
	blocks.pop();
	blocks.top()->append(curblock.cast<ASTNode>());
	curblock = blocks.top();
}

PycRef<ASTBlock> BuildFromCode::pop_top_block()
{
	PycRef<ASTBlock> top_block = blocks.top();
	blocks.pop();
	curblock = blocks.top();
	return top_block;
}

void BuildFromCode::push_block(PycRef<ASTBlock> newBlk)
{
	blocks.push(newBlk);
	curblock = blocks.top();
}
