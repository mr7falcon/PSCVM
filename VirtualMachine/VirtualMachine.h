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
	ASSERT,
	XOR,
	NEG,
	SHL,
	SHR,
	BOR,
	BAND,
	LEN,
};

class VirtualMachine
{
public:

	class ex_argDoesntExists : exception
	{
	public:
		ex_argDoesntExists(const byte i)
			: exception(("Argument does not exists " + std::to_string(i) + "\n").c_str())
		{}
	};

	class ex_zeroDiv : exception
	{
	public:
		ex_zeroDiv()
			: exception("Division by zero\n")
		{}
	};

	inline VirtualMachine()
		: m_nCapacity(64),
		  m_pc(0)
	{
		m_pStack = new Variant[m_nCapacity];
		m_sp = m_pStack + m_nCapacity;
		m_bp = m_pStack - 1;
		
		m_pFirstChunk = m_pCurrentChunk = new HeapChunk;
		m_pCurrentSlot = m_pCurrentChunk->vData;

#ifdef _DEBUG
		log.open("log.log");
		if (!log.is_open())
		{
			throw exception("file opening error");
		}

		g_memVar = 0;
		g_memChunk = 1;
#endif
	}

	inline ~VirtualMachine()
	{
		for (Variant* iter = m_pStack; iter <= m_bp; ++iter)
		{
			if (iter->usNull == Variant::c_null && iter->usType == VarType::STR)
			{
				char* str = (char*)iter->pValue - sizeof(unsigned int);
				delete[](str);
			}
		}

		const Variant* pStop = m_pStack + m_nCapacity;
		for (Variant* iter = m_sp; iter < pStop; ++iter)
		{
			if (iter->usNull == Variant::c_null && iter->usType == VarType::STR)
			{
				char* str = (char*)iter->pValue - sizeof(unsigned int);
				delete[](str);
			}
		}

		HeapChunk* iter = m_pFirstChunk;
		while (iter)
		{
			HeapChunk* next = iter->pNext;
			delete(iter);
			iter = next;
		}

#ifdef _DEBUG
		LogMemory();
#endif
	}

	inline Variant* HeapAlloc(const unsigned int count, bool local = false)
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

#ifdef _DEBUG
			++g_memChunk;
#endif

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

#ifdef _DEBUG
				++g_memChunk;
#endif

				m_pCurrentChunk = m_pCurrentChunk->pNext;
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant((unsigned int)c_nChunkCapacity);

#ifdef _DEBUG
				++g_memVar;
#endif
			}

			m_pCurrentChunk->pNext = new HeapChunk;

#ifdef _DEBUG
			++g_memChunk;
