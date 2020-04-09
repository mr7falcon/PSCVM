#pragma once

#include <string>
#include <functional>

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
	static const char c_maxListSize = 4;

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

	inline void CheckType(VarType expected) const
	{
		if (usNull != c_null || usType != expected)
		{
			throw ex_wrongType((VarType)usType);
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

	inline void ForEach(std::function<void(Variant*)> f)
	{
		Variant* pGlobalDesc = (Variant*)pValue;
		Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue;
		Variant* pArr = pLocalDesc + 1;
		Variant* pStop = pArr + pLocalDesc->nCap;

		for (unsigned int i = 0; i < nLength; ++i, ++pArr)
		{
			if (pArr == pStop)
			{
				pLocalDesc = (Variant*)pLocalDesc->pValue;
				pArr = pLocalDesc + 1;
				pStop = pArr + pLocalDesc->nCap;
			}

			f(pArr);
		}
	}
	
	static inline void ForEach2(Variant* var1, Variant* var2, std::function<void(Variant*, Variant*)> f)
	{
		Variant* pGlobalDesc1 = (Variant*)var1->pValue;
		Variant* pGlobalDesc2 = (Variant*)var2->pValue;
		Variant* pLocalDesc1 = (Variant*)pGlobalDesc1->pValue;
		Variant* pLocalDesc2 = (Variant*)pGlobalDesc2->pValue;
		Variant* pArr1 = pLocalDesc1 + 1;
		Variant* pArr2 = pLocalDesc2 + 1;
		Variant* stop1 = pArr1 + pLocalDesc1->nCap;
		Variant* stop2 = pArr2 + pLocalDesc2->nCap;
		const unsigned int count = var1->nLength;

		for (unsigned int i = 0; i < count; ++i, ++pArr1, ++pArr2)
		{
			if (pArr1 == stop1)
			{
				pLocalDesc1 = (Variant*)pLocalDesc1->pValue;
				pArr1 = pLocalDesc1 + 1;
				stop1 = pArr1 + pLocalDesc1->nCap;
			}

			if (pArr2 == stop2)
			{
				pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
				pArr2 = pLocalDesc2 + 1;
				stop2 = pArr2 + pLocalDesc2->nCap;
			}

			f(pArr1, pArr2);
		}
	}

	inline void ForEachBucket(std::function<void(Variant*)> f)
	{
		Variant* pGlobalDesc = (Variant*)pValue;
		Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue;
		Variant* pArr = pLocalDesc + 1;
		Variant* pStop = pArr + pLocalDesc->nCap;
		Variant* pBucket;
		unsigned int len = nLength;

		while (true)
		{
			for (; pArr < pStop; ++pArr)
			{
				if (pArr->pValue)
				{
					pBucket = (Variant*)pArr->pValue;
					while (pBucket)
					{
						f(pBucket);

						pBucket = (Variant*)(pBucket + NEXT)->pValue;
						if (--len == 0)
						{
							return;
						}
					}
				}
			}

			pLocalDesc = (Variant*)pLocalDesc->pValue;
			pArr = pLocalDesc + 1;
			pStop = pArr + pLocalDesc->nCap;
		}
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

	inline void Insert(Variant* bucket, Variant* entry)
	{
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
	}

	inline void DictResize(Variant* pNewDesc)
	{
		PushBack(pNewDesc++);

		Variant* pGlobalDesc = (Variant*)pValue;
		const unsigned int cap = pGlobalDesc->nCap;
		Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue;
		Variant* pVar = pLocalDesc + 1;
		const Variant* pStop = pVar + pLocalDesc->nCap;

		for (unsigned int i = 0; i < cap; ++i)
		{
			if (pVar == pStop)
			{
				pLocalDesc = (Variant*)pLocalDesc->pValue;
				pVar = pLocalDesc + 1;
				pStop = pVar + pLocalDesc->nCap;
			}

			if (pVar->pValue)
			{
				Variant* prev = pVar;
				Variant* pBucket = (Variant*)pVar->pValue;
				while (pBucket)
				{
					Variant* next = pBucket + NEXT;
					if (((next->lValue--) & 1) == 1)
					{
						prev->pValue = next->pValue;
						next->pValue = nullptr;
						Variant* pEntry = pNewDesc + i;
						Insert(pBucket, pEntry);
						--pVar->lValue;
						pBucket = (Variant*)prev->pValue;
					}
					else
					{
						prev = next;
						pBucket = (Variant*)next->pValue;
					}
				}
			}

			++pVar;
		}

		pGlobalDesc->nCap += cap;
	}

	const unsigned long GetHash();

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

		return Variant(pGlobalDesc2, nLength, VarType::ARR);
	}

	inline void DictToArr(Variant* pGlobalDesc2)
	{
		static const unsigned short doubVariantSize = sizeof(Variant) << 1;

		Variant* pGlobalDesc1 = (Variant*)pValue;
		Variant* pLocalDesc2 = (Variant*)pGlobalDesc2->pValue;
		Variant* pArr2 = pLocalDesc2 + 1;
		Variant* stop2 = pArr2 + pLocalDesc2->nCap;

		auto f = [&](Variant* elem)
		{
			if (pArr2 == stop2)
			{
				pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
				pArr2 = pLocalDesc2 + 1;
				stop2 = pArr2 + pLocalDesc2->nCap;
			}

			*pArr2++ = *(elem + KEY);
			if (pArr2 == stop2)
			{
				pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
				pArr2 = pLocalDesc2 + 1;
				stop2 = pArr2 + pLocalDesc2->nCap;
			}

			*pArr2++ = *(elem + VALUE);
		};

		ForEachBucket(f);
	}

	inline bool Contains(Variant* key) const
	{
		if (usNull != c_null || usType != VarType::DICT)
		{
			throw ex_wrongType((VarType)usType);
		}

		const unsigned int capacity = ((Variant*)pValue)->nCap;
		unsigned int index = key->GetHash() % capacity;

		Variant* bucket = (Variant*)Get(index)->pValue;

		while (bucket)
		{
			if (Equal(key, bucket + KEY))
			{
				return true;
			}

			bucket = (Variant*)(bucket + NEXT)->pValue;
		}

		return false;
	}

	const string ToString();

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
		unsigned long lValue;

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

inline const unsigned int DictSizGen(const unsigned int num) noexcept
{
	unsigned int res = 1;
	while (num > res)
	{
		res <<= 1;
	}
	return res;
}