#include "ASTree.h"

ASTree::ASTree(PycModule* param_mod, std::ostream& param_pyc_output) :
	mod(param_mod),
	pyc_output(param_pyc_output),
	cleanBuild(false),
	inLambda(false),
	printDocstringAndGlobals(false),
	printClassDocstring(true),
	cur_indent(-1),
	inComprehension(false)
{
}

int ASTree::cmp_prec(PycRef<ASTNode> parent, PycRef<ASTNode> child)
{
	/* Determine whether the parent has higher precedence than therefore
	   child, so we don't flood the source code with extraneous parens.
	   Else we'd have expressions like (((a + b) + c) + d) when therefore
	   equivalent, a + b + c + d would suffice. */

	if (parent.type() == ASTNode::NODE_UNARY && parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT)
		return 1;   // Always parenthesize not(x)
	if (child.type() == ASTNode::NODE_BINARY) {
		PycRef<ASTBinary> binChild = child.cast<ASTBinary>();
		if (parent.type() == ASTNode::NODE_BINARY)
			return binChild->op() - parent.cast<ASTBinary>()->op();
		else if (parent.type() == ASTNode::NODE_COMPARE)
			return (binChild->op() == ASTBinary::BIN_LOG_AND ||
				binChild->op() == ASTBinary::BIN_LOG_OR) ? 1 : -1;
		else if (parent.type() == ASTNode::NODE_UNARY)
			return (binChild->op() == ASTBinary::BIN_POWER) ? -1 : 1;
	}
	else if (child.type() == ASTNode::NODE_UNARY) {
		PycRef<ASTUnary> unChild = child.cast<ASTUnary>();
		if (parent.type() == ASTNode::NODE_BINARY) {
			PycRef<ASTBinary> binParent = parent.cast<ASTBinary>();
			if (binParent->op() == ASTBinary::BIN_LOG_AND ||
				binParent->op() == ASTBinary::BIN_LOG_OR)
				return -1;
			else if (unChild->op() == ASTUnary::UN_NOT)
				return 1;
			else if (binParent->op() == ASTBinary::BIN_POWER)
				return 1;
			else
				return -1;
		}
		else if (parent.type() == ASTNode::NODE_COMPARE) {
			return (unChild->op() == ASTUnary::UN_NOT) ? 1 : -1;
		}
		else if (parent.type() == ASTNode::NODE_UNARY) {
			return unChild->op() - parent.cast<ASTUnary>()->op();
		}
	}
	else if (child.type() == ASTNode::NODE_COMPARE) {
		PycRef<ASTCompare> cmpChild = child.cast<ASTCompare>();
		if (parent.type() == ASTNode::NODE_BINARY)
			return (parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_AND ||
				parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_OR) ? -1 : 1;
		else if (parent.type() == ASTNode::NODE_COMPARE)
			return cmpChild->op() - parent.cast<ASTCompare>()->op();
		else if (parent.type() == ASTNode::NODE_UNARY)
			return (parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT) ? -1 : 1;
	}

	/* For normal nodes, don't parenthesize anything */
	return -1;
}

void ASTree::start_line(int indent)
{
	if (inLambda)
		return;
	for (int i = 0; i < indent; i++)
		pyc_output << "    ";
}

void ASTree::end_line()
{
	if (inLambda)
		return;
	pyc_output << "\n";
}

void ASTree::print_block(PycRef<ASTBlock> blk, const PycRef<PycCode>& prev_code)
{
	ASTBlock::list_t lines = blk->nodes();

	if (lines.size() == 0) {
		PycRef<ASTNode> pass = new ASTKeyword(ASTKeyword::KW_PASS);
		start_line(cur_indent);
		print_src(pass, prev_code);
	}

	for (auto ln = lines.cbegin(); ln != lines.cend();) {
		if ((*ln).cast<ASTNode>().type() != ASTNode::NODE_NODELIST) {
			start_line(cur_indent);
		}
		print_src(*ln, prev_code);
		if (++ln != lines.end()) {
			end_line();
		}
	}
}

void ASTree::print_formatted_value(PycRef<ASTFormattedValue> formatted_value, const PycRef<PycCode>& prev_code)
{
	pyc_output << "{";
	print_src(formatted_value->val(), prev_code);

	switch (formatted_value->conversion()) {
	case ASTFormattedValue::ConversionFlag::NONE:
		break;
	case ASTFormattedValue::ConversionFlag::STR:
		pyc_output << "!s";
		break;
	case ASTFormattedValue::ConversionFlag::REPR:
		pyc_output << "!r";
		break;
	case ASTFormattedValue::ConversionFlag::ASCII:
		pyc_output << "!a";
		break;
	case ASTFormattedValue::ConversionFlag::FMTSPEC:
		pyc_output << ":" << formatted_value->format_spec().cast<ASTObject>()->object().cast<PycString>()->value();
		break;
	default:
		fprintf(stderr, "Unsupported NODE_FORMATTEDVALUE conversion flag: %d\n", formatted_value->conversion());
	}
	pyc_output << "}";
}

