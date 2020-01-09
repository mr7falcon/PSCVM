#include "Variant.h"
#include "VirtualMachine.h"

Variant::~Variant()
{
}

const string Variant::ToString() const
{
	if (usNull != c_null)
	{
		return std::to_string(dValue);
	}
	else if (usType == VarType::STR)
	{
		string str = "\'";
		str += (char*)pValue;
		str[(size_t)usLength + 1] = '\'';
		str.resize((size_t)usLength + 2);
		return str;
	}
	else if (usType == VarType::ARR)
	{
		Variant* pGlobalDesc = (Variant*)pValue;
		string str = "{";
		
		if (Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue)
		{
			unsigned short length = usLength;
			Variant* arr = pLocalDesc + 1;
			str += (arr++)->ToString();

			while (pLocalDesc->pValue)
			{
				const unsigned short capacity = (unsigned short)pLocalDesc->nCap;

				for (; arr <= pLocalDesc + capacity; ++arr)
				{
					str = str + " | " + arr->ToString();
				}

				length -= capacity;
				pLocalDesc = (Variant*)pLocalDesc->pValue;
				arr = pLocalDesc + 1;
			}

			for (; arr <= pLocalDesc + length; ++arr)
			{
				str = str + " | " + arr->ToString();
			}

			str += '}';
		}
		else
		{
			Variant* arr = pGlobalDesc + 1;
			str += (arr++)->ToString();

			for (; arr <= pGlobalDesc + usLength; ++arr)
			{
				str = str + " | " + arr->ToString();
			}

			str += '}';
		}

		return str;
	}
	else if (usType == VarType::DICT)
	{
		Variant* p = (Variant*)pValue;
		string str = '{' + (p++)->ToString() + " : " + (p++)->ToString();
		const unsigned int len = usLength << 1;
		while (p < (Variant*)pValue + len)
		{
			str = str + " | " + (p++)->ToString() + " : " + (p++)->ToString();
		}
		str += '}';
		return str;
	}
	else if (!pValue)
	{
		return "NULL";
	}

	return "";
}

Variant Variant::FromBytes(byte** pc)
{
	Variant var(*((double*)*pc));
	*pc += sizeof(double);

	if (var.usNull == c_null)
	{
		const unsigned short length = var.usLength;
		if (var.usType == VarType::STR)
		{
			char* str = new char[length];
			memcpy(str, *pc, length);
			*pc += length;
			var.pValue = str;
		}
		else if (var.usType == VarType::ARR)
		{
			const unsigned short length = var.usLength;
			Variant* arr = VirtualMachine::HeapAlloc(length + 1);
			*arr = Variant((const unsigned int)length);

			for (Variant* p = arr + 1; p <= arr + length; ++p)
			{
				*p = Variant::FromBytes(pc);
			}

			var.pValue = arr;
		}
		else if (var.usType == VarType::DICT)
		{
			const unsigned short length = var.usLength;
			const unsigned short cap = GetPrime(length);
			Variant* dict = VirtualMachine::HeapAlloc(cap + 1);
			*(dict++) = Variant((const unsigned int)cap);

			for (unsigned short i = 0; i < length; ++i)
			{
				Variant key = Variant::FromBytes(pc);
				const unsigned short index = key.GetHash() % cap;

				Variant* cell = dict + index;
				Bucket* bucket = (Bucket*)VirtualMachine::HeapAlloc(3);
				bucket->key = key;
				bucket->value = Variant::FromBytes(pc);

				if (cell->pValue == nullptr)
				{
					cell->lValue = 0;
					cell->pValue = bucket;
				}
				else
				{
					bucket->next.pValue = cell->pValue;
					++cell->lValue;
					cell->pValue = bucket;
				}
			}

			var.pValue = dict;
		}
	}

	return var;
}

