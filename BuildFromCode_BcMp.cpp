#include "BuildFromCode.h"

// cpp file to implement the bytecode manipulations (BcMp) functions

bool BuildFromCode::isOpSeqMatch(OpSeq opcodeSequence, size_t firstSkipOpcodesNum, bool onlyFirstMatch)
{
	return getOpSeqMatchIndex(opcodeSequence, firstSkipOpcodesNum, onlyFirstMatch) != -1;
}

int BuildFromCode::getOpSeqMatchIndex(OpSeq opcodeSequence, size_t firstSkipOpcodesNum, bool onlyFirstMatch)
{
	BcPeeker peekSequence(*this, firstSkipOpcodesNum);
	bool isMatch = !onlyFirstMatch;

	for (OpSeq::iterator it = opcodeSequence.begin();
		it != opcodeSequence.end() &&
		((isMatch && !onlyFirstMatch) ||
			(!isMatch && onlyFirstMatch)); ++it)
	{
		isMatch = (*it == opcode);
		peekSequence.peekOne();
	}
	return (isMatch) ? (int)bc_i : -1;
}

bool BuildFromCode::skipOpSeqIfExists(OpSeq opcodeSequence, size_t firstSkipOpcodesNum)
{
	int new_bc_i = this->getOpSeqMatchIndex(opcodeSequence, firstSkipOpcodesNum);
	if (new_bc_i != -1)
	{
		this->bc_set((size_t)new_bc_i + 1);
		return true;
	}
	else
	{
		return false;
	}
}

bool BuildFromCode::skipCopyPopExceptReraiseIfExists(size_t firstSkipOpcodesNum)
{
	return this->skipOpSeqIfExists(OpSeq{ Pyc::COPY_A , Pyc::POP_EXCEPT , Pyc::RERAISE_A }, firstSkipOpcodesNum);
}

bool BuildFromCode::isOpcodeReturnAfterN(size_t n)
{
	return this->isOpSeqMatch(OpSeq{ Pyc::RETURN_VALUE }, n);
}