bool ASTree::print_docstring(PycRef<PycObject> obj, int indent)
{
	// docstrings are translated from the bytecode __doc__ = 'string' to simply '''string'''
	auto doc = obj.try_cast<PycString>();
	if (doc != nullptr) {
		start_line(indent);
		doc->print(pyc_output, mod, true);
		pyc_output << "\n";
		return true;
	}
	return false;
}

void ASTree::print_ordered(PycRef<ASTNode> parent, PycRef<ASTNode> child, const PycRef<PycCode>& prev_code)
{
	if (child.type() == ASTNode::NODE_BINARY ||
		child.type() == ASTNode::NODE_COMPARE) {
		if (cmp_prec(parent, child) > 0) {
			pyc_output << "(";
			print_src(child, prev_code);
			pyc_output << ")";
		}
		else {
			print_src(child, prev_code);
		}
	}
	else if (child.type() == ASTNode::NODE_UNARY) {
		if (cmp_prec(parent, child) > 0) {
			pyc_output << "(";
			print_src(child, prev_code);
			pyc_output << ")";
		}
		else {
			print_src(child, prev_code);
		}
	}
	else {
		print_src(child, prev_code);
	}
}

void ASTree::print_src(PycRef<ASTNode> node, const PycRef<PycCode>& prev_code)
{
	if (node == NULL) {
		pyc_output << "None";
		cleanBuild = true;
		return;
	}

	switch (node->type()) {
	case ASTNode::NODE_BINARY:
	case ASTNode::NODE_COMPARE:
	{
		PycRef<ASTBinary> bin = node.cast<ASTBinary>();
		print_ordered(node, bin->left(), prev_code);
		pyc_output << bin->op_str();
		print_ordered(node, bin->right(), prev_code);
	}
	break;
	case ASTNode::NODE_UNARY:
	{
		PycRef<ASTUnary> un = node.cast<ASTUnary>();
		pyc_output << un->op_str();
		print_ordered(node, un->operand(), prev_code);
	}
	break;
	case ASTNode::NODE_CALL:
	{
		PycRef<ASTCall> call = node.cast<ASTCall>();
		bool is_comp_lambda = call->func().type() == ASTNode::NODE_FUNCTION
			&& call->func().cast<ASTFunction>()->is_comp_lambda();
		/* if its a lambda of comprehension
		* don't print the brackets with the arguments, just the comprehension */
		if (is_comp_lambda) {
			inComprehension = true;
			comprehension_iterable = *call->pparams().begin();
		}
		print_src(call->func(), prev_code);
		if (!is_comp_lambda) {
			pyc_output << "(";
			bool first = true;
			for (const auto& param : call->pparams()) {
				if (!first)
					pyc_output << ", ";
				print_src(param, prev_code);
				first = false;
			}
			for (const auto& param : call->kwparams()) {
				if (!first)
					pyc_output << ", ";
				if (param.first.type() == ASTNode::NODE_NAME) {
					pyc_output << param.first.cast<ASTName>()->name()->value() << " = ";
				}
				else {
					PycRef<PycString> str_name = param.first.cast<ASTObject>()->object().cast<PycString>();
					pyc_output << str_name->value() << " = ";
				}
				print_src(param.second, prev_code);
				first = false;
			}
			if (call->hasVar()) {
				if (!first)
					pyc_output << ", ";
				pyc_output << "*";
				print_src(call->var(), prev_code);
				first = false;
			}
			if (call->hasKW()) {
				if (!first)
					pyc_output << ", ";
				pyc_output << "**";
				print_src(call->kw(), prev_code);
				first = false;
			}
			pyc_output << ")";
		}
	}
	break;
	case ASTNode::NODE_DELETE:
	{
		pyc_output << "del ";
		print_src(node.cast<ASTDelete>()->value(), prev_code);
	}
	break;
	case ASTNode::NODE_EXEC:
	{
		PycRef<ASTExec> exec = node.cast<ASTExec>();
		pyc_output << "exec ";
		print_src(exec->statement(), prev_code);

		if (exec->globals() != NULL) {
			pyc_output << " in ";
			print_src(exec->globals(), prev_code);

			if (exec->locals() != NULL
				&& exec->globals() != exec->locals()) {
				pyc_output << ", ";
				print_src(exec->locals(), prev_code);
			}
		}
	}
	break;
	case ASTNode::NODE_FORMATTEDVALUE:
		pyc_output << "f" F_STRING_QUOTE;
		print_formatted_value(node.cast<ASTFormattedValue>(), prev_code);
		pyc_output << F_STRING_QUOTE;
		break;
	case ASTNode::NODE_JOINEDSTR:
		pyc_output << "f" F_STRING_QUOTE;
		for (const auto& val : node.cast<ASTJoinedStr>()->values()) {
			switch (val.type()) {
			case ASTNode::NODE_FORMATTEDVALUE:
				print_formatted_value(val.cast<ASTFormattedValue>(), prev_code);
				break;
			case ASTNode::NODE_OBJECT:
				// When printing a piece of the f-string, keep the quote style consistent.
				// This avoids problems when ''' or """ is part of the string.
				print_const(pyc_output, val.cast<ASTObject>()->object(), mod, F_STRING_QUOTE);
				break;
			default:
				fprintf(stderr, "Unsupported node type %d in NODE_JOINEDSTR\n", val.type());
			}
		}
		pyc_output << F_STRING_QUOTE;
		break;
	case ASTNode::NODE_KEYWORD:
		pyc_output << node.cast<ASTKeyword>()->word_str();
		break;
	case ASTNode::NODE_LIST:
	{
		pyc_output << "[ ";
		bool first = true;
		bool isUnpack = node.cast<ASTList>()->isUnpack();
		for (const auto& val : node.cast<ASTList>()->values()) {
			if (!first)
				pyc_output << ", ";
			if (isUnpack) {
				pyc_output << "*";
			}
			print_src(val, prev_code);
			first = false;
		}
		pyc_output << " ]";
	}
	break;
	case ASTNode::NODE_SET:
	{
		pyc_output << "{ ";
		bool first = true;
		bool isUnpack = node.cast<ASTSet>()->isUnpack();
		for (const auto& val : node.cast<ASTSet>()->values()) {
			if (!first)
				pyc_output << ", ";
			if (isUnpack) {
				pyc_output << "*";
			}
			print_src(val, prev_code);
			first = false;
		}
		pyc_output << " }";
	}
	break;
	case ASTNode::NODE_COMPREHENSION:
	{
		PycRef<ASTComprehension> comp = node.cast<ASTComprehension>();

		pyc_output << "[ ";
		print_src(comp->result(), prev_code);

		for (const auto& gen : comp->generators()) {
			pyc_output << " for ";
			print_src(gen->index(), prev_code);
			pyc_output << " in ";
			print_src(gen->iter(), prev_code);
			if (gen->condition()) {
				pyc_output << " if ";
				print_src(gen->condition(), prev_code);
			}
		}
		pyc_output << " ]";
	}
	break;
	case ASTNode::NODE_MAP_UNPACK:
	{
		pyc_output << "{ ";
		bool first = true;
		for (const auto& val : node.cast<ASTMapUnpack>()->values()) {
			if (!first)
				pyc_output << ", ";
			pyc_output << "**";
			print_src(val, prev_code);
			first = false;
		}
		pyc_output << " }";
	}
	break;
	case ASTNode::NODE_MAP:
	{
		pyc_output << "{ ";
		bool first = true;
		for (const auto& val : node.cast<ASTMap>()->values()) {
			if (!first)
				pyc_output << ", ";
			print_src(val.first, prev_code);
			pyc_output << ": ";
			print_src(val.second, prev_code);
			first = false;
		}
		pyc_output << " }";
	}
	break;
	case ASTNode::NODE_CONST_MAP:
	{
		PycRef<ASTConstMap> const_map = node.cast<ASTConstMap>();
		PycTuple::value_t keys = const_map->keys().cast<ASTObject>()->object().cast<PycTuple>()->values();
		ASTConstMap::values_t values = const_map->values();

		auto map = new ASTMap;
		for (size_t i = 0; i < values.size(); i++) {
			map->addBack(new ASTObject(keys[i]), values[i]);
		}

		print_src(map, prev_code);
	}
	break;
	case ASTNode::NODE_NAME:
	{
		if (inComprehension && strcmp(node.cast<ASTName>()->name()->value(), ".0") == 0) {
			inComprehension = false;
			print_src(comprehension_iterable, prev_code);
		}
		else {
			pyc_output << node.cast<ASTName>()->name()->value();
		}
	}
	break;
	case ASTNode::NODE_NODELIST:
	{
		cur_indent++;
		size_t nodes_count = node.cast<ASTNodeList>()->nodes().size() - 1;
		for (const auto& ln : node.cast<ASTNodeList>()->nodes()) {
			if (ln.cast<ASTNode>().type() != ASTNode::NODE_NODELIST) {
				start_line(cur_indent);
			}
			print_src(ln, prev_code);
			if (nodes_count) {
				end_line();
			}
			nodes_count--;
		}
		cur_indent--;
	}
	break;
	case ASTNode::NODE_BLOCK:
	{
		PycRef<ASTBlock> blk = node.cast<ASTBlock>();
		if (blk->blktype() == ASTBlock::BLK_ELSE)
		{
			if (blk->size() == 0)
			{
				break;
			}
		}
			

		if (blk->blktype() == ASTBlock::BLK_TRY_EXCEPT || blk->blktype() == ASTBlock::BLK_TRY_FINALLY) {
			if (blk->blktype() == ASTBlock::BLK_TRY_FINALLY)
			{
				// if the try-finally has only try-except inside its try,
				// then extract the contents of the try-except
				// so prints it has try-except-finally instead of try-{try-except}-finally
				// has the same logic
				PycRef<ASTBlock> tryBlock = blk->nodes().begin()->cast<ASTBlock>();
				if (tryBlock->hasOnlyBlockOf(ASTBlock::BLK_TRY_EXCEPT))
				{
					// extract inner of try (inner is try-except)
					blk->extractInnerOfFirstBlock();
					// extract inner of try-except (inner is try, except(s), else(?))
					blk->extractInnerOfFirstBlock();
				}
			}
			else if(blk.cast<ASTTryExceptBlock>()->getElseBlock() != NULL)
			{
				blk->append(blk.cast<ASTTryExceptBlock>()->getElseBlock().cast<ASTNode>());
			}
			end_line();
			print_block(blk, prev_code);
			end_line();
			break;
		}

		pyc_output << blk->type_str();
		if (blk->blktype() == ASTBlock::BLK_IF
			|| blk->blktype() == ASTBlock::BLK_ELIF
			|| blk->blktype() == ASTBlock::BLK_WHILE) {
			if (blk.cast<ASTCondBlock>()->negative())
				pyc_output << " not ";
			else
				pyc_output << " ";

			print_src(blk.cast<ASTCondBlock>()->cond(), prev_code);
		}
		else if (blk->blktype() == ASTBlock::BLK_FOR || blk->blktype() == ASTBlock::BLK_ASYNCFOR) {
			pyc_output << " ";
			print_src(blk.cast<ASTIterBlock>()->index(), prev_code);
			pyc_output << " in ";
			print_src(blk.cast<ASTIterBlock>()->iter(), prev_code);
		}
		else if (blk->blktype() == ASTBlock::BLK_EXCEPT)
		{
			// if the block has only try-finally then convert it,
			// its just a cleanup finally but it has the same logic
			if (blk->hasOnlyBlockOf(ASTBlock::BLK_TRY_FINALLY))
			{
				// extract inner of try-finally (inner is try, finally)
				blk->extractInnerOfFirstBlock();
				// remove finally
				blk->removeSecond();
				// extract inner of try
				blk->extractInnerOfFirstBlock();
			}
			if (blk.cast<ASTExceptBlock>()->exceptType() != NULL)
			{
				pyc_output << " ";
				print_src(blk.cast<ASTExceptBlock>()->exceptType(), prev_code);
				if (blk.cast<ASTExceptBlock>()->exceptAs() != NULL)
				{
					pyc_output << " as ";
					print_src(blk.cast<ASTExceptBlock>()->exceptAs(), prev_code);
				}
			}
		}
		else if (blk->blktype() == ASTBlock::BLK_WITH) {
			pyc_output << " ";
			print_src(blk.cast<ASTWithBlock>()->expr(), prev_code);
			PycRef<ASTNode> var = blk.try_cast<ASTWithBlock>()->var();
			if (var != NULL) {
				pyc_output << " as ";
				print_src(var, prev_code);
			}
		}
		pyc_output << ":\n";

		cur_indent++;
		print_block(blk, prev_code);
		cur_indent--;
	}
	break;
	case ASTNode::NODE_OBJECT:
	{
		PycRef<PycObject> obj = node.cast<ASTObject>()->object();
		if (obj.type() == PycObject::TYPE_CODE) {
			PycRef<PycCode> code = obj.cast<PycCode>();
			PycRef<PycSequence> freeVars = code->freeVars();
			cur_indent++;
			for (int i = 0; i < freeVars->size(); i++) {
				start_line(cur_indent);
				pyc_output << "nonlocal ";
				pyc_output << freeVars->get(i).cast<PycString>()->value();
				end_line();
			}
			cur_indent--;
			code->setOuterCells(prev_code->getCellsToBeUsed());
			decompyle(code);
		}
		else {
			print_const(pyc_output, obj, mod);
		}
	}
	break;
	case ASTNode::NODE_PRINT:
	{
		pyc_output << "print ";
		bool first = true;
		if (node.cast<ASTPrint>()->stream() != nullptr) {
			pyc_output << ">>";
			print_src(node.cast<ASTPrint>()->stream(), prev_code);
			first = false;
		}

		for (const auto& val : node.cast<ASTPrint>()->values()) {
			if (!first)
				pyc_output << ", ";
			print_src(val, prev_code);
			first = false;
		}
		if (!node.cast<ASTPrint>()->eol())
			pyc_output << ",";
	}
	break;
	case ASTNode::NODE_RAISE:
	{
		PycRef<ASTRaise> raise = node.cast<ASTRaise>();
		pyc_output << "raise ";
		print_src(raise->exception(), prev_code);
		if (raise->hasExceptionCause()) {
			pyc_output << " from ";
			print_src(raise->exceptionCause(), prev_code);
		}
	}
	break;
	case ASTNode::NODE_RETURN:
	{
		PycRef<ASTReturn> ret = node.cast<ASTReturn>();
		PycRef<ASTNode> value = ret->value();
		if (!inLambda) {
			switch (ret->rettype()) {
			case ASTReturn::RETURN:
				pyc_output << "return ";
				break;
			case ASTReturn::YIELD:
				pyc_output << "yield ";
				break;
			case ASTReturn::YIELD_FROM:
				if (value.type() == ASTNode::NODE_AWAITABLE) {
					pyc_output << "await ";
					value = value.cast<ASTAwaitable>()->expression();
				}
				else {
					pyc_output << "yield from ";
				}
				break;
			}
		}
		print_src(value, prev_code);
	}
	break;
	case ASTNode::NODE_SLICE:
	{
		PycRef<ASTSlice> slice = node.cast<ASTSlice>();

		if (slice->op() & ASTSlice::SLICE1) {
			print_src(slice->left(), prev_code);
		}
		pyc_output << ":";
		if (slice->op() & ASTSlice::SLICE2) {
			print_src(slice->right(), prev_code);
		}
	}
	break;
	case ASTNode::NODE_IMPORT:
	{
		PycRef<ASTImport> import = node.cast<ASTImport>();
		if (import->stores().size()) {
			ASTImport::list_t stores = import->stores();

			pyc_output << "from ";
			if (import->name().type() == ASTNode::NODE_IMPORT)
				print_src(import->name().cast<ASTImport>()->name(), prev_code);
			else
				print_src(import->name(), prev_code);
			pyc_output << " import ";

			if (stores.size() == 1) {
				auto src = stores.front()->src();
				auto dest = stores.front()->dest();
				print_src(src, prev_code);

				if (src.cast<ASTName>()->name()->value() != dest.cast<ASTName>()->name()->value()) {
					pyc_output << " as ";
					print_src(dest, prev_code);
				}
			}
			else {
				bool first = true;
				for (const auto& st : stores) {
					if (!first)
						pyc_output << ", ";
					print_src(st->src(), prev_code);
					first = false;

					if (st->src().cast<ASTName>()->name()->value() != st->dest().cast<ASTName>()->name()->value()) {
						pyc_output << " as ";
						print_src(st->dest(), prev_code);
					}
				}
			}
		}
		else {
			pyc_output << "import ";
			print_src(import->name(), prev_code);
		}
	}
	break;
	case ASTNode::NODE_FUNCTION:
	{
		/* Actual named functions are NODE_STORE with a name */
		PycRef<ASTNode> code = node.cast<ASTFunction>()->code();
		bool is_lambda_a_comprehension = inComprehension;
		if (!is_lambda_a_comprehension) {
			pyc_output << "(lambda ";
			PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
			ASTFunction::defarg_t defargs = node.cast<ASTFunction>()->defargs();
			ASTFunction::defarg_t kwdefargs = node.cast<ASTFunction>()->kwdefargs();
			auto da = defargs.cbegin();
			int narg = 0;
			for (int i = 0; i < code_src->argCount(); i++) {
				if (narg)
					pyc_output << ", ";
				pyc_output << code_src->getLocal(narg++)->value();
				if ((code_src->argCount() - i) <= (int)defargs.size()) {
					pyc_output << " = ";
					print_src(*da++, prev_code);
				}
			}
			da = kwdefargs.cbegin();
			if (code_src->kwOnlyArgCount() != 0) {
				pyc_output << (narg == 0 ? "*" : ", *");
				for (int i = 0; i < code_src->argCount(); i++) {
					pyc_output << ", ";
					pyc_output << code_src->getLocal(narg++)->value();
					if ((code_src->kwOnlyArgCount() - i) <= (int)kwdefargs.size()) {
						pyc_output << " = ";
						print_src(*da++, prev_code);
					}
				}
			}
			pyc_output << ": ";
		}

		inLambda = true;
		print_src(code, prev_code);
		inLambda = false;

		if (!is_lambda_a_comprehension) {
			pyc_output << ")";
		}
	}
	break;
	case ASTNode::NODE_STORE:
	{
		PycRef<ASTNode> src = node.cast<ASTStore>()->src();
		PycRef<ASTNode> dest = node.cast<ASTStore>()->dest();
		if (src.type() == ASTNode::NODE_FUNCTION) {
			PycRef<ASTNode> code = src.cast<ASTFunction>()->code();
			PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
			bool isLambda = false;

			if (strcmp(code_src->name()->value(), "<lambda>") == 0) {
				pyc_output << "\n";
				start_line(cur_indent);
				print_src(dest, prev_code);
				pyc_output << " = lambda ";
				isLambda = true;
			}
			else {
				pyc_output << "\n";
				start_line(cur_indent);
				if (code_src->flags() & PycCode::CO_COROUTINE)
					pyc_output << "async ";
				pyc_output << "def ";
				print_src(dest, prev_code);
				pyc_output << "(";
			}

			ASTFunction::annotMap_t annotMap = src.cast<ASTFunction>()->annotations();
			ASTFunction::defarg_t defargs = src.cast<ASTFunction>()->defargs();
			ASTFunction::defarg_t kwdefargs = src.cast<ASTFunction>()->kwdefargs();

			auto da = defargs.cbegin();
			int narg = 0;
			size_t annot_i = 0;
			for (int i = 0; i < code_src->argCount(); ++i) {
				if (narg)
					pyc_output << ", ";
				const char* param_name = code_src->getLocal(narg++)->value();
				pyc_output << param_name;
				if (annot_i < annotMap.size() && strcmp(param_name, annotMap[annot_i].first->name()->value()) == 0) {
					pyc_output << ": ";
					pyc_output << annotMap[annot_i].second->name()->value();
					annot_i++;
				}
				if ((code_src->argCount() - i) <= (int)defargs.size()) {
					pyc_output << " = ";
					print_src(*da++, prev_code);
				}
			}
			da = kwdefargs.cbegin();
			if (code_src->kwOnlyArgCount() != 0) {
				pyc_output << (narg == 0 ? "*" : ", *");
				for (int i = 0; i < code_src->kwOnlyArgCount(); ++i) {
					pyc_output << ", ";
					const char* param_name = code_src->getLocal(narg++)->value();
					pyc_output << param_name;
					if (annot_i < annotMap.size() && strcmp(param_name, annotMap[annot_i].first->name()->value()) == 0) {
						pyc_output << ": ";
						pyc_output << annotMap[annot_i].second->name()->value();
						annot_i++;
					}
					if ((code_src->kwOnlyArgCount() - i) <= (int)kwdefargs.size()) {
						pyc_output << " = ";
						print_src(*da++, prev_code);
					}
				}
			}
			if (code_src->flags() & PycCode::CO_VARARGS) {
				if (narg)
					pyc_output << ", ";
				pyc_output << "*" << code_src->getLocal(narg++)->value();
			}
			if (code_src->flags() & PycCode::CO_VARKEYWORDS) {
				if (narg)
					pyc_output << ", ";
				pyc_output << "**" << code_src->getLocal(narg++)->value();
			}

			if (isLambda) {
				pyc_output << ": ";
			}
			else {
				pyc_output << "):\n";
				printDocstringAndGlobals = true;
			}

			bool preLambda = inLambda;
			inLambda |= isLambda;

			print_src(code, prev_code);

			inLambda = preLambda;
		}
		else if (src.type() == ASTNode::NODE_CLASS) {
			pyc_output << "\n";
			start_line(cur_indent);
			pyc_output << "class ";
			print_src(dest, prev_code);
			PycRef<ASTTuple> bases = src.cast<ASTClass>()->bases().cast<ASTTuple>();
			if (bases->values().size() > 0) {
				pyc_output << "(";
				bool first = true;
				for (const auto& val : bases->values()) {
					if (!first)
						pyc_output << ", ";
					print_src(val, prev_code);
					first = false;
				}
				pyc_output << "):\n";
			}
			else {
				// Don't put parens if there are no base classes
				pyc_output << ":\n";
			}
			printClassDocstring = true;
			PycRef<ASTNode> code = src.cast<ASTClass>()->code().cast<ASTCall>()
				->func().cast<ASTFunction>()->code();
			print_src(code, prev_code);
		}
		else if (src.type() == ASTNode::NODE_IMPORT) {
			PycRef<ASTImport> import = src.cast<ASTImport>();
			if (import->fromlist() != NULL) {
				PycRef<PycObject> fromlist = import->fromlist().cast<ASTObject>()->object();
				if (fromlist != Pyc_None) {
					pyc_output << "from ";
					if (import->name().type() == ASTNode::NODE_IMPORT)
						print_src(import->name().cast<ASTImport>()->name(), prev_code);
					else
						print_src(import->name(), prev_code);
					pyc_output << " import ";
					if (fromlist.type() == PycObject::TYPE_TUPLE ||
						fromlist.type() == PycObject::TYPE_SMALL_TUPLE) {
						bool first = true;
						for (const auto& val : fromlist.cast<PycTuple>()->values()) {
							if (!first)
								pyc_output << ", ";
							pyc_output << val.cast<PycString>()->value();
							first = false;
						}
					}
					else {
						pyc_output << fromlist.cast<PycString>()->value();
					}
				}
				else {
					pyc_output << "import ";
					print_src(import->name(), prev_code);
				}
			}
			else {
				pyc_output << "import ";
				PycRef<ASTNode> import_name = import->name();
				print_src(import_name, prev_code);
				if (!dest.cast<ASTName>()->name()->isEqual(import_name.cast<ASTName>()->name().cast<PycObject>())) {
					pyc_output << " as ";
					print_src(dest, prev_code);
				}
			}
		}
		else if (src.type() == ASTNode::NODE_BINARY
			&& src.cast<ASTBinary>()->is_inplace()) {
			print_src(src, prev_code);
		}
		else {
			print_src(dest, prev_code);
			pyc_output << " = ";
			print_src(src, prev_code);
		}
	}
	break;
	case ASTNode::NODE_CHAINSTORE:
	{
		for (auto& dest : node.cast<ASTChainStore>()->nodes()) {
			print_src(dest, prev_code);
			pyc_output << " = ";
		}
		print_src(node.cast<ASTChainStore>()->src(), prev_code);
	}
	break;
	case ASTNode::NODE_SUBSCR:
	{
		print_src(node.cast<ASTSubscr>()->name(), prev_code);
		pyc_output << "[";
		print_src(node.cast<ASTSubscr>()->key(), prev_code);
		pyc_output << "]";
	}
	break;
	case ASTNode::NODE_CONVERT:
	{
		pyc_output << "`";
		print_src(node.cast<ASTConvert>()->name(), prev_code);
		pyc_output << "`";
	}
	break;
	case ASTNode::NODE_TUPLE:
	{
		PycRef<ASTTuple> tuple = node.cast<ASTTuple>();
		ASTTuple::value_t values = tuple->values();
		if (tuple->requireParens())
			pyc_output << "(";
		bool first = true;
		for (const auto& val : values) {
			if (!first)
				pyc_output << ", ";
			if (tuple->isUnpack()) {
				pyc_output << "*";
			}
			print_src(val, prev_code);
			first = false;
		}
		if (values.size() == 1)
			pyc_output << ',';
		if (tuple->requireParens())
			pyc_output << ')';
	}
	break;
	case ASTNode::NODE_ANNOTATED_VAR:
	{
		PycRef<ASTAnnotatedVar> annotated_var = node.cast<ASTAnnotatedVar>();
		PycRef<ASTObject> name = annotated_var->name().cast<ASTObject>();
		PycRef<ASTNode> annotation = annotated_var->annotation();

		pyc_output << name->object().cast<PycString>()->value();
		pyc_output << ": ";
		print_src(annotation, prev_code);
	}
	break;
	case ASTNode::NODE_TERNARY:
	{
		/* parenthesis might be needed
		 *
		 * when if-expr is part of numerical expression, ternary has the LOWEST precedence
		 *     print(a + b if False else c)
		 * output is c, not a+c (a+b is calculated first)
		 *
		 * but, let's not add parenthesis - to keep the source as close to original as possible in most cases
		 */
		PycRef<ASTTernary> ternary = node.cast<ASTTernary>();
		//pyc_output << "(";
		print_src(ternary->if_expr(), prev_code);
		const auto if_block = ternary->if_block().cast<ASTCondBlock>();
		pyc_output << " if ";
		if (if_block->negative())
			pyc_output << "not ";
		print_src(if_block->cond(), prev_code);
		pyc_output << " else ";
		print_src(ternary->else_expr(), prev_code);
		//pyc_output << ")";
	}
	break;
	default:
		pyc_output << "<NODE:" << node->type() << ">";
		fprintf(stderr, "Unsupported Node type: %d\n", node->type());
		cleanBuild = false;
		return;
	}

	cleanBuild = true;
}

