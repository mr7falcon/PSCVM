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

	Variant(Variant* arr, const unsigned short length)
		: usNull(c_null),
		  usType(VarType::ARR),
		  usLength(length),
		  usRef(1),
		  pValue(arr)
	{}

	~Variant();

	inline void Free()
	{
		if (pValue)
		{
			if (usType == VarType::STR && --usRef <= 0)
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

	inline Variant ArrGet(const unsigned short i)
	{
		if (usNull == c_null && usType == VarType::ARR && i < usLength)
		{
			Variant* var = (Variant*)pValue + i;
			var->Copy();
			return *var;
		}
		else
		{
			return Variant();
		}
	}

	inline void ArrSet(const unsigned short i, Variant var)
	{
		if (usNull == c_null && usType == VarType::ARR && i < usLength)
		{
			*((Variant*)pValue + i) = var;
		}
		//else throw any exception
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