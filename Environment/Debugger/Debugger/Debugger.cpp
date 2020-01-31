#include <fstream>
#include <iostream>
#include <ctime>
#include <string>
#include <Windows.h>

typedef unsigned char byte;
typedef void (__stdcall* vRun) (byte*);
typedef double(__stdcall* nRun) (byte*);
typedef void(__stdcall* sRun) (byte*, char*);

enum ReturnTypes : byte
{
	V,
	N,
	S
};

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		std::cout << "expected file name\n";
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
	
	vRun Run = (vRun)GetProcAddress(hVMDLL, "Run");
	nRun NumRun = (nRun)GetProcAddress(hVMDLL, "NumRun");
	sRun StrRun = (sRun)GetProcAddress(hVMDLL, "StrRun");
	
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

	switch (rtype)
	{
	case ReturnTypes::V:
		Run(code);
		break;
	case ReturnTypes::N:
		std::cout << NumRun(code) << std::endl;
		break;
	case ReturnTypes::S:
		char str[100];
		StrRun(code, str);
		std::cout << str << std::endl;
		break;
	}

	const unsigned int tEnd = clock();
	std::cout << tEnd - tStart << std::endl;

	FreeLibrary(hVMDLL);

	return 0;
}