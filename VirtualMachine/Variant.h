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

	Variant(char* str, const int length)
		: usNull(c_null),
		  usType(VarType::STR),
		  nLength(length)
	{
		pValue = new SmartPtr(str);
	}

	Variant(Variant* arr, const int length)
		: usNull(c_null),
		  usType(VarType::ARR),
		  nLength(length)
	{
		pValue = new SmartPtr(arr);
	}

	Variant(Variant& var)
		: pValue(var.pValue),
		  dValue(var.dValue)
	{
		if (pValue)
			++pValue->rc;
	}

	~Variant();

	inline void Free()
	{
		if (pValue && --pValue->rc <= 0)
		{
			if (usType == VarType::STR)
			{
				delete[]((char*)pValue->p);
			}
			else if (usType == VarType::ARR)
			{
				delete[]((Variant*)pValue->p);
			}

			delete(pValue);
			pValue = nullptr;
			usNull = c_null;
		}
	}

	inline Variant operator[](const int pos)
	{
		if (usNull == c_null && usType == VarType::ARR)
		{
			if (pos < nLength)
			{
				return *((Variant*)pValue->p + pos);
			}
		}

		return {};
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
			int nLength;
		};
	};

	struct SmartPtr
	{
		SmartPtr(void* p)
			: p(p),
			  rc(1)
		{}

		void* p;
		int rc;
	} *pValue;
};