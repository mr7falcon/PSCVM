#pragma once

#include "Variant.h"

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

	NONE
};

class VirtualMachine
{
public:
	VirtualMachine();

	inline Variant* HeapAlloc(const int count = 1);

	bool Run(byte* program);

	const string GetStack() const;
	const Variant* GetStack(int* size) const;

private:
	static const int c_nChunkSize = 64;

	inline void Resize();

	void HeapCollect();
	void HeapMove(Variant* from, Variant* to);

	Variant* m_pStack;
	int m_nCapacity;
	Variant* m_sp;
	Variant* m_bp;

	struct HeapChunk
	{
		HeapChunk* pNext = nullptr;
		Variant vData[c_nChunkSize];
	};
	HeapChunk* m_pFirstChunk;
	HeapChunk* m_pCurrentChunk;
	Variant* m_pCurrentSlot;
};