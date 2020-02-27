#pragma once

#include <string>

using std::string;
using std::exception;

typedef unsigned char byte;

#define KEY 0
#define VALUE 1
#define NEXT 2
#define BUCKET_SIZE 3

enum VarType : unsigned short
{
	STR,
	ARR,
	DICT,
	NIL,
};

struct Variant
{
	class ex_keyMissing : exception
	{
	public:
		ex_keyMissing(Variant* key)
			: exception(("key is missing " + key->ToString() + "\n").c_str())
		{}
	};

	class ex_typeMismatch : exception
	{
	public:
		ex_typeMismatch(VarType type1, VarType type2)
			: exception(("type mismatch " + TypeToString(type1) + " : " + TypeToString(type2) + "\n").c_str())
		{}
	};

	class ex_wrongType : exception
	{
	public:
		ex_wrongType(VarType type)
			: exception(("operation can't be used with this type " + TypeToString(type) + "\n").c_str())
		{}
	};

	class ex_outOfBounds : exception
	{
	public:
		ex_outOfBounds(const unsigned int i)
			: exception(("index is out of bounds " + std::to_string(i) + "\n").c_str())
		{}
	};

	static const unsigned short c_null = 0x7FF0;
	static const char c_capInc = 2;

	inline Variant()
		: pValue(nullptr),
		  usNull(c_null),
		  usType(VarType::NIL),
		  nLength(0)
	{}

	inline Variant(const double val)
		: pValue(nullptr),
		  dValue(val)
	{}

	inline Variant(char* str, const unsigned int length)
		: usNull(c_null),
		  usType(VarType::STR),
		  nLength(length),
		  pValue(str)
	{}

	inline Variant(Variant* var, const unsigned int length, const unsigned short type)
		: usNull(c_null),
		  usType(type),
		  nLength(length),
		  pValue(var)
	{}

	inline Variant(const unsigned int cap, Variant* next = nullptr)
		: nCap(cap),
		  nReplaced(0),
		  pValue(next)
	{}

	inline ~Variant() {}

	inline void Free()
	{
		if (usNull == c_null && usType == VarType::STR)
		{
			char* ptr = (char*)pValue - sizeof(unsigned int);
			if (--(*((unsigned int*)ptr)) == 0)
			{
				delete[](ptr);
			}
		}
	}

	inline void Copy()
	{
		if (usNull == c_null && usType == VarType::STR)
		{
			if (usType == VarType::STR)
			{
				char* ptr = (char*)pValue - sizeof(unsigned int);
				++(*((unsigned int*)ptr));
			}
		}
	}

	inline Variant* Get(const unsigned int i) const
	{
		Variant* p = (Variant*)((Variant*)pValue)->pValue;
		unsigned int index = i;
		while (index >= p->nCap)
		{
			index -= p->nCap;
			p = (Variant*)p->pValue;
		}
		return p + 1 + index;
	}

	inline void PushBack(Variant* pLocalDesc)
	{
		Variant* pGlobalDesc = (Variant*)pValue;
		Variant* p = (Variant*)pGlobalDesc->pValue;
		while (p->pValue)
		{
			p = (Variant*)p->pValue;
		}
		p->pValue = pLocalDesc;
	}

	inline Variant* Find(Variant* key) const
	{
		const unsigned int capacity = ((Variant*)pValue)->nCap;
		unsigned int index = key->GetHash() % capacity;

		Variant* bucket = (Variant*)Get(index)->pValue;

		while (bucket)
		{
			if (Equal(key, bucket + KEY))
			{
				return bucket + VALUE;
			}

			bucket = (Variant*)(bucket + NEXT)->pValue;
		}

		throw ex_keyMissing(key);
	}

	inline void Insert(Variant* key, Variant* val, Variant* bucket)
	{
		const unsigned int capacity = ((Variant*)pValue)->nCap;
		unsigned int index = key->GetHash() % capacity;

		Variant* entry = (Variant*)Get(index);
		//if entry->lValue more then trashhold constante, hash would be resized

		*(bucket + KEY) = *key;
		*(bucket + VALUE) = *val;

		if (entry->pValue == nullptr)
		{
			entry->lValue = 1;
			entry->pValue = bucket;
		}
		else
		{
			(bucket + NEXT)->pValue = entry->pValue;
			++entry->lValue;
			entry->pValue = bucket;
		}

		++nLength;
	}

