#include "BuildFromCode.h"

// cpp file to implement the bytecode manipulations (BcMp) functions

bool BuildFromCode::isOpSeqMatch(OpSeq opcodeSequence, size_t firstSkipOpcodesNum, bool onlyFirstMatch)
{
	return getOpSeqMatchIndex(opcodeSequence, firstSkipOpcodesNum, onlyFirstMatch) != -1;
}

int BuildFromCode::getOpSeqMatchIndex(OpSeq opcodeSequence, size_t firstSkipOpcodesNum, bool onlyFirstMatch)
{
	BcPeeker peekSequence(*this);
	bool isMatch = !onlyFirstMatch;

	for (size_t i = 1; i < firstSkipOpcodesNum; i++)
	{
		peekSequence.peekOne();
	}

	for (OpSeq::iterator it = opcodeSequence.begin();
		it != opcodeSequence.end() &&
		((isMatch && !onlyFirstMatch) ||
			(!isMatch && onlyFirstMatch)); ++it)
	{
		peekSequence.peekOne();
		isMatch = (*it == opcode);
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

bool BuildFromCode::skipCopyPopExceptIfExists(size_t firstSkipOpcodesNum)
{
	return this->skipOpSeqIfExists(OpSeq{ Pyc::COPY_A, Pyc::POP_EXCEPT }, firstSkipOpcodesNum);
}

bool BuildFromCode::isOpcodeReturnAfterN(size_t n)
{
	return this->isOpSeqMatch(OpSeq{ Pyc::RETURN_VALUE }, n);
}
