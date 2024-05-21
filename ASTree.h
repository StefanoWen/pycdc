#ifndef _PYC_ASTREE_H
#define _PYC_ASTREE_H

#include <cstring>
#include <cstdint>
#include <stdexcept>
#include "ASTNode.h"
#include "BuildFromCode.h"

// This must be a triple quote (''' or """), to handle interpolated string literals containing the opposite quote style.
// E.g. f'''{"interpolated "123' literal"}'''    -> valid.
// E.g. f"""{"interpolated "123' literal"}"""    -> valid.
// E.g. f'{"interpolated "123' literal"}'        -> invalid, unescaped quotes in literal.
// E.g. f'{"interpolated \"123\' literal"}'      -> invalid, f-string expression does not allow backslash.
// NOTE: Nested f-strings not supported.
#define F_STRING_QUOTE "'''"

class ASTree
{
public:
	ASTree(PycModule* mod, std::ostream& pyc_output);
	void decompyle(PycRef<PycCode> new_code);
private:
	int cmp_prec(PycRef<ASTNode> parent, PycRef<ASTNode> child);
	void start_line(int indent);
	void end_line();
	void print_block(PycRef<ASTBlock> blk, const PycRef<PycCode>& prev_code);
	void print_formatted_value(PycRef<ASTFormattedValue> formatted_value, const PycRef<PycCode>& prev_code);
	bool print_docstring(PycRef<PycObject> obj, int indent);
	void print_ordered(PycRef<ASTNode> parent, PycRef<ASTNode> child, const PycRef<PycCode>& prev_code);
	void print_src(PycRef<ASTNode> node, const PycRef<PycCode>& prev_code);

	PycRef<PycCode> m_code;
	PycModule* mod;
	std::ostream& pyc_output;

	/* Use this to determine if an error occurred (and therefore, if we should
	* avoid cleaning the output tree) */
	bool cleanBuild;

	/* Use this to prevent printing return keywords and newlines in lambdas. */
	bool inLambda = false;

	/* Use this to keep track of whether we need to print out any docstring and
	 * the list of global variables that we are using (such as inside a function). */
	bool printDocstringAndGlobals = false;

	/* Use this to keep track of whether we need to print a class or module docstring */
	bool printClassDocstring = true;

	int cur_indent;

	/* Use this to prevent printing the lambda and
	* to print directrly the iterable in the list comprehensioninstead of a argument of lambda function */
	bool inComprehension;
	PycRef<ASTNode> comprehension_iterable;
};

#endif