	const unsigned long GetHash() const;

	inline void PopBack() noexcept
	{
		--nLength;
	}

	void Erase(Variant* key)
	{
		const unsigned int cap = ((Variant*)pValue)->nCap;
		unsigned int index = key->GetHash() % cap;
		Variant* pEntry = Get(index);
		Variant* prev = pEntry;
		Variant* pBucket = (Variant*)pEntry->pValue;

		while (pBucket)
		{
			if (Equal(key, pBucket + KEY))
			{
				--nLength;
				--pEntry->nCap;
				pBucket = (Variant*)(pBucket + NEXT)->pValue;
				prev->pValue = pBucket;
				return;
			}
			prev = pBucket + NEXT;
			pBucket = (Variant*)(pBucket + NEXT)->pValue;
		}

		throw ex_keyMissing(key);
	}

	inline void Concat(Variant* op)
	{
		if (usNull == c_null && op->usNull == c_null)
		{
			if (usType != op->usType)
			{
				throw ex_typeMismatch((VarType)usType, (VarType)op->usType);
			}

			const unsigned int len = op->nLength;
			const unsigned int newLength = nLength + len;
			if (usType == VarType::STR)
			{
				char* res = new char[newLength + sizeof(unsigned int)];
				char* ptr = (char*)pValue - sizeof(unsigned int);
				*((unsigned int*)res) = *((unsigned int*)ptr);
				res += sizeof(unsigned int);
				memcpy(res, pValue, nLength);
				delete[]((char*)ptr);
				memcpy(res + nLength, op->pValue, len);
				pValue = res;
			}
			else if (usType == VarType::ARR)
			{
				Variant* pGlobalDesc = (Variant*)pValue;
				Variant* p = (Variant*)pGlobalDesc->pValue;
				int i = nLength - p->nCap;
				while (i > 0)
				{
					p = (Variant*)p->pValue;
					i -= p->nCap;
				}
				const Variant* pGlobalDescOp = (Variant*)op->pValue;
				p->pValue = pGlobalDescOp->pValue;
				pGlobalDesc->nCap += (i + pGlobalDescOp->nCap);
				p->nCap += i;
			}
			else
			{
				throw ex_wrongType((VarType)usType);
			}
			nLength = newLength;
		}
		else
		{
			throw ex_wrongType((VarType)usType);
		}
	}

	static bool Equal(Variant* op1, Variant* op2);
	
	inline Variant Duplicate(Variant* pGlobalDesc2)
	{
		Variant* pGlobalDesc1 = (Variant*)pValue;
		Variant* pLocalDesc1 = (Variant*)pGlobalDesc1->pValue;
		Variant* pLocalDesc2 = (Variant*)pGlobalDesc2->pValue;
		Variant* pArr1 = pLocalDesc1 + 1;
		Variant* pArr2 = pLocalDesc2 + 1;
		unsigned int stop1 = pLocalDesc1->nCap;
		unsigned int stop2 = pLocalDesc2->nCap;

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

	const string ToString() const;

	inline static string TypeToString(VarType type)
	{
		switch (type)
		{
		case VarType::STR:
			return "string";
			break;
		case VarType::ARR:
			return "array";
			break;
		case VarType::DICT:
			return "dictionary";
			break;
		case VarType::NIL:
			return "null";
			break;
		default:
			return "number";
		}
	}

	union
	{
		double dValue;
		long lValue;

		struct
		{
			unsigned short usNull;
			unsigned short usType;
			unsigned int nLength;
		};

		struct
		{
			unsigned int nCap;
			unsigned int nReplaced;
		};
	};

	void* pValue;
};

const unsigned int c_primes[] = {
			3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
			1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
			17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
			187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
			1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369 };

inline const unsigned int GetPrime(const unsigned int num) noexcept
{
	for (int i = 0; i < 72; ++i)
	{
		const unsigned int prime = *(c_primes + i);
		if (prime > num)
		{
			return prime;
		}
	}

	return UINT_MAX;
}