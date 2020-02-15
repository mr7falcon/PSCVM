#include <fstream>
#include <iostream>
#include <ctime>
#include <string>
#include <Windows.h>

typedef unsigned char byte;
typedef void (__stdcall* vRun0) (byte*);
typedef void(__stdcall* vRun1) (byte*, byte*);
typedef void(__stdcall* vRun2) (byte*, byte*, byte*);
typedef void(__stdcall* vRun3) (byte*, byte*, byte*, byte*);
typedef void(__stdcall* vRun4) (byte*, byte*, byte*, byte*, byte*);
typedef double(__stdcall* nRun0) (byte*);
typedef double(__stdcall* nRun1) (byte*, byte*);
typedef double(__stdcall* nRun2) (byte*, byte*, byte*);
typedef double(__stdcall* nRun3) (byte*, byte*, byte*, byte*);
typedef double(__stdcall* nRun4) (byte*, byte*, byte*, byte*, byte*);
typedef void(__stdcall* sRun0) (byte*, char*);
typedef void(__stdcall* sRun1) (byte*, char*, byte*);
typedef void(__stdcall* sRun2) (byte*, char*, byte*, byte*);
typedef void(__stdcall* sRun3) (byte*, char*, byte*, byte*, byte*);
typedef void(__stdcall* sRun4) (byte*, char*, byte*, byte*, byte*, byte*);

enum ReturnTypes : byte
{
	V,
	N,
	S
};

const byte maxArgs = 4;
byte* args[maxArgs];
byte numArg = 0;

byte* ArgToByte(char* arg)
{
	byte* res;

	if (*arg == '\'')
	{
		const char* sStart = arg + 1;
		const unsigned short len = strchr(sStart, '\'') - sStart;
		res = new byte[len + 1];
		memcpy(res, sStart, len);
		res[len] = '\0';
	}
	else
	{
		res = new byte[sizeof(double)];
		char* ptr;
		*((double*)res) = strtod(arg, &ptr);
	}

	args[numArg++] = res;
	return res;
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		std::cout << "expected format: >Debugger.exe <-v/-n/-s> <filename> <args>\n";
		return 1;
	}

	ReturnTypes rtype;

	const char* key = argv[1];
	if (!strcmp(key, "-v"))
	{
		rtype = ReturnTypes::V;
	}
	else if (!strcmp(key, "-n"))
	{
		rtype = ReturnTypes::N;
	}
	else if (!strcmp(key, "-s"))
	{
		rtype = ReturnTypes::S;
	}

	const std::string path = argv[2];
	const std::string ext = path.substr(path.find_last_of('.'));
	if (ext != ".bpsc")
	{
		std::cout << "wrong file extension\n";
		return 1;
	}

#if defined _DEBUG
	HINSTANCE hVMDLL = LoadLibrary("../../../../x64/Debug/VirtualMachine_x64.dll");
#else
	HINSTANCE hVMDLL = LoadLibrary("../../../../x64/Release/VirtualMachine_x64.dll");