void ASTree::decompyle(PycRef<PycCode> new_code)
{
	this->m_code = new_code;
	BuildFromCode buildFromCode = BuildFromCode(this->m_code, this->mod);
	PycRef<ASTNode> source = buildFromCode.build();
	cleanBuild = buildFromCode.getCleanBuild();

	PycRef<ASTNodeList> clean = source.cast<ASTNodeList>();
	if (cleanBuild) {
		// The Python compiler adds some stuff that we don't really care
		// about, and would add extra code for re-compilation anyway.
		// We strip these lines out here, and then add a "pass" statement
		// if the cleaned up code is empty
		if (clean->nodes().front().type() == ASTNode::NODE_STORE) {
			PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
			if (store->src().type() == ASTNode::NODE_NAME
				&& store->dest().type() == ASTNode::NODE_NAME) {
				PycRef<ASTName> src = store->src().cast<ASTName>();
				PycRef<ASTName> dest = store->dest().cast<ASTName>();
				if (src->name()->isEqual("__name__")
					&& dest->name()->isEqual("__module__")) {
					// __module__ = __name__
					// Automatically added by Python 2.2.1 and later
					clean->removeFirst();
				}
			}
		}
		if (clean->nodes().front().type() == ASTNode::NODE_STORE) {
			PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
			if (store->src().type() == ASTNode::NODE_OBJECT
				&& store->dest().type() == ASTNode::NODE_NAME) {
				PycRef<ASTObject> src = store->src().cast<ASTObject>();
				PycRef<PycString> srcString = src->object().try_cast<PycString>();
				PycRef<ASTName> dest = store->dest().cast<ASTName>();
				if (srcString != nullptr && srcString->isEqual(m_code->name().cast<PycObject>())
					&& dest->name()->isEqual("__qualname__")) {
					// __qualname__ = '<Class Name>'
					// Automatically added by Python 3.3 and later
					clean->removeFirst();
				}
			}
		}

		// Class and module docstrings may only appear at the beginning of their source
		if (printClassDocstring && clean->nodes().front().type() == ASTNode::NODE_STORE) {
			PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
			if (store->dest().type() == ASTNode::NODE_NAME &&
				store->dest().cast<ASTName>()->name()->isEqual("__doc__") &&
				store->src().type() == ASTNode::NODE_OBJECT) {
				if (print_docstring(store->src().cast<ASTObject>()->object(),
					cur_indent + (m_code->name()->isEqual("<module>") ? 0 : 1)))
					clean->removeFirst();
			}
		}
		while (clean->nodes().back().type() == ASTNode::NODE_RETURN) {
			PycRef<ASTReturn> ret = clean->nodes().back().cast<ASTReturn>();

			if (ret->value() == NULL || ret->value().type() == ASTNode::NODE_LOCALS) {
				clean->removeLast();  // Always an extraneous return statement
			}
		}
	}
	if (printClassDocstring)
		printClassDocstring = false;
	// This is outside the clean check so a source block will always
	// be compilable, even if decompylation failed.
	if (clean->nodes().size() == 0 && !m_code.isIdent(mod->code()))
		clean->append(new ASTKeyword(ASTKeyword::KW_PASS));

	bool part1clean = cleanBuild;

	if (printDocstringAndGlobals) {
		if (m_code->consts()->size())
			print_docstring(m_code->getConst(0), cur_indent + 1);

		PycCode::cells_t outerCells = m_code->getOuterCells();
		PycCode::globals_t globs = m_code->getGlobalsUsed();
		if (globs.size()) {
			bool first = true;
			for (const auto& glob : globs) {
				if (glob.second || outerCells.find(glob.first) != outerCells.end()) {
					if (first) {
						start_line(cur_indent + 1);
						pyc_output << "global ";
					} else
						pyc_output << ", ";
					pyc_output << glob.first->value();
					first = false;
				}
			}
			if (!first)
				pyc_output << "\n";
		}
		PycCode::cells_t cellsStored = m_code->getCellsStored();
		if (cellsStored.size()) {
			start_line(cur_indent + 1);
			pyc_output << "nonlocal ";
			bool first = true;
			for (const auto& cell : cellsStored) {
				if (!first)
					pyc_output << ", ";
				pyc_output << cell->value();
				first = false;
			}
			pyc_output << "\n";
		}
		printDocstringAndGlobals = false;
	}

	print_src(source, new_code);

	if (!cleanBuild || !part1clean) {
		start_line(cur_indent);
		pyc_output << "# WARNING: Decompyle incomplete\n";
	}
}