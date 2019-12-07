#include "VirtualMachine.h"
#include <cstring>
#include <iostream>

VirtualMachine::VirtualMachine()
	: m_nCapacity(64)
{
	m_pStack = new Variant[m_nCapacity];
	m_sp = m_pStack + m_nCapacity;
	m_bp = m_pStack - 1;

	m_pFirstChunk = m_pCurrentChunk = new HeapChunk;
	m_pCurrentSlot = m_pCurrentChunk->vData;

#ifdef _DEBUG
	log.open("log.log");
	//throw any exception if file did not opened
#endif
}

VirtualMachine::~VirtualMachine()
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

inline Variant* VirtualMachine::HeapAlloc(const unsigned short count)
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

void VirtualMachine::HeapCollect()
{
	HeapChunk* pFirstChunk = m_pCurrentChunk = new HeapChunk;
	m_pCurrentSlot = m_pCurrentChunk->vData;

	for (Variant* sp = m_sp; sp < m_pStack + m_nCapacity; ++sp)
	{
		if (sp->usNull == Variant::c_null)
		{
			if (sp->usType == VarType::ARR)
			{
				if (((Variant*)sp->pValue)->usNull != c_bReplaced)
				{
					Variant* pArr = (Variant*)sp->pValue;
					const unsigned short capacity = (unsigned short)(pArr - 1)->dValue + 1;
					Variant* iter = HeapAlloc(capacity);
					sp->pValue = iter + 1;
					for (Variant* p = pArr - 1; p < pArr + sp->usLength; ++p, ++iter)
					{
						HeapMove(p, iter);
					}
					pArr->usNull = c_bReplaced;
					pArr->pValue = sp->pValue;
				}
				else
				{
					sp->pValue = ((Variant*)sp->pValue)->pValue;
				}
			}
			else if (sp->usType == VarType::DICT)
			{
				if (((Variant*)sp->pValue)->usNull != c_bReplaced)
				{
					Variant* pDict = (Variant*)sp->pValue;
					const unsigned int capacity = (unsigned int)(pDict - 1)->dValue + 1;
					Variant* iter = HeapAlloc(capacity);
					sp->pValue = iter + 1;
					const unsigned int len = (unsigned int)sp->usLength << 1;
					for (Variant* p = pDict - 1; p < pDict + len; ++p, ++iter)
					{
						HeapMove(p, iter);
					}
					pDict->usNull = c_bReplaced;
					pDict->pValue = sp->pValue;
				}
				else
				{
					sp->pValue = ((Variant*)sp->pValue)->pValue;
				}
			}
		}
	}

	HeapChunk* iter = m_pFirstChunk;
	m_pFirstChunk = pFirstChunk;
	while (iter)
	{
		HeapChunk* next = iter->pNext;
		delete(iter);
		iter = next;
	}
}

void VirtualMachine::HeapMove(Variant* from, Variant* to)
{
	from->Copy();
	to->dValue = from->dValue;
	to->pValue = from->pValue;

	if (to->usNull == Variant::c_null)
	{
		if (to->usType == VarType::ARR)
		{
			if (((Variant*)to->pValue)->usNull != c_bReplaced)
			{
				Variant* pArr = (Variant*)to->pValue;
				const unsigned short capacity = (unsigned short)(pArr - 1)->dValue + 1;
				Variant* iter = HeapAlloc(capacity);
				to->pValue = (void*)iter;
				for (Variant* p = pArr - 1; p < pArr + to->usLength; ++p, ++iter)
				{
					HeapMove(p, iter);
				}
				pArr->usNull = c_bReplaced;
				pArr->pValue = to->pValue;
			}
			else
			{
				to->pValue = ((Variant*)to->pValue)->pValue;
			}
		}
		else if (to->usType == VarType::DICT)
		{
			if (((Variant*)to->pValue)->usNull != c_bReplaced)
			{
				Variant* pDict = (Variant*)to->pValue;
				const unsigned int capacity = (unsigned int)(pDict - 1)->dValue + 1;
				Variant* iter = HeapAlloc(capacity);
				to->pValue = iter + 1;
				const unsigned int len = (unsigned int)to->usLength << 1;
				for (Variant* p = pDict - 1; p < pDict + len; ++p, ++iter)
				{
					HeapMove(p, iter);
				}
				pDict->usNull = c_bReplaced;
				pDict->pValue = to->pValue;
			}
			else
			{
				to->pValue = ((Variant*)to->pValue)->pValue;
			}
		}
	}
}

VirtualMachine::HeapChunk::~HeapChunk()
{
	for (Variant* iter = vData; iter < vData + c_nChunkSize; ++iter)
	{
		iter->Free();
	}
}

inline void VirtualMachine::HeapChangeRefs(Variant* from, Variant* to)
{
	for (Variant* sp = m_sp; sp < m_pStack + m_nCapacity; ++sp)
	{
		HeapChangeRefs(sp, from, to);
	}
}

void VirtualMachine::HeapChangeRefs(Variant* var, Variant* from, Variant* to)
{
	if (var->usNull == Variant::c_null)
	{
		if (var->usType == VarType::ARR)
		{
			for (Variant* p = (Variant*)var->pValue; p < (Variant*)var->pValue + var->usLength; ++p)
			{
				HeapChangeRefs(p, from, to);
			}

			if (var->pValue == from)
			{
				var->pValue = to;
			}
		}
		else if (var->usType == VarType::DICT)
		{
			const unsigned int len = var->usLength << 1;
			for (Variant* p = (Variant*)var->pValue; p < (Variant*)var->pValue + len; ++p)
			{
				HeapChangeRefs(p, from, to);
			}

			if (var->pValue == from)
			{
				var->pValue = to;
			}
		}
	}

}

