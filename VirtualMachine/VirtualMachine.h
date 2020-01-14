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

	static inline Variant* HeapAlloc(const unsigned short count, bool local = false)
	{
		Variant* pGlobalDesc = m_pCurrentSlot;
		Variant* pLocalDesc = pGlobalDesc;
		unsigned short nChunkRemain = (unsigned short)(m_pCurrentChunk->vData + c_nChunkSize - pGlobalDesc - 1);

		if (!local)
		{
			*pGlobalDesc = Variant((const unsigned int)count);
			--nChunkRemain;

			if (nChunkRemain > 0)
			{
				*(++pLocalDesc) = Variant((const unsigned int)nChunkRemain);
				pGlobalDesc->pValue = pLocalDesc;
			}
		}
		else
		{
			if (nChunkRemain > 0)
			{
				*pLocalDesc = Variant((const unsigned int)nChunkRemain);
			}
		}

		if (count < nChunkRemain)
		{
			pLocalDesc->nCap = (unsigned int)count;
			m_pCurrentSlot += count;
		}
		else if (count == nChunkRemain)
		{
			pLocalDesc->nCap = (unsigned int)count;
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			m_pCurrentSlot = m_pCurrentChunk->vData;
		}
		else
		{
			unsigned short i = count - nChunkRemain;
			for (; i > c_nChunkCapacity; i -= c_nChunkCapacity)
			{
				m_pCurrentChunk->pNext = new HeapChunk;
				m_pCurrentChunk = m_pCurrentChunk->pNext;
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant((const unsigned int)c_nChunkCapacity);
			}

			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			pLocalDesc->pValue = m_pCurrentChunk->vData;
			pLocalDesc = m_pCurrentChunk->vData;
			*pLocalDesc = Variant((const unsigned int)i);

			if (i == c_nChunkCapacity)
			{
				m_pCurrentChunk->pNext = new HeapChunk;
				m_pCurrentChunk = m_pCurrentChunk->pNext;
				i = 0;
			}

			m_pCurrentSlot = m_pCurrentChunk->vData + i;
		}

		return pGlobalDesc;
	}

	static inline Variant* HeapAllocStructArr(const unsigned short count)
	{
		const unsigned short size = 3;
		Variant* pLocalDesc = m_pCurrentSlot;
		unsigned short nChunkRemain = (unsigned short)(m_pCurrentChunk->vData + c_nChunkSize - m_pCurrentSlot - 1) / size;

		if (nChunkRemain == 0)
		{
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			pLocalDesc = m_pCurrentChunk->vData;
			nChunkRemain = c_nChunkSizeStruct;
		}

		if (count < nChunkRemain)
		{
			*pLocalDesc = Variant((unsigned int)count);
			m_pCurrentSlot = m_pCurrentSlot + count * size;
		}
		else if (count == nChunkRemain)
		{
			*pLocalDesc = Variant((unsigned int)count);
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			m_pCurrentSlot = m_pCurrentChunk->vData;
		}
		else
		{
			*pLocalDesc = Variant((const unsigned int)nChunkRemain);

			unsigned short i = count - nChunkRemain;
			for (; i > nChunkRemain; i -= c_nChunkSizeStruct)
			{
				m_pCurrentChunk->pNext = new HeapChunk;
				m_pCurrentChunk = m_pCurrentChunk->pNext;
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant((const unsigned int)c_nChunkSizeStruct);
			}

			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			pLocalDesc->pValue = m_pCurrentChunk->vData;
			pLocalDesc = m_pCurrentChunk->vData;
			*pLocalDesc = Variant((const unsigned int)i);

			const unsigned short nChunkUsed = i * size;
			if (nChunkUsed == c_nChunkCapacity)
			{
				m_pCurrentChunk->pNext = new HeapChunk;
				m_pCurrentChunk = m_pCurrentChunk->pNext;
				m_pCurrentSlot = m_pCurrentChunk->vData;
			}
			else
			{
				m_pCurrentSlot = m_pCurrentChunk->vData + nChunkUsed;
			}
		}

		return pLocalDesc;
	}

	static inline Variant* HeapAllocStruct()
	{
		const unsigned short size = 3;
		const unsigned short nChunkRemain = (unsigned short)(m_pCurrentChunk->vData + c_nChunkSize - m_pCurrentSlot);
		Variant* p = m_pCurrentSlot;

		if (nChunkRemain < size)
		{
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			p = m_pCurrentChunk->vData;
			m_pCurrentSlot = p + size;
		}
		else if (nChunkRemain == size)
		{
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			m_pCurrentSlot = m_pCurrentChunk->vData;
		}
		else
		{
			m_pCurrentSlot += size;
		}

		return p;
	}

	static inline bool Run(byte* program);

	static inline const string GetStack();
	static inline const Variant* GetStack(int* size);

private:
	VirtualMachine();

	static const unsigned short c_nChunkSize = 511;
	static const unsigned short c_nChunkCapacity = 510;
	static const unsigned short c_nChunkSizeStruct = 170;

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