#endif

	if (hVMDLL == NULL)
	{
		std::cout << "VirtualMachine.dll loading error\n";
		return 1;
	}
	
	vRun0 Run0 = (vRun0)GetProcAddress(hVMDLL, "Run0");
	vRun1 Run1 = (vRun1)GetProcAddress(hVMDLL, "Run1");
	vRun2 Run2 = (vRun2)GetProcAddress(hVMDLL, "Run2");
	vRun3 Run3 = (vRun3)GetProcAddress(hVMDLL, "Run3");
	vRun4 Run4 = (vRun4)GetProcAddress(hVMDLL, "Run4");
	nRun0 NumRun0 = (nRun0)GetProcAddress(hVMDLL, "NumRun0");
	nRun1 NumRun1 = (nRun1)GetProcAddress(hVMDLL, "NumRun1");
	nRun2 NumRun2 = (nRun2)GetProcAddress(hVMDLL, "NumRun2");
	nRun3 NumRun3 = (nRun3)GetProcAddress(hVMDLL, "NumRun3");
	nRun4 NumRun4 = (nRun4)GetProcAddress(hVMDLL, "NumRun4");
	sRun0 StrRun0 = (sRun0)GetProcAddress(hVMDLL, "StrRun0");
	sRun1 StrRun1 = (sRun1)GetProcAddress(hVMDLL, "StrRun1");
	sRun2 StrRun2 = (sRun2)GetProcAddress(hVMDLL, "StrRun2");
	sRun3 StrRun3 = (sRun3)GetProcAddress(hVMDLL, "StrRun3");
	sRun4 StrRun4 = (sRun4)GetProcAddress(hVMDLL, "StrRun4");

	std::ifstream file(path, std::ifstream::ate | std::ifstream::binary);
	if (!file.is_open())
	{
		std::cout << "file opening error\n";
		return 1;
	}

	const long length = (long)file.tellg();
	file.seekg(0);
	byte* code = new byte[length];
	file.read((char*)code, length);

	const unsigned int tStart = clock();

	try
	{
		switch (rtype)
		{
		case ReturnTypes::V:
		{
			switch (argc)
			{
			case 3:
				Run0(code);
				break;
			case 4:
				Run1(code, ArgToByte(argv[3]));
				break;
			case 5:
				Run2(code, ArgToByte(argv[3]), ArgToByte(argv[4]));
				break;
			case 6:
				Run3(code, ArgToByte(argv[3]), ArgToByte(argv[4]), ArgToByte(argv[5]));
				break;
			case 7:
				Run4(code, ArgToByte(argv[3]), ArgToByte(argv[4]), ArgToByte(argv[5]), ArgToByte(argv[6]));
				break;
			default:
				std::cout << "max arguments count is 4" << std::endl;
				return 1;
			}
		}
		break;
		case ReturnTypes::N:
		{
			double num = 0;

			switch (argc)
			{
			case 3:
				num = NumRun0(code);
				break;
			case 4:
				num = NumRun1(code, ArgToByte(argv[3]));
				break;
			case 5:
				num = NumRun2(code, ArgToByte(argv[3]), ArgToByte(argv[4]));
				break;
			case 6:
				num = NumRun3(code, ArgToByte(argv[3]), ArgToByte(argv[4]), ArgToByte(argv[5]));
				break;
			case 7:
				num = NumRun4(code, ArgToByte(argv[3]), ArgToByte(argv[4]), ArgToByte(argv[5]), ArgToByte(argv[6]));
				break;
			default:
				std::cout << "max arguments count is 4" << std::endl;
				return 1;
			}

			std::cout << num << std::endl;
		}
		break;
		case ReturnTypes::S:
		{
			char str[100];

			switch (argc)
			{
			case 3:
				StrRun0(code, str);
				break;
			case 4:
				StrRun1(code, str, ArgToByte(argv[3]));
				break;
			case 5:
				StrRun2(code, str, ArgToByte(argv[3]), ArgToByte(argv[4]));
				break;
			case 6:
				StrRun3(code, str, ArgToByte(argv[3]), ArgToByte(argv[4]), ArgToByte(argv[5]));
				break;
			case 7:
				StrRun4(code, str, ArgToByte(argv[3]), ArgToByte(argv[4]), ArgToByte(argv[5]), ArgToByte(argv[6]));
				break;
			default:
				std::cout << "max arguments count is 4" << std::endl;
				return 1;
			}

			std::cout << str << std::endl;
		}
		break;
		}
	}
	catch (std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
	}

	const unsigned int tEnd = clock();
	std::cout << tEnd - tStart << std::endl;

	FreeLibrary(hVMDLL);

	for (byte i = 0; i < maxArgs; ++i)
	{
		if (args[i])
		{
			delete[](args[i]);
		}
	}

	return 0;
}