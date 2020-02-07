#include "VirtualMachine.h"
#include <cstring>
#include <iostream>

Variant* VirtualMachine::m_pStack;
VirtualMachine::HeapChunk* VirtualMachine::m_pFirstChunk;
VirtualMachine::HeapChunk* VirtualMachine::m_pCurrentChunk;
Variant* VirtualMachine::m_sp;
Variant* VirtualMachine::m_pCurrentSlot;
Variant* VirtualMachine::m_bp;
int VirtualMachine::m_nCapacity;
#ifdef _DEBUG
std::ofstream VirtualMachine::log;
long VirtualMachine::g_memVar;
int VirtualMachine::g_memChunk;
#endif

void VirtualMachine::Initialize()
{
	m_nCapacity = 64;
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

void VirtualMachine::ShutDown()
{
	for (Variant* iter = m_pStack; iter < m_pStack + m_nCapacity; ++iter)
	{
		if (iter->usNull == Variant::c_null && iter->usType == VarType::STR)
		{
			delete[]((char*)iter->pValue);
			iter->pValue = nullptr;
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

inline void VirtualMachine::Resize()
{
#ifdef _DEBUG
	Log("Resize " + std::to_string(m_nCapacity));
#endif

	HeapCollect(); //is here the best place for it?

	const int inc = m_nCapacity >> 1;
	Variant* pStack = new Variant[m_nCapacity + inc];
	memcpy(pStack, m_pStack, (byte*)(m_bp + 1) - (byte*)m_pStack);
	Variant* sp = pStack + (m_sp - m_pStack + inc);
	memcpy(sp, m_sp, (byte*)(m_pStack + m_nCapacity) - (byte*)m_sp);
	m_sp = sp;
	m_bp = m_bp >= m_pStack ? pStack + (m_bp - m_pStack) : pStack - 1;
	delete[](m_pStack);
	m_pStack = pStack;
	m_nCapacity = m_nCapacity + inc;
}

void VirtualMachine::DictEntryMove(Variant* from, Variant* to, Bucket** newBucket)
{
	Bucket* pBucket;
	Bucket* pNewBucket;

	if (from->pValue)
	{
		to->lValue = from->lValue;
		void** prev = &to->pValue;
		pBucket = (Bucket*)from->pValue;

		while (pBucket)
		{
			pNewBucket = *newBucket;
			HeapMove(&pBucket->key, &pNewBucket->key);
			HeapMove(&pBucket->value, &pNewBucket->value);
			*prev = pNewBucket;
			pBucket = (Bucket*)pBucket->next.pValue;
			prev = &pNewBucket->next.pValue;
			++(*newBucket);
		}
	}
}

void VirtualMachine::CheckReferences(Variant* from, Variant* to)
{
	Variant* pValue = (Variant*)from->pValue;
	if (pValue)
	{
		const unsigned short type = from->usType;

		if (type == VarType::STR)
		{
			to->pValue = pValue;
			return;
		}

		if (!pValue->usReplaced)
		{
			const unsigned short length = from->usLength;
			Variant* pGlobDesc = pValue;
			const unsigned short capacity = pGlobDesc->usCap;
			Variant* iter = HeapAlloc(capacity);
			to->pValue = iter;
			Variant* pIterLocalDesc = (Variant*)iter->pValue;
			iter = pIterLocalDesc + 1;
			Variant* iterStop = iter + pIterLocalDesc->usCap;
			Variant* pLocalDesc = (Variant*)pGlobDesc->pValue;
			Variant* pArr = pLocalDesc + 1;
			Variant* stop = pArr + pLocalDesc->usCap;

			if (type == ARR)
			{
				for (unsigned short i = 0; i < length; ++i)
				{
					if (pArr == stop)
					{
						pLocalDesc = (Variant*)pLocalDesc->pValue;
						pArr = pLocalDesc + 1;
						stop = pArr + pLocalDesc->usCap;
					}

					if (iter == iterStop)
					{
						pIterLocalDesc = (Variant*)pIterLocalDesc->pValue;
						iter = pLocalDesc + 1;
						iterStop = iter + pIterLocalDesc->usCap;
					}

					HeapMove(pArr, iter);
					++pArr;
					++iter;
				}
			}
			else if (type == VarType::DICT)
			{
				Variant* pBucketDesc = HeapAllocStructArr(length);
				unsigned short nBucketCap = pBucketDesc->usCap;
				Bucket* pBucketArr = (Bucket*)(pBucketDesc + 1);
				Bucket* pNewBucket = pBucketArr;
				Bucket* pBucket;

				while (pLocalDesc)
				{
					const unsigned short capacity = pLocalDesc->usCap;
					
					for (; pArr <= pLocalDesc + capacity; ++pArr, ++iter)
					{
						if (from->pValue)
						{
							to->lValue = from->lValue;
							void** prev = &to->pValue;
							pBucket = (Bucket*)from->pValue;

							while (pBucket)
							{
								HeapMove(&pBucket->key, &pNewBucket->key);
								HeapMove(&pBucket->value, &pNewBucket->value);
								*prev = pNewBucket;
								pBucket = (Bucket*)pBucket->next.pValue;
								prev = &pNewBucket->next.pValue;
								
								if (++pNewBucket == pBucketArr + nBucketCap)
								{
									pBucketDesc = (Variant*)pBucketDesc->pValue;
									nBucketCap = pBucketDesc->usCap;
									pBucketArr = (Bucket*)(pBucketDesc + 1);
									pNewBucket = pBucketArr;
								}
							}
						}
					}

					pLocalDesc = (Variant*)pLocalDesc->pValue;
					pArr = pLocalDesc + 1;
				}
			}

			pGlobDesc->usReplaced = 1;
			pGlobDesc->pValue = to->pValue;
		}
		else
		{
			to->pValue = pValue->pValue;
		}
	}
}

void VirtualMachine::HeapCollect()
{
	HeapChunk* pFirstChunk = m_pCurrentChunk = new HeapChunk;
	m_pCurrentSlot = m_pCurrentChunk->vData;

	for (Variant* sp = m_sp; sp < m_pStack + m_nCapacity; ++sp)
	{
		CheckReferences(sp, sp);
	}

	HeapChunk* iter = m_pFirstChunk;
	m_pFirstChunk = pFirstChunk;
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

void VirtualMachine::HeapMove(Variant* from, Variant* to)
{
	from->Copy();
	to->dValue = from->dValue;

	CheckReferences(from, to);
}

VirtualMachine::HeapChunk::~HeapChunk()
{
	for (Variant* iter = vData; iter < vData + c_nChunkSize; ++iter)
	{
		iter->Free();
	}

#ifdef _DEBUG
	const unsigned short diff = (unsigned short)(m_pCurrentSlot - vData);
	if (diff < c_nChunkCapacity)
	{
		g_memVar -= diff;
	}
	else
	{
		g_memVar -= c_nChunkCapacity;
	}
	--g_memChunk;
#endif
}

void VirtualMachine::Run(byte* program)
{
	byte* pc = program;

	while (true)
	{
		switch ((ByteCommand) * (pc++))
		{
		case ByteCommand::CALL:
		{
			const int mark = *((long*)pc);
			pc += sizeof(long long);

#ifdef _DEBUG
			Log("CALL " + std::to_string(mark));
#endif

			if (m_bp + 2 >= m_sp)
			{
				Resize();
			}
			(++m_bp)->lValue = (long)(pc - program);
			pc = program + mark;
		}
		break;
		case ByteCommand::RET:
		{
#ifdef _DEBUG
			Log("RET");
#endif

			pc = program + (long)(m_bp--)->lValue;
		}
		break;
		case ByteCommand::FETCH:
		{
			if (m_sp - 1 == m_bp)
			{
				Resize();
			}
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("FETCH " + std::to_string(offset));
#endif

			* (--m_sp) = *(m_pStack + m_nCapacity - offset);
			m_sp->Copy();
		}
		break;
		case ByteCommand::STORE:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("STORE " + m_sp->ToString() + " " + std::to_string(offset));
#endif

			* (m_pStack + m_nCapacity - offset) = *(m_sp++);
		}
		break;
		case ByteCommand::LFETCH:
		{
			if (m_sp - 1 == m_bp)
			{
				Resize();
			}
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("LFETCH " + std::to_string(offset));
#endif

			* (--m_sp) = *(m_bp - offset);
			m_sp->Copy();
		}
		break;
		case ByteCommand::LSTORE:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("LSTORE " + m_sp->ToString() + " " + std::to_string(offset));
#endif

			* (m_bp - offset) = *(m_sp++);
		}
		break;
		case ByteCommand::AFETCH:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

			if (m_sp->usNull == Variant::c_null)
			{
				throw Variant::ex_wrongType;
			}

			const unsigned short index = (unsigned short)m_sp->dValue;

#ifdef _DEBUG
			Log("AFETCH " + std::to_string(offset) + " " + std::to_string(index));

			const clock_t tStart = clock();
#endif

			Variant* arr = m_pStack + m_nCapacity - offset;
			if (index >= arr->usLength)
			{
				throw Variant::ex_outOfBounds;
			}
			Variant* var = arr->Get(index);
			var->Copy();
			*m_sp = *var;

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::ASTORE:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

			if (m_sp->usNull == Variant::c_null)
			{
				throw Variant::ex_wrongType;
			}

			const unsigned short index = (unsigned short)(m_sp++)->dValue;

#ifdef _DEBUG
			Log("ASTORE " + m_sp->ToString() + " " + std::to_string(offset) + " " + std::to_string(index));

			const clock_t tStart = clock();
#endif

			Variant* arr = m_pStack + m_nCapacity - offset;
			if (index >= arr->usLength)
			{
				throw Variant::ex_outOfBounds;
			}
			*(arr->Get(index)) = *(m_sp++);

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::APUSH:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("APUSH " + m_sp->ToString() + " " + std::to_string(offset));

			const clock_t tStart = clock();
#endif

			(m_pStack + m_nCapacity - offset)->PushBack(m_sp++);

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::DFETCH:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("DFETCH " + std::to_string(offset) + " " + m_sp->ToString());

			const clock_t tStart = clock();
#endif

			Variant* val = (m_pStack + m_nCapacity - offset)->Find(m_sp);
			val->Copy();
			m_sp->Free();
			*m_sp = *val;

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::DSTORE:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

			Variant* key = m_sp++;

#ifdef _DEBUG
			Log("DSTORE " + m_sp->ToString() + " " + std::to_string(offset) + " " + key->ToString());

			const clock_t tStart = clock();
#endif

			* ((m_pStack + m_nCapacity - offset)->Find(key)) = *(m_sp++);
			key->Free();

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::DINSERT:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

			Variant* key = m_sp++;

#ifdef _DEBUG
			Log("DINS " + m_sp->ToString() + " " + std::to_string(offset) + " " + key->ToString());

			const clock_t tStart = clock();
#endif

			(m_pStack + m_nCapacity - offset)->Insert(key, m_sp++);

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::LALLOC:
		{
			const int size = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("LALLOC " + std::to_string(size));
#endif

			while (m_bp + size + 1 >= m_sp)
			{
				Resize();
			}
			m_bp = m_bp + size + 1;
			m_bp->dValue = size;
		}
		break;
		case ByteCommand::LFREE:
		{
#ifdef _DEBUG
			Log("LFREE");
#endif

			Variant* bp = m_bp - 1 - (int)(m_bp--)->dValue;
			while (m_bp > bp)
			{
				(m_bp--)->Free();
			}
		}
		break;
		case ByteCommand::ARRAY:
		{
			if (m_sp->usNull == Variant::c_null)
			{
				throw Variant::ex_wrongType;
			}
			
			const unsigned short len = (unsigned short)m_sp->dValue;
			
#ifdef _DEBUG
			Log("ARR " + std::to_string(len));

			const clock_t tStart = clock();
#endif

			unsigned short capacity = len + (len >> Variant::c_capInc);
			const unsigned short minCapacity = 8;
			if (capacity < minCapacity)
			{
				capacity = minCapacity;
			}
			Variant* arr = VirtualMachine::HeapAlloc(capacity);
			*m_sp = Variant(arr, len, VarType::ARR);

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::DICTIONARY:
		{
			if (m_sp->usNull == Variant::c_null)
			{
				throw Variant::ex_wrongType;
			}
			
			const unsigned short len = (unsigned short)m_sp->dValue;
			
#ifdef _DEBUG
			Log("DICT " + std::to_string(len));

			const clock_t tStart = clock();
#endif

			const unsigned short capacity = GetPrime(len + (len >> Variant::c_capInc));
			Variant* dict = VirtualMachine::HeapAlloc(capacity);
			*m_sp = Variant(dict, 0, VarType::DICT);

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::CONCAT:
		{
			Variant* op2 = m_sp;
			Variant* op1 = ++m_sp;

#ifdef _DEBUG
			Log("CONCAT " + op1->ToString() + " " + op2->ToString());

			const clock_t tStart = clock();
#endif

			op1->Concat(op2);
			op2->Free();

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::APOP:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("APOP " + std::to_string(offset));

			const clock_t tStart = clock();
#endif

			(m_pStack + m_nCapacity - offset)->PopBack();

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::DERASE:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("DERASE " + std::to_string(offset) + " " + m_sp->ToString());

			const clock_t tStart = clock();
#endif

			(m_pStack + m_nCapacity - offset)->Erase(m_sp);
			(m_sp++)->Free();

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::PUSH:
		{
			if (m_sp - 1 == m_bp)
			{
				Resize();
			}
			*(--m_sp) = Variant::FromBytes(&pc);

#ifdef _DEBUG
			Log("PUSH " + m_sp->ToString());
#endif
		}
		break;
		case ByteCommand::POP:
		{
#ifdef _DEBUG
			Log("POP " + m_sp->ToString());
#endif

			(m_sp++)->Free();
		}
		break;
		case ByteCommand::ADD:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("ADD " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue += op2->dValue;
		}
		break;
		case ByteCommand::SUB:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("SUB " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue -= op2->dValue;
		}
		break;
		case ByteCommand::INC:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("INC " + std::to_string(offset));
#endif

			Variant* var = (m_pStack + m_nCapacity - offset);
			var->Free();
			++var->dValue;
		}
		break;
		case ByteCommand::DEC:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("DEC " + std::to_string(offset));
#endif

			Variant* var = (m_pStack + m_nCapacity - offset);
			var->Free();
			--var->dValue;
		}
		break;
		case ByteCommand::MULT:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("MULT " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue *= op2->dValue;
		}
		break;
		case ByteCommand::DIV:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("DIV " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();

			if (op2->dValue != 0.0)
			{
				op1->dValue /= op2->dValue;
			}
			else
			{
				op1->usNull = Variant::c_null;
			}
		}
		break;
		case ByteCommand::MOD:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("MOD " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();

			if (op2->dValue != 0.0)
			{
				op1->dValue = (int)op1->dValue % (int)op2->dValue;
			}
			else
			{
				op1->usNull = Variant::c_null;
			}
		}
		break;
		case ByteCommand::AND:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("AND " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue = (op1->dValue != 0.0 && op2->dValue != 0.0
				&& op1->usNull != Variant::c_null && op2->usNull != Variant::c_null) ? 1.0 : 0.0;
		}
		break;
		case ByteCommand::OR:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("OR " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue = (op1->dValue != 0.0 && op1->usNull != Variant::c_null
				|| op2->dValue != 0.0 && op2->usNull != Variant::c_null) ? 1.0 : 0.0;
		}
		break;
		case ByteCommand::NOT:
		{
#ifdef _DEBUG
			Log("NOT " + m_sp->ToString());
#endif

			m_sp->dValue = (m_sp->dValue == 0.0 || m_sp->usNull == Variant::c_null) ? 1.0 : 0.0;
			m_sp->Free();
		}
		break;
		case ByteCommand::LT:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("LT " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue = op1->dValue < op2->dValue ? 1.0 : 0.0;
		}
		break;
		case ByteCommand::GT:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("GT " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue = op1->dValue > op2->dValue ? 1.0 : 0.0;
		}
		break;
		case ByteCommand::LET:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("LET " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue = op1->dValue <= op2->dValue ? 1.0 : 0.0;
		}
		break;
		case ByteCommand::GET:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("GET " + op1->ToString() + " " + op2->ToString());
#endif

			op1->Free();
			op2->Free();
			op1->dValue = op1->dValue >= op2->dValue ? 1.0 : 0.0;
		}
		break;
		case ByteCommand::EQ:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("EQ " + op1->ToString() + " " + op2->ToString());

			const clock_t tStart = clock();
#endif

			const double bRes = Variant::Equal(op1, op2) ? 1.0 : 0.0;
			op1->Free();
			op2->Free();
			op1->dValue = bRes;

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::NEQ:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("NEQ " + op1->ToString() + " " + op2->ToString());

			const clock_t tStart = clock();
#endif

			const double bRes = Variant::Equal(op1, op2) ? 0.0 : 1.0;
			op1->Free();
			op2->Free();
			op1->dValue = bRes;

#ifdef _DEBUG
			const clock_t tEnd = clock();
			LogTime(tEnd - tStart);
#endif
		}
		break;
		case ByteCommand::JZ:
		{
			if ((m_sp++)->dValue == 0.0)
			{
				const long offset = *((long*)pc);
				pc = program + offset;

#ifdef _DEBUG
				Log("JZ " + std::to_string(offset) + " TRUE");
#endif
			}
			else
			{
				pc += sizeof(long long);

#ifdef _DEBUG
				Log("JZ FALSE");
#endif
			}
		}
		break;
		case ByteCommand::JNZ:
		{
			if ((m_sp++)->dValue != 0.0)
			{
				const long offset = *((long*)pc);
				pc = program + offset;

#ifdef _DEBUG
				Log("JNZ " + std::to_string(offset) + " TRUE");
#endif
			}
			else
			{
				pc += sizeof(long long);

#ifdef _DEBUG
				Log("JNZ FALSE");
#endif
			}
		}
		break;
		case ByteCommand::JMP:
		{
			const long offset = *((long*)pc);
			pc = program + offset;

#ifdef _DEBUG
			Log("JMP " + std::to_string(offset));
#endif
		}
		break;
		case ByteCommand::PRINT:
		{
#ifdef _DEBUG
			Log("PRINT");
#endif

			std::cout << m_sp->ToString() << std::endl;
			(m_sp++)->Free();
		}
		break;
		case ByteCommand::NONE:
		{
#ifdef _DEBUG
			Log("NONE");
#endif
		}
		break;
		case ByteCommand::HALT:
		{
#ifdef _DEBUG
			Log("HALT");
#endif

			return;
		}
		break;
		case ByteCommand::DUP:
		{
#ifdef _DEBUG
			Log("DUP " + m_sp->ToString());
#endif

			*m_sp = m_sp->Duplicate();
		}
		break;
		default:
		{
			return;
		}
		}
	}
}

extern "C"
{
	__declspec(dllexport) void __stdcall Run(byte* program)
	{
		VirtualMachine::Initialize();
		VirtualMachine::Run(program);
		VirtualMachine::ShutDown();
	}

	__declspec(dllexport) double __stdcall NumRun(byte* program)
	{
		VirtualMachine::Initialize();
		VirtualMachine::Run(program);
		double num = VirtualMachine::Return().dValue;
		VirtualMachine::ShutDown();
		return num;
	}

	__declspec(dllexport) void __stdcall StrRun(byte* program, char* res)
	{
		VirtualMachine::Initialize();
		VirtualMachine::Run(program);
		Variant var = VirtualMachine::Return();
		memcpy(res, var.pValue, var.usLength);
		*(res + var.usLength) = '\0';
		VirtualMachine::ShutDown();
	}
}