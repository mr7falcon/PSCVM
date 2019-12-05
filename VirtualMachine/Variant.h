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
				const unsigned short len = usLength << 1;
				for (Variant* p = (Variant*)pValue + 1; p < (Variant*)pValue + len; p += 2)
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

	inline Variant DictGet(Variant* key) const
	{
		const unsigned short index = key->GetHash();
		const unsigned short len = usLength << 1;

		Variant* var = (Variant*)pValue + index;

		while (!Equal(key, var))
		{
			var += 2;

			if (var >= (Variant*)pValue + len)
			{
				var = (Variant*)pValue;
			}
		}

		var->Copy();
		return *var;
	}

	inline void DictSet(Variant* key, Variant* val)
	{
		const unsigned short index = key->GetHash();
		const unsigned short len = usLength << 1;

		Variant* cell = (Variant*)pValue + index;

		while (!Equal(key, cell))
		{
			cell += 2;

			if (cell >= (Variant*)pValue + len)
			{
				cell = (Variant*)pValue;
			}
		}

		*(++cell) = *val;
	}

	inline const unsigned short GetHash() const
	{

	}

	static bool Equal(Variant* op1, Variant* op2);

	const string ToString() const;
	static Variant FromBytes(byte** pc, VirtualMachine* pvm);

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