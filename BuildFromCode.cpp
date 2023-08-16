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

void BuildFromCode::print_blocks()
{
	for (unsigned int i = 0; i < blocks.size(); i++)
		fprintf(stderr, "    ");
	fprintf(stderr, "%s (%d)", curblock->type_str(), curblock->end());
}

void BuildFromCode::debug_print()
{
#if defined(BLOCK_DEBUG) || defined(STACK_DEBUG)
	fprintf(stderr, "%-7d", curpos);
#ifdef STACK_DEBUG
	fprintf(stderr, "%-5d", (unsigned int)stack_hist.size() + 1);
#endif
#ifdef BLOCK_DEBUG
	this->print_blocks();
#endif
	fprintf(stderr, "\n");
#endif
}

void BuildFromCode::bc_set_print_skipped_blocks(size_t new_bc_i)
{
	bc_i++;
	this->bc_update();
	while (bc_i < new_bc_i)
	{
		fprintf(stderr, "%-7d", curpos);
		this->print_blocks();
		fprintf(stderr, " ** Skipped **");
		fprintf(stderr, "\n");

		bc_i++;
		this->bc_update();
	}
}

void BuildFromCode::bc_set(size_t new_bc_i)
{
#ifdef BLOCK_DEBUG
	this->bc_set_print_skipped_blocks(new_bc_i);
#else
	bc_i = new_bc_i;
	this->bc_update();
#endif
}

void BuildFromCode::bc_next()
{
	if (bc_i < bc_size)
	{
		this->bc_set(bc_i + 1);
	}
}

void BuildFromCode::bc_update()
{
	opcode = bc[bc_i].opcode;
	operand = bc[bc_i].operand;
	curpos = bc[bc_i].curpos;
	pos = bc[bc_i].pos;
}

void BuildFromCode::append_to_chain_store(const PycRef<ASTNode>& chainStore, PycRef<ASTNode> item)
{
	stack.pop();    // ignore identical source object.
	chainStore.cast<ASTChainStore>()->append(item);
	if (stack.top().type() == PycObject::TYPE_NULL) {
		curblock->append(chainStore);
	}
	else {
		stack.push(chainStore);
	}
}

// shortcut for all top/pop calls
PycRef<ASTNode> BuildFromCode::StackPopTop()
{
	const auto node{ stack.top() };
	stack.pop();
	return node;
}

/* compiler generates very, VERY similar byte code for if/else statement block and if-expression
 *  statement
 *      if a: b = 1
 *      else: b = 2
 *  expression:
 *      b = 1 if a else 2
 *  (see for instance https://stackoverflow.com/a/52202007)
 *  here, try to guess if just finished else statement is part of if-expression (ternary operator)
 *  if it is, remove statements from the block and put a ternary node on top of stack
 */
void BuildFromCode::checkIfExpr()
{
	if (stack.empty())
		return;
	if (curblock->nodes().size() < 2)
		return;
	auto rit = curblock->nodes().crbegin();
	// the last is "else" block, the one before should be "if" (could be "for", ...)
	if ((*rit)->type() != ASTNode::NODE_BLOCK ||
		(*rit).cast<ASTBlock>()->blktype() != ASTBlock::BLK_ELSE)
		return;
	++rit;
	if ((*rit)->type() != ASTNode::NODE_BLOCK ||
		(*rit).cast<ASTBlock>()->blktype() != ASTBlock::BLK_IF)
		return;
	auto else_expr = this->StackPopTop();
	curblock->removeLast();
	auto if_block = curblock->nodes().back();
	auto if_expr = this->StackPopTop();
	curblock->removeLast();
	stack.push(new ASTTernary(std::move(if_block), std::move(if_expr), std::move(else_expr)));
}

void BuildFromCode::binary_or_inplace()
{
	ASTBinary::BinOp op = ASTBinary::from_opcode(opcode);
	if (op == ASTBinary::BIN_INVALID)
		throw std::runtime_error("Unhandled opcode from ASTBinary::from_opcode");
	PycRef<ASTNode> right = stack.top();
	stack.pop();
	PycRef<ASTNode> left = stack.top();
	stack.pop();
	stack.push(new ASTBinary(left, right, op));
}

void BuildFromCode::exceptionsChecker()
{
	if (mod->verCompare(3, 11) >= 0 && !exceptTableStack.empty())
	{
		if (curblock->blktype() == ASTBlock::BLK_TRY && curblock->end() == curpos)
		{
			this->pop_try();

			/*
			if (this->isOpcodeReturnAfterN(1))
			{
				this->convert_try_finally_to_try_except();
				this->add_except_block(0);
			}
			else
			{
				this->add_finally_no_op_block(curblock.cast<ASTTryFinallyBlock>()->getFinallyStart());
			}
			*/
		}

		int start = exceptTableStack.top().start;
		// start of some block
		if (start == curpos)
		{
			int length = exceptTableStack.top().length;
			int target = exceptTableStack.top().target;
			bool depth = exceptTableStack.top().depth;

			exceptTableStack.pop();
			if (depth)
			{

				// finally / except
				
			}
			else if (previous_depth)
			{
				// else
				if (start != length)
				{
					this->add_else_block(curpos + length);
				}
			}
			else
			{
				// try
				this->add_try_finally_block(target, false);
				this->add_try_block(length);
			}
		}
	}

	if (curblock->blktype() == ASTBlock::BLK_TRY_FINALLY)
	{
		if (curblock->inited())
		{
			this->add_finally_block();
		}
		else
		{
			if (opcode == Pyc::BEGIN_FINALLY)
			{
				this->add_finally_block();
			}
			else
			{
				bool is_finally = true;
				if (opcode == Pyc::JUMP_FORWARD_A)
				{
					is_finally = false;
					this->convert_try_finally_to_try_except();
				}
				else
				{
					if(this->isOpcodeReturnAfterN(1))
					{
						is_finally = false;
						this->convert_try_finally_to_try_except();
						this->add_except_block(0);
					}
				}
				if (is_finally)
				{
					this->add_finally_no_op_block(curblock.cast<ASTTryFinallyBlock>()->getFinallyStart());
				}
			}
		}
	}
	else if (curblock->blktype() == ASTBlock::BLK_TRY_EXCEPT)
	{
		if (curblock.cast<ASTTryExceptBlock>()->getElseStart() == curpos)
		{
			this->pop_try_except_or_try_finally_block();
		}
	}
	else if (curblock->blktype() == ASTBlock::BLK_NO_OP_FINALLY && curblock->end() == pos)
	{
		if (opcode == Pyc::JUMP_FORWARD_A)
		{
			this->end_finally();
		}
		else if (this->isOpcodeReturnAfterN(1))
		{
			this->end_finally();
			this->add_finally_block();
		}
	}
	else if (curblock->blktype() == ASTBlock::BLK_EXCEPT)
	{
		if (curblock.cast<ASTExceptBlock>()->elseStart() == curpos)
		{
			// pop the except we added in the last JUMP_FORWARD
			stack = stack_hist.top();
			stack_hist.pop();

			blocks.pop();
			curblock = blocks.top();

			if (curblock.cast<ASTTryExceptBlock>()->isElseStartBelowElseEnd())
			{
				this->add_else_block(curblock.cast<ASTTryExceptBlock>()->getElseEnd());
			}
			else
			{
				this->pop_try_except_or_try_finally_block();
			}
		}
	}
	else if (curblock->blktype() == ASTBlock::BLK_ELSE && curblock->end() == curpos)
	{
		stack = stack_hist.top();
		stack_hist.pop();

		blocks.pop();
		blocks.top()->append(curblock.cast<ASTNode>());
		curblock = blocks.top();

		this->pop_try_except_or_try_finally_block();
	}
}

void BuildFromCode::checker()
{
	this->exceptionsChecker();

	if (comprehension_counter == 2 && opcode != Pyc::PRECALL_A)
	{
		if (opcode == Pyc::PRECALL_A)
		{
			comprehension_counter = 1;
		}
		else
		{
			// reset counter to 3 
			// because next opcode is not precall
			comprehension_counter = 3;
		}
	}

	if (else_pop
		&& opcode != Pyc::JUMP_FORWARD_A
		&& opcode != Pyc::JUMP_IF_FALSE_A
		&& opcode != Pyc::JUMP_IF_TRUE_A
		&& opcode != Pyc::JUMP_IF_FALSE_OR_POP_A
		&& opcode != Pyc::JUMP_IF_TRUE_OR_POP_A
		&& opcode != Pyc::POP_JUMP_IF_FALSE_A
		&& opcode != Pyc::POP_JUMP_IF_TRUE_A
		&& opcode != Pyc::JUMP_IF_NOT_EXC_MATCH_A
		&& opcode != Pyc::POP_JUMP_FORWARD_IF_FALSE_A
		&& opcode != Pyc::POP_JUMP_FORWARD_IF_TRUE_A
		&& opcode != Pyc::POP_BLOCK)
	{
		else_pop = false;

		PycRef<ASTBlock> prev = curblock;
		while (prev->end() < pos
			&& prev->blktype() != ASTBlock::BLK_MAIN)
		{
			/*
			if (prev->blktype() != ASTBlock::BLK_CONTAINER)
			{
				if (prev->end() == 0)
				{
					break;
				}

				// We want to keep the stack the same, but we need to pop
				// a level off the history.
				// stack = stack_hist.top();
				if (!stack_hist.empty())
					stack_hist.pop();
			}
			*/
			blocks.pop();

			if (blocks.empty())
				break;

			curblock = blocks.top();
			curblock->append(prev.cast<ASTNode>());

			prev = curblock;

			checkIfExpr();
		}
	}
}

