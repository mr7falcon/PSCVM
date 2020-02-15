#pragma once

#include <string>

using std::string;
using std::exception;

typedef unsigned char byte;

struct Bucket;

enum VarType : unsigned short
{
	STR,
	ARR,
	DICT,
	NIL,
};

struct Variant
{
	static const exception ex_keyMissing;
	static const exception ex_typeMismatch;
	static const exception ex_wrongType;
	static const exception ex_outOfBounds;

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

	~Variant();

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

	void PushBack(Variant* var); //better to be inline but cant be that with VM methods

	Variant* Find(Variant* key) const;		//same with Bucket

	void Insert(Variant* key, Variant* val);	//same as PushBack

	const unsigned long GetHash() const;

	inline void PopBack() noexcept
	{
		--nLength;
	}

	void Erase(Variant* key); //same with Bucket

	inline void Concat(Variant* op)
	{
		if (usNull == c_null && op->usNull == c_null)
		{
			if (usType != op->usType)
			{
				throw ex_typeMismatch;
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
				throw ex_wrongType;
			}
			nLength = newLength;
		}
		else
		{
			throw ex_wrongType;
		}
	}

	static bool Equal(Variant* op1, Variant* op2);
	Variant Duplicate();

	const string ToString() const;
	static Variant FromBytes(byte** pc);

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

struct Bucket
{
	Variant key;
	Variant value;
	Variant next;
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