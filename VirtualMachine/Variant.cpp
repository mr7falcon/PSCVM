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
	else if (!pValue)
	{
		return "NULL";
	}
	else
	{
		Variant* pGlobalDesc = (Variant*)pValue;
		string str = "{";
		
		if (Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue)
		{
			unsigned short length = usLength;
			Variant* arr = pLocalDesc + 1;

			if (usType == VarType::ARR)
			{
				while (pLocalDesc->pValue)
				{
					const unsigned short capacity = (unsigned short)pLocalDesc->nCap;
					const Variant* stop = pLocalDesc + capacity;

					for (; arr <= stop; ++arr)
					{
						str = str + arr->ToString() + " | ";
					}

					length -= capacity;
					pLocalDesc = (Variant*)pLocalDesc->pValue;
					arr = pLocalDesc + 1;
				}

				const Variant* stop = pLocalDesc + length;
				str = str + (arr++)->ToString();
				for (; arr <= stop; ++arr)
				{
					str = str + " | " + arr->ToString();
				}

				str += '}';
				return str;
			}
			else if (usType == VarType::DICT)
			{
				Bucket* bucket;
				while (true)
				{
					const unsigned short capacity = (unsigned short)pLocalDesc->nCap;
					const Variant* stop = pLocalDesc + capacity;

					for (; arr <= stop; ++arr)
					{
						if (arr->pValue)
						{
							bucket = (Bucket*)arr->pValue;
							while (bucket)
							{
								str = str + bucket->key.ToString() + " : " + bucket->value.ToString();
								bucket = (Bucket*)bucket->next.pValue;
								if (--length == 0)
								{
									str += '}';
									return str;
								}
								else
								{
									str += " | ";
								}
							}
						}
					}

					pLocalDesc = (Variant*)pLocalDesc->pValue;
					arr = pLocalDesc + 1;
				}
			}
		}
		else
		{
			if (usType == VarType::ARR)
			{
				const Variant* stop = pGlobalDesc + usLength;
				Variant* arr = pGlobalDesc + 1;
				str = str + (arr++)->ToString();
				for (; arr <= stop; ++arr)
				{
					str = str + " | " + arr->ToString();
				}

				str += '}';
				return str;
			}
			else if (usType == VarType::DICT)
			{
				Bucket* bucket = nullptr;
				unsigned short length = usLength;
				for (Variant* arr = pGlobalDesc + 1; ; ++arr)
				{
					if (arr->pValue)
					{
						bucket = (Bucket*)arr->pValue;
						while (bucket)
						{
							str = str + bucket->key.ToString() + " : " + bucket->value.ToString();
							bucket = (Bucket*)bucket->next.pValue;
							if (--length == 0)
							{
								str += '}';
								return str;
							}
							else
							{
								str += " | ";
							}
						}
					}
				}
			}
		}
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
			Variant* pGlobalDesc = VirtualMachine::HeapAlloc(cap + 1);
			*pGlobalDesc = Variant((const unsigned int)cap);
			Variant* dict = pGlobalDesc + 1;
			Bucket* bucket = (Bucket*)VirtualMachine::HeapAlloc(3 * length);

			for (unsigned short i = 0; i < length; ++i)
			{
				Variant key = Variant::FromBytes(pc);
				const unsigned short index = key.GetHash() % cap;

				Variant* cell = dict + index;
				bucket->key = key;
				bucket->value = Variant::FromBytes(pc);

				if (cell->pValue == nullptr)
				{
					cell->lValue = 1;
					cell->pValue = bucket;
				}
				else
				{
					bucket->next.pValue = cell->pValue;
					++cell->lValue;
					cell->pValue = bucket;
				}

				++bucket;
			}

			var.pValue = pGlobalDesc;
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
	else
	{
		unsigned short type = op1->usType;
		
		if (type != op2->usType)
		{
			return false;
		}

		if (!op1->pValue || !op2->pValue)
		{
			return false;
		}

		if (op1->usLength != op2->usLength)
		{
			return false;
		}

		if (type == VarType::STR)
		{
			for (unsigned short i = 0; i < op1->usLength; ++i)
			{
				if (*((char*)op1->pValue + i) != *((char*)op2->pValue + i))
				{
					return false;
				}
			}

			return true;
		}
		else
		{
			Variant* pGlobalDesc1 = (Variant*)op1->pValue;
			Variant* pGlobalDesc2 = (Variant*)op2->pValue;
			Variant* pLocalDesc1 = (Variant*)pGlobalDesc1->pValue;
			Variant* pLocalDesc2 = (Variant*)pGlobalDesc2->pValue;
			Variant* pArr1 = pLocalDesc1 ? pLocalDesc1 + 1 : pGlobalDesc1 + 1;
			Variant* pArr2 = pLocalDesc2 ? pLocalDesc2 + 1 : pGlobalDesc2 + 1;

			if (type == VarType::ARR)
			{
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
			else if (type == VarType::DICT)
			{
				Bucket* bucket1 = nullptr;
				Bucket* bucket2 = nullptr;

				for (unsigned short i = 0; i < op1->usLength; ++i)
				{
					while (!bucket1)
					{
						if (pLocalDesc1 && pArr1 == pLocalDesc1 + (unsigned short)pLocalDesc1->nCap + 1)
						{
							pLocalDesc1 = (Variant*)pLocalDesc1->pValue;
							pArr1 = pLocalDesc1 + 1;
						}

						if (pArr1->pValue)
						{
							bucket1 = (Bucket*)pArr1->pValue;
						}
						else
						{
							++pArr1;
						}
					}

					while (!bucket2)
					{
						if (pLocalDesc2 && pArr2 == pLocalDesc2 + (unsigned short)pLocalDesc2->nCap + 1)
						{
							pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
							pArr2 = pLocalDesc2 + 1;
						}

						if (pArr2->lValue > 0)
						{
							bucket2 = (Bucket*)pArr2->pValue;
						}
						else
						{
							++pArr2;
						}
					}

					if (!Equal(&bucket1->key, &bucket2->key) || !Equal(&bucket1->value, &bucket2->value))
					{
						return false;
					}

					bucket1 = (Bucket*)bucket1->next.pValue;
					bucket2 = (Bucket*)bucket2->next.pValue;
				}
			}

			return false;
		}
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
		entry->lValue = 1;
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

Variant* Variant::Find(Variant* key) const
{
	const unsigned short capacity = (unsigned short)((Variant*)pValue)->nCap;
	unsigned short index = key->GetHash() % capacity;

	Bucket* bucket = (Bucket*)Get(index)->pValue;

	while (bucket)
	{
		if (Equal(key, &bucket->key))
		{
			return &bucket->value;
		}

		bucket = (Bucket*)bucket->next.pValue;
	}

	//key is missing - throw any exception
}