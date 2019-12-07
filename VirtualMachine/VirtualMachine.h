#pragma once

#include "Variant.h"

#ifdef _DEBUG
#include <fstream>
#endif


enum ByteCommand : byte
{
	CALL,
	RET,
	FETCH,
	STORE,
	LFETCH,
	LSTORE,
	LALLOC,
	LFREE,
	PUSH,
	POP,
	ADD,
	SUB,
	INC,
	DEC,
	MULT,
	DIV,
	MOD,
	AND,
	OR,
	NOT,
	LT,
	GT,
	LET,
	GET,
	EQ,
	NEQ,
	JZ,
	JNZ,
	JMP,
	HALT,
	ARRAY,
	ASTORE,
	AFETCH,
	APUSH,
	DICTIONARY,
	DSTORE,
	DFETCH,
	DINSERT,

	NONE
};

class VirtualMachine
{
public:
	VirtualMachine();
	~VirtualMachine();

	inline Variant* HeapAlloc(const unsigned short count = 1);
	inline void HeapChangeRefs(Variant* from, Variant* to);

	bool Run(byte* program);

	const string GetStack() const;
	const Variant* GetStack(int* size) const;

private:
	static const int c_nChunkSize = 64;
	static const unsigned short c_bReplaced = 0x7FF1;

	inline void Resize();

	void HeapCollect();
	void HeapMove(Variant* from, Variant* to);
	void HeapChangeRefs(Variant* var, Variant* from, Variant* to);

	Variant* m_pStack;
	int m_nCapacity;
	Variant* m_sp;
	Variant* m_bp;

	struct HeapChunk
	{
		HeapChunk* pNext = nullptr;
		Variant vData[c_nChunkSize];

		~HeapChunk();
	};
	HeapChunk* m_pFirstChunk;
	HeapChunk* m_pCurrentChunk;
	Variant* m_pCurrentSlot;

#ifdef _DEBUG
	std::ofstream log;
	inline void Log(const string messege)
	{
		log << messege << std::endl;
	}
#endif
};