#ifndef _BUILDFROMCODE_H
#define _BUILDFROMCODE_H

#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <exception>
#include "FastStack.h"
#include "pyc_numeric.h"
#include "bytecode.h"

class UnsupportedOpcodeException : public std::exception
{
	const char* what() const throw ()
	{
		return "Error: Unsupported Opcode";
	}
};

class BuildFromCode
{
public:
	BuildFromCode(PycRef<PycCode> param_code, PycModule* param_mod);
	virtual ~BuildFromCode();
	virtual PycRef<ASTNode> build();
	bool getCleanBuild() const;
private:
	void append_to_chain_store(const PycRef<ASTNode>& chainStore, PycRef<ASTNode> item);
	PycRef<ASTNode> StackPopTop();
	void CheckIfExpr();
	void binary_or_inplace();
	void checker();
	void switchOpcode();
	void begin_finally();
	void end_finally();

	PycRef<PycCode> code;
	PycModule* mod;
	PycBuffer source;
	FastStack stack;
	stackhist_t stack_hist;
	std::stack<PycRef<ASTBlock> > blocks;
	PycRef<ASTBlock> defblock;
	PycRef<ASTBlock> curblock;

	int opcode, operand;
	int curpos;
	int pos;
	int unpack;
	bool else_pop;
	bool need_try;
	bool variable_annotations;

	/* Use this to determine if an error occurred (and therefore, if we should
	* avoid cleaning the output tree) */
	bool cleanBuild;

	bool check_if_finally_begins_now;
	bool is_except_begins_now;
	int comprehension_counter;
};

#endif