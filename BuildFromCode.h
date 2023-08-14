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
	void bc_next();
	void append_to_chain_store(const PycRef<ASTNode>& chainStore, PycRef<ASTNode> item);
	PycRef<ASTNode> StackPopTop();
	void checkIfExpr();
	void binary_or_inplace();
	void checker();
	void switchOpcode();
	void end_finally();
	void add_finally_block();
	void add_except_block();
	void pop_except();
	void pop_try_except_or_try_finally_block();

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
	bool variable_annotations;

	/* Use this to determine if an error occurred (and therefore, if we should
	* avoid cleaning the output tree) */
	bool cleanBuild;

	bool check_is_try_finally;
	int comprehension_counter;
};

#endif