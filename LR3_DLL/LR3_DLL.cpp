#include "pch.h"
#include <Windows.h>
#include <process.h>
#include <vector>

#define DLL_EXPORT __declspec(dllexport)

using namespace std;

const char SEARCH_STR[] = "TestData";
const char REPLACEMENT[] = "Tester";

extern "C" void DLL_EXPORT __stdcall ReplaceData(DWORD pid, const char* data, const char* replacement)
{
	HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_TRUST_LABEL_SECURITY_INFORMATION | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (process)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		MEMORY_BASIC_INFORMATION info;
		const size_t dataLength = sizeof(data);
		const size_t replacementLength = sizeof(replacement);
		vector<char> chunk;
		char* p = 0;
		while (p < si.lpMaximumApplicationAddress)
		{
			if (VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info) && (info.State == MEM_COMMIT && info.AllocationProtect == PAGE_READWRITE))
			{
				p = (char*)info.BaseAddress;
				chunk.resize(info.RegionSize);
				SIZE_T bytesRead;
				if (ReadProcessMemory(process, p, &chunk[0], info.RegionSize, &bytesRead))
				{
					for (size_t i = 0; i < (bytesRead - dataLength); ++i)
					{
						if (memcmp(data, &chunk[i], dataLength) == 0)
						{
							char* ref = p + i;
							for (int j = 0; j < replacementLength; j++) {
								ref[j] = replacement[j];
							}
							ref[replacementLength] = 0;
						}
					}
				}
			}
			p += info.RegionSize;
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	DWORD pid = _getpid();
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
		ReplaceData(pid, SEARCH_STR, REPLACEMENT);
		break;
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}