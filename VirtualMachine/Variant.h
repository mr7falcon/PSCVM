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

	inline Variant(const unsigned short cap, Variant* next = nullptr)
		: usCap(cap),
		  usReplaced(0),
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

	inline void Copy() noexcept
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
		while (index >= p->usCap)
		{
			index -= p->usCap;
			p = (Variant*)p->pValue;
		}
		return p + 1 + index;
	}

	void PushBack(Variant* var); //better to be inline but cant be that with VM methods

	Variant* Find(Variant* key) const;		//same with Bucket

	void Insert(Variant* key, Variant* val);	//same as PushBack

	inline const long GetHash() const
	{
		long hash = 0;

		if (usNull == c_null)
		{
			if (usType == VarType::STR)
			{
				static const unsigned short p = 73;
				long long p_pow = p;
				const char* pStop = (char*)pValue + usLength;
				for (char* str = (char*)pValue; str < pStop; ++str)
				{
					hash += (long)(*str * p_pow);
					p_pow *= p;
				}
			}
			else if (usType == VarType::ARR)
			{
			}
			else if (usType == VarType::DICT)
			{
			}
		}
		else
		{
			hash = lValue;
		}

		return hash;
	}

	inline void PopBack() noexcept
	{
		--usLength;
	}

	void Erase(Variant* key); //same with Bucket

	inline void Concat(Variant* op)
	{
		if (usNull == c_null && op->usNull == c_null)
		{
			if (usType != op->usType)
			{
				//different types - exception
			}

			const unsigned short len = op->usLength;
			const unsigned short newLength = usLength + len;
			if (usType == VarType::STR)
			{
				char* res = new char[newLength];
				memcpy(res, pValue, usLength);
				delete[]((char*)pValue);
				memcpy(res + usLength, op->pValue, len);
				pValue = res;
			}
			else if (usType == VarType::ARR)
			{
				Variant* pGlobalDesc = (Variant*)pValue;
				Variant* p = (Variant*)pGlobalDesc->pValue;
				int i = usLength - p->usCap;
				while (i > 0)
				{
					p = (Variant*)p->pValue;
					i -= p->usCap;
				}
				const Variant* pGlobalDescOp = (Variant*)op->pValue;
				p->pValue = pGlobalDescOp->pValue;
				pGlobalDesc->usCap += (i + pGlobalDescOp->usCap);
				p->usCap += i;
			}
			else
			{
				//exception if dict or null
			}
			usLength = newLength;
		}
		else
		{
			//exception if num
		}
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
			unsigned short usCap;
			unsigned short usReplaced;
			unsigned int null;
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

inline const unsigned short GetPrime(const unsigned short num) noexcept
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