#pragma once

#include "Variant.h"

#ifdef _DEBUG
#include <fstream>
#include <ctime>
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
	DUP,
	NARG,
	SARG,
};

class VirtualMachine
{
public:
	static const exception ex_argDoesntExists;
	static const exception ex_zeroDiv;

	static inline void Initialize();
	static inline void ShutDown();

	static inline Variant* HeapAlloc(const unsigned int count, bool local = false)
	{
		Variant* pGlobalDesc = m_pCurrentSlot;
		Variant* pLocalDesc = pGlobalDesc;
		short nChunkRemain = (short)(m_pCurrentChunk->vData + c_nChunkSize - pGlobalDesc - 1);
		unsigned int nCountRemain = count;

		if (!local)
		{
			*pGlobalDesc = Variant(count);
			--nChunkRemain;
			++pLocalDesc;

#ifdef _DEBUG
			++g_memVar;
#endif
		}

		if (nChunkRemain <= 0)
		{
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			pLocalDesc = m_pCurrentChunk->vData;

#ifdef _DEBUG
			g_memVar += (nChunkRemain + 1);
#endif

			nChunkRemain = c_nChunkCapacity;
		}

		if (count < (unsigned short)nChunkRemain)
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
			*pLocalDesc = Variant((unsigned int)nChunkRemain);
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
				*pLocalDesc = Variant((unsigned int)c_nChunkCapacity);

#ifdef _DEBUG
				++g_memVar;
#endif
			}

			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;

			if (nCountRemain > 0)
			{
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant(nCountRemain);
				++nCountRemain;

#ifdef _DEBUG
				++g_memVar;
#endif
			}
			else
			{
				pLocalDesc = m_pCurrentChunk->vData;
			}
		}

		m_pCurrentSlot = pLocalDesc + nCountRemain;

#ifdef _DEBUG
		g_memVar += (count + 1);
		LogMemory();
#endif

		return pGlobalDesc;
	}

	static inline Variant* HeapAllocStructArr(const unsigned int count)
	{
		const unsigned short size = 3;
		Variant* pGlobalDesc = m_pCurrentSlot;
		Variant* pLocalDesc = pGlobalDesc;
		short nChunkRemain = (short)(m_pCurrentChunk->vData + c_nChunkSize - pGlobalDesc - 1) / size;
		unsigned int nCountRemain = count;

		if (nChunkRemain <= 0)
		{
			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;
			pLocalDesc = m_pCurrentChunk->vData;

#ifdef _DEBUG
			g_memVar += ((nChunkRemain + 1) * size);
#endif

			nChunkRemain = c_nChunkSizeStruct;
			pGlobalDesc = pLocalDesc;
		}

		if (count < (unsigned short)nChunkRemain)
		{
			*pLocalDesc = Variant(count);
			nCountRemain = nCountRemain * size + 1;
		}
		else
		{
			*pLocalDesc = Variant((const unsigned int)nChunkRemain);
			nCountRemain -= nChunkRemain;

			for (; nCountRemain >= c_nChunkSizeStruct; nCountRemain -= c_nChunkSizeStruct)
			{
				m_pCurrentChunk->pNext = new HeapChunk;
				m_pCurrentChunk = m_pCurrentChunk->pNext;
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant((unsigned int)c_nChunkSizeStruct);

#ifdef _DEBUG
				++g_memVar;
#endif
			}

			m_pCurrentChunk->pNext = new HeapChunk;
			m_pCurrentChunk = m_pCurrentChunk->pNext;

			if (nCountRemain > 0)
			{
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant(nCountRemain);
				nCountRemain = nCountRemain * size + 1;

#ifdef _DEBUG
				++g_memVar;
#endif
			}
			else
			{
				pLocalDesc = m_pCurrentChunk->vData;
			}
		}

		m_pCurrentSlot = pLocalDesc + nCountRemain;

#ifdef _DEBUG
		g_memVar += (count * size + 1);
		LogMemory();
#endif

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

#ifdef _DEBUG
			g_memVar += nChunkRemain;
#endif
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

#ifdef _DEBUG
		g_memVar += 3;
		LogMemory();
#endif

		return p;
	}

	static inline void Run(byte* program);
	static inline Variant Return() { return *m_sp; }
	static inline void ProvideArgs(byte* arg0 = nullptr, byte* arg1 = nullptr, byte* arg2 = nullptr, byte* arg3 = nullptr)
	{
		m_bArgs[0] = arg0;
		m_bArgs[1] = arg1;
		m_bArgs[2] = arg2;
		m_bArgs[3] = arg3;
	}

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

#ifdef _DEBUG
		inline HeapChunk()
		{
			++g_memChunk;
		}
#endif

		inline ~HeapChunk();
	};
	static HeapChunk* m_pFirstChunk;
	static HeapChunk* m_pCurrentChunk;
	static Variant* m_pCurrentSlot;

	static const byte c_bArgsCount = 4;
	static byte* m_bArgs[c_bArgsCount];

#ifdef _DEBUG
	static std::ofstream log;
	static inline void Log(const string messege)
	{
		log << messege << std::endl;
	}
	static void LogMemory()
	{
		log << "Memory: chunk " << g_memChunk << " var " << g_memVar << std::endl;
	}
	static void LogTime(const clock_t time)
	{
		log << "Time: " << time << std::endl;
	}

	static long g_memVar;
	static int g_memChunk;
#endif
};

inline void Run(byte* program)
{
	VirtualMachine::Initialize();
	VirtualMachine::Run(program);
	VirtualMachine::ShutDown();
}

inline double NumRun(byte* program)
{
	VirtualMachine::Initialize();
	VirtualMachine::Run(program);
	double num = VirtualMachine::Return().dValue;
	VirtualMachine::ShutDown();
	return num;
}

inline void StrRun(byte* program, char* res)
{
	VirtualMachine::Initialize();
	VirtualMachine::Run(program);
	Variant var = VirtualMachine::Return();
	memcpy(res, var.pValue, var.nLength);
	*(res + var.nLength) = '\0';
	VirtualMachine::ShutDown();
}