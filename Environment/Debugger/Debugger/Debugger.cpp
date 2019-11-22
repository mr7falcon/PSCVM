#include <fstream>
#include <iostream>
#include <ctime>
#include <string>
#include <Windows.h>

typedef unsigned char byte;
typedef bool (__stdcall *RunFunc) (byte*);

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "expected file name\n";
		return 1;
	}

	std::string path = argv[1];
	std::string ext = path.substr(path.find_last_of('.'));
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

	RunFunc VMRun = (RunFunc)GetProcAddress(hVMDLL, "VMRun");
	if (!VMRun)
	{
		std::cout << "Can't get Virtual Machine Run function\n";
		return 1;
	}

	std::ifstream file(argv[1], std::ifstream::ate | std::ifstream::binary);
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

	VMRun(code);

	const unsigned int tEnd = clock();
	std::cout << tEnd - tStart << std::endl;

	FreeLibrary(hVMDLL);

	return 0;
}