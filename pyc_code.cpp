#include "pyc_code.h"
#include "pyc_module.h"
#include "data.h"

void PycCode::load(PycData* stream, PycModule* mod)
{
	if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
		m_argCount = stream->get16();
	else if (mod->verCompare(2, 3) >= 0)
		m_argCount = stream->get32();

	if (mod->verCompare(3, 8) >= 0)
		m_posOnlyArgCount = stream->get32();
	else
		m_posOnlyArgCount = 0;

	if (mod->majorVer() >= 3)
		m_kwOnlyArgCount = stream->get32();
	else
		m_kwOnlyArgCount = 0;

	if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
		m_numLocals = stream->get16();
	else if (mod->verCompare(2, 3) >= 0 && mod->verCompare(3, 11) < 0)
		m_numLocals = stream->get32();
	else
		m_numLocals = 0;

	if (mod->verCompare(1, 5) >= 0 && mod->verCompare(2, 3) < 0)
		m_stackSize = stream->get16();
	else if (mod->verCompare(2, 3) >= 0)
		m_stackSize = stream->get32();
	else
		m_stackSize = 0;

	if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
		m_flags = stream->get16();
	else if (mod->verCompare(2, 3) >= 0)
		m_flags = stream->get32();
	else
		m_flags = 0;

	m_code = LoadObject(stream, mod).cast<PycString>();
	m_consts = LoadObject(stream, mod).cast<PycSequence>();
	m_names = LoadObject(stream, mod).cast<PycSequence>();

	if (mod->verCompare(1, 3) >= 0)
		m_localNames = LoadObject(stream, mod).cast<PycSequence>();
	else
		m_localNames = new PycTuple;

	if (mod->verCompare(3, 11) >= 0)
		m_localKinds = LoadObject(stream, mod).cast<PycString>();
	else
		m_localKinds = new PycString;

	if (mod->verCompare(2, 1) >= 0 && mod->verCompare(3, 11) < 0)
		m_freeVars = LoadObject(stream, mod).cast<PycSequence>();
	else
		m_freeVars = new PycTuple;

	if (mod->verCompare(2, 1) >= 0 && mod->verCompare(3, 11) < 0)
		m_cellVars = LoadObject(stream, mod).cast<PycSequence>();
	else
		m_cellVars = new PycTuple;

	m_fileName = LoadObject(stream, mod).cast<PycString>();
	m_name = LoadObject(stream, mod).cast<PycString>();

	if (mod->verCompare(3, 11) >= 0)
		m_qualName = LoadObject(stream, mod).cast<PycString>();
	else
		m_qualName = new PycString;

	if (mod->verCompare(1, 5) >= 0 && mod->verCompare(2, 3) < 0)
		m_firstLine = stream->get16();
	else if (mod->verCompare(2, 3) >= 0)
		m_firstLine = stream->get32();

	if (mod->verCompare(1, 5) >= 0)
		m_lnTable = LoadObject(stream, mod).cast<PycString>();
	else
		m_lnTable = new PycString;

	if (mod->verCompare(3, 11) >= 0)
	{
		m_exceptTable = LoadObject(stream, mod).cast<PycString>();
		this->initExceptTableEntries();
	}
	else
	{
		m_exceptTable = new PycString;
	}
}

PycRef<PycString> PycCode::getCellVar(PycModule* mod, int idx) const
{
	if (mod->verCompare(3, 11) >= 0)
		return getLocal(idx);

	return (idx >= m_cellVars->size())
		? m_freeVars->get(idx - m_cellVars->size()).cast<PycString>()
		: m_cellVars->get(idx).cast<PycString>();
}

unsigned char PycCode::nextByte(std::string::const_iterator& it, const std::string::iterator& end) const
{
	if (it == end) {
		throw EndOfIteratorException();
	}
	return *it++;
}

unsigned int PycCode::parseVarInt(std::string::const_iterator& it, const std::string::iterator& end) const {
	unsigned char b = nextByte(it, end);
	unsigned int val = b & 63;
	while (b & 64) {
		val <<= 6;
		b = nextByte(it, end);
		val |= b & 63;
	}
	return val;
}

void PycCode::initExceptTableEntries() {
	std::string exceptTableStr = std::string(this->exceptTable()->strValue());
	std::vector<ExceptTableEntry> exceptTableEntries;
	std::string::const_iterator it = exceptTableStr.begin();
	try {
		while (true) {
			unsigned int start = parseVarInt(it, exceptTableStr.end()) * 2;
			unsigned int length = parseVarInt(it, exceptTableStr.end()) * 2;
			unsigned int target = parseVarInt(it, exceptTableStr.end()) * 2;
			unsigned int dl = parseVarInt(it, exceptTableStr.end());
			exceptTableEntries.push_back(ExceptTableEntry{
				start,
				(unsigned int)(start + length),
				target,
				(unsigned int)(dl >> 1),
				(bool)(dl & 1)
				});
		}
	}
	catch (const EndOfIteratorException&) {
		this->m_exceptTableEntries = exceptTableEntries;
	}
}
