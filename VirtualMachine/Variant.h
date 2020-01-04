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
	DESC
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

	inline static Variant ArrCreate(const unsigned short length, bool fixed = false)
	{
		Variant* arr;

		if (fixed)
		{
			arr = VirtualMachine::HeapAlloc(length + 1);
			*arr = Variant((const unsigned int)length);
		}
		else
		{
			const short int capacity = length + (length >> c_capInc);
			arr = VirtualMachine::HeapAlloc(capacity + 2);
			Variant* pLocalDesc = arr + 1;
			*pLocalDesc = Variant((const unsigned int)capacity);
			*arr = Variant(capacity, pLocalDesc);
		}

		return Variant(arr, length, VarType::ARR);
	}

	inline static Variant DictCreate(const unsigned short length, bool fixed = false)
	{
		Variant* dict;

		if (fixed)
		{
			const unsigned int realLen = (unsigned int)length << 1;
			dict = VirtualMachine::HeapAlloc(realLen + 1);
			*dict = Variant((const unsigned int)length);
		}
		else
		{
			const unsigned short capacity = length + (length >> Variant::c_capInc);
			dict = VirtualMachine::HeapAlloc((capacity << 1) + 2);
			Variant* pLocalDesc = dict + 1;
			*pLocalDesc = Variant((unsigned int)capacity);
			*dict = Variant(capacity, pLocalDesc);
		}

		return Variant(dict, length, VarType::DICT);
	}

	inline Variant ArrGet(const unsigned short i) const
	{
		if (i >= usLength)
		{
			//throw some exception
		}

		Variant* var;

		if (Variant* p = (Variant*)((Variant*)pValue)->pValue)
		{
			unsigned short index = i;
			while (index >= (unsigned short)p->nCap)
			{
				index -= (unsigned short)p->nCap;
				p = (Variant*)p->pValue;
			}
			var = p + 1 + index;
		}
		else
		{
			var = (Variant*)pValue + 1 + i;
		}

		var->Copy();
		return *var;
	}

	inline void ArrSet(const unsigned short i, Variant* var)
	{
		if (i >= usLength)
		{
			//throw some exception
		}

		if (Variant* p = (Variant*)((Variant*)pValue)->pValue)
		{
			unsigned short index = i;
			while (index >= (unsigned short)p->nCap)
			{
				index -= (unsigned short)p->nCap;
				p = (Variant*)p->pValue;
			}
			*(p + 1 + index) = *var;
		}
		else
		{
			*((Variant*)pValue + 1 + i) = *var;
		}
	}

	inline void PushBack(Variant* var)
	{
		Variant* pGlobalDesc = (Variant*)pValue;
		if (!pGlobalDesc->pValue)
		{
			//array is fixed, throw exception
		}

		const unsigned short cap = (unsigned short)((Variant*)pValue)->nCap;
		if (usLength == cap)
		{
			const unsigned short inc = cap >> c_capInc;
			Variant* pLocalDesc = VirtualMachine::HeapAlloc(inc + 1);
			*pLocalDesc = Variant((unsigned int)inc);
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
			unsigned short length = usLength;
			while (p->pValue)
			{
				length -= (unsigned short)p->nCap;
				p = (Variant*)p->pValue;
			}

			*(p + 1 + length) = *var;
		}

		++usLength;
	}

	inline Variant DictGet(Variant* key) const
	{
		const unsigned short capacity = (unsigned short)((Variant*)pValue)->nCap;
		unsigned short index = key->GetHash(capacity);

		Variant* cell;

		if (Variant* p = (Variant*)((Variant*)pValue)->pValue)
		{
			while (index >= (unsigned short)p->nCap)
			{
				index -= (unsigned short)p->nCap;
				p = (Variant*)p->pValue;
			}
			cell = p + 1 + ((unsigned int)index << 1);
		}
		else
		{
			cell = (Variant*)pValue + 1 + ((unsigned int)index << 1);
		}

		//while (!Equal(key, cell))
		//{
		//	cell += 2;

		//	if (cell->usNull == c_null && cell->pValue)
		//	{
		//		//key is missing, throw any exception
		//	}

		//	if (cell >= (Variant*)pValue + capacity)
		//	{
		//		cell = (Variant*)pValue;
		//	}
		//}

		(++cell)->Copy();
		return *cell;
	}

	inline void DictSet(Variant* key, Variant* val)
	{
		const unsigned short capacity = (unsigned short)((Variant*)pValue)->nCap;
		unsigned short index = key->GetHash(capacity);

		Variant* cell;

		if (Variant* p = (Variant*)((Variant*)pValue)->pValue)
		{
			while (index >= (unsigned short)p->nCap)
			{
				index -= (unsigned short)p->nCap;
				p = (Variant*)p->pValue;
			}
			cell = p + 1 + ((unsigned int)index << 1);
		}
		else
		{
			cell = (Variant*)pValue + 1 + ((unsigned int)index << 1);
		}

		//while (!Equal(key, cell))
		//{
		//	cell += 2;

		//	if (cell->usNull == c_null && cell->pValue)
		//	{
		//		//key is missing, throw any exception
		//	}

		//	if (cell >= (Variant*)pValue + capacity)
		//	{
		//		cell = (Variant*)pValue;
		//	}
		//}

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

		struct
		{
			unsigned int nCap;
			unsigned int nReplaced;
		};
	};

	void* pValue;
};