PycRef<ASTNode> BuildFromCode::build()
{
	while (bc_i < bc_size-1)
	{
		if (bc_i == 61)
		{
			printf("");
		}
		this->checker();
		
		this->debug_print();

		try {
			this->switchOpcode();
		}
		catch (UnsupportedOpcodeException&) {
			return new ASTNodeList(defblock->nodes());
		}

		else_pop = ((curblock->blktype() == ASTBlock::BLK_ELSE)
			|| (curblock->blktype() == ASTBlock::BLK_IF)
			|| (curblock->blktype() == ASTBlock::BLK_ELIF))
			&& (curblock->end() == pos);

		this->bc_next();
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
	case Pyc::BINARY_OP_A:
	{
		ASTBinary::BinOp op = ASTBinary::from_binary_op(operand);
		if (op == ASTBinary::BIN_INVALID)
			fprintf(stderr, "Unsupported `BINARY_OP` operand value: %d\n", operand);
		PycRef<ASTNode> right = stack.top();
		stack.pop();
		PycRef<ASTNode> left = stack.top();
		stack.pop();
		stack.push(new ASTBinary(left, right, op));
	}
	break;
	case Pyc::BINARY_ADD:
	case Pyc::BINARY_AND:
	case Pyc::BINARY_DIVIDE:
	case Pyc::BINARY_FLOOR_DIVIDE:
	case Pyc::BINARY_LSHIFT:
	case Pyc::BINARY_MODULO:
	case Pyc::BINARY_MULTIPLY:
	case Pyc::BINARY_OR:
	case Pyc::BINARY_POWER:
	case Pyc::BINARY_RSHIFT:
	case Pyc::BINARY_SUBTRACT:
	case Pyc::BINARY_TRUE_DIVIDE:
	case Pyc::BINARY_XOR:
	case Pyc::BINARY_MATRIX_MULTIPLY:
	case Pyc::INPLACE_ADD:
	case Pyc::INPLACE_AND:
	case Pyc::INPLACE_DIVIDE:
	case Pyc::INPLACE_FLOOR_DIVIDE:
	case Pyc::INPLACE_LSHIFT:
	case Pyc::INPLACE_MODULO:
	case Pyc::INPLACE_MULTIPLY:
	case Pyc::INPLACE_OR:
	case Pyc::INPLACE_POWER:
	case Pyc::INPLACE_RSHIFT:
	case Pyc::INPLACE_SUBTRACT:
	case Pyc::INPLACE_TRUE_DIVIDE:
	case Pyc::INPLACE_XOR:
	case Pyc::INPLACE_MATRIX_MULTIPLY:
	{
		ASTBinary::BinOp op = ASTBinary::from_opcode(opcode);
		if (op == ASTBinary::BIN_INVALID)
			throw std::runtime_error("Unhandled opcode from ASTBinary::from_opcode");
		PycRef<ASTNode> right = stack.top();
		stack.pop();
		PycRef<ASTNode> left = stack.top();
		stack.pop();
		stack.push(new ASTBinary(left, right, op));
	}
	break;
	case Pyc::BINARY_SUBSCR:
	{
		PycRef<ASTNode> subscr = stack.top();
		stack.pop();
		PycRef<ASTNode> src = stack.top();
		stack.pop();
		stack.push(new ASTSubscr(src, subscr));
	}
	break;
	case Pyc::BREAK_LOOP:
		curblock->append(new ASTKeyword(ASTKeyword::KW_BREAK));
		break;
	case Pyc::BUILD_CLASS:
	{
		PycRef<ASTNode> class_code = stack.top();
		stack.pop();
		PycRef<ASTNode> bases = stack.top();
		stack.pop();
		PycRef<ASTNode> name = stack.top();
		stack.pop();
		stack.push(new ASTClass(class_code, bases, name));
	}
	break;
	case Pyc::BUILD_FUNCTION:
	{
		PycRef<ASTNode> fun_code = stack.top();
		stack.pop();

		stack.push(new ASTFunction(fun_code, {}, {}));
	}
	break;
	case Pyc::BUILD_LIST_UNPACK_A:
	case Pyc::BUILD_LIST_A:
	{
		ASTList::value_t values;
		for (int i = 0; i < operand; i++) {
			values.push_front(stack.top());
			stack.pop();
		}
		PycRef<ASTList> new_list = new ASTList(values);
		if (opcode == Pyc::BUILD_LIST_UNPACK_A)
			new_list->setIsUnpack(true);
		stack.push(new_list.cast<ASTNode>());
	}
	break;
	case Pyc::BUILD_SET_UNPACK_A:
	case Pyc::BUILD_SET_A:
	{
		ASTSet::value_t values;
		for (int i = 0; i < operand; i++) {
			values.push_front(stack.top());
			stack.pop();
		}
		PycRef<ASTSet> new_set = new ASTSet(values);
		if (opcode == Pyc::BUILD_SET_UNPACK_A)
			new_set->setIsUnpack(true);
		stack.push(new_set.cast<ASTNode>());
	}
	break;
	case Pyc::BUILD_MAP_UNPACK_WITH_CALL_A:
	{
		operand = operand & 0xff;
		// continue to BUILD_MAP_UNPACK (below)
	}
	/* Fall through */
	case Pyc::BUILD_MAP_UNPACK_A:
	{
		auto map = new ASTMapUnpack;
		for (int i = 0; i < operand; ++i) {
			PycRef<ASTNode> value = stack.top();
			stack.pop();
			map->add(value);
		}
		stack.push(map);
	}
	break;
	case Pyc::BUILD_MAP_A:
		if (mod->verCompare(3, 5) >= 0) {
			auto map = new ASTMap;
			for (int i = 0; i < operand; ++i) {
				PycRef<ASTNode> value = stack.top();
				stack.pop();
				PycRef<ASTNode> key = stack.top();
				stack.pop();
				map->addFront(key, value);
			}
			stack.push(map);
		}
		else {
			if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
				stack.pop();
			}
			stack.push(new ASTMap());
		}
		break;
	case Pyc::BUILD_CONST_KEY_MAP_A:
	{
		// Top of stack will be a tuple of keys.
		// Values will start at TOS - 1.
		PycRef<ASTNode> keys = stack.top();
		stack.pop();

		ASTConstMap::values_t values;
		values.reserve(operand);
		for (int i = 0; i < operand; ++i) {
			PycRef<ASTNode> value = stack.top();
			stack.pop();
			values.push_back(value);
		}
		// reverse values order
		std::reverse(values.begin(), values.end());
		stack.push(new ASTConstMap(keys, values));
	}
	break;
	case Pyc::STORE_MAP:
	{
		PycRef<ASTNode> key = stack.top();
		stack.pop();
		PycRef<ASTNode> value = stack.top();
		stack.pop();
		PycRef<ASTMap> map = stack.top().cast<ASTMap>();
		map->addBack(key, value);
	}
	break;
	case Pyc::BUILD_SLICE_A:
	{
		if (operand == 2) {
			PycRef<ASTNode> end = stack.top();
			stack.pop();
			PycRef<ASTNode> start = stack.top();
			stack.pop();

			if (start.type() == ASTNode::NODE_OBJECT
				&& start.cast<ASTObject>()->object() == Pyc_None) {
				start = NULL;
			}

			if (end.type() == ASTNode::NODE_OBJECT
				&& end.cast<ASTObject>()->object() == Pyc_None) {
				end = NULL;
			}

			if (start == NULL && end == NULL) {
				stack.push(new ASTSlice(ASTSlice::SLICE0));
			}
			else if (start == NULL) {
				stack.push(new ASTSlice(ASTSlice::SLICE2, start, end));
			}
			else if (end == NULL) {
				stack.push(new ASTSlice(ASTSlice::SLICE1, start, end));
			}
			else {
				stack.push(new ASTSlice(ASTSlice::SLICE3, start, end));
			}
		}
		else if (operand == 3) {
			PycRef<ASTNode> step = stack.top();
			stack.pop();
			PycRef<ASTNode> end = stack.top();
			stack.pop();
			PycRef<ASTNode> start = stack.top();
			stack.pop();

			if (start.type() == ASTNode::NODE_OBJECT
				&& start.cast<ASTObject>()->object() == Pyc_None) {
				start = NULL;
			}

			if (end.type() == ASTNode::NODE_OBJECT
				&& end.cast<ASTObject>()->object() == Pyc_None) {
				end = NULL;
			}

			if (step.type() == ASTNode::NODE_OBJECT
				&& step.cast<ASTObject>()->object() == Pyc_None) {
				step = NULL;
			}

			/* We have to do this as a slice where one side is another slice */
			/* [[a:b]:c] */

			if (start == NULL && end == NULL) {
				stack.push(new ASTSlice(ASTSlice::SLICE0));
			}
			else if (start == NULL) {
				stack.push(new ASTSlice(ASTSlice::SLICE2, start, end));
			}
			else if (end == NULL) {
				stack.push(new ASTSlice(ASTSlice::SLICE1, start, end));
			}
			else {
				stack.push(new ASTSlice(ASTSlice::SLICE3, start, end));
			}

			PycRef<ASTNode> lhs = stack.top();
			stack.pop();

			if (step == NULL) {
				stack.push(new ASTSlice(ASTSlice::SLICE1, lhs, step));
			}
			else {
				stack.push(new ASTSlice(ASTSlice::SLICE3, lhs, step));
			}
		}
	}
	break;
	case Pyc::BUILD_STRING_A:
	{
		// Nearly identical logic to BUILD_LIST
		ASTList::value_t values;
		for (int i = 0; i < operand; i++) {
			values.push_front(stack.top());
			stack.pop();
		}
		stack.push(new ASTJoinedStr(values));
	}
	break;
	case Pyc::BUILD_TUPLE_UNPACK_WITH_CALL_A:
	case Pyc::BUILD_TUPLE_UNPACK_A:
	case Pyc::BUILD_TUPLE_A:
	{
		ASTTuple::value_t values;
		values.resize(operand);
		for (int i = 0; i < operand; i++) {
			values[operand - i - 1] = stack.top();
			stack.pop();
		}
		PycRef<ASTTuple> new_tuple = new ASTTuple(values);
		if (opcode == Pyc::BUILD_TUPLE_UNPACK_A
			|| opcode == Pyc::BUILD_TUPLE_UNPACK_WITH_CALL_A)
			new_tuple->setIsUnpack(true);
		stack.push(new_tuple.cast<ASTNode>());
	}
	break;
	case Pyc::KW_NAMES_A:
	{

		int kwparams = code->getConst(operand).cast<PycTuple>()->size();
		ASTKwNamesMap kwparamList;
		std::vector<PycRef<PycObject>> keys = code->getConst(operand).cast<PycSimpleSequence>()->values();
		for (int i = 0; i < kwparams; i++) {
			kwparamList.add(new ASTObject(keys[kwparams - i - 1]), stack.top());
			stack.pop();
		}
		stack.push(new ASTKwNamesMap(kwparamList));
	}
	break;
	case Pyc::CALL_A:
	case Pyc::CALL_FUNCTION_A:
	{
		int kwparams = (operand & 0xFF00) >> 8;
		int pparams = (operand & 0xFF);
		ASTCall::kwparam_t kwparamList;
		ASTCall::pparam_t pparamList;

		if (comprehension_counter) {
			comprehension_counter = 0;
			pparams = 1;
		}

		/* Test for the load build class function */
		stack_hist.push(stack);
		int basecnt = 0;
		ASTTuple::value_t bases;
		bases.resize(basecnt);
		PycRef<ASTNode> TOS = stack.top();
		int TOS_type = TOS.type();
		// bases are NODE_NAME at TOS
		while (TOS_type == ASTNode::NODE_NAME) {
			bases.resize(basecnt + 1);
			bases[basecnt] = TOS;
			basecnt++;
			stack.pop();
			TOS = stack.top();
			TOS_type = TOS.type();
		}
		// qualified name is PycString at TOS
		PycRef<ASTNode> name = stack.top();
		stack.pop();
		PycRef<ASTNode> function = stack.top();
		stack.pop();
		PycRef<ASTNode> loadbuild = stack.top();
		stack.pop();
		int loadbuild_type = loadbuild.type();
		if (loadbuild_type == ASTNode::NODE_LOADBUILDCLASS) {
			PycRef<ASTNode> call = new ASTCall(function, pparamList, kwparamList);
			stack.push(new ASTClass(call, new ASTTuple(bases), name));
			stack_hist.pop();
			break;
		}
		else
		{
			stack = stack_hist.top();
			stack_hist.pop();
		}

		/*
		KW_NAMES(i)
			Stores a reference to co_consts[consti] into an internal variable for use by CALL.
			co_consts[consti] must be a tuple of strings.
			New in version 3.11.
		*/
		if (mod->verCompare(3, 11) >= 0) {
			PycRef<ASTNode> object_or_map = stack.top();
			if (object_or_map.type() == ASTNode::NODE_KW_NAMES_MAP) {
				stack.pop();
				PycRef<ASTKwNamesMap> kwparams_map = object_or_map.cast<ASTKwNamesMap>();
				for (ASTKwNamesMap::map_t::const_iterator it = kwparams_map->values().begin(); it != kwparams_map->values().end(); it++) {
					kwparamList.push_front(std::make_pair(it->first, it->second));
					pparams -= 1;
				}
			}
		}
		else {
			for (int i = 0; i < kwparams; i++) {
				PycRef<ASTNode> val = stack.top();
				stack.pop();
				PycRef<ASTNode> key = stack.top();
				stack.pop();
				kwparamList.push_front(std::make_pair(key, val));
			}
		}
		for (int i = 0; i < pparams; i++) {
			PycRef<ASTNode> param = stack.top();
			stack.pop();
			if (param.type() == ASTNode::NODE_FUNCTION) {
				PycRef<ASTNode> fun_code = param.cast<ASTFunction>()->code();
				PycRef<PycCode> code_src = fun_code.cast<ASTObject>()->object().cast<PycCode>();
				PycRef<PycString> function_name = code_src->name();
				if (function_name->isEqual("<lambda>")) {
					pparamList.push_front(param);
				}
				else {
					// Decorator used
					PycRef<ASTNode> decor_name = new ASTName(function_name);
					curblock->append(new ASTStore(param, decor_name));

					pparamList.push_front(decor_name);
				}
			}
			else {
				pparamList.push_front(param);
			}
		}
		PycRef<ASTNode> func = stack.top();
		stack.pop();
		if (opcode == Pyc::CALL_A && stack.top() == nullptr)
			stack.pop();

		stack.push(new ASTCall(func, pparamList, kwparamList));
	}
	break;
	case Pyc::CALL_FUNCTION_VAR_A:
	{
		PycRef<ASTNode> var;
		if (mod->verCompare(3, 5) < 0) {
			var = stack.top();
			stack.pop();
		}

		int kwparams = (operand & 0xFF00) >> 8;
		int pparams = (operand & 0xFF);
		ASTCall::kwparam_t kwparamList;
		ASTCall::pparam_t pparamList;

		for (int i = 0; i < kwparams; i++) {
			PycRef<ASTNode> val = stack.top();
			stack.pop();
			PycRef<ASTNode> key = stack.top();
			stack.pop();
			kwparamList.push_front(std::make_pair(key, val));
		}

		if (mod->verCompare(3, 5) >= 0) {
			var = stack.top();
			stack.pop();
		}

		for (int i = 0; i < pparams; i++) {
			pparamList.push_front(stack.top());
			stack.pop();
		}
		PycRef<ASTNode> func = stack.top();
		stack.pop();

		PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
		call.cast<ASTCall>()->setVar(var);
		stack.push(call);
	}
	break;
	case Pyc::CALL_FUNCTION_KW_A:
	{
		PycRef<ASTNode> kw;
		ASTCall::kwparam_t kwparamList;
		ASTCall::pparam_t pparamList;
		int pparams;

		if (mod->verCompare(3, 6) >= 0) {
			PycSimpleSequence::value_t kw_keys = stack.top().cast<ASTObject>()->object().cast<PycTuple>()->values();
			stack.pop();
			int kwparams = (int)kw_keys.size();
			pparams = operand - kwparams;

			for (int i = 0; i < kwparams; i++) {
				PycRef<ASTNode> value = stack.top();
				stack.pop();
				kwparamList.push_front(std::make_pair(new ASTObject(kw_keys[(size_t)kwparams - 1 - (size_t)i]), value));
			}
		}
		else {
			kw = stack.top();
			stack.pop();
			int kwparams = (operand & 0xFF00) >> 8;
			pparams = (operand & 0xFF);

			for (int i = 0; i < kwparams; i++) {
				PycRef<ASTNode> val = stack.top();
				stack.pop();
				PycRef<ASTNode> key = stack.top();
				stack.pop();
				kwparamList.push_front(std::make_pair(key, val));
			}
		}

		for (int i = 0; i < pparams; i++) {
			pparamList.push_front(stack.top());
			stack.pop();
		}
		PycRef<ASTNode> func = stack.top();
		stack.pop();

		PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
		if (mod->verCompare(3, 6) < 0) {
			call.cast<ASTCall>()->setKW(kw);
		}
		stack.push(call);
	}
	break;
	case Pyc::CALL_FUNCTION_VAR_KW_A:
	{
		PycRef<ASTNode> var;
		PycRef<ASTNode> kw = stack.top();
		stack.pop();

		if (mod->verCompare(3, 5) < 0) {
			var = stack.top();
			stack.pop();
		}

		int kwparams = (operand & 0xFF00) >> 8;
		int pparams = (operand & 0xFF);
		ASTCall::kwparam_t kwparamList;
		ASTCall::pparam_t pparamList;

		for (int i = 0; i < kwparams; i++) {
			PycRef<ASTNode> val = stack.top();
			stack.pop();
			PycRef<ASTNode> key = stack.top();
			stack.pop();
			kwparamList.push_front(std::make_pair(key, val));
		}

		if (mod->verCompare(3, 5) >= 0) {
			var = stack.top();
			stack.pop();
		}

		for (int i = 0; i < pparams; i++) {
			pparamList.push_front(stack.top());
			stack.pop();
		}

		PycRef<ASTNode> func = stack.top();
		stack.pop();

		PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
		call.cast<ASTCall>()->setKW(kw);
		call.cast<ASTCall>()->setVar(var);
		stack.push(call);
	}
	break;
	case Pyc::CALL_FUNCTION_EX_A:
	{
		ASTCall::kwparam_t kwparamList;
		bool isKwUnpack = false;
		PycRef<ASTNode> kw;

		// there is kw parameters (**a, **b) or regular (a=1,b=2) or both
		if (operand & 1) {
			kw = stack.top();
			stack.pop();
			if (kw.type() == ASTNode::NODE_MAP_UNPACK) {
				isKwUnpack = true;
			}
			else if (kw.type() == ASTNode::NODE_MAP) {
				ASTMap::map_t kw_map_values = kw.cast<ASTMap>()->values();
				for (ASTMap::map_t::iterator it = kw_map_values.begin(); it != kw_map_values.end(); it++) {
					kwparamList.push_back(std::make_pair(it->first, it->second));
				}
			}
			else if (kw.type() == ASTNode::NODE_CONST_MAP) {
				PycRef<ASTConstMap> kw_const_map = kw.cast<ASTConstMap>();
				PycSimpleSequence::value_t kw_keys = kw_const_map->keys().cast<ASTObject>()->object().cast<PycTuple>()->values();
				const ASTConstMap::values_t values = kw_const_map->values();
				int kwparams = (int)kw_keys.size();
				for (int i = 0; i < kwparams; i++) {
					kwparamList.push_back(std::make_pair(new ASTObject(kw_keys[i]), values[i]));
				}
			}
			else {
				isKwUnpack = true;
			}
		}

		ASTCall::pparam_t pparamList;
		bool isVarUnpack = false;
		PycRef<ASTNode> var = stack.top();
		stack.pop();
		if (var.type() == ASTNode::NODE_TUPLE) {
			PycRef<ASTTuple> params_tuple = var.cast<ASTTuple>();
			isVarUnpack = params_tuple->isUnpack();
			if (!isVarUnpack) {
				ASTTuple::value_t params = params_tuple->values();
				for (size_t i = 0; i < params.size(); i++) {
					pparamList.push_back(params[i]);
				}
			}
		}
		else {
			isVarUnpack = true;
		}

		PycRef<ASTNode> func = stack.top();
		stack.pop();

		PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
		if (isKwUnpack) {
			call.cast<ASTCall>()->setKW(kw);
		}
		if (isVarUnpack) {
			call.cast<ASTCall>()->setVar(var);
		}
		stack.push(call);
	}
	break;
	case Pyc::CALL_METHOD_A:
	{
		ASTCall::pparam_t pparamList;
		for (int i = 0; i < operand; i++) {
			PycRef<ASTNode> param = stack.top();
			stack.pop();
			if (param.type() == ASTNode::NODE_FUNCTION) {
				PycRef<ASTNode> fun_code = param.cast<ASTFunction>()->code();
				PycRef<PycCode> code_src = fun_code.cast<ASTObject>()->object().cast<PycCode>();
				PycRef<PycString> function_name = code_src->name();
				if (function_name->isEqual("<lambda>")) {
					pparamList.push_front(param);
				}
				else {
					// Decorator used
					PycRef<ASTNode> decor_name = new ASTName(function_name);
					curblock->append(new ASTStore(param, decor_name));

					pparamList.push_front(decor_name);
				}
			}
			else {
				pparamList.push_front(param);
			}
		}
		PycRef<ASTNode> func = stack.top();
		stack.pop();
		stack.push(new ASTCall(func, pparamList, ASTCall::kwparam_t()));
	}
	break;
	case Pyc::CONTINUE_LOOP_A:
		curblock->append(new ASTKeyword(ASTKeyword::KW_CONTINUE));
		break;
	case Pyc::COMPARE_OP_A:
	{
		PycRef<ASTNode> right = stack.top();
		stack.pop();
		PycRef<ASTNode> left = stack.top();
		stack.pop();
		stack.push(new ASTCompare(left, right, operand));
	}
	break;
	case Pyc::CONTAINS_OP_A:
	{
		PycRef<ASTNode> right = stack.top();
		stack.pop();
		PycRef<ASTNode> left = stack.top();
		stack.pop();
		// The operand will be 0 for 'in' and 1 for 'not in'.
		stack.push(new ASTCompare(left, right, operand ? ASTCompare::CMP_NOT_IN : ASTCompare::CMP_IN));
	}
	break;
	case Pyc::DELETE_ATTR_A:
	{
		PycRef<ASTNode> name = stack.top();
		stack.pop();
		curblock->append(new ASTDelete(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR)));
	}
	break;
	case Pyc::DELETE_GLOBAL_A:
		code->markGlobal(code->getName(operand));
		/* Fall through */
	case Pyc::DELETE_NAME_A:
	{
		PycRef<PycString> varname = code->getName(operand);

		if (varname->length() >= 2 && varname->value()[0] == '_'
			&& varname->value()[1] == '[') {
			/* Don't show deletes that are a result of list comps. */
			break;
		}

		PycRef<ASTNode> name = new ASTName(varname);
		curblock->append(new ASTDelete(name));
	}
	break;
	case Pyc::DELETE_FAST_A:
	{
		PycRef<ASTNode> name;

		if (mod->verCompare(1, 3) < 0)
			name = new ASTName(code->getName(operand));
		else
			name = new ASTName(code->getLocal(operand));

		if (name.cast<ASTName>()->name()->value()[0] == '_'
			&& name.cast<ASTName>()->name()->value()[1] == '[') {
			/* Don't show deletes that are a result of list comps. */
			break;
		}

		curblock->append(new ASTDelete(name));
	}
	break;
	case Pyc::DELETE_SLICE_0:
	{
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE0))));
	}
	break;
	case Pyc::DELETE_SLICE_1:
	{
		PycRef<ASTNode> upper = stack.top();
		stack.pop();
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE1, upper))));
	}
	break;
	case Pyc::DELETE_SLICE_2:
	{
		PycRef<ASTNode> lower = stack.top();
		stack.pop();
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE2, NULL, lower))));
	}
	break;
	case Pyc::DELETE_SLICE_3:
	{
		PycRef<ASTNode> lower = stack.top();
		stack.pop();
		PycRef<ASTNode> upper = stack.top();
		stack.pop();
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE3, upper, lower))));
	}
	break;
	case Pyc::DELETE_SUBSCR:
	{
		PycRef<ASTNode> key = stack.top();
		stack.pop();
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		curblock->append(new ASTDelete(new ASTSubscr(name, key)));
	}
	break;
	case Pyc::DUP_TOP:
	{
		if (stack.top().type() == PycObject::TYPE_NULL) {
			stack.push(stack.top());
		}
		else if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
			auto chainstore = stack.top();
			stack.pop();
			stack.push(stack.top());
			stack.push(chainstore);
		}
		else {
			stack.push(stack.top());
			ASTNodeList::list_t targets;
			stack.push(new ASTChainStore(targets, stack.top()));
		}
	}
	break;
	case Pyc::DUP_TOP_TWO:
	{
		PycRef<ASTNode> first = stack.top();
		stack.pop();
		PycRef<ASTNode> second = stack.top();

		stack.push(first);
		stack.push(second);
		stack.push(first);
	}
	break;
	case Pyc::DUP_TOPX_A:
	{
		std::stack<PycRef<ASTNode> > first;
		std::stack<PycRef<ASTNode> > second;

		for (int i = 0; i < operand; i++) {
			PycRef<ASTNode> node = stack.top();
			stack.pop();
			first.push(node);
			second.push(node);
		}

		while (first.size()) {
			stack.push(first.top());
			first.pop();
		}

		while (second.size()) {
			stack.push(second.top());
			second.pop();
		}
	}
	break;
	case Pyc::END_FINALLY:
	{
		this->end_finally();
	}
	break;
	case Pyc::EXEC_STMT:
	{
		if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
			stack.pop();
		}
		PycRef<ASTNode> loc = stack.top();
		stack.pop();
		PycRef<ASTNode> glob = stack.top();
		stack.pop();
		PycRef<ASTNode> stmt = stack.top();
		stack.pop();

		curblock->append(new ASTExec(stmt, glob, loc));
	}
	break;
	case Pyc::FOR_ITER_A:
	{
		PycRef<ASTNode> iter = stack.top(); // Iterable
		stack.pop();
		/* Pop it? Don't pop it? */

		int end;
		bool comprehension = false;

		// before 3.8, there is a SETUP_LOOP instruction with block start and end position,
		//    the operand is usually a jump to a POP_BLOCK instruction
		// after 3.8, block extent has to be inferred implicitly; the operand is a jump to a position after the for block
		if (mod->majorVer() == 3 && mod->minorVer() >= 8) {
			end = operand;
			if (mod->verCompare(3, 10) >= 0)
				end *= sizeof(uint16_t); // // BPO-27129
			end += pos;
			comprehension = strcmp(code->name()->value(), "<listcomp>") == 0;
		}
		else {
			PycRef<ASTBlock> top = blocks.top();
			end = top->end(); // block end position from SETUP_LOOP
			if (top->blktype() == ASTBlock::BLK_WHILE) {
				blocks.pop();
			}
			else {
				comprehension = true;
			}
		}

		PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_FOR, curpos, end, iter);
		forblk->setComprehension(comprehension);
		blocks.push(forblk.cast<ASTBlock>());
		curblock = blocks.top();

		stack.push(NULL);
	}
	break;
	case Pyc::FOR_LOOP_A:
	{
		PycRef<ASTNode> curidx = stack.top(); // Current index
		stack.pop();
		PycRef<ASTNode> iter = stack.top(); // Iterable
		stack.pop();

		bool comprehension = false;
		PycRef<ASTBlock> top = blocks.top();
		if (top->blktype() == ASTBlock::BLK_WHILE) {
			blocks.pop();
		}
		else {
			comprehension = true;
		}
		PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_FOR, curpos, top->end(), iter);
		forblk->setComprehension(comprehension);
		blocks.push(forblk.cast<ASTBlock>());
		curblock = blocks.top();

		/* Python Docs say:
			  "push the sequence, the incremented counter,
			   and the current item onto the stack." */
		stack.push(iter);
		stack.push(curidx);
		stack.push(NULL); // We can totally hack this >_>
	}
	break;
	case Pyc::GET_AITER:
	{
		// Logic similar to FOR_ITER_A
		PycRef<ASTNode> iter = stack.top(); // Iterable
		stack.pop();

		PycRef<ASTBlock> top = blocks.top();
		if (top->blktype() == ASTBlock::BLK_WHILE) {
			blocks.pop();
			PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_ASYNCFOR, curpos, top->end(), iter);
			blocks.push(forblk.cast<ASTBlock>());
			curblock = blocks.top();
			stack.push(nullptr);
		}
		else {
			fprintf(stderr, "Unsupported use of GET_AITER outside of SETUP_LOOP\n");
		}
	}
	break;
	case Pyc::GET_ANEXT:
		break;
	case Pyc::FORMAT_VALUE_A:
	{
		auto conversion_flag = static_cast<ASTFormattedValue::ConversionFlag>(operand);
		switch (conversion_flag) {
		case ASTFormattedValue::ConversionFlag::NONE:
		case ASTFormattedValue::ConversionFlag::STR:
		case ASTFormattedValue::ConversionFlag::REPR:
		case ASTFormattedValue::ConversionFlag::ASCII:
		{
			auto val = stack.top();
			stack.pop();
			stack.push(new ASTFormattedValue(val, conversion_flag, nullptr));
		}
		break;
		case ASTFormattedValue::ConversionFlag::FMTSPEC:
		{
			auto format_spec = stack.top();
			stack.pop();
			auto val = stack.top();
			stack.pop();
			stack.push(new ASTFormattedValue(val, conversion_flag, format_spec));
		}
		break;
		default:
			fprintf(stderr, "Unsupported FORMAT_VALUE_A conversion flag: %d\n", operand);
		}
	}
	break;
	case Pyc::GET_AWAITABLE:
	{
		PycRef<ASTNode> object = stack.top();
		stack.pop();
		stack.push(new ASTAwaitable(object));
	}
	break;
	case Pyc::GET_ITER:
	{
		if (comprehension_counter) {
			comprehension_counter = 2;
		}
	}
	break;
	case Pyc::GET_YIELD_FROM_ITER:
		/* We just entirely ignore this */
		break;
	case Pyc::IMPORT_NAME_A:
		if (mod->majorVer() == 1) {
			stack.push(new ASTImport(new ASTName(code->getName(operand)), NULL));
		}
		else {
			PycRef<ASTNode> fromlist = stack.top();
			stack.pop();
			if (mod->verCompare(2, 5) >= 0)
				stack.pop();    // Level -- we don't care
			stack.push(new ASTImport(new ASTName(code->getName(operand)), fromlist));
		}
		break;
	case Pyc::IMPORT_FROM_A:
		stack.push(new ASTName(code->getName(operand)));
		break;
	case Pyc::IMPORT_STAR:
	{
		PycRef<ASTNode> import = stack.top();
		stack.pop();
		curblock->append(new ASTStore(import, NULL));
	}
	break;
	case Pyc::IS_OP_A:
	{
		PycRef<ASTNode> right = stack.top();
		stack.pop();
		PycRef<ASTNode> left = stack.top();
		stack.pop();
		// The operand will be 0 for 'is' and 1 for 'is not'.
		stack.push(new ASTCompare(left, right, operand ? ASTCompare::CMP_IS_NOT : ASTCompare::CMP_IS));
	}
	break;
	case Pyc::JUMP_IF_FALSE_A:
	case Pyc::JUMP_IF_TRUE_A:
	case Pyc::JUMP_IF_FALSE_OR_POP_A:
	case Pyc::JUMP_IF_TRUE_OR_POP_A:
	case Pyc::POP_JUMP_IF_FALSE_A:
	case Pyc::POP_JUMP_IF_TRUE_A:
	case Pyc::JUMP_IF_NOT_EXC_MATCH_A:
	case Pyc::POP_JUMP_FORWARD_IF_FALSE_A:
	case Pyc::POP_JUMP_FORWARD_IF_TRUE_A:
	{
		PycRef<ASTNode> cond = stack.top();
		int popped = ASTCondBlock::UNINITED;

		if (opcode == Pyc::POP_JUMP_IF_FALSE_A
			|| opcode == Pyc::POP_JUMP_IF_TRUE_A
			|| opcode == Pyc::POP_JUMP_FORWARD_IF_FALSE_A
			|| opcode == Pyc::POP_JUMP_FORWARD_IF_TRUE_A
			|| opcode == Pyc::JUMP_IF_NOT_EXC_MATCH_A) {
			/* Pop condition before the jump */
			stack.pop();
			popped = ASTCondBlock::PRE_POPPED;
		}

		if (opcode == Pyc::JUMP_IF_FALSE_OR_POP_A
			|| opcode == Pyc::JUMP_IF_TRUE_OR_POP_A) {
			/* Pop condition only if condition is met */
			stack.pop();
			popped = ASTCondBlock::POPPED;
		}

		/* "Jump if true" means "Jump if not false" */
		bool neg = opcode == Pyc::JUMP_IF_TRUE_A
			|| opcode == Pyc::JUMP_IF_TRUE_OR_POP_A
			|| opcode == Pyc::POP_JUMP_IF_TRUE_A
			|| opcode == Pyc::POP_JUMP_FORWARD_IF_TRUE_A;

		int offs = operand;
		if (mod->verCompare(3, 10) >= 0)
			offs *= sizeof(uint16_t); // // BPO-27129
		if ((opcode == Pyc::JUMP_IF_FALSE_A
			|| opcode == Pyc::JUMP_IF_TRUE_A)
			|| mod->verCompare(3, 11) >= 0) {
			/* Offset is relative in these cases */
			offs += pos;
		}

		if ((cond.type() == ASTNode::NODE_COMPARE
			&& cond.cast<ASTCompare>()->op() == ASTCompare::CMP_EXCEPTION)

			|| opcode == Pyc::JUMP_IF_NOT_EXC_MATCH_A)
		{

			curblock->setEnd(pos + offs);
			curblock.cast<ASTExceptBlock>()->setExceptType((opcode == Pyc::JUMP_IF_NOT_EXC_MATCH_A) ? cond : cond.cast<ASTCompare>()->right());

			if (mod->verCompare(3, 11) < 0)
			{
				if (mod->verCompare(3, 0) == 0)
				{
					// POP_TOP
					bc_next();
					stack.pop();
				}
				// POP_TOP
				bc_next();
				stack.pop();
			}

			// POP_TOP / STORE_NAME
			bc_next();
			if (opcode == Pyc::POP_TOP)
			{
				stack.pop();
			}
			else if(opcode == Pyc::STORE_NAME_A)
			{
				PycRef<ASTNode> name = new ASTName(code->getName(operand));
				curblock.cast<ASTExceptBlock>()->setExceptAs(name);
			}

			break;
		}


		/* Store the current stack for the else statement(s) */
		stack_hist.push(stack);

		PycRef<ASTBlock> condBlock;
		if (curblock->blktype() == ASTBlock::BLK_ELSE
			&& curblock->size() == 0) {
			/* Collapse into elif statement */
			blocks.pop();
			stack = stack_hist.top();
			stack_hist.pop();
			condBlock = new ASTCondBlock(ASTBlock::BLK_ELIF, offs, cond, neg);
		}
		else if (curblock->size() == 0 && !curblock->inited()
			&& curblock->blktype() == ASTBlock::BLK_WHILE) {
			/* The condition for a while loop */
			PycRef<ASTBlock> top = blocks.top();
			blocks.pop();
			condBlock = new ASTCondBlock(top->blktype(), offs, cond, neg);

			/* We don't store the stack for loops! Pop it! */
			stack_hist.pop();
		}
		else if (curblock->size() == 0 && curblock->end() <= offs
			&& (curblock->blktype() == ASTBlock::BLK_IF
				|| curblock->blktype() == ASTBlock::BLK_ELIF
				|| curblock->blktype() == ASTBlock::BLK_WHILE)) {
			PycRef<ASTNode> newcond;
			PycRef<ASTCondBlock> top = curblock.cast<ASTCondBlock>();
			PycRef<ASTNode> cond1 = top->cond();
			blocks.pop();

			if (curblock->blktype() == ASTBlock::BLK_WHILE) {
				stack_hist.pop();
			}
			else {
				FastStack s_top = stack_hist.top();
				stack_hist.pop();
				stack_hist.pop();
				stack_hist.push(s_top);
			}

			if (curblock->end() == offs
				|| (curblock->end() == curpos && !top->negative())) {
				/* if blah and blah */
				newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_AND);
			}
			else {
				/* if blah or blah */
				newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_OR);
			}
			condBlock = new ASTCondBlock(top->blktype(), offs, newcond, neg);
		}
		else if (curblock->blktype() == ASTBlock::BLK_FOR
			&& curblock.cast<ASTIterBlock>()->isComprehension()
			&& mod->verCompare(2, 7) >= 0) {
			/* Comprehension condition */
			curblock.cast<ASTIterBlock>()->setCondition(cond);
			stack_hist.pop();
			// TODO: Handle older python versions, where condition
			// is laid out a little differently.
			break;
		}
		else {
			/* Plain old if statement */
			condBlock = new ASTCondBlock(ASTBlock::BLK_IF, offs, cond, neg);
		}

		if (popped)
			condBlock->init(popped);

		blocks.push(condBlock.cast<ASTBlock>());
		curblock = blocks.top();
	}
	break;
	case Pyc::JUMP_ABSOLUTE_A:
	{
		int offs = operand;
		if (mod->verCompare(3, 10) >= 0)
			offs *= sizeof(uint16_t); // // BPO-27129

		if (offs < pos) {
			if (curblock->blktype() == ASTBlock::BLK_FOR) {
				bool is_jump_to_start = offs == curblock.cast<ASTIterBlock>()->start();
				bool should_pop_for_block = curblock.cast<ASTIterBlock>()->isComprehension();
				// in v3.8, SETUP_LOOP is deprecated and for blocks aren't terminated by POP_BLOCK, so we add them here
				bool should_add_for_block = mod->majorVer() == 3 && mod->minorVer() >= 8 && is_jump_to_start && !curblock.cast<ASTIterBlock>()->isComprehension();

				if (should_pop_for_block || should_add_for_block) {
					PycRef<ASTNode> top = stack.top();

					if (top.type() == ASTNode::NODE_COMPREHENSION) {
						PycRef<ASTComprehension> comp = top.cast<ASTComprehension>();

						comp->addGenerator(curblock.cast<ASTIterBlock>());
					}

					PycRef<ASTBlock> tmp = curblock;
					blocks.pop();
					curblock = blocks.top();
					if (should_add_for_block) {
						curblock->append(tmp.cast<ASTNode>());
					}
				}
			}
			else if (curblock->blktype() == ASTBlock::BLK_ELSE) {
				stack = stack_hist.top();
				stack_hist.pop();

				blocks.pop();
				blocks.top()->append(curblock.cast<ASTNode>());
				curblock = blocks.top();
			}
			else {
				curblock->append(new ASTKeyword(ASTKeyword::KW_CONTINUE));
			}

			/* We're in a loop, this jumps back to the start */
			/* I think we'll just ignore this case... */
			break; // Bad idea? Probably!
		}

		stack = stack_hist.top();
		stack_hist.pop();

		PycRef<ASTBlock> prev = curblock;
		PycRef<ASTBlock> nil;
		bool push = true;

		do {
			blocks.pop();

			blocks.top()->append(prev.cast<ASTNode>());

			if (prev->blktype() == ASTBlock::BLK_IF
				|| prev->blktype() == ASTBlock::BLK_ELIF) {
				if (push) {
					stack_hist.push(stack);
				}
				PycRef<ASTBlock> next = new ASTBlock(ASTBlock::BLK_ELSE, blocks.top()->end());
				if (prev->inited() == ASTCondBlock::PRE_POPPED) {
					next->init(ASTCondBlock::PRE_POPPED);
				}

				blocks.push(next.cast<ASTBlock>());
				prev = nil;
			}
			else if (prev->blktype() == ASTBlock::BLK_ELSE) {
				/* Special case */
				prev = blocks.top();
				if (!push) {
					stack = stack_hist.top();
					stack_hist.pop();
				}
				push = false;
			}
			else {
				prev = nil;
			}

		} while (prev != nil);

		curblock = blocks.top();
	}
	break;
	case Pyc::JUMP_FORWARD_A:
	{
		int offs = operand;
		if (mod->verCompare(3, 10) >= 0)
			offs *= sizeof(uint16_t); // // BPO-27129
		int target_pos = pos + offs;

		if (curblock->blktype() == ASTBlock::BLK_TRY_FINALLY) {
			if (curblock.cast<ASTTryFinallyBlock>()->getFinallyStart() == pos)
			{
				this->add_finally_block();
			}
			else
			{
				this->add_finally_no_op_block(target_pos);
			}

			break;
		}
		else if (curblock->blktype() == ASTBlock::BLK_TRY_EXCEPT)
		{
			if (curblock.cast<ASTTryExceptBlock>()->hasElseStart())
			{
				// later on JUMP_FORWARD is else END
				curblock.cast<ASTTryExceptBlock>()->setElseEnd(target_pos);
			}
			else
			{
				// first JUMP_FORWARD is else START
				curblock.cast<ASTTryExceptBlock>()->setElseStart(target_pos);
			}

			this->add_except_block(curblock.cast<ASTTryExceptBlock>()->getElseStart());

			this->skipCopyPopExceptIfExists(1);

			break;
		}

		if (!stack_hist.empty()) {
			if (stack.empty()) // if it's part of if-expression, TOS at the moment is the result of "if" part
				stack = stack_hist.top();
			stack_hist.pop();
		}

		PycRef<ASTBlock> prev = curblock;
		PycRef<ASTBlock> nil;
		bool push = true;

		do {
			blocks.pop();

			if (!blocks.empty())
				blocks.top()->append(prev.cast<ASTNode>());

			if (prev->blktype() == ASTBlock::BLK_IF
				|| prev->blktype() == ASTBlock::BLK_ELIF) {
				if (offs == 0) {
					prev = nil;
					continue;
				}

				if (push) {
					stack_hist.push(stack);
				}
				PycRef<ASTBlock> next = new ASTBlock(ASTBlock::BLK_ELSE, target_pos);
				if (prev->inited() == ASTCondBlock::PRE_POPPED) {
					next->init(ASTCondBlock::PRE_POPPED);
				}

				blocks.push(next.cast<ASTBlock>());
				prev = nil;
			}
			else if (prev->blktype() == ASTBlock::BLK_ELSE) {
				/* Special case */
				prev = blocks.top();
				if (!push) {
					stack = stack_hist.top();
					stack_hist.pop();
				}
				push = false;

				if (prev->blktype() == ASTBlock::BLK_MAIN) {
					/* Something went out of control! */
					prev = nil;
				}
			}
			else {
				prev = nil;
			}

		} while (prev != nil);

		curblock = blocks.top();
	}
	break;
	case Pyc::JUMP_BACKWARD_A:
	{
		if (curblock->blktype() == ASTBlock::BLK_FOR) {
			int target_pos = pos - operand * sizeof(uint16_t); // equals to jump_absolute
			bool is_jump_to_start = target_pos == curblock.cast<ASTIterBlock>()->start();
			bool should_pop_for_block = curblock.cast<ASTIterBlock>()->isComprehension();
			// in v3.8, SETUP_LOOP is deprecated and for blocks aren't terminated by POP_BLOCK, so we add them here
			bool should_add_for_block = is_jump_to_start && !curblock.cast<ASTIterBlock>()->isComprehension();

			if (should_pop_for_block || should_add_for_block) {
				PycRef<ASTNode> top = stack.top();

				if (top.type() == ASTNode::NODE_COMPREHENSION) {
					PycRef<ASTComprehension> comp = top.cast<ASTComprehension>();

					comp->addGenerator(curblock.cast<ASTIterBlock>());
				}

				PycRef<ASTBlock> tmp = curblock;
				blocks.pop();
				curblock = blocks.top();
				if (should_add_for_block) {
					curblock->append(tmp.cast<ASTNode>());
				}
			}
		}
		else if (curblock->blktype() == ASTBlock::BLK_ELSE) {
			stack = stack_hist.top();
			stack_hist.pop();

			blocks.pop();
			blocks.top()->append(curblock.cast<ASTNode>());
			curblock = blocks.top();
		}
		else {
			curblock->append(new ASTKeyword(ASTKeyword::KW_CONTINUE));
		}
	}
	break;
	case Pyc::LIST_APPEND:
	case Pyc::LIST_APPEND_A:
	{
		PycRef<ASTNode> value = stack.top();
		stack.pop();

		PycRef<ASTNode> list = stack.top();


		if (curblock->blktype() == ASTBlock::BLK_FOR
			&& curblock.cast<ASTIterBlock>()->isComprehension()) {
			stack.pop();
			stack.push(new ASTComprehension(value));
		}
		else {
			stack.push(new ASTSubscr(list, value)); /* Total hack */
		}
	}
	break;
	case Pyc::SET_UPDATE_A:
	{
		PycRef<ASTNode> rhs = stack.top();
		stack.pop();
		PycRef<ASTSet> lhs = stack.top().cast<ASTSet>();
		stack.pop();

		if (rhs.type() != ASTNode::NODE_OBJECT) {
			fprintf(stderr, "Unsupported argument found for SET_UPDATE\n");
			break;
		}

		// I've only ever seen this be a TYPE_FROZENSET, but let's be careful...
		PycRef<PycObject> obj = rhs.cast<ASTObject>()->object();
		if (obj->type() != PycObject::TYPE_FROZENSET) {
			fprintf(stderr, "Unsupported argument type found for SET_UPDATE\n");
			break;
		}

		ASTSet::value_t result = lhs->values();
		for (const auto& it : obj.cast<PycSet>()->values()) {
			result.push_back(new ASTObject(it));
		}

		stack.push(new ASTSet(result));
	}
	break;
	case Pyc::LIST_EXTEND_A:
	{
		PycRef<ASTNode> rhs = stack.top();
		stack.pop();
		PycRef<ASTList> lhs = stack.top().cast<ASTList>();
		stack.pop();

		if (rhs.type() != ASTNode::NODE_OBJECT) {
			fprintf(stderr, "Unsupported argument found for LIST_EXTEND\n");
			break;
		}

		// I've only ever seen this be a SMALL_TUPLE, but let's be careful...
		PycRef<PycObject> obj = rhs.cast<ASTObject>()->object();
		if (obj->type() != PycObject::TYPE_TUPLE && obj->type() != PycObject::TYPE_SMALL_TUPLE) {
			fprintf(stderr, "Unsupported argument type found for LIST_EXTEND\n");
			break;
		}

		ASTList::value_t result = lhs->values();
		for (const auto& it : obj.cast<PycTuple>()->values()) {
			result.push_back(new ASTObject(it));
		}

		stack.push(new ASTList(result));
	}
	break;
	case Pyc::LOAD_ATTR_A:
	{
		PycRef<ASTNode> name = stack.top();
		if (name.type() != ASTNode::NODE_IMPORT) {
			stack.pop();
			stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
		}
	}
	break;
	case Pyc::LOAD_BUILD_CLASS:
		stack.push(new ASTLoadBuildClass(new PycObject()));
		break;
	case Pyc::LOAD_CLOSURE_A:
		/* Ignore this */
		break;
	case Pyc::LOAD_CONST_A:
	{
		PycRef<ASTObject> t_ob = new ASTObject(code->getConst(operand));

		if ((t_ob->object().type() == PycObject::TYPE_TUPLE ||
			t_ob->object().type() == PycObject::TYPE_SMALL_TUPLE) &&
			!t_ob->object().cast<PycTuple>()->values().size()) {
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
	case Pyc::LOAD_DEREF_A:
		stack.push(new ASTName(code->getCellVar(mod, operand)));
		break;
	case Pyc::LOAD_FAST_A:
		if (mod->verCompare(1, 3) < 0)
			stack.push(new ASTName(code->getName(operand)));
		else
			stack.push(new ASTName(code->getLocal(operand)));
		break;
	case Pyc::LOAD_GLOBAL_A:
		if (mod->verCompare(3, 11) >= 0) {
			if (operand & 1) {
				/* Changed in version 3.11:
				If the low bit of "NAMEI" (operand) is set,
				then a NULL is pushed to the stack before the global variable. */
				stack.push(nullptr);
			}
			// Special case for Python 3.11+
			operand >>= 1;
		}
		stack.push(new ASTName(code->getName(operand)));
		break;
	case Pyc::LOAD_LOCALS:
		stack.push(new ASTNode(ASTNode::NODE_LOCALS));
		break;
	case Pyc::STORE_LOCALS:
		stack.pop();
		break;
	case Pyc::LOAD_METHOD_A:
	{
		// Behave like LOAD_ATTR
		PycRef<ASTNode> name = stack.top();
		stack.pop();
		stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
	}
	break;
	case Pyc::LOAD_NAME_A:
		stack.push(new ASTName(code->getName(operand)));
		break;
	case Pyc::LOAD_ASSERTION_ERROR:
	{
		PycRef<PycString> assertionErrorString = new PycString();
		assertionErrorString->setValue("AssertionError");
		stack.push(new ASTName(assertionErrorString));
	}
	break;
	case Pyc::MAKE_CLOSURE_A:
	case Pyc::MAKE_FUNCTION_A:
	{
		PycRef<ASTNode> func_code_obj = stack.top();
		stack.pop();

		/* Test for the qualified name of the function (at TOS) */
		int tos_type = func_code_obj.cast<ASTObject>()->object().type();
		if (tos_type != PycObject::TYPE_CODE &&
			tos_type != PycObject::TYPE_CODE2) {
			func_code_obj = stack.top();
			stack.pop();
		}

		bool is_closure;
		if (mod->verCompare(3, 6) >= 0) {
			is_closure = operand & 0x8;
		}
		else {
			is_closure = opcode == Pyc::MAKE_CLOSURE_A;
		}
		if (is_closure) {
			// its a closure function
			// pop the tuple of the LOAD_CLOSURE's, dont need it
			stack.pop();
		}

		ASTFunction::defarg_t defaultsValues, kwDefArgs;
		ASTTuple::value_t annot_tuple_values;

		if (mod->verCompare(3, 6) >= 0) {
			// annotations
			if (operand & 0x4) {
				if (mod->verCompare(3, 10) >= 0) {
					// (ASTTuple)
					ASTTuple::value_t curr_annot_tuple_values = stack.top().cast<ASTTuple>()->values();
					stack.pop();

					for (size_t i = 0; i < curr_annot_tuple_values.size(); i++)
					{
						if (curr_annot_tuple_values[i]->type() == ASTNode::NODE_NAME) {
							annot_tuple_values.push_back(curr_annot_tuple_values[i].cast<ASTNode>());
						}
						else {
							annot_tuple_values.push_back(new ASTName(curr_annot_tuple_values[i].cast<ASTObject>()->object().cast<PycString>()));
						}
					}
				}
				else {
					// (ASTConstMap)
					PycRef<ASTConstMap> annot_const_map = stack.top().cast<ASTConstMap>();
					stack.pop();

					PycSimpleSequence::value_t annot_const_map_keys = annot_const_map->keys().cast<ASTObject>()->object().cast<PycTuple>()->values();
					ASTConstMap::values_t annot_const_map_values = annot_const_map->values();
					for (size_t i = 0; i < annot_const_map_values.size(); i++) {
						annot_tuple_values.push_back(new ASTName(annot_const_map_keys[i].cast<PycString>()));
						annot_tuple_values.push_back(annot_const_map_values[i]);
					}
				}
			}

			// keyword-only parameters (a const map)
			if (operand & 0x2) {
				ASTConstMap::values_t kwdefArgsValues = stack.top().cast<ASTConstMap>()->values();
				stack.pop();
				kwDefArgs.assign(kwdefArgsValues.begin(), kwdefArgsValues.end());
			}

			// positional-only and positional-or-keyword parameters (a tuple)
			if (operand & 0x1) {
				if (mod->verCompare(3, 10) >= 0) {
					// (ASTTuple)
					ASTTuple::value_t defaultsTupleValues = stack.top().cast<ASTTuple>()->values();
					stack.pop();
					defaultsValues.assign(defaultsTupleValues.begin(), defaultsTupleValues.end());
				}
				else {
					// (PycTuple)
					PycSimpleSequence::value_t defaultsTupleValues = stack.top().cast<ASTObject>()->object().cast<PycTuple>()->values();
					stack.pop();
					for (PycSimpleSequence::value_t::const_iterator it = defaultsTupleValues.begin(); it != defaultsTupleValues.end(); it++) {
						defaultsValues.push_back(new ASTObject(*it));
					}
				}
			}
		}
		else {
			const int defaultsCount = operand & 0xFF;
			const int kwDefCount = (operand >> 8) & 0xFF;
			int annotCount = (operand >> 16) & 0x7FFF;

			if (annotCount) {
				// annotations count also counting the tuple (so decrement 1)
				annotCount--;

				// TOS will be a tuple of the parameters names that has the annotations.
				// Annotations values will start at TOS - 1.

				// TODO: bad cast HERE
				PycSimpleSequence::value_t paramsAnnotNames = stack.top().cast<ASTObject>()->object().cast<PycTuple>()->values();
				stack.pop();

				ASTConstMap::values_t annotValues;
				annotValues.reserve(annotCount);
				for (int i = 0; i < annotCount; ++i) {
					PycRef<ASTNode> annot_value = stack.top();
					stack.pop();
					annotValues.push_back(annot_value);
				}
				// reverse values order
				std::reverse(annotValues.begin(), annotValues.end());

				for (size_t i = 0; i < annotValues.size(); i++) {
					annot_tuple_values.push_back(new ASTName(paramsAnnotNames[i].cast<PycString>()));
					annot_tuple_values.push_back(annotValues[i]);
				}
			}
			for (int i = 0; i < defaultsCount; ++i) {
				defaultsValues.push_front(stack.top());
				stack.pop();
			}
			for (int i = 0; i < kwDefCount; ++i) {
				kwDefArgs.push_front(stack.top());
				stack.pop();
				/* pop the parameter name, we already have the parameters names and
				* all we need it in this pyc object */
				stack.pop();
			}
		}

		// checking if this function is a lambda of list comprehension
		PycRef<PycCode> func_code_ref = func_code_obj.cast<ASTObject>()->object().cast<PycCode>();
		bool is_comp_lambda = strcmp(func_code_ref->name()->value(), "<listcomp>") == 0;

		PycRef<ASTTuple> annot_tuple = new ASTTuple(annot_tuple_values);
		stack.push(new ASTFunction(func_code_obj, annot_tuple.cast<ASTNode>(), defaultsValues, kwDefArgs, is_comp_lambda));

		if (mod->verCompare(3, 11) >= 0) {
			/* start counting towards CALL,
			need GET_ITER then immediately a CALL
			when encountered, pop TOS as argument,
			from 3.11 this CALL always has 0
			argc as operand
			(probably because there is always 1)
			as it is a comprehension lambda*/
			if (is_comp_lambda) {
				comprehension_counter = 3;
			}
		}
	}
	break;
	case Pyc::NOP:
		break;
	case Pyc::POP_BLOCK:
	{
		if (curblock->blktype() == ASTBlock::BLK_TRY)
		{
			this->pop_try();

			break;
		}

		if (curblock->blktype() == ASTBlock::BLK_WITH) {
			// This should only be popped by a WITH_CLEANUP
			break;
		}

		if (curblock->nodes().size() &&
			curblock->nodes().back().type() == ASTNode::NODE_KEYWORD) {
			curblock->removeLast();
		}

		if (curblock->blktype() == ASTBlock::BLK_IF
			|| curblock->blktype() == ASTBlock::BLK_ELIF
			|| curblock->blktype() == ASTBlock::BLK_ELSE)
		{
			if (!stack_hist.empty()) {
				stack = stack_hist.top();
				stack_hist.pop();
			}
			else {
				fprintf(stderr, "Warning: Stack history is empty, something wrong might have happened\n");
			}
		}
		PycRef<ASTBlock> tmp = curblock;
		blocks.pop();

		if (!blocks.empty())
			curblock = blocks.top();

		if (!(tmp->blktype() == ASTBlock::BLK_ELSE
			&& tmp->nodes().size() == 0)) {
			curblock->append(tmp.cast<ASTNode>());
		}

		if (tmp->blktype() == ASTBlock::BLK_FOR && tmp->end() >= pos) {
			stack_hist.push(stack);

			PycRef<ASTBlock> blkelse = new ASTBlock(ASTBlock::BLK_ELSE, tmp->end());
			blocks.push(blkelse);
			curblock = blocks.top();
		}
	}
	break;
	case Pyc::POP_EXCEPT:
	{
		if (curblock->blktype() == ASTBlock::BLK_FINALLY ||
			curblock->blktype() == ASTBlock::BLK_NO_OP_FINALLY)
		{
			// WTF bro, how a finally got to here
			// that means its a try-{try-finally}}
			// for cleaning up the `varname`, except as `varname`
			// and its like this
			// SETUP_FINALLY
			// [try block]
			// POP_BLOCK
			// POP_EXCEPT
			// [finally block]
			// END_FINALLY / RERAISE / RERAISE_A

			// pop the finally block
			stack = stack_hist.top();
			stack_hist.pop();
			blocks.pop();
			curblock = blocks.top();

			// pop the try-finally and append it to except block
			blocks.pop();
			blocks.top()->append(curblock.cast<ASTNode>());
			curblock = blocks.top();

			if (mod->verCompare(3, 11) < 0)
			{
				// move try block nodes to try-finally
				curblock->extractInnerOfFirstBlock();

				// move try-finally nodes to except block
				curblock->extractInnerOfFirstBlock();

				this->pop_except();
				if (mod->verCompare(3, 9) >= 0)
				{
					this->add_finally_no_op_block(0);
				}
				this->add_finally_no_op_block(0);
			}
		}
		else
		{
			this->pop_except();

			if (this->isOpcodeReturnAfterN(2))
			{
				this->pop_try_except_or_try_finally_block();
				this->skipCopyPopExceptIfExists(3);
			}
		}
	}
	break;
	case Pyc::POP_TOP:
	{
		PycRef<ASTNode> value = stack.top();
		stack.pop();
		if (!curblock->inited()) {
			if (curblock->blktype() == ASTBlock::BLK_WITH) {
				curblock.cast<ASTWithBlock>()->setExpr(value);
			}
			else {
				curblock->init();
			}
			break;
		}
		else if (value == nullptr || value->processed()) {
			break;
		}

		curblock->append(value);

		if (curblock->blktype() == ASTBlock::BLK_FOR
			&& curblock.cast<ASTIterBlock>()->isComprehension()) {
			/* This relies on some really uncertain logic...
			 * If it's a comprehension, the only POP_TOP should be
			 * a call to append the iter to the list.
			 */
			if (value.type() == ASTNode::NODE_CALL) {
				auto& pparams = value.cast<ASTCall>()->pparams();
				if (!pparams.empty()) {
					PycRef<ASTNode> res = pparams.front();
					stack.push(new ASTComprehension(res));
				}
			}
		}
	}
	break;
	case Pyc::PRINT_ITEM:
	{
		PycRef<ASTPrint> printNode;
		if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
			printNode = curblock->nodes().back().try_cast<ASTPrint>();
		if (printNode && printNode->stream() == nullptr && !printNode->eol())
			printNode->add(stack.top());
		else
			curblock->append(new ASTPrint(stack.top()));
		stack.pop();
	}
	break;
	case Pyc::PRINT_ITEM_TO:
	{
		PycRef<ASTNode> stream = stack.top();
		stack.pop();

		PycRef<ASTPrint> printNode;
		if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
			printNode = curblock->nodes().back().try_cast<ASTPrint>();
		if (printNode && printNode->stream() == stream && !printNode->eol())
			printNode->add(stack.top());
		else
			curblock->append(new ASTPrint(stack.top(), stream));
		stack.pop();
		stream->setProcessed();
	}
	break;
	case Pyc::PRINT_NEWLINE:
	{
		PycRef<ASTPrint> printNode;
		if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
			printNode = curblock->nodes().back().try_cast<ASTPrint>();
		if (printNode && printNode->stream() == nullptr && !printNode->eol())
			printNode->setEol(true);
		else
			curblock->append(new ASTPrint(nullptr));
		stack.pop();
	}
	break;
	case Pyc::PRINT_NEWLINE_TO:
	{
		PycRef<ASTNode> stream = stack.top();
		stack.pop();

		PycRef<ASTPrint> printNode;
		if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
			printNode = curblock->nodes().back().try_cast<ASTPrint>();
		if (printNode && printNode->stream() == stream && !printNode->eol())
			printNode->setEol(true);
		else
			curblock->append(new ASTPrint(nullptr, stream));
		stack.pop();
		stream->setProcessed();
	}
	break;
	case Pyc::RAISE_VARARGS_A:
	{
		PycRef<ASTNode> raise;
		if (operand == 1) {
			PycRef<ASTNode> exception = stack.top();
			stack.pop();
			raise = new ASTRaise(exception);
		}
		else if (operand == 2) {
			PycRef<ASTNode> exceptionCause = stack.top();
			stack.pop();
			PycRef<ASTNode> exception = stack.top();
			stack.pop();
			raise = new ASTRaise(exception, exceptionCause);
		}

		curblock->append(raise);
	}
	break;
	case Pyc::RETURN_VALUE:
	{
		PycRef<ASTNode> value = stack.top();
		stack.pop();
		curblock->append(new ASTReturn(value));
	}
	break;
	case Pyc::ROT_TWO:
	{
		PycRef<ASTNode> one = stack.top();
		stack.pop();
		if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
			stack.pop();
		}
		PycRef<ASTNode> two = stack.top();
		stack.pop();

		stack.push(one);
		stack.push(two);
	}
	break;
	case Pyc::ROT_THREE:
	{
		PycRef<ASTNode> one = stack.top();
		stack.pop();
		PycRef<ASTNode> two = stack.top();
		stack.pop();
		if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
			stack.pop();
		}
		PycRef<ASTNode> three = stack.top();
		stack.pop();
		stack.push(one);
		stack.push(three);
		stack.push(two);
	}
	break;
	case Pyc::ROT_FOUR:
	{
		PycRef<ASTNode> one = stack.top();
		stack.pop();
		PycRef<ASTNode> two = stack.top();
		stack.pop();
		PycRef<ASTNode> three = stack.top();
		stack.pop();
		if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
			stack.pop();
		}
		PycRef<ASTNode> four = stack.top();
		stack.pop();
		stack.push(one);
		stack.push(four);
		stack.push(three);
		stack.push(two);
	}
	break;
	case Pyc::SET_LINENO_A:
		// Ignore
		break;
	case Pyc::SETUP_WITH_A:
	{
		PycRef<ASTBlock> withblock = new ASTWithBlock(pos + operand);
		blocks.push(withblock);
		curblock = blocks.top();
	}
	break;
	case Pyc::WITH_CLEANUP:
	{
		// Stack top should be a None. Ignore it.
		PycRef<ASTNode> none = stack.top();
		stack.pop();

		if (none != NULL) {
			fprintf(stderr, "Something TERRIBLE happened!\n");
			break;
		}

		if (curblock->blktype() == ASTBlock::BLK_WITH
			&& curblock->end() == curpos) {
			PycRef<ASTBlock> with = curblock;
			blocks.pop();
			curblock = blocks.top();
			curblock->append(with.cast<ASTNode>());
		}
		else {
			fprintf(stderr, "Something TERRIBLE happened! No matching with block found for WITH_CLEANUP at %d\n", curpos);
		}
	}
	break;
	case Pyc::CHECK_EXC_MATCH:
	{
		PycRef<ASTNode> right = stack.top();
		stack.pop();
		stack.push(new ASTCompare(NULL, right, ASTCompare::CompareOp::CMP_EXCEPTION));
	}
	break;
	case Pyc::PUSH_EXC_INFO:
	{
		// ignore this one
	}
	break;
	case Pyc::RERAISE:
	{
		this->end_finally();
	}
	break;
	case Pyc::RERAISE_A:
	{
		this->end_finally();
	}
	break;
	case Pyc::COPY_A:
	{
		std::list<PycRef<ASTNode>> values;
		for (int i = 0; i < operand; i++) {
			values.push_back(stack.top());
			stack.pop();
		}
		values.push_front(stack.top());
		for (int i = 0; i < operand + 1; i++) {
			stack.push(values.back());
			values.pop_back();
		}
	}
	break;
	case Pyc::SWAP_A:
	{
		if (operand > 0)
		{
			std::list<PycRef<ASTNode>> values_between;
			PycRef<ASTNode> tos_to_swap = stack.top();
			stack.pop();
			for (int i = 0; i < operand - 1; i++)
			{
				values_between.push_back(stack.top());
				stack.pop();
			}
			values_between.push_front(stack.top());
			stack.pop();
			stack.push(tos_to_swap);
			for (int i = 0; i < operand - 1; i++)
			{
				stack.push(values_between.back());
				values_between.pop_back();
			}
		}
	}
	break;
	case Pyc::SETUP_EXCEPT_A:
	{
		PycRef<ASTBlock> tryExceptBlock = new ASTTryExceptBlock(pos + operand);
		blocks.push(tryExceptBlock.cast<ASTBlock>());
		curblock = blocks.top();

		this->add_try_block(0);
	}
	break;
	case Pyc::SETUP_FINALLY_A:
	{
		int offs = operand;
		if (mod->verCompare(3, 10) >= 0)
			offs *= sizeof(uint16_t); // // BPO-27129

		// for versions 3.8-3.10
		// assume its try finally and NOT try except
		// (checking after POP_BLOCK of TRY in "checker" function)

		this->add_try_finally_block(pos + offs, (mod->verCompare(3, 7) <= 0));
		this->add_try_block(0);
	}
	break;
	case Pyc::BEGIN_FINALLY:
	{
		stack.push(nullptr);
	}
	break;
	case Pyc::SETUP_LOOP_A:
	{
		PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_WHILE, pos + operand, NULL, false);
		blocks.push(next.cast<ASTBlock>());
		curblock = blocks.top();
	}
	break;
	case Pyc::SLICE_0:
	{
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE0);
		stack.push(new ASTSubscr(name, slice));
	}
	break;
	case Pyc::SLICE_1:
	{
		PycRef<ASTNode> lower = stack.top();
		stack.pop();
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE1, lower);
		stack.push(new ASTSubscr(name, slice));
	}
	break;
	case Pyc::SLICE_2:
	{
		PycRef<ASTNode> upper = stack.top();
		stack.pop();
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE2, NULL, upper);
		stack.push(new ASTSubscr(name, slice));
	}
	break;
	case Pyc::SLICE_3:
	{
		PycRef<ASTNode> upper = stack.top();
		stack.pop();
		PycRef<ASTNode> lower = stack.top();
		stack.pop();
		PycRef<ASTNode> name = stack.top();
		stack.pop();

		PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE3, lower, upper);
		stack.push(new ASTSubscr(name, slice));
	}
	break;
	case Pyc::STORE_ATTR_A:
	{
		if (unpack) {
			PycRef<ASTNode> name = stack.top();
			stack.pop();
			PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);

			PycRef<ASTNode> tup = stack.top();
			if (tup.type() == ASTNode::NODE_TUPLE)
				tup.cast<ASTTuple>()->add(attr);
			else
				fputs("Something TERRIBLE happened!\n", stderr);

			if (--unpack <= 0) {
				stack.pop();
				PycRef<ASTNode> seq = stack.top();
				stack.pop();
				if (seq.type() == ASTNode::NODE_CHAINSTORE) {
					append_to_chain_store(seq, tup);
				}
				else {
					curblock->append(new ASTStore(seq, tup));
				}
			}
		}
		else {
			PycRef<ASTNode> name = stack.top();
			stack.pop();
			PycRef<ASTNode> value = stack.top();
			stack.pop();
			PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);
			if (value.type() == ASTNode::NODE_CHAINSTORE) {
				append_to_chain_store(value, attr);
			}
			else {
				curblock->append(new ASTStore(value, attr));
			}
		}
	}
	break;
	case Pyc::STORE_DEREF_A:
	{
		if (unpack) {
			PycRef<ASTNode> name = new ASTName(code->getCellVar(mod, operand));

			PycRef<ASTNode> tup = stack.top();
			if (tup.type() == ASTNode::NODE_TUPLE)
				tup.cast<ASTTuple>()->add(name);
			else
				fputs("Something TERRIBLE happened!\n", stderr);

			if (--unpack <= 0) {
				stack.pop();
				PycRef<ASTNode> seq = stack.top();
				stack.pop();

				if (seq.type() == ASTNode::NODE_CHAINSTORE) {
					append_to_chain_store(seq, tup);
				}
				else {
					curblock->append(new ASTStore(seq, tup));
				}
			}
		}
		else {
			PycRef<ASTNode> value = stack.top();
			stack.pop();
			PycRef<ASTNode> name = new ASTName(code->getCellVar(mod, operand));

			if (value.type() == ASTNode::NODE_CHAINSTORE) {
				append_to_chain_store(value, name);
			}
			else {
				curblock->append(new ASTStore(value, name));
			}
		}
	}
	break;
	case Pyc::STORE_FAST_A:
	{
		if (unpack) {
			PycRef<ASTNode> name;

			if (mod->verCompare(1, 3) < 0)
				name = new ASTName(code->getName(operand));
			else
				name = new ASTName(code->getLocal(operand));

			PycRef<ASTNode> tup = stack.top();
			if (tup.type() == ASTNode::NODE_TUPLE)
				tup.cast<ASTTuple>()->add(name);
			else
				fputs("Something TERRIBLE happened!\n", stderr);

			if (--unpack <= 0) {
				stack.pop();
				PycRef<ASTNode> seq = stack.top();
				stack.pop();

				if (curblock->blktype() == ASTBlock::BLK_FOR
					&& !curblock->inited()) {
					PycRef<ASTTuple> tuple = tup.try_cast<ASTTuple>();
					if (tuple != NULL)
						tuple->setRequireParens(false);
					curblock.cast<ASTIterBlock>()->setIndex(tup);
				}
				else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
					append_to_chain_store(seq, tup);
				}
				else {
					curblock->append(new ASTStore(seq, tup));
				}
			}
		}
		else {
			PycRef<ASTNode> value = stack.top();
			stack.pop();
			PycRef<ASTNode> name;

			if (mod->verCompare(1, 3) < 0)
				name = new ASTName(code->getName(operand));
			else
				name = new ASTName(code->getLocal(operand));

			if (name.cast<ASTName>()->name()->value()[0] == '_'
				&& name.cast<ASTName>()->name()->value()[1] == '[') {
				/* Don't show stores of list comp append objects. */
				break;
			}

			if (curblock->blktype() == ASTBlock::BLK_FOR
				&& !curblock->inited()) {
				curblock.cast<ASTIterBlock>()->setIndex(name);
			}
			else if (curblock->blktype() == ASTBlock::BLK_WITH
				&& !curblock->inited()) {
				curblock.cast<ASTWithBlock>()->setExpr(value);
				curblock.cast<ASTWithBlock>()->setVar(name);
			}
			else if (value.type() == ASTNode::NODE_CHAINSTORE) {
				append_to_chain_store(value, name);
			}
			else {
				curblock->append(new ASTStore(value, name));
			}
		}
	}
	break;
	case Pyc::STORE_GLOBAL_A:
	{
		PycRef<ASTNode> name = new ASTName(code->getName(operand));

		if (unpack) {
			PycRef<ASTNode> tup = stack.top();
			if (tup.type() == ASTNode::NODE_TUPLE)
				tup.cast<ASTTuple>()->add(name);
			else
				fputs("Something TERRIBLE happened!\n", stderr);

			if (--unpack <= 0) {
				stack.pop();
				PycRef<ASTNode> seq = stack.top();
				stack.pop();

				if (curblock->blktype() == ASTBlock::BLK_FOR
					&& !curblock->inited()) {
					PycRef<ASTTuple> tuple = tup.try_cast<ASTTuple>();
					if (tuple != NULL)
						tuple->setRequireParens(false);
					curblock.cast<ASTIterBlock>()->setIndex(tup);
				}
				else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
					append_to_chain_store(seq, tup);
				}
				else {
					curblock->append(new ASTStore(seq, tup));
				}
			}
		}
		else {
			PycRef<ASTNode> value = stack.top();
			stack.pop();
			if (value.type() == ASTNode::NODE_CHAINSTORE) {
				append_to_chain_store(value, name);
			}
			else {
				curblock->append(new ASTStore(value, name));
			}
		}

		/* Mark the global as used */
		code->markGlobal(name.cast<ASTName>()->name());
	}
	break;
	case Pyc::STORE_NAME_A:
	{
		if (unpack) {
			PycRef<ASTNode> name = new ASTName(code->getName(operand));

			PycRef<ASTNode> tup = stack.top();
			if (tup.type() == ASTNode::NODE_TUPLE)
				tup.cast<ASTTuple>()->add(name);
			else
				fputs("Something TERRIBLE happened!\n", stderr);

			if (--unpack <= 0) {
				stack.pop();
				PycRef<ASTNode> seq = stack.top();
				stack.pop();

				if (curblock->blktype() == ASTBlock::BLK_FOR
					&& !curblock->inited()) {
					PycRef<ASTTuple> tuple = tup.try_cast<ASTTuple>();
					if (tuple != NULL)
						tuple->setRequireParens(false);
					curblock.cast<ASTIterBlock>()->setIndex(tup);
				}
				else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
					append_to_chain_store(seq, tup);
				}
				else {
					curblock->append(new ASTStore(seq, tup));
				}
			}
		}
		else {
			PycRef<ASTNode> value = stack.top();
			stack.pop();

			PycRef<PycString> varname = code->getName(operand);
			if (varname->length() >= 2 && varname->value()[0] == '_'
				&& varname->value()[1] == '[') {
				/* Don't show stores of list comp append objects. */
				break;
			}

			// Return private names back to their original name
			const std::string class_prefix = std::string("_") + code->name()->strValue();
			if (varname->startsWith(class_prefix + std::string("__")))
				varname->setValue(varname->strValue().substr(class_prefix.size()));

			PycRef<ASTNode> name = new ASTName(varname);

			if (curblock->blktype() == ASTBlock::BLK_FOR
				&& !curblock->inited()) {
				curblock.cast<ASTIterBlock>()->setIndex(name);
			}
			else if (stack.top().type() == ASTNode::NODE_IMPORT) {
				PycRef<ASTImport> import = stack.top().cast<ASTImport>();

				import->add_store(new ASTStore(value, name));
			}
			else if (curblock->blktype() == ASTBlock::BLK_WITH
				&& !curblock->inited()) {
				curblock.cast<ASTWithBlock>()->setExpr(value);
				curblock.cast<ASTWithBlock>()->setVar(name);
			}
			else if (value.type() == ASTNode::NODE_CHAINSTORE) {
				append_to_chain_store(value, name);
			}
			else {
				curblock->append(new ASTStore(value, name));

				if (value.type() == ASTNode::NODE_INVALID)
					break;
			}
		}
	}
	break;
	case Pyc::STORE_SLICE_0:
	{
		PycRef<ASTNode> dest = stack.top();
		stack.pop();
		PycRef<ASTNode> value = stack.top();
		stack.pop();

		curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE0))));
	}
	break;
	case Pyc::STORE_SLICE_1:
	{
		PycRef<ASTNode> upper = stack.top();
		stack.pop();
		PycRef<ASTNode> dest = stack.top();
		stack.pop();
		PycRef<ASTNode> value = stack.top();
		stack.pop();

		curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE1, upper))));
	}
	break;
	case Pyc::STORE_SLICE_2:
	{
		PycRef<ASTNode> lower = stack.top();
		stack.pop();
		PycRef<ASTNode> dest = stack.top();
		stack.pop();
		PycRef<ASTNode> value = stack.top();
		stack.pop();

		curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE2, NULL, lower))));
	}
	break;
	case Pyc::STORE_SLICE_3:
	{
		PycRef<ASTNode> lower = stack.top();
		stack.pop();
		PycRef<ASTNode> upper = stack.top();
		stack.pop();
		PycRef<ASTNode> dest = stack.top();
		stack.pop();
		PycRef<ASTNode> value = stack.top();
		stack.pop();

		curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE3, upper, lower))));
	}
	break;
	case Pyc::STORE_SUBSCR:
	{
		if (unpack) {
			PycRef<ASTNode> subscr = stack.top();
			stack.pop();
			PycRef<ASTNode> dest = stack.top();
			stack.pop();

			PycRef<ASTNode> save = new ASTSubscr(dest, subscr);

			PycRef<ASTNode> tup = stack.top();
			if (tup.type() == ASTNode::NODE_TUPLE)
				tup.cast<ASTTuple>()->add(save);
			else
				fputs("Something TERRIBLE happened!\n", stderr);

			if (--unpack <= 0) {
				stack.pop();
				PycRef<ASTNode> seq = stack.top();
				stack.pop();
				if (seq.type() == ASTNode::NODE_CHAINSTORE) {
					append_to_chain_store(seq, tup);
				}
				else {
					curblock->append(new ASTStore(seq, tup));
				}
			}
		}
		else {
			PycRef<ASTNode> subscr = stack.top();
			stack.pop();
			PycRef<ASTNode> dest = stack.top();
			stack.pop();
			PycRef<ASTNode> src = stack.top();
			stack.pop();

			// If variable annotations are enabled, we'll need to check for them here.
			// Python handles a varaible annotation by setting:
			// __annotations__['var-name'] = type
			const bool found_annotated_var = (variable_annotations && dest->type() == ASTNode::Type::NODE_NAME
				&& dest.cast<ASTName>()->name()->isEqual("__annotations__"));

			if (found_annotated_var) {
				// Annotations can be done alone or as part of an assignment.
				// In the case of an assignment, we'll see a NODE_STORE on the stack.
				if (!curblock->nodes().empty() && curblock->nodes().back()->type() == ASTNode::Type::NODE_STORE) {
					// Replace the existing NODE_STORE with a new one that includes the annotation.
					PycRef<ASTStore> store = curblock->nodes().back().cast<ASTStore>();
					curblock->removeLast();
					curblock->append(new ASTStore(store->src(),
						new ASTAnnotatedVar(subscr, src)));
				}
				else {
					curblock->append(new ASTAnnotatedVar(subscr, src));
				}
			}
			else {
				if (dest.type() == ASTNode::NODE_MAP) {
					dest.cast<ASTMap>()->addBack(subscr, src);
				}
				else if (src.type() == ASTNode::NODE_CHAINSTORE) {
					append_to_chain_store(src, new ASTSubscr(dest, subscr));
				}
				else {
					curblock->append(new ASTStore(src, new ASTSubscr(dest, subscr)));
				}
			}
		}
	}
	break;
	case Pyc::UNARY_CALL:
	{
		PycRef<ASTNode> func = stack.top();
		stack.pop();
		stack.push(new ASTCall(func, ASTCall::pparam_t(), ASTCall::kwparam_t()));
	}
	break;
	case Pyc::UNARY_CONVERT:
	{
		PycRef<ASTNode> name = stack.top();
		stack.pop();
		stack.push(new ASTConvert(name));
	}
	break;
	case Pyc::UNARY_INVERT:
	{
		PycRef<ASTNode> arg = stack.top();
		stack.pop();
		stack.push(new ASTUnary(arg, ASTUnary::UN_INVERT));
	}
	break;
	case Pyc::UNARY_NEGATIVE:
	{
		PycRef<ASTNode> arg = stack.top();
		stack.pop();
		stack.push(new ASTUnary(arg, ASTUnary::UN_NEGATIVE));
	}
	break;
	case Pyc::UNARY_NOT:
	{
		PycRef<ASTNode> arg = stack.top();
		stack.pop();
		stack.push(new ASTUnary(arg, ASTUnary::UN_NOT));
	}
	break;
	case Pyc::UNARY_POSITIVE:
	{
		PycRef<ASTNode> arg = stack.top();
		stack.pop();
		stack.push(new ASTUnary(arg, ASTUnary::UN_POSITIVE));
	}
	break;
	case Pyc::UNPACK_LIST_A:
	case Pyc::UNPACK_TUPLE_A:
	case Pyc::UNPACK_SEQUENCE_A:
	{
		unpack = operand;
		if (unpack > 0) {
			ASTTuple::value_t vals;
			stack.push(new ASTTuple(vals));
		}
		else {
			// Unpack zero values and assign it to top of stack or for loop variable.
			// E.g. [] = TOS / for [] in X
			ASTTuple::value_t vals;
			auto tup = new ASTTuple(vals);
			if (curblock->blktype() == ASTBlock::BLK_FOR
				&& !curblock->inited()) {
				tup->setRequireParens(true);
				curblock.cast<ASTIterBlock>()->setIndex(tup);
			}
			else if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
				auto chainStore = stack.top();
				stack.pop();
				append_to_chain_store(chainStore, tup);
			}
			else {
				curblock->append(new ASTStore(stack.top(), tup));
				stack.pop();
			}
		}
	}
	break;
	case Pyc::YIELD_FROM:
	{
		PycRef<ASTNode> dest = stack.top();
		stack.pop();
		// TODO: Support yielding into a non-null destination
		PycRef<ASTNode> value = stack.top();
		if (value) {
			value->setProcessed();
			curblock->append(new ASTReturn(value, ASTReturn::YIELD_FROM));
		}
	}
	break;
	case Pyc::YIELD_VALUE:
	{
		PycRef<ASTNode> value = stack.top();
		stack.pop();
		curblock->append(new ASTReturn(value, ASTReturn::YIELD));
	}
	break;
	case Pyc::SETUP_ANNOTATIONS:
		variable_annotations = true;
		break;
	case Pyc::PRECALL_A:
	case Pyc::RESUME_A:
		/* We just entirely ignore this / no-op */
		break;
	case Pyc::CACHE:
		/* These "fake" opcodes are used as placeholders for optimizing
		   certain opcodes in Python 3.11+.  Since we have no need for
		   that during disassembly/decompilation, we can just treat these
		   as no-ops. */
		break;
	case Pyc::PUSH_NULL:
		stack.push(nullptr);
		break;
	default:
		fprintf(stderr, "Unsupported opcode: %s\n", Pyc::OpcodeName(opcode & 0xFF));
		cleanBuild = false;
		throw UnsupportedOpcodeException();
	}
}