bool Variant::Equal(Variant* op1, Variant* op2)
{
	if (op1->usNull != c_null && op2->usNull != c_null)
	{
		return op1->dValue == op2->dValue;
	}
	else if (op1->usType == VarType::STR && op2->usType == VarType::STR)
	{

		if (op1->usLength != op2->usLength)
		{
			return false;
		}

		for (unsigned short i = 0; i < op1->usLength; ++i)
		{
			if (((char*)op1->pValue + i) != ((char*)op2->pValue + i))
			{
				return false;
			}
		}

		return true;
	}
	else if (op1->usType == VarType::ARR && op2->usType == VarType::ARR)
	{
		if (op1->usLength != op2->usLength)
		{
			return false;
		}

		Variant* pGlobalDesc1 = (Variant*)op1->pValue;
		Variant* pGlobalDesc2 = (Variant*)op2->pValue;
		Variant* pLocalDesc1 = (Variant*)pGlobalDesc1->pValue;
		Variant* pLocalDesc2 = (Variant*)pGlobalDesc2->pValue;
		Variant* pArr1 = pLocalDesc1 ? pLocalDesc1 + 1 : pGlobalDesc1 + 1;
		Variant* pArr2 = pLocalDesc2 ? pLocalDesc2 + 1 : pGlobalDesc2 + 1;

		for (unsigned short i = 0; i < op1->usLength; ++i)
		{
			if (pLocalDesc1 && pArr1 == pLocalDesc1 + (unsigned short)pLocalDesc1->nCap + 1)
			{
				pLocalDesc1 = (Variant*)pLocalDesc1->pValue;
				pArr1 = pLocalDesc1 + 1;
			}

			if (pLocalDesc2 && pArr2 == pLocalDesc2 + (unsigned short)pLocalDesc2->nCap + 1)
			{
				pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
				pArr2 = pLocalDesc2 + 1;
			}

			if (!Equal(pArr1, pArr2))
			{
				return false;
			}

			++pArr1;
			++pArr2;
		}

		return true;
	}
	else if (op1->usType == VarType::DICT && op2->usType == VarType::DICT)
	{
		if (op1->usLength != op2->usLength)
		{
			return false;
		}

		for (unsigned int i = 0; i < ((unsigned int)op1->usLength << 1); ++i)
		{
			if (!Equal((Variant*)op1->pValue + i, (Variant*)op2->pValue + i))
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}

void Variant::PushBack(Variant* var)
{
	Variant* pGlobalDesc = (Variant*)pValue;
	if (!pGlobalDesc->pValue)
	{
		//array is fixed, throw exception
	}

	const unsigned short cap = (unsigned short)((Variant*)pValue)->nCap;
	if (usLength == cap)
	{
		const unsigned short inc = cap >> c_capInc;
		Variant* pLocalDesc = VirtualMachine::HeapAlloc(inc + 1);
		*pLocalDesc = Variant((unsigned int)inc);
		*(pLocalDesc + 1) = *var;

		Variant* p = (Variant*)pGlobalDesc->pValue;
		while (p->pValue)
		{
			p = (Variant*)p->pValue;
		}
		p->pValue = pLocalDesc;
		pGlobalDesc->nCap += inc;
	}
	else
	{
		Variant* p = (Variant*)pGlobalDesc->pValue;
		unsigned short length = usLength;
		while (p->pValue)
		{
			length -= (unsigned short)p->nCap;
			p = (Variant*)p->pValue;
		}

		*(p + 1 + length) = *var;
	}

	++usLength;
}

void Variant::Insert(Variant* key, Variant* val)
{
	const unsigned short capacity = (unsigned short)((Variant*)pValue)->nCap;
	unsigned short index = key->GetHash() % capacity;

	Variant* entry = (Variant*)Get(index);
	//if entry->lValue more then trashhold constante, hash would be resized
	
	Bucket* bucket = (Bucket*)VirtualMachine::HeapAlloc(3);
	bucket->key = *key;
	bucket->value = *val;

	if (entry->pValue == nullptr)
	{
		entry->lValue = 0;
		entry->pValue = bucket;
	}
	else
	{
		bucket->next.pValue = entry->pValue;
		++entry->lValue;
		entry->pValue = bucket;
	}

	++usLength;
}