#endif

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

	inline Variant* HeapAllocStructArr(const unsigned int count)
	{
		Variant* pGlobalDesc = m_pCurrentSlot;
		Variant* pLocalDesc = pGlobalDesc;
		short nChunkRemain = (short)(m_pCurrentChunk->vData + c_nChunkSize - pGlobalDesc - 1) / BUCKET_SIZE;
		unsigned int nCountRemain = count;

		if (nChunkRemain <= 0)
		{
			m_pCurrentChunk->pNext = new HeapChunk;

#ifdef _DEBUG
			++g_memChunk;
#endif

			m_pCurrentChunk = m_pCurrentChunk->pNext;
			pLocalDesc = m_pCurrentChunk->vData;

#ifdef _DEBUG
			g_memVar += ((nChunkRemain + 1) * BUCKET_SIZE);
#endif

			nChunkRemain = c_nChunkSizeStruct;
			pGlobalDesc = pLocalDesc;
		}

		if (count < (unsigned short)nChunkRemain)
		{
			*pLocalDesc = Variant(count);
			nCountRemain = nCountRemain * BUCKET_SIZE + 1;
		}
		else
		{
			*pLocalDesc = Variant((const unsigned int)nChunkRemain);
			nCountRemain -= nChunkRemain;

			for (; nCountRemain >= c_nChunkSizeStruct; nCountRemain -= c_nChunkSizeStruct)
			{
				m_pCurrentChunk->pNext = new HeapChunk;

#ifdef _DEBUG
				++g_memChunk;
#endif

				m_pCurrentChunk = m_pCurrentChunk->pNext;
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant((unsigned int)c_nChunkSizeStruct);

#ifdef _DEBUG
				++g_memVar;
#endif
			}

			m_pCurrentChunk->pNext = new HeapChunk;

#ifdef _DEBUG
			++g_memChunk;
#endif

			m_pCurrentChunk = m_pCurrentChunk->pNext;

			if (nCountRemain > 0)
			{
				pLocalDesc->pValue = m_pCurrentChunk->vData;
				pLocalDesc = m_pCurrentChunk->vData;
				*pLocalDesc = Variant(nCountRemain);
				nCountRemain = nCountRemain * BUCKET_SIZE + 1;

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
		g_memVar += (count * BUCKET_SIZE + 1);
		LogMemory();
#endif

		return pGlobalDesc;
	}

	inline Variant* HeapAllocStruct()
	{
		const unsigned short nChunkRemain = (unsigned short)(m_pCurrentChunk->vData + c_nChunkSize - m_pCurrentSlot);
		Variant* p = m_pCurrentSlot;

		if (nChunkRemain < BUCKET_SIZE)
		{
			m_pCurrentChunk->pNext = new HeapChunk;

#ifdef _DEBUG
			++g_memChunk;
#endif

			m_pCurrentChunk = m_pCurrentChunk->pNext;
			p = m_pCurrentChunk->vData;
			m_pCurrentSlot = p + BUCKET_SIZE;

#ifdef _DEBUG
			g_memVar += nChunkRemain;
#endif
		}
		else if (nChunkRemain == BUCKET_SIZE)
		{
			m_pCurrentChunk->pNext = new HeapChunk;

#ifdef _DEBUG
			++g_memChunk;
#endif

			m_pCurrentChunk = m_pCurrentChunk->pNext;
			m_pCurrentSlot = m_pCurrentChunk->vData;
		}
		else
		{
			m_pCurrentSlot += BUCKET_SIZE;
		}

#ifdef _DEBUG
		g_memVar += 3;
		LogMemory();
#endif

		return p;
	}

	inline void Run(byte* program);
	inline Variant Return() { return *m_sp; }
	inline void ProvideArgs(byte* arg0 = nullptr, byte* arg1 = nullptr, byte* arg2 = nullptr, byte* arg3 = nullptr)
	{
		m_bArgs[0] = arg0;
		m_bArgs[1] = arg1;
		m_bArgs[2] = arg2;
		m_bArgs[3] = arg3;
	}

private:

	static const unsigned short c_nChunkSize = 511;
	static const unsigned short c_nChunkCapacity = 510;
	static const unsigned short c_nChunkSizeStruct = 170;

	inline void Resize();

	inline void HeapCollect();
	inline void CheckReferences(Variant* from, Variant* to);
	void HeapMove(Variant* from, Variant* to);
	inline void DictEntryMove(Variant* from, Variant* to, Variant** newBucket);

	Variant FromBytes();

	Variant* m_pStack;
	int m_nCapacity;
	Variant* m_sp;
	Variant* m_bp;
	byte* m_pc;

	struct HeapChunk
	{
		Variant vData[c_nChunkSize];
		HeapChunk* pNext = nullptr;

		inline ~HeapChunk();
	};
	HeapChunk* m_pFirstChunk;
	HeapChunk* m_pCurrentChunk;
	Variant* m_pCurrentSlot;

	static const byte c_bArgsCount = 4;
	byte* m_bArgs[c_bArgsCount] = { nullptr };

#ifdef _DEBUG
	std::ofstream log;
	inline void Log(const string messege)
	{
		log << messege << std::endl;
	}
	void LogMemory()
	{
		log << "Memory: chunk " << g_memChunk << " var " << g_memVar << std::endl;
	}
	void LogTime(const clock_t time)
	{
		log << "Time: " << time << std::endl;
	}

	long g_memVar;
	int g_memChunk;
#endif
};