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
	else if (!pValue)
	{
		return "NULL";
	}
	else if (usType == VarType::STR)
	{
		string str = "\'";
		str.append((char*)pValue, nLength);
		str += '\'';
		return str;
	}
	else
	{
		string str = "{";
		
		Variant* pGlobalDesc = (Variant*)pValue;
		Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue;
		Variant* pArr = pLocalDesc + 1;
		Variant* stop = pArr + pLocalDesc->nCap;

		if (usType == VarType::ARR)
		{
			const unsigned int length = nLength - 1;

			str += (pArr++)->ToString();

			for (unsigned int i = 0; i < length; ++i)
			{
				if (pArr == stop)
				{
					pLocalDesc = (Variant*)pLocalDesc->pValue;
					pArr = pLocalDesc + 1;
					stop = pArr + pLocalDesc->nCap;
				}

				str = str + " | " + pArr->ToString();
				++pArr;
			}

			str += '}';
			return str;
		}
		else if (usType == VarType::DICT)
		{
			unsigned int length = nLength;
			Bucket* bucket;
			while (true)
			{
				const unsigned int capacity = pLocalDesc->nCap;

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
				stop = pArr + pLocalDesc->nCap;
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
		const unsigned int length = var.nLength;
		if (var.usType == VarType::STR)
		{
			char* str = new char[length + sizeof(unsigned int)];
			*((unsigned int*)str) = 1;
			str += sizeof(unsigned int);
			memcpy(str, *pc, length);
			*pc += length;
			var.pValue = str;
		}
		else if (var.usType == VarType::ARR)
		{
			unsigned int length = var.nLength;
			Variant* pArr = VirtualMachine::HeapAlloc(length);
			Variant* pLocalDesc = (Variant*)pArr->pValue;

			while (pLocalDesc->pValue)
			{
				const unsigned int capacity = pLocalDesc->nCap;
				const Variant* pStop = pLocalDesc + capacity;
				for (Variant* p = pLocalDesc + 1; p <= pStop; ++p)
				{
					*p = Variant::FromBytes(pc);
				}
				pLocalDesc = (Variant*)pLocalDesc->pValue;
				length -= capacity;
			}

			const Variant* pStop = pLocalDesc + length;
			for (Variant* p = pLocalDesc + 1; p <= pStop; ++p)
			{
				*p = Variant::FromBytes(pc);
			}

			var.pValue = pArr;
		}
		else if (var.usType == VarType::DICT)
		{
			const unsigned int length = var.nLength;
			const unsigned int cap = GetPrime(length);
			Variant* pGlobalDesc = VirtualMachine::HeapAlloc(cap);
			var.pValue = pGlobalDesc;
			Variant* pBucketDesc = VirtualMachine::HeapAllocStructArr(length);

			while (pBucketDesc)
			{
				const unsigned int bucketCap = pBucketDesc->nCap;
				Bucket* pBucketArr = (Bucket*)(pBucketDesc + 1);
				for (Bucket* pBucket = pBucketArr; pBucket < pBucketArr + bucketCap; ++pBucket)
				{
					Variant key = Variant::FromBytes(pc);
					const unsigned int index = key.GetHash() % cap;

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
	if (op1->usNull == c_null && op2->usNull == c_null)
	{
		Variant* pValue1 = (Variant*)op1->pValue;
		Variant* pValue2 = (Variant*)op2->pValue;
		if (!pValue1 && !pValue2)
		{
			return true;
		}

		unsigned short type = op1->usType;
		
		if (type != op2->usType)
		{
			return false;
		}

		if (!pValue1 || !pValue2)
		{
			return false;
		}

		if (op1->nLength != op2->nLength)
		{
			return false;
		}

		if (type == VarType::STR)
		{
			for (unsigned int i = 0; i < op1->nLength; ++i)
			{
				if (*((char*)pValue1 + i) != *((char*)pValue2 + i))
				{
					return false;
				}
			}

			return true;
		}
		else
		{
			Variant* pGlobalDesc1 = pValue1;
			Variant* pGlobalDesc2 = pValue2;
			Variant* pLocalDesc1 = (Variant*)pGlobalDesc1->pValue;
			Variant* pLocalDesc2 = (Variant*)pGlobalDesc2->pValue;
			Variant* pArr1 = pLocalDesc1 + 1;
			Variant* pArr2 = pLocalDesc2 + 1;
			Variant* stop1 = pArr1 + pLocalDesc1->nCap;
			Variant* stop2 = pArr2 + pLocalDesc2->nCap;

			if (type == VarType::ARR)
			{
				for (unsigned int i = 0; i < op1->nLength; ++i)
				{
					if (pArr1 == stop1)
					{
						pLocalDesc1 = (Variant*)pLocalDesc1->pValue;
						pArr1 = pLocalDesc1 + 1;
						stop1 = pArr1 + pLocalDesc1->nCap;
					}

					if (pArr2 == stop2)
					{
						pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
						pArr2 = pLocalDesc2 + 1;
						stop2 = pArr2 + pLocalDesc2->nCap;
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

				for (unsigned int i = 0; i < op1->nLength; ++i)
				{
					while (!bucket1)
					{
						if (pArr1 == stop1)
						{
							pLocalDesc1 = (Variant*)pLocalDesc1->pValue;
							pArr1 = pLocalDesc1 + 1;
							stop1 = pArr1 + pLocalDesc1->nCap;
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
							stop2 = pArr2 + pLocalDesc2->nCap;
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
	else
	{
		return op1->dValue == op2->dValue;
	}
}

void Variant::PushBack(Variant* var)
{
	Variant* pGlobalDesc = (Variant*)pValue;
	unsigned int cap = pGlobalDesc->nCap;
	if (nLength == cap)
	{
		const int short inc = cap >> c_capInc;
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
		cap = p->nCap;
		unsigned int length = nLength;
		while (length > cap)
		{
			length -= cap;
			p = (Variant*)p->pValue;
			cap = p->nCap;
		}

		*(p + 1 + length) = *var;
	}

	++nLength;
}

void Variant::Insert(Variant* key, Variant* val)
{
	const unsigned int capacity = ((Variant*)pValue)->nCap;
	unsigned int index = key->GetHash() % capacity;

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

	++nLength;
}

Variant* Variant::Find(Variant* key) const
{
	const unsigned int capacity = ((Variant*)pValue)->nCap;
	unsigned int index = key->GetHash() % capacity;

	Bucket* bucket = (Bucket*)Get(index)->pValue;

	while (bucket)
	{
		if (Equal(key, &bucket->key))
		{
			return &bucket->value;
		}

		bucket = (Bucket*)bucket->next.pValue;
	}

	throw ex_keyMissing(key);
}

void Variant::Erase(Variant* key)
{
	const unsigned int cap = ((Variant*)pValue)->nCap;
	unsigned int index = key->GetHash() % cap;
	Variant* pEntry = Get(index);
	Variant* prev = pEntry;
	Bucket* pBucket = (Bucket*)pEntry->pValue;

	while (pBucket)
	{
		if (Equal(key, &pBucket->key))
		{
			--nLength;
			--pEntry->nCap;
			pBucket = (Bucket*)pBucket->next.pValue;
			prev->pValue = pBucket;
			return;
		}
		prev = &pBucket->next;
		pBucket = (Bucket*)pBucket->next.pValue;
	}

	throw ex_keyMissing(key);
}

Variant Variant::Duplicate()
{
	if (usNull == c_null && usType == VarType::ARR)
	{
		Variant* pGlobalDesc1 = (Variant*)pValue;
		Variant* pGlobalDesc2 = VirtualMachine::HeapAlloc(pGlobalDesc1->nCap);
		Variant* pLocalDesc1 = (Variant*)pGlobalDesc1->pValue;
		Variant* pLocalDesc2 = (Variant*)pGlobalDesc2->pValue;
		Variant* pArr1 = pLocalDesc1 + 1;
		Variant* pArr2 = pLocalDesc2 + 1;
		unsigned int stop1 = pLocalDesc1->nCap;
		unsigned int stop2 = pLocalDesc2->nCap;

		if (usType == VarType::ARR)
		{
			while (true)
			{
				if (stop1 <= stop2)
				{
					memcpy(pArr2, pArr1, (unsigned int)stop1 * sizeof(Variant));
					stop2 -= stop1;
					pLocalDesc1 = (Variant*)pLocalDesc1->pValue;

					if (!pLocalDesc1)
					{
						break;
					}

					pArr2 += stop1;
					stop1 = pLocalDesc1->nCap;
					pArr1 = pLocalDesc1 + 1;
				}
				else
				{
					memcpy(pArr2, pArr1, (unsigned int)stop2 * sizeof(Variant));
					stop1 -= stop2;
					pLocalDesc2 = (Variant*)pLocalDesc2->pValue;

					if (!pLocalDesc2)
					{
						break;
					}

					pArr1 += stop2;
					stop2 = pLocalDesc2->nCap;
					pArr2 = pLocalDesc2 + 1;
				}
			}

			Variant var = Variant(pGlobalDesc2, nLength, VarType::ARR);
			Free();
			return var;
		}
	}

	return *this;
}

const unsigned long Variant::GetHash() const
{
	unsigned long hash = 0;

	if (usNull == c_null)
	{
		if (usType == VarType::STR)
		{
			static const unsigned short p = 73;
			unsigned long p_pow = p;
			const char* pStop = (char*)pValue + nLength;
			for (char* str = (char*)pValue; str < pStop; ++str)
			{
				hash += (unsigned long)(*str * p_pow);
				p_pow *= p;
			}
		}
		else
		{
			Variant* pGlobalDesc = (Variant*)pValue;
			Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue;
			Variant* pArr = pLocalDesc + 1;
			Variant* stop = pArr + pLocalDesc->nCap;

			if (usType == VarType::ARR)
			{
				for (unsigned int i = 0; i < nLength; ++i)
				{
					if (pArr == stop)
					{
						pLocalDesc = (Variant*)pLocalDesc->pValue;
						pArr = pLocalDesc + 1;
						stop = pArr + pLocalDesc->nCap;
					}

					hash ^= pArr->GetHash();
					++pArr;
				}
			}
			else if (usType == VarType::DICT)
			{
				unsigned int length = nLength;
				Bucket* bucket;
				while (true)
				{
					const unsigned int capacity = pLocalDesc->nCap;

					for (; pArr <= stop; ++pArr)
					{
						if (pArr->pValue)
						{
							bucket = (Bucket*)pArr->pValue;
							while (bucket)
							{
								hash ^= (bucket->key.GetHash() - bucket->value.GetHash());
								bucket = (Bucket*)bucket->next.pValue;
								if (--length == 0)
								{
									return hash;
								}
							}
						}
					}

					pLocalDesc = (Variant*)pLocalDesc->pValue;
					pArr = pLocalDesc + 1;
					stop = pArr + pLocalDesc->nCap;
				}
			}
		}
	}
	else
	{
		hash = lValue;
	}

	return hash;
}