bool VirtualMachine::Run(byte* program)
{
	byte* pc = program;

	while (true)
	{
		switch ((ByteCommand)*(pc++))
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

			*(--m_sp) = *(m_pStack + m_nCapacity - offset);
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

			*(m_pStack + m_nCapacity - offset) = *(m_sp++);
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

			*(--m_sp) = *(m_bp - offset);
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

			*(m_bp - offset) = *(m_sp++);
		}
		break;
		//do we need to check a type for following commands?
		case ByteCommand::AFETCH:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

			m_sp->Free();
			const unsigned short index = (unsigned short)m_sp->dValue;

#ifdef _DEBUG
			Log("AFETCH " + std::to_string(offset) + " " + std::to_string(index));
#endif

			*m_sp = (m_bp - offset)->ArrGet(index);
		}
		break;
		case ByteCommand::ASTORE:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

			m_sp->Free();
			const unsigned short index = (unsigned short)m_sp->dValue;

#ifdef _DEBUG
			Log("ASTORE " + (++m_sp)->ToString() + " " + std::to_string(offset) + " " + std::to_string(index));
#endif

			(m_pStack + m_nCapacity - offset)->ArrSet(index, m_sp++);
		}
		break;
		case ByteCommand::APUSH:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("APUSH " + m_sp->ToString() + " " + std::to_string(offset));
#endif

			(m_pStack + m_nCapacity - offset)->PushBack(m_sp++, this);
		}
		break;
		case ByteCommand::DFETCH:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

#ifdef _DEBUG
			Log("DFETCH " + std::to_string(offset) + " " + m_sp->ToString());
#endif

			Variant val = (m_bp - offset)->DictGet(m_sp);
			m_sp->Free();
			*m_sp = val;
		}
		break;
		case ByteCommand::DSTORE:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

			Variant* key = m_sp;

#ifdef _DEBUG
			Log("DSTORE " + (++m_sp)->ToString() + " " + std::to_string(offset) + " " + key->ToString());
#endif

			(m_pStack + m_nCapacity - offset)->DictSet(key, m_sp++);
			key->Free();
		}
		break;
		case ByteCommand::DINSERT:
		{
			const int offset = *((int*)pc);
			pc += sizeof(int);

			Variant* key = m_sp;

#ifdef _DEBUG
			Log("DINSERT " + (++m_sp)->ToString() + " " + std::to_string(offset) + " " + key->ToString());
#endif

			(m_pStack + m_nCapacity - offset)->Insert(key, m_sp++);
			key->Free();
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
			m_sp->Free();
			const unsigned short len = (unsigned short)m_sp->dValue;
			//throw any exception if there's NaN;

#ifdef _DEBUG
			Log("ARR " + std::to_string(len));
#endif

			const unsigned int capacity = len + (len >> Variant::c_capInc);
			Variant* arr = HeapAlloc(capacity + 1);
			*(arr++) = Variant(capacity);
			*m_sp = Variant(arr, len, VarType::ARR);
		}
		case ByteCommand::DICTIONARY:
		{
			m_sp->Free();
			const unsigned short len = (unsigned short)m_sp->dValue;
			//throw any exception if there's NaN;

#ifdef _DEBUG
			Log("DICT " + std::to_string(len));
#endif

			const unsigned int realLen = (unsigned int)len << 1;
			const unsigned int capacity = realLen + (realLen >> Variant::c_capInc);
			Variant* dict = HeapAlloc(capacity + 1);
			*(dict++) = Variant(capacity);
			*m_sp = Variant(dict, len, VarType::DICT);
		}
		break;
		case ByteCommand::PUSH:
		{
			if (m_sp - 1 == m_bp)
			{
				Resize();
			}
			*(--m_sp) = Variant::FromBytes(&pc, this);

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
#endif

			const double bRes = Variant::Equal(op1, op2) ? 1.0 : 0.0;
			op1->Free();
			op2->Free();
			op1->dValue = bRes;
		}
		break;
		case ByteCommand::NEQ:
		{
			Variant* op2 = m_sp++;
			Variant* op1 = m_sp;

#ifdef _DEBUG
			Log("NEQ " + op1->ToString() + " " + op2->ToString());
#endif

			const double bRes = Variant::Equal(op1, op2) ? 0.0 : 1.0;
			op1->Free();
			op2->Free();
			op1->dValue = bRes;
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

			return true;
		}
		default:
		{
			return false;
		}
		}
	}
}

const Variant* VirtualMachine::GetStack(int* size) const
{
	*size = m_nCapacity;
	return m_pStack;
}

const string VirtualMachine::GetStack() const
{
	string sStack = "";
	for (Variant* p = m_sp; p < m_pStack + m_nCapacity; ++p)
	{
		sStack = sStack + p->ToString() + '\n';
	}
	return sStack;
}

extern "C"
{
	__declspec(dllexport) bool __stdcall VMRun(byte* program)
	{
		VirtualMachine vm;
		bool bSuccsess = vm.Run(program);
		string sStack = vm.GetStack();
		std::cout << sStack << std::endl;
		return bSuccsess;
	}
}