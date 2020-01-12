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
	FARRAY,
	ASTORE,
	AFETCH,
	APUSH,
	DICTIONARY,
	FDICTIONARY,
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
		Variant* p = m_pCurrentSlot;
		const unsigned short nChunkRemain = (unsigned short)(m_pCurrentChunk->vData + c_nChunkSize - p);
		unsigned short nCountRemain = count;

		if (nCountRemain > nChunkRemain)
		{
			nCountRemain -= nChunkRemain;
			const unsigned short chunks = nChunkRemain / c_nChunkSize + 1;
			for (unsigned int i = 0; i < chunks; ++i)
			{
				m_pCurrentChunk->pNext = new HeapChunk;
				m_pCurrentChunk = m_pCurrentChunk->pNext;
			}
			nCountRemain %= c_nChunkSize;
			m_pCurrentSlot = m_pCurrentChunk->vData + nCountRemain;
		}
		else
		{
			m_pCurrentSlot += nCountRemain;
		}

		return p;
	}

	static inline bool Run(byte* program);

	static inline const string GetStack();
	static inline const Variant* GetStack(int* size);

private:
	VirtualMachine();

	static const unsigned short c_nChunkSize = 256;

	static inline void Resize();

	static inline void HeapCollect();
	static inline void CheckReferences(Variant* from, Variant* to);
	static void HeapMove(Variant* from, Variant* to);
	static inline void DictEntryMove(Variant* from, Variant* to, Bucket** newBucket);

	static Variant* m_pStack;
	static int m_nCapacity;
	static Variant* m_sp;
	static Variant* m_bp;

	struct HeapChunk
	{
		Variant vData[c_nChunkSize];
		HeapChunk* pNext = nullptr;

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