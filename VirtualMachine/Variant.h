#pragma once

#include <string>

using std::string;

typedef unsigned char byte;

class VirtualMachine;

enum VarType : unsigned short
{
	STR,
	ARR,
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
		if (pValue && --usRef <= 0)
		{
			if (usType == VarType::STR)
			{
				delete[]((char*)pValue);
			}

			pValue = nullptr;
			usNull = c_null;
		}
	}

	inline void Copy()
	{
		if (pValue)
		{
			++usRef;
		}
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