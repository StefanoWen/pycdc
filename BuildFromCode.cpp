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
	this->defblock = new ASTBlock(ASTBlock::BLK_MAIN);
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

PycRef<ASTNode> BuildFromCode::build()
{
	while (bc_i < bc_size-1)
	{
		this->debug_print();

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
	case Pyc::NOP:
	case Pyc::RESUME_A:
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
