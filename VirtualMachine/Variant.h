#pragma once

#include <string>

using std::string;

typedef unsigned char byte;

class VirtualMachine;

enum VarType : unsigned short
{
	STR,
	ARR,
	DICT,
};

struct Variant
{
	static const unsigned short c_null = 0x7FF0;
	static const char c_capInc = 2;

	Variant()
		: pValue(nullptr),
		  usNull(c_null)
	{}

	Variant(const double val)
		: pValue(nullptr),
		  dValue(val)
	{}

	Variant(char* str, const unsigned short length)
		: usNull(c_null),
		  usType(VarType::STR),
		  usLength(length),
		  usRef(1),
		  pValue(str)
	{}

	Variant(Variant* var, const unsigned short length, const unsigned short type)
		: usNull(c_null),
		  usType(type),
		  usLength(length),
		  usRef(1),
		  pValue(var)
	{}

	~Variant();

	inline void Free()
	{
		if (pValue && --usRef <= 0)
		{
			if (usType == VarType::STR)
			{
				delete[]((char*)pValue);
			}
			else if (usType == VarType::ARR)
			{
				for (Variant* p = (Variant*)pValue; p < (Variant*)pValue + usLength; ++p)
				{
					p->Free();
				}
			}
			else if (usType == VarType::DICT)
			{
				const unsigned int len = usLength << 1;
				for (Variant* p = (Variant*)pValue; p < (Variant*)pValue + len; ++p)
				{
					p->Free();
				}
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

	inline Variant ArrGet(const unsigned short i) const
	{
		Variant* var = (Variant*)pValue + i;
		var->Copy();
		return *var;
	}

	inline void ArrSet(const unsigned short i, Variant* var)
	{
		*((Variant*)pValue + i) = *var;
	}

	void PushBack(Variant* var); //better to be inline but i can't use VM methods in this header

	inline Variant DictGet(Variant* key) const
	{
		const unsigned int capacity = (unsigned int)((Variant*)pValue - 1)->dValue;
		const unsigned int index = key->GetHash(capacity) << 1;
		const unsigned int len = usLength << 1;

		Variant* cell = (Variant*)pValue + index;

		while (!Equal(key, cell))
		{
			cell += 2;

			if (cell->usNull == c_null && cell->pValue)
			{
				//key is missing, throw any exception
			}

			if (cell >= (Variant*)pValue + capacity)
			{
				cell = (Variant*)pValue;
			}
		}

		(++cell)->Copy();
		return *cell;
	}

	inline void DictSet(Variant* key, Variant* val)
	{
		const unsigned int capacity = (unsigned int)((Variant*)pValue - 1)->dValue;
		const unsigned int index = key->GetHash(capacity) << 1;
		const unsigned int len = usLength << 1;

		Variant* cell = (Variant*)pValue + index;

		while (!Equal(key, cell))
		{
			cell += 2;

			if (cell->usNull == c_null && cell->pValue)
			{
				//key is missing, throw any exception
			}

			if (cell >= (Variant*)pValue + capacity)
			{
				cell = (Variant*)pValue;
			}
		}

		*(++cell) = *val;
	}

	inline void Insert(Variant* key, Variant* val)
	{
		++usLength;
		const unsigned int len = usLength << 1;
		const unsigned int capacity = (unsigned int)((Variant*)pValue - 1)->dValue;
		if (len > capacity)
		{
			//throw any exception, i think
		}

		const unsigned int index = key->GetHash(capacity) << 1;
		Variant* cell = (Variant*)pValue + index;

		while (!(cell->usNull == c_null && cell->pValue))
		{
			cell += 2;

			if (cell >= (Variant*)pValue + capacity)
			{
				cell = (Variant*)pValue;
			}
		}
	}

	inline const unsigned short GetHash(const unsigned int cap) const
	{
		long res = lValue;

		if (usNull == c_null)
		{
			if (usType == VarType::STR)
			{
				for (char* c = (char*)pValue; c < (char*)pValue + usLength; ++c)
				{
					res -= *c;
				}
			}
			else if (usType == VarType::ARR)
			{
				//ToDo
			}
			else if (usType == VarType::DICT)
			{
				//ToDo
			}
		}

		return (const unsigned short)(res % cap);
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
	};

	void* pValue;
};