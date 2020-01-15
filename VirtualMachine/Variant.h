#pragma once

#include <string>

using std::string;

typedef unsigned char byte;

class VirtualMachine;
struct Bucket;

enum VarType : unsigned short
{
	STR,
	ARR,
	DICT
};

struct Variant
{
	static const unsigned short c_null = 0x7FF0;
	static const char c_capInc = 2;

	inline Variant()
		: pValue(nullptr),
		  usNull(c_null)
	{}

	inline Variant(const double val)
		: pValue(nullptr),
		  dValue(val)
	{}

	inline Variant(char* str, const unsigned short length)
		: usNull(c_null),
		  usType(VarType::STR),
		  usLength(length),
		  usRef(1),
		  pValue(str)
	{}

	inline Variant(Variant* var, const unsigned short length, const unsigned short type)
		: usNull(c_null),
		  usType(type),
		  usLength(length),
		  usRef(1),
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
		if (usNull == c_null && --usRef == 0)
		{
			if (usType == VarType::STR)
			{
				delete[]((char*)pValue);
			}

			usNull = c_null;
			pValue = nullptr;
		}
	}

	inline void Copy()
	{
		if (pValue)
		{
			++usRef;
		}
	}

	inline Variant* Get(const unsigned short i) const
	{
		Variant* p = (Variant*)((Variant*)pValue)->pValue;
		unsigned short index = i;
		while (index >= (unsigned short)p->nCap)
		{
			index -= (unsigned short)p->nCap;
			p = (Variant*)p->pValue;
		}
		return p + 1 + index;
	}

	void PushBack(Variant* var); //better to be inline but cant be that with VM methods

	Variant* Find(Variant* key) const;		//same with Bucket

	void Insert(Variant* key, Variant* val);	//same as PushBack

	inline const long GetHash() const
	{
		long res = lValue;

		if (usNull == c_null)
		{
			if (usType == VarType::STR)
			{
				/*for (char* c = (char*)pValue; c < (char*)pValue + usLength; ++c)
				{
					res -= *c;
				}*/
			}
			else if (usType == VarType::ARR)
			{
			}
			else if (usType == VarType::DICT)
			{
			}
		}

		return res;
	}

	static bool Equal(Variant* op1, Variant* op2);

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
			unsigned short usLength;
			unsigned short usRef;
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

const unsigned short c_primes[] = { 3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353,
	431, 521, 631, 761, 919, 1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
	17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851 };

inline const unsigned short GetPrime(const unsigned short num)
{
	for (int i = 0; i < 42; ++i)
	{
		const unsigned short prime = *(c_primes + i);
		if (prime > num)
		{
			return prime;
		}
	}

	return USHRT_MAX;
}