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
	case Pyc::RETURN_CONST_A:
	{
		PycRef<ASTObject> t_ob = new ASTObject(code->getConst(operand));

		if ((t_ob->object().type() == PycObject::TYPE_TUPLE ||
			t_ob->object().type() == PycObject::TYPE_SMALL_TUPLE)) {
			ASTTuple::value_t astValues;
			PycSimpleSequence::value_t pycValues = t_ob->object().cast<PycTuple>()->values();
			size_t num_values = pycValues.size();
			for (size_t i = 0; i < num_values; i++) {
				astValues.push_back(new ASTObject(pycValues[i]));
			}
			PycRef<ASTNode> astTuple = new ASTTuple(astValues);
			stack.push(astTuple); 
		} else if (t_ob->object().type() == PycObject::TYPE_NONE) {
			stack.push(NULL);
		} else {
			stack.push(t_ob.cast<ASTNode>());
		}

		if (opcode == Pyc::RETURN_CONST_A) {
			curblock->append(new ASTReturn(pop_top()));
		}
	}
	break;
	case Pyc::LOAD_NAME_A:
	case Pyc::LOAD_GLOBAL_A:
	{
		if (opcode == Pyc::LOAD_GLOBAL_A && mod->verCompare(3, 11) >= 0) {
			// Loads the global named co_names[namei>>1] onto the stack.
			if (operand & 1) {
				/* Changed in version 3.11:
				If the low bit of "NAMEI" (operand) is set,
				then a NULL is pushed to the stack before the global variable. */
				stack.push(nullptr);
			}
			operand >>= 1;
		}
		stack.push(new ASTName(code->getName(operand)));
	}
	break;
	case Pyc::LOAD_FAST_A:
	case Pyc::LOAD_CLOSURE_A:
	case Pyc::LOAD_DEREF_A:
	{
		stack.push(new ASTName(code->getLocal(operand)));
	}
	break;
	case Pyc::STORE_NAME_A:
	case Pyc::STORE_FAST_A:
	case Pyc::STORE_DEREF_A:
	{
		PycRef<ASTNode> value = pop_top();

		PycRef<PycString> varname = (opcode == Pyc::STORE_NAME_A) ? code->getName(operand) :
			(opcode == Pyc::STORE_FAST_A) ? code->getLocal(operand) : code->getCellVar(mod, operand);
		PycRef<ASTNode> name = new ASTName(varname);

		curblock->append(new ASTStore(value, name));
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
	case Pyc::MAKE_FUNCTION_A:
	{
		PycRef<ASTNode> func_code_obj = pop_top();
		ASTFunction::annotMap_t annotMap;
		ASTFunction::defarg_t defaults, kwDefaults;

		/* Test for the qualified name of the function (at TOS) */
		int tos_type = func_code_obj.cast<ASTObject>()->object().type();
		if (tos_type != PycObject::TYPE_CODE &&
			tos_type != PycObject::TYPE_CODE2) {
			func_code_obj = pop_top();
		}

		if (operand & 0x8) {
			// 0x08 a tuple containing cells for free variables, making a closure
			// pop that tuple (we don't actually need it)
			pop_top();
		}
		if (operand & 0x4) {
			// 0x04 a tuple of strings containing parameters’ annotations
			// param names gets LOAD_CONST, and annotation is LOAD_NAME
			// def func(x: int, y: str) would create tuple like this: ('x', int, 'y', str)
			const ASTTuple::value_t annotTupleValues = pop_top().cast<ASTTuple>()->values();
			size_t annotTupleValues_size = annotTupleValues.size();
			for (size_t i = 0; i < annotTupleValues_size-1; i+=2) {
				PycRef<PycString> varname_str = annotTupleValues[i].cast<ASTObject>()->object().cast<PycString>();
				PycRef<ASTName> varname = new ASTName(varname_str);
				annotMap.emplace_back(std::move(varname), std::move(annotTupleValues[i + 1].cast<ASTName>()));
			}
		}
		if (operand & 0x2) {
			// 0x02 a dictionary of keyword - only parameters’ default values
			ASTConstMap::values_t kwDefaultsDict = pop_top().cast<ASTConstMap>()->values();
			kwDefaults.assign(kwDefaultsDict.begin(), kwDefaultsDict.end());
		}
		if (operand & 0x1) {
			// 0x01 a tuple of default values for positional-only and positional-or-keyword parameters in positional order
			ASTTuple::value_t defaultsTuple = pop_top().cast<ASTTuple>()->values();
			defaults.assign(defaultsTuple.begin(), defaultsTuple.end());
		}

		stack.push(new ASTFunction(func_code_obj, annotMap, defaults, kwDefaults, false));
	}
	break;
	case Pyc::KW_NAMES_A:
	{
		ASTKwNamesMap kwparamList;
		PycSimpleSequence::value_t keys = code->getConst(operand).cast<PycTuple>()->values();
		size_t num_keys = keys.size();
		for (size_t i = 0; i < num_keys; i++) {
			kwparamList.add(new ASTObject(keys[num_keys - i - 1]), pop_top());
		}
		stack.push(new ASTKwNamesMap(kwparamList));
	}
	break;
	case Pyc::PUSH_NULL:
	{
		stack.push(NULL);
	}
	break;
	case Pyc::CALL_A:
	{
		int argc = operand;
		ASTCall::kwparam_t kwparamList;
		ASTCall::pparam_t pparamList;

		if (stack.top().type() == ASTNode::NODE_KW_NAMES_MAP) {
			PycRef<ASTKwNamesMap> kwNamesMap = pop_top().cast<ASTKwNamesMap>();
			for (ASTKwNamesMap::map_t::const_iterator it = kwNamesMap->values().begin(); it != kwNamesMap->values().end(); it++) {
				kwparamList.push_front(std::make_pair(it->first, it->second));
				argc -= 1;
			}
		}
		for (int i = 0; i < argc; i++) {
			pparamList.push_front(pop_top());
		}
		PycRef<ASTNode> func = pop_top();
		if (opcode == Pyc::CALL_A && stack.top() == NULL) {
			pop_top(); // we don't need it
		}

		stack.push(new ASTCall(func, pparamList, kwparamList));
	}
	break;
	case Pyc::POP_TOP:
	{
		curblock->append(pop_top());
	}
	break;
	case Pyc::BUILD_TUPLE_A:
	{
		ASTTuple::value_t values;
		values.resize(operand);
		for (int i = 0; i < operand; i++) {
			values[operand - i - 1] = pop_top();
		}
		stack.push(new ASTTuple(values));
	}
	break;
	case Pyc::BUILD_CONST_KEY_MAP_A:
	{
		// Top of stack will be a tuple of keys.
		// Values will start at TOS - 1.
		PycRef<ASTNode> keys = pop_top();

		ASTConstMap::values_t values;
		values.reserve(operand);
		for (int i = 0; i < operand; ++i) {
			PycRef<ASTNode> value = pop_top();
			values.push_back(value);
		}
		// reverse values order
		std::reverse(values.begin(), values.end());
		stack.push(new ASTConstMap(keys, values));
	}
	case Pyc::NOP:
	case Pyc::RESUME_A:
	case Pyc::CACHE:
	case Pyc::COPY_FREE_VARS_A:
	case Pyc::MAKE_CELL_A:
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
