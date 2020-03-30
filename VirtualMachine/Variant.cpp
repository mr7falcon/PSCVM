#include "Variant.h"
#include "VirtualMachine.h"

const string Variant::ToString() const
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
		
		Variant* pGlobalDesc = (Variant*)pValue;
		Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue;
		Variant* pArr = pLocalDesc + 1;
		Variant* stop = pArr + pLocalDesc->nCap;

		if (usType == VarType::ARR)
		{
			const unsigned int length = nLength - 1;

			str += (pArr++)->ToString();

			for (unsigned int i = 0; i < length; ++i)
			{
				if (pArr == stop)
				{
					pLocalDesc = (Variant*)pLocalDesc->pValue;
					pArr = pLocalDesc + 1;
					stop = pArr + pLocalDesc->nCap;
				}

				str = str + " | " + pArr->ToString();
				++pArr;
			}

			str += '}';
			return str;
		}
		else if (usType == VarType::DICT)
		{
			unsigned int length = nLength;
			Variant* bucket;
			while (true)
			{
				for (; pArr <= stop; ++pArr)
				{
					if (pArr->pValue)
					{
						bucket = (Variant*)pArr->pValue;
						while (bucket)
						{
							str = str + (bucket + KEY)->ToString() + " : " + (bucket + VALUE)->ToString();
							bucket = (Variant*)(bucket + NEXT)->pValue;
							if (--length == 0)
							{
								str += '}';
								return str;
							}
							else
							{
								str += " | ";
							}
						}
					}
				}

				pLocalDesc = (Variant*)pLocalDesc->pValue;
				pArr = pLocalDesc + 1;
				stop = pArr + pLocalDesc->nCap;
			}
		}
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

const unsigned long Variant::GetHash() const
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
		else
		{
			Variant* pGlobalDesc = (Variant*)pValue;
			Variant* pLocalDesc = (Variant*)pGlobalDesc->pValue;
			Variant* pArr = pLocalDesc + 1;
			Variant* stop = pArr + pLocalDesc->nCap;

			if (usType == VarType::ARR)
			{
				for (unsigned int i = 0; i < nLength; ++i)
				{
					if (pArr == stop)
					{
						pLocalDesc = (Variant*)pLocalDesc->pValue;
						pArr = pLocalDesc + 1;
						stop = pArr + pLocalDesc->nCap;
					}

					hash ^= pArr->GetHash();
					++pArr;
				}
			}
			else if (usType == VarType::DICT)
			{
				unsigned int length = nLength;
				Variant* bucket;
				while (true)
				{
					const unsigned int capacity = pLocalDesc->nCap;

					for (; pArr <= stop; ++pArr)
					{
						if (pArr->pValue)
						{
							bucket = (Variant*)pArr->pValue;
							while (bucket)
							{
								hash ^= ((bucket + KEY)->GetHash() - (bucket + VALUE)->GetHash());
								bucket = (Variant*)(bucket + NEXT)->pValue;
								if (--length == 0)
								{
									return hash;
								}
							}
						}
					}

					pLocalDesc = (Variant*)pLocalDesc->pValue;
					pArr = pLocalDesc + 1;
					stop = pArr + pLocalDesc->nCap;
				}
			}
		}
	}
	else
	{
		hash = (unsigned long)dValue;
	}

	return hash;
}