void BuildFromCode::end_finally()
{
	if (curblock->blktype() == ASTBlock::BLK_NO_OP_FINALLY)
	{
		// ignore for now, if (curblock->end() == pos)
		
		// if its a duplicated finally that got to its end, just pop it, and end this opcode
		stack = stack_hist.top();
		stack_hist.pop();

		blocks.pop();
		curblock = blocks.top();
	}
	else if (curblock->blktype() == ASTBlock::BLK_FINALLY)
	{
		// pop and append finally to try finally
		stack = stack_hist.top();
		stack_hist.pop();

		blocks.pop();
		blocks.top()->append(curblock.cast<ASTNode>());
		curblock = blocks.top();

		this->pop_try_except_or_try_finally_block();
	}
	else if (curblock->blktype() == ASTBlock::BLK_TRY_EXCEPT)
	{
		this->pop_try_except_or_try_finally_block();
	}

	this->skipCopyPopExceptIfExists(0);
}

void BuildFromCode::convert_try_finally_to_try_except()
{
	blocks.pop();
	PycRef<ASTTryExceptBlock> tryExceptBlock = new ASTTryExceptBlock(curblock.cast<ASTTryFinallyBlock>()->getFinallyStart());
	tryExceptBlock->moveNodesFromAnother(curblock);
	blocks.push(tryExceptBlock.cast<ASTBlock>());
	curblock = blocks.top();
}

