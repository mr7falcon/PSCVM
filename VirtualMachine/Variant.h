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

	//Функция уменьшения счетчика ссылок элемента.
	//В случае, если элемент хранит строку, и ее счетчик
	//ссылок обнулился, память под нее очищается
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

	//Функция увеличения счетчика ссылок элемента
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

	//Функция проверки типа элемента. В случае несовпадения
	//генерирует исключение.
	//Аргументы
	//expected - ожидаемый тип
	inline void CheckType(VarType expected) const
	{
		if (usNull != c_null || usType != expected)
		{
			throw ex_wrongType((VarType)usType);
		}
	}

	inline void CheckBounds(const unsigned int pos) const
	{
		if (pos >= nLength)
		{
			throw ex_outOfBounds(pos);
		}
	}

	//Функция получения элемента массива по индексу
	//Аргументы
	//i - индекс элемента в массиве
	//Возвращаемое значение - указатель на элемент в куче
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

	//Функция для прохода по элементам массива
	//Аргументы
	//f - указатель на процедуру-обработчик
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
	
	//Функция для симметричного прохода по элементам двух массивов
	//Аргументы
	//var1 - указатель на элемент первого массива
	//var2 - указатель на элемент второго массива
	//f - указатель на процедуру-обработчик
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

	//Функция для прохода по элементам словаря
	//Аргументы
	//f - указатель на процедуру-обработчик
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

	//Функция добавления локального дескриптора в конец
	//списка дескрипторов массива в куче
	//Аргументы
	//pLocalDesc - указатель на добавляемый локальный дескриптор
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

	//Функция поиска ключа в словаре
	//Аргументы
	//key - указатель на ключ
	//Возвращаемое значение - указатель на значение по ключу в массиве,
	//если такой ключ существует. Иначе - генерируется исключение
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

	//Функция вставки структуры ключ-значение в цепочку словаря
	//Аргументы
	//bucket - указатель на структуру ключ-значение
	//entry - указатель на точку входа в цепочку словаря
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

	//Функция увеличения вместимости словаря
	//Аргументы
	//pNewDesc - указатель на новый локальный дескриптор
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

	//Функция получения хэша элемента
	//Возвращаемое значение - хэш элемента
	const unsigned long GetHash();

	inline void PopBack() noexcept
	{
		--nLength;
	}

	//Функция удаления элемента из словаря по ключу
	//Аргументы
	//key - указатель на ключ
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

	//Функция конкатенации строк/массивов
	//Аргументы
	//op - указатель на присоединяемый элемент
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

	//Функция сравнения элементов
	//Аргументы
	//op1 - указатель первый элемент
	//op2 - указатель на второй элемент
	static bool Equal(Variant* op1, Variant* op2);
	
	//Функция дупликации массива в куче
	//Аргументы
	//pGlobalDesc2 - указатель на глобальный дескриптор
	//нового массива
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

	//Функция преобразования словаря в массив
	//Аргументы
	//pGlobalDesc2 - указатель на глобальный дескриптор нового массива
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

	//Функция проверки содержания словарем элемента по ключу
	//Аргументы
	//key - указатель на ключ
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

	//Функция проверки повторения суффикса в строке
	//Аргументы
	//str - указатель на строку
	//strlen - длина строки
	//offset - смещение поиска
	//sufflen - длина суффикса
	//Возвращаемое значение - переменная, сингализирующая о повторении суффикса
	static inline bool SuffMatch(char* str, const unsigned int strlen, const unsigned int offset, const unsigned int sufflen)
	{
		return offset > sufflen ?
			str[offset - sufflen - 1] != str[strlen - sufflen - 1] && memcmp(str + strlen - sufflen, str + offset - sufflen, sufflen) == 0 :
			memcmp(str + strlen - offset, str, offset) == 0;
	}

	static inline unsigned int Max(const unsigned int a, const unsigned int b) noexcept
	{
		return a > b ? a : b;
	}

	//Функция поиска подстроки в строке
	//Аргументы
	//varstr - подстрока
	//Возвращаемое значение - индекс подстроки в строке в случае
	//ее нахождения, иначе - -1
	inline long Match(Variant* varstr) const
	{
		CheckType(VarType::STR);

		const unsigned int substrlen = varstr->nLength;
		CheckBounds(substrlen);

		unsigned int* suffTable = new unsigned int[substrlen];
		constexpr unsigned int alphSize = UCHAR_MAX + 1;
		long stopTable[alphSize];

		char* str = (char*)pValue;
		char* substr = (char*)varstr->pValue;
		
		const long* stopTableEnd = stopTable + alphSize;
		for (long* i = stopTable; i < stopTableEnd; ++i)
		{
			*i = -1;
		}

		for (unsigned int i = 0; i < substrlen - 1; ++i)
		{
			stopTable[substr[i]] = i;
		}

		for (unsigned int i = 0; i < substrlen; ++i)
		{
			unsigned int offset = substrlen;
			while (offset > 0 && SuffMatch(substr, substrlen, offset, i))
			{
				--offset;
			}
			suffTable[substrlen - i - 1] = substrlen - offset;
		}

		for (unsigned int i = 0; i <= nLength - substrlen;)
		{
			unsigned int j = substrlen - 1;
			char* a = substr + j;
			char* b = str + j + i;
			for ( ; *a == *b; --a, --b)
			{
				if (j == 0)
				{
					return i;
				}

				--j;
			}

			i += Max(suffTable[j], j - stopTable[*b]);
		}

		delete[](suffTable);

		return -1;
	}

	//Функция преобразования элемента в строку
	//Возвращаемое значение - результирующая строка
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