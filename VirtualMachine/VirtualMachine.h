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
	static inline void Initialize();
	static inline void ShutDown();

	static inline Variant* HeapAlloc(const unsigned short count = 1)
	{
		//throw any exception in case if count > c_nChunkSize

		if (m_pCurrentSlot + count > m_pCurrentChunk->vData + c_nChunkSize)
		{
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			m_pCurrentSlot = m_pCurrentChunk->vData;
		}

		Variant* p = m_pCurrentSlot;
		m_pCurrentSlot = m_pCurrentSlot + count + 1;

		return p;
	}

	static inline void HeapChangeRefs(Variant* from, Variant* to)
	{
		for (Variant* sp = m_sp; sp < m_pStack + m_nCapacity; ++sp)
		{
			HeapChangeRefs(sp, from, to);
		}
	}

	static inline bool Run(byte* program);

	static inline const string GetStack();
	static inline const Variant* GetStack(int* size);

private:
	VirtualMachine();

	static const int c_nChunkSize = 64;
	static const unsigned short c_bReplaced = 0x7FF1;

	static inline void Resize();

	static inline void HeapCollect();
	static void HeapMove(Variant* from, Variant* to);
	static void HeapChangeRefs(Variant* var, Variant* from, Variant* to);

	static Variant* m_pStack;
	static int m_nCapacity;
	static Variant* m_sp;
	static Variant* m_bp;

	struct HeapChunk
	{
		HeapChunk* pNext = nullptr;
		Variant vData[c_nChunkSize];

		~HeapChunk();
	};
	static HeapChunk* m_pFirstChunk;
	static HeapChunk* m_pCurrentChunk;
	static Variant* m_pCurrentSlot;

#ifdef _DEBUG
	static std::ofstream log;
	static inline void Log(const string messege)
	{
		log << messege << std::endl;
	}
#endif
};