void BuildFromCode::add_try_finally_block(int start, bool inited)
{
	PycRef<ASTBlock> tryFinallyBlock = new ASTTryFinallyBlock(start, inited);
	blocks.push(tryFinallyBlock.cast<ASTBlock>());
	curblock = blocks.top();
}

void BuildFromCode::add_try_block(int end)
{
	stack_hist.push(stack);
	PycRef<ASTBlock> tryblock = new ASTBlock(ASTBlock::BLK_TRY, end, true);
	blocks.push(tryblock.cast<ASTBlock>());
	curblock = blocks.top();
}

void BuildFromCode::add_finally_block()
{
	stack_hist.push(stack);
	PycRef<ASTBlock> finallyBlock = new ASTBlock(ASTBlock::BLK_FINALLY, 0, true);
	blocks.push(finallyBlock);
	curblock = blocks.top();
}

void BuildFromCode::add_finally_no_op_block(int end)
{
	stack_hist.push(stack);
	PycRef<ASTBlock> no_op_finally = new ASTBlock(ASTBlock::BLK_NO_OP_FINALLY, end, true);
	blocks.push(no_op_finally);
	curblock = blocks.top();
}

void BuildFromCode::add_except_block(int elseStart)
{
	stack_hist.push(stack);
	PycRef<ASTExceptBlock> exceptBlock = new ASTExceptBlock(elseStart);
	blocks.push(exceptBlock.cast<ASTBlock>());
	curblock = blocks.top();
}

void BuildFromCode::add_else_block(int end)
{
	stack_hist.push(stack);
	PycRef<ASTBlock> elseOfExceptBlock = new ASTBlock(ASTBlock::BLK_ELSE, end, true);
	blocks.push(elseOfExceptBlock);
	curblock = blocks.top();
}

void BuildFromCode::pop_try()
{
	stack = stack_hist.top();
	stack_hist.pop();

	blocks.pop();
	blocks.top()->append(curblock.cast<ASTNode>());
	curblock = blocks.top();
}

void BuildFromCode::pop_except()
{
	// pop and append except to try-except
	stack = stack_hist.top();
	stack_hist.pop();

	blocks.pop();
	blocks.top()->append(curblock.cast<ASTNode>());
	curblock = blocks.top();
}

void BuildFromCode::pop_try_except_or_try_finally_block()
{
	// pop the try except block
	blocks.pop();
	blocks.top()->append(curblock.cast<ASTNode>());
	curblock = blocks.top();
}
