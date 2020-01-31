#pragma once

#include "Variant.h"

#ifdef _DEBUG
#include <fstream>
#endif

//DO NOT CHANGE ORDER!
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
	NONE,
	CONCAT,
	APOP,
	DERASE,
	PRINT,
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
		short nChunkRemain = (unsigned short)(m_pCurrentChunk->vData + c_nChunkSize - pGlobalDesc - 1);
		unsigned short nCountRemain = count;

		if (!local)
		{
			*pGlobalDesc = Variant(count);
			--nChunkRemain;
			++pLocalDesc;
		}

		if (nChunkRemain <= 0)
		{
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			pLocalDesc = m_pCurrentChunk->vData;
			nChunkRemain = c_nChunkCapacity;
		}

		if (count < nChunkRemain)
		{
			*pLocalDesc = Variant(count);
			++nCountRemain;
			
			if (!local)
			{
				pGlobalDesc->pValue = pLocalDesc;
			}
		}
		else
		{
			*pLocalDesc = Variant((unsigned short)nChunkRemain);
			nCountRemain -= nChunkRemain;

			if (local)
			{
				pGlobalDesc = pLocalDesc;
			}
			else
			{
				pGlobalDesc->pValue = pLocalDesc;
			}

			for (; nCountRemain >= c_nChunkCapacity; nCountRemain -= c_nChunkCapacity)
			{
				m_pCurrentChunk->pNext = new HeapChunk;
				m_pCurrentChunk = m_pCurrentChunk->pNext;
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant(c_nChunkCapacity);
			}

			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;

			if (nCountRemain > 0)
			{
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant(nCountRemain);
				++nCountRemain;
			}
			else
			{
				pLocalDesc = m_pCurrentChunk->vData;
			}
		}

		m_pCurrentSlot = pLocalDesc + nCountRemain;

		return pGlobalDesc;
	}

	static inline Variant* HeapAllocStructArr(const unsigned short count)
	{
		const unsigned short size = 3;
		Variant* pGlobalDesc = m_pCurrentSlot;
		Variant* pLocalDesc = pGlobalDesc;
		short nChunkRemain = (unsigned short)(m_pCurrentChunk->vData + c_nChunkSize - pGlobalDesc - 1) / size;
		unsigned short nCountRemain = count;

		if (nChunkRemain <= 0)
		{
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			pLocalDesc = m_pCurrentChunk->vData;
			nChunkRemain = c_nChunkSizeStruct;
			pGlobalDesc = pLocalDesc;
		}

		if (count < nChunkRemain)
		{
			*pLocalDesc = Variant(count);
			nCountRemain = nCountRemain * size + 1;
		}
		else
		{
			*pLocalDesc = Variant((const unsigned short)nChunkRemain);
			nCountRemain -= nChunkRemain;

			for (; nCountRemain >= c_nChunkSizeStruct; nCountRemain -= c_nChunkSizeStruct)
			{
				m_pCurrentChunk->pNext = new HeapChunk;
				m_pCurrentChunk = m_pCurrentChunk->pNext;
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant(c_nChunkSizeStruct);
			}

			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;

			if (nCountRemain > 0)
			{
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant(nCountRemain);
				nCountRemain = nCountRemain * size + 1;
			}
			else
			{
				pLocalDesc = m_pCurrentChunk->vData;
			}
		}

		m_pCurrentSlot = pLocalDesc + nCountRemain;

		return pGlobalDesc;
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

	static inline void Run(byte* program);

	static inline Variant Return() { return *m_sp; }

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