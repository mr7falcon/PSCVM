#include "Variant.h"
#include "VirtualMachine.h"

Variant::~Variant()
{
}

const string Variant::ToString() const
{
	if (usNull != c_null)
	{
		return std::to_string(dValue);
	}
	else if (usType == VarType::STR)
	{
		string str = "\'";
		str += (char*)pValue;
		str[(size_t)usLength + 1] = '\'';
		str.resize((size_t)usLength + 2);
		return str;
	}
	else if (usType == VarType::ARR)
	{
		string str = '{' + ((Variant*)pValue)->ToString();
		for (Variant* p = (Variant*)pValue + 1; p < (Variant*)pValue + usLength; ++p)
		{
			str = str + " | " + p->ToString();
		}
		str += '}';
		return str;
	}
	else if (usType == VarType::DICT)
	{
		Variant* p = (Variant*)pValue;
		string str = '{' + (p++)->ToString() + " : " + (p++)->ToString();
		const unsigned int len = usLength << 1;
		while (p < (Variant*)pValue + len)
		{
			str = str + " | " + (p++)->ToString() + " : " + (p++)->ToString();
		}
		str += '}';
		return str;
	}
	else if (!pValue)
	{
		return "NULL";
	}

	return "";
}

Variant Variant::FromBytes(byte** pc, VirtualMachine* pvm)
{
	Variant var(*((double*)*pc));
	*pc += sizeof(double);

	if (var.usNull == c_null)
	{
		if (var.usType == VarType::STR)
		{
			char* str = new char[var.usLength];
			memcpy(str, *pc, var.usLength);
			*pc += var.usLength;
			var.pValue = str;
		}
		else if (var.usType == VarType::ARR)
		{
			const unsigned short capacity = var.usLength + (var.usLength >> c_capInc);
			Variant* arr = pvm->HeapAlloc(capacity + 1);
			*(arr++) = Variant(capacity);

			for (Variant* p = arr; p < arr + var.usLength; ++p)
			{
				*p = Variant::FromBytes(pc, pvm);
			}

			var.pValue = arr;
		}
		else if (var.usType == VarType::DICT)
		{
			const unsigned int len = var.usLength << 1;
			const unsigned int capacity = len + (len >> c_capInc);
			Variant* dict = pvm->HeapAlloc(capacity + 1);
			*(dict++) = Variant(capacity);

			for (unsigned short i = 0; i < var.usLength; ++i)
			{
				Variant key = Variant::FromBytes(pc, pvm);
				Variant* cell = dict + key.GetHash(capacity);

				while (!(cell->usNull == c_null && cell->pValue))
				{
					cell += 2;

					if (cell >= dict + capacity)
					{
						cell = dict;
					}
				}

				*cell = key;
				*(++cell) = Variant::FromBytes(pc, pvm);
			}

			var.pValue = dict;
		}
	}

	return var;
}

bool Variant::Equal(Variant* op1, Variant* op2)
{
	if (op1->usNull != c_null && op2->usNull != c_null)
	{
		return op1->dValue == op2->dValue;
	}
	else if (op1->usType == VarType::STR && op2->usType == VarType::STR)
	{

		if (op1->usLength != op2->usLength)
		{
			return false;
		}

		for (unsigned short i = 0; i < op1->usLength; ++i)
		{
			if (((char*)op1->pValue + i) != ((char*)op2->pValue + i))
			{
				return false;
			}
		}

		return true;
	}
	else if (op1->usType == VarType::ARR && op2->usType == VarType::ARR)
	{
		if (op1->usLength != op2->usLength)
		{
			return false;
		}

		for (unsigned short i = 0; i < op1->usLength; ++i)
		{
			if (!Equal((Variant*)op1->pValue + i, (Variant*)op2->pValue + i))
			{
				return false;
			}
		}

		return true;
	}
	else if (op1->usType == VarType::DICT && op2->usType == VarType::DICT)
	{
		if (op1->usLength != op2->usLength)
		{
			return false;
		}

		for (unsigned int i = 0; i < ((unsigned int)op1->usLength << 1); ++i)
		{
			if (!Equal((Variant*)op1->pValue + i, (Variant*)op2->pValue + i))
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}