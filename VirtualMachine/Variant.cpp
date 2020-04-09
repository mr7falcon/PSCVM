#include "Variant.h"
#include "VirtualMachine.h"

const string Variant::ToString()
{
	if (usNull != c_null)
	{
		return std::to_string(dValue);
	}
	else if (!pValue)
	{
		return "NULL";
	}
	else if (usType == VarType::STR)
	{
		string str = "\'";
		str.append((char*)pValue, nLength);
		str += '\'';
		return str;
	}
	else
	{
		string str = "{";
		
		if (usType == VarType::ARR)
		{
			auto f = [&](Variant* elem)
			{
				str = str + " " + elem->ToString() + " |";
			};

			ForEach(f);

			str[str.length() - 1] = '}';
		}
		else if (usType == VarType::DICT)
		{
			auto f = [&](Variant* elem)
			{
				str = str + " " + (elem + KEY)->ToString() + " : " + (elem + VALUE)->ToString() + " |";
			};

			ForEachBucket(f);

			str[str.length() - 1] = '}';
		}

		return str;
	}

	return "";
}

bool Variant::Equal(Variant* op1, Variant* op2)
{
	if (op1->usNull == c_null && op2->usNull == c_null)
	{
		Variant* pValue1 = (Variant*)op1->pValue;
		Variant* pValue2 = (Variant*)op2->pValue;
		if (!pValue1 && !pValue2)
		{
			return true;
		}

		unsigned short type = op1->usType;
		
		if (type != op2->usType)
		{
			return false;
		}

		if (!pValue1 || !pValue2)
		{
			return false;
		}

		if (op1->nLength != op2->nLength)
		{
			return false;
		}

		if (type == VarType::STR)
		{
			for (unsigned int i = 0; i < op1->nLength; ++i)
			{
				if (*((char*)pValue1 + i) != *((char*)pValue2 + i))
				{
					return false;
				}
			}

			return true;
		}
		else
		{
			Variant* pGlobalDesc1 = pValue1;
			Variant* pGlobalDesc2 = pValue2;
			Variant* pLocalDesc1 = (Variant*)pGlobalDesc1->pValue;
			Variant* pLocalDesc2 = (Variant*)pGlobalDesc2->pValue;
			Variant* pArr1 = pLocalDesc1 + 1;
			Variant* pArr2 = pLocalDesc2 + 1;
			Variant* stop1 = pArr1 + pLocalDesc1->nCap;
			Variant* stop2 = pArr2 + pLocalDesc2->nCap;

			if (type == VarType::ARR)
			{
				for (unsigned int i = 0; i < op1->nLength; ++i)
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

					if (!Equal(pArr1, pArr2))
					{
						return false;
					}

					++pArr1;
					++pArr2;
				}
			}
			else if (type == VarType::DICT)
			{
				Variant* bucket1 = nullptr;
				Variant* bucket2 = nullptr;

				for (unsigned int i = 0; i < op1->nLength; ++i)
				{
					while (!bucket1)
					{
						if (pArr1 == stop1)
						{
							pLocalDesc1 = (Variant*)pLocalDesc1->pValue;
							pArr1 = pLocalDesc1 + 1;
							stop1 = pArr1 + pLocalDesc1->nCap;
						}

						if (pArr1->pValue)
						{
							bucket1 = (Variant*)pArr1->pValue;
						}
						else
						{
							++pArr1;
						}
					}

					while (!bucket2)
					{
						if (pArr2 == stop2)
						{
							pLocalDesc2 = (Variant*)pLocalDesc2->pValue;
							pArr2 = pLocalDesc2 + 1;
							stop2 = pArr2 + pLocalDesc2->nCap;
						}

						if (pArr2->pValue)
						{
							bucket2 = (Variant*)pArr2->pValue;
						}
						else
						{
							++pArr2;
						}
					}

					if (!Equal(bucket1 + KEY, bucket2 + KEY) || !Equal(bucket1 + VALUE, bucket2 + VALUE))
					{
						return false;
					}

					bucket1 = (Variant*)(bucket1 + NEXT)->pValue;
					bucket2 = (Variant*)(bucket2 + NEXT)->pValue;
				}
			}

			return true;
		}
	}
	else
	{
		return op1->dValue == op2->dValue;
	}
}

const unsigned long Variant::GetHash()
{
	unsigned long hash = 0;

	if (usNull == c_null)
	{
		if (usType == VarType::STR)
		{
			static const unsigned short p = 73;
			unsigned long p_pow = p;
			const char* pStop = (char*)pValue + nLength;
			for (char* str = (char*)pValue; str < pStop; ++str)
			{
				hash += (unsigned long)(*str * p_pow);
				p_pow *= p;
			}
		}
		else if (usType == VarType::ARR)
		{
			auto f = [&](Variant* elem)
			{
				hash ^= elem->GetHash();
			};

			ForEach(f);
		}
		else if (usType == VarType::DICT)
		{
			auto f = [&](Variant* elem)
			{
				hash ^= ((elem + KEY)->GetHash() - (elem + VALUE)->GetHash());
			};

			ForEachBucket(f);
		}
	}
	else
	{
		hash = (unsigned long)dValue;
	}

	return hash;
}