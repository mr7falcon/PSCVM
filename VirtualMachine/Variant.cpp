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
		str.append((char*)pValue, usLength);
		str += '\'';
		return str;
	}
	else if (!pValue)
	{
		return "NULL";
	}
	else
	{
		string str = "{";
		
		Variant* pGlobalDesc = (Variant*)pValue;
		Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue;
		Variant* pArr = pLocalDesc + 1;
		Variant* stop = pArr + (unsigned short)pLocalDesc->nCap;

		if (usType == VarType::ARR)
		{
			const unsigned short length = usLength - 1;

			str += (pArr++)->ToString();

			for (unsigned short i = 0; i < length; ++i)
			{
				if (pArr == stop)
				{
					pLocalDesc = (Variant*)pLocalDesc->pValue;
					pArr = pLocalDesc + 1;
					stop = pArr + (unsigned short)pLocalDesc->nCap;
				}

				str = str + " | " + pArr->ToString();
				++pArr;
			}

			str += '}';
			return str;
		}
		else if (usType == VarType::DICT)
		{
			unsigned short length = usLength;
			Bucket* bucket;
			while (true)
			{
				const unsigned short capacity = (unsigned short)pLocalDesc->nCap;

				for (; pArr <= stop; ++pArr)
				{
					if (pArr->pValue)
					{
						bucket = (Bucket*)pArr->pValue;
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
				pArr = pLocalDesc + 1;
				stop = pArr + (unsigned short)pLocalDesc->nCap;
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
			unsigned short length = var.usLength;
			Variant* pArr = VirtualMachine::HeapAlloc(length);
			Variant* pLocalDesc = (Variant*)pArr->pValue;

			while (pLocalDesc->pValue)
			{
				const unsigned short capacity = (unsigned short)pLocalDesc->nCap;

				for (; pArr <= pLocalDesc + capacity; ++pArr)
				{
					*pArr = Variant::FromBytes(pc);
				}

				length -= capacity;
			}

			for (; pArr <= pLocalDesc + length; ++pArr)
			{
				*pArr = Variant::FromBytes(pc);
			}

			var.pValue = pArr;
		}
		else if (var.usType == VarType::DICT)
		{
			const unsigned short length = var.usLength;
			const unsigned short cap = GetPrime(length);
			Variant* pGlobalDesc = VirtualMachine::HeapAlloc(cap);
			var.pValue = pGlobalDesc;
			Variant* pBucketDesc = VirtualMachine::HeapAllocStructArr(length);

			while (pBucketDesc)
			{
				const unsigned short bucketCap = (unsigned short)pBucketDesc->nCap;
				Bucket* pBucketArr = (Bucket*)pBucketDesc + 1;
				for (Bucket* pBucket = pBucketArr; pBucket < pBucketArr + bucketCap; ++pBucket)
				{
					Variant key = Variant::FromBytes(pc);
					const unsigned short index = key.GetHash() % cap;

					Variant* cell = var.Get(index);
					pBucket->key = key;
					pBucket->value = Variant::FromBytes(pc);

					if (cell->pValue == nullptr)
					{
						cell->lValue = 1;
						cell->pValue = pBucket;
					}
					else
					{
						pBucket->next.pValue = cell->pValue;
						++cell->lValue;
						cell->pValue = pBucket;
					}
				}

				pBucketDesc = (Variant*)pBucketDesc->pValue;
			}
		}
	}

	return var;
}

bool Variant::Equal(Variant* op1, Variant* op2)
{
	if (!op1->pValue && !op2->pValue)
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
			Variant* pArr1 = pLocalDesc1 + 1;
			Variant* pArr2 = pLocalDesc2 + 1;
			Variant* stop1 = pArr1 + (unsigned short)pLocalDesc1->nCap;
			Variant* stop2 = pArr2 + (unsigned short)pLocalDesc2->nCap;

			if (type == VarType::ARR)
			{
				for (unsigned short i = 0; i < op1->usLength; ++i)
				{
					if (pArr1 == stop1)
					{
						pLocalDesc1 = (Variant*)pLocalDesc1->pValue;
						pArr1 = pLocalDesc1 + 1;
						stop1 = pArr1 + (unsigned short)pLocalDesc1->nCap;
					}

					if (pArr2 == stop2)
					{
						pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
						pArr2 = pLocalDesc2 + 1;
						stop2 = pArr2 + (unsigned short)pLocalDesc2->nCap;
					}

					if (!Equal(pArr1, pArr2))
					{
						return false;
					}

					++pArr1;
					++pArr2;
				}
			}
			else if (type == VarType::DICT)
			{
				Bucket* bucket1 = nullptr;
				Bucket* bucket2 = nullptr;

				for (unsigned short i = 0; i < op1->usLength; ++i)
				{
					while (!bucket1)
					{
						if (pArr1 == stop1)
						{
							pLocalDesc1 = (Variant*)pLocalDesc1->pValue;
							pArr1 = pLocalDesc1 + 1;
							stop1 = pArr1 + (unsigned short)pLocalDesc1->nCap;
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
						if (pArr2 == stop2)
						{
							pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
							pArr2 = pLocalDesc2 + 1;
							stop2 = pArr2 + (unsigned short)pLocalDesc2->nCap;
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

			return true;
		}
	}
}

void Variant::PushBack(Variant* var)
{
	Variant* pGlobalDesc = (Variant*)pValue;
	const unsigned short cap = (unsigned short)pGlobalDesc->nCap;
	if (usLength == cap)
	{
		const unsigned short inc = cap >> c_capInc;
		Variant* pLocalDesc = VirtualMachine::HeapAlloc(inc, true);
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
		unsigned short cap = (unsigned short)p->nCap;
		unsigned short length = usLength;
		while (length > cap)
		{
			length -= cap;
			p = (Variant*)p->pValue;
			cap = (unsigned short)p->nCap;
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
	
	Bucket* bucket = (Bucket*)VirtualMachine::HeapAllocStruct();
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
	throw new std::exception("idi rabotay, suka!");
}