#include <stdio.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <TlHelp32.h>
#include <conio.h>
#include <fstream>
#include <iostream>
#include <functional>

#define DLL_IMPORT __declspec(dllimport)
#define VALUE "TestData"
#define ARGS DWORD pid, const char* data, const char* replacement

const char FUNCTION_NAME[] = "ReplaceData";
const char SEARCH_STR[] = "TestData";
const char REPLACEMENT[] = "Testerer";
const char PATH_TO_DLL[] = "LR3_DLL.dll";

typedef HMODULE(WINAPI* LPLoadLibrary)(LPCSTR);
typedef HMODULE(WINAPI* LPGetProcAddress)(HMODULE, LPCSTR);
typedef void __stdcall TReplaceData(ARGS);
extern "C" void DLL_IMPORT __stdcall ReplaceData(ARGS);

int callFunctionInStaticDll(DWORD pid);
int callFunctionInDynamicDll(DWORD pid);
TReplaceData* staticImport();
TReplaceData* dynamicImport();
void injectLibrary(DWORD procID);

int main()
{
	setlocale(LC_ALL, "Russian");
	const char data1[] = VALUE;
	const char data2[] = VALUE;
	DWORD pid = GetCurrentProcessId();

	printf("PID: %lu\n", pid);
	printf("Выберите тип (0 - динамически, 1 - статически, 2 - внедрение в процесс).\n");

	auto a = (char)getchar();

	switch (a)
	{
	case '0':
		callFunctionInDynamicDll(pid);
		break;
	case '1':
		callFunctionInStaticDll(pid);
		break;
	case '2':
		DWORD injectionPid;
		printf("Enter pid\n");
		scanf_s("%d", &injectionPid);
		injectLibrary(injectionPid);
		break;
	default:
		break;
	}

	printf("Оригинал: %s. Изменённое: %s\n", VALUE, data1);
	printf("Оригинал: %s. Изменённое: %s\n", VALUE, data2);

	system("pause");
	return 0;
}

// Вызов DLL-функции
int makeCall(TReplaceData func, DWORD pid) {
	if (func != NULL)
	{
		func(pid, SEARCH_STR, REPLACEMENT);
		return 1;
	}
	else
	{
		puts("DLL не найдено.");
		return 0;
	}
}

// Статически
int callFunctionInStaticDll(DWORD pid)
{
	TReplaceData* func = NULL;
	func = staticImport();
	return makeCall(func, pid);
}

TReplaceData* staticImport()
{
	return (TReplaceData*)ReplaceData;
}

// Динамически 
int callFunctionInDynamicDll(DWORD pid)
{
	TReplaceData* func = NULL;
	func = dynamicImport();
	return makeCall(func, pid);
}

TReplaceData* dynamicImport()
{
	HMODULE hDll = LoadLibraryA(PATH_TO_DLL);
	if (hDll == NULL)
		return NULL;
	auto* func = (TReplaceData*)GetProcAddress(hDll, FUNCTION_NAME);
	FreeLibrary(hDll);
	return func;
}

// Injection 
void injectLibrary(DWORD pid)
{
	HMODULE hKernel = LoadLibraryA("kernel32.dll"); //статическая библиотека
	if (hKernel) {
		auto loadLibrary = (LPLoadLibrary)GetProcAddress(hKernel, "LoadLibraryA"); //адрес функции
		HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		LPVOID path = VirtualAllocEx(hProc, NULL, strlen(PATH_TO_DLL) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		if (path)
		{
			WriteProcessMemory(hProc, path, PATH_TO_DLL, strlen(PATH_TO_DLL) + 1, NULL);
			HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibrary, path, NULL, NULL);

			if (hThread == NULL) {
				printf("Error.\n");
			}
			else {
				WaitForSingleObject(hThread, INFINITE);
				CloseHandle(hThread);
			}
		}

		FreeLibrary(hKernel);
		CloseHandle(hProc);
	}
}
