#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <VersionHelpers.h>

#pragma pack(1)


#include <Windows.h>
#include <string>
#include <Strsafe.h>
#include <iostream>
#include <time.h> 
#include <fstream>
#include <Windows.h>
#include <string>
#include <iostream>
#include <time.h> 
#define SELF_REMOVE_STRING  TEXT("cmd.exe /C ping 1.1.1.1 -n 1 -w 3000 > Nul & Del \"%s\"")
void DelMe()
{
	remove("C:\\Windows\\Proxy.dll");
	using namespace std;
	TCHAR szModuleName[MAX_PATH];
	TCHAR szCmd[2 * MAX_PATH];
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	GetModuleFileName(NULL, szModuleName, MAX_PATH);

	StringCbPrintf(szCmd, 2 * MAX_PATH, SELF_REMOVE_STRING, szModuleName);

	CreateProcess(NULL, szCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

// Start Protection
bool FoundDebugger = false;
inline bool Has2DBreakpointHandler()
{
	__try { __asm INT 0x2D }
	__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	return true;
}
inline bool Has03BreakpointHandler()
{
	__try { __asm INT 0x03 }
	__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	return true;
}
typedef void(*_recurse)();
void recurse1(); void recurse2();
void recurse3(); void recurse4();
void recurse5();
_recurse recfuncs[5] = {
	&recurse1, &recurse2, &recurse3,
	&recurse4, &recurse5
};
void recurse1() { recfuncs[rand() % 5](); }
void recurse2() { recfuncs[(rand() % 3) + 2](); }
void recurse3()
{
	if (rand() % 100 < 50) recurse1();
	else recfuncs[(rand() % 3) + 1]();
}
void recurse4() { recfuncs[rand() % 2](); }
void recurse5()
{
	for (int i = 0; i < 100; i++)
		if (rand() % 50 == 1)
			recfuncs[i % 5]();
	recurse5();
}

void SelfDestruct()
{
	std::vector<char*> explosion;
	while (true)
		explosion.push_back(new char[10000]);
}

void BlueScreenOfDeath()
{
	typedef long (WINAPI *RtlSetProcessIsCritical)
		(BOOLEAN New, BOOLEAN *Old, BOOLEAN NeedScb);
	auto ntdll = LoadLibraryA("ntdll.dll");
	if (ntdll) {
		auto SetProcessIsCritical = (RtlSetProcessIsCritical)
			GetProcAddress(ntdll, "RtlSetProcessIsCritical");
		if (SetProcessIsCritical)
			SetProcessIsCritical(1, 0, 0);
	}
}

bool HasHardwareBreakpoints()
{
	CONTEXT ctx = { 0 };
	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	auto hThread = GetCurrentThread();
	if (GetThreadContext(hThread, &ctx) == 0)
		return false;
	FoundDebugger = true;
	return (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0);
}

inline bool CanCallOutputDebugString()
{
	SetLastError(0);
	OutputDebugStringA("Nice Try");
	return (GetLastError() == 0);
}

bool DebuggerDriversPresent()
{
	// an array of common debugger driver device names
	const char drivers[9][20] = {
		"\\\\.\\EXTREM", "\\\\.\\ICEEXT",
		"\\\\.\\NDBGMSG.VXD", "\\\\.\\RING0",
		"\\\\.\\SIWVID", "\\\\.\\SYSER",
		"\\\\.\\TRW", "\\\\.\\SYSERBOOT",
		"\0"
	};
	for (int i = 0; drivers[i][0] != '\0'; i++) {
		auto h = CreateFileA(drivers[i], 0, 0, 0, OPEN_EXISTING, 0, 0);
		if (h != INVALID_HANDLE_VALUE)
		{
			CloseHandle(h);
			FoundDebugger = true;
			return true;
		}
	}
	return false;
}

bool IsRemoteDebuggerPresent()
{
	BOOL dbg = false;
	CheckRemoteDebuggerPresent(GetCurrentProcess(), &dbg);
	if (dbg == true)
	{
		FoundDebugger = true;
	}
	return dbg;
}


typedef struct _LSA_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING, UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_TYPE_INFORMATION {
	UNICODE_STRING TypeName;
	ULONG TotalNumberOfHandles;
	ULONG TotalNumberOfObjects;
}OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

// Returned by the ObjectAllTypeInformation class
// passed to NtQueryObject
typedef struct _OBJECT_ALL_INFORMATION {
	ULONG NumberOfObjects;
	OBJECT_TYPE_INFORMATION ObjectTypeInformation[1];
}OBJECT_ALL_INFORMATION, *POBJECT_ALL_INFORMATION;

#pragma pack()

// CheckCloseHandle will call CloseHandle on an invalid
// DWORD aligned value and if a debugger is running an exception
// will occur and the function will return true otherwise it'll
// return false
inline bool CheckDbgPresentCloseHandle()
{
	HANDLE Handle = (HANDLE)0x8000;
	__try
	{
		CloseHandle(Handle);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return true;
	}

	return false;
}

// This function will erase the current images
// PE header from memory preventing a successful image
// if dumped
inline void ErasePEHeaderFromMemory()
{
	DWORD OldProtect = 0;

	// Get base address of module
	char *pBaseAddr = (char*)GetModuleHandle(NULL);

	// Change memory protection
	VirtualProtect(pBaseAddr, 4096, // Assume x86 page size
		PAGE_READWRITE, &OldProtect);

	// Erase the header
	ZeroMemory(pBaseAddr, 4096);
}

// This function uses the toolhelp32 api to enumerate all running processes
// on the computer and does a comparison of the process name against the 
// ProcessName parameter. The function will return 0 on failure.
inline DWORD GetProcessIdFromName(LPCTSTR ProcessName)
{
	PROCESSENTRY32 pe32;
	HANDLE hSnapshot = NULL;
	ZeroMemory(&pe32, sizeof(PROCESSENTRY32));

	// We want a snapshot of processes
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	// Check for a valid handle, in this case we need to check for
	// INVALID_HANDLE_VALUE instead of NULL
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	// Now we can enumerate the running process, also 
	// we can't forget to set the PROCESSENTRY32.dwSize member
	// otherwise the following functions will fail
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hSnapshot, &pe32) == FALSE)
	{
		// Cleanup the mess
		CloseHandle(hSnapshot);
		return 0;
	}

	// Do our first comparison
	if (_tcsicmp(pe32.szExeFile, ProcessName) == FALSE)
	{
		// Cleanup the mess
		CloseHandle(hSnapshot);
		return pe32.th32ProcessID;
	}

	// Most likely it won't match on the first try so 
	// we loop through the rest of the entries until
	// we find the matching entry or not one at all
	while (Process32Next(hSnapshot, &pe32))
	{
		if (_tcsicmp(pe32.szExeFile, ProcessName) == 0)
		{
			// Cleanup the mess
			CloseHandle(hSnapshot);
			return pe32.th32ProcessID;
		}
	}

	// If we made it this far there wasn't a match
	// so we'll return 0
	CloseHandle(hSnapshot);
	return 0;
}

// This function will return the process id of csrss.exe
// and will do so in two different ways. If the OS is XP or 
// greater NtDll has a CsrGetProcessId otherwise I'll use 
// GetProcessIdFromName. Like other functions it will 
// return 0 on failure.
inline DWORD GetCsrssProcessId()
{


	// Visit http://msdn.microsoft.com/en-us/library/ms724833(VS.85).aspx
	// for a full table of versions however what I have set will
	// trigger on anything XP and newer including Server 2003
	if (IsWindowsVistaOrGreater())

	{
		// Gotta love functions pointers
		typedef DWORD(__stdcall *pCsrGetId)();

		// Grab the export from NtDll
		pCsrGetId CsrGetProcessId = (pCsrGetId)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "CsrGetProcessId");

		if (CsrGetProcessId)
			return CsrGetProcessId();
		else
			return 0;
	}
	return GetProcessIdFromName(TEXT("csrss.exe"));
}

// This function will return the process id of Explorer.exe by using the
// GetShellWindow function and the GetWindowThreadProcessId function
inline DWORD GetExplorerPIDbyShellWindow()
{
	DWORD PID = 0;

	// Get the PID 
	GetWindowThreadProcessId(GetShellWindow(), &PID);

	return PID;
}
typedef struct _smPEB_LDR_DATA {
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} smPEB_LDR_DATA, *smPPEB_LDR_DATA;
typedef struct _smPROCESS_BASIC_INFORMATION {
	LONG ExitStatus;
	smPEB_LDR_DATA PebBaseAddress;
	ULONG_PTR AffinityMask;
	LONG BasePriority;
	ULONG_PTR UniqueProcessId;
	ULONG_PTR InheritedFromUniqueProcessId;
} smPROCESS_BASIC_INFORMATION, *smPPROCESS_BASIC_INFORMATION;
// GetParentProcessId will use the NtQueryInformationProcess function
// exported by NtDll to retrieve the parent process id for the current 
// process and if for some reason it doesn't work, it returns 0
DWORD GetParentProcessId()
{
	ULONG_PTR ParentProcessId;
	// Much easier in ASM but C/C++ looks so much better
	typedef NTSTATUS(WINAPI *pNtQueryInformationProcess)
		(HANDLE, UINT, PVOID, ULONG, PULONG);

	// Some locals
	NTSTATUS Status = 0;
	smPROCESS_BASIC_INFORMATION pbi;
	ZeroMemory(&pbi, sizeof(smPROCESS_BASIC_INFORMATION));

	// Get NtQueryInformationProcess
	pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)
		GetProcAddress(
			GetModuleHandle(TEXT("ntdll.dll")), "NtQueryInformationProcess");

	// Sanity check although there's no reason for it to have failed
	if (NtQIP == 0)
		return 0;

	// Now we can call NtQueryInformationProcess, the second
	// param 0 == ProcessBasicInformation
	Status = NtQIP(GetCurrentProcess(), 0, (void*)&pbi,
		sizeof(smPROCESS_BASIC_INFORMATION), 0);

	if (Status != 0x00000000)
		return 0;
	else
		return ParentProcessId;
}

// The function will attempt to open csrss.exe with 
// PROCESS_ALL_ACCESS rights if it fails we're 
// not being debugged however, if its successful we probably are
inline bool CanOpenCsrss()
{
	HANDLE Csrss = 0;

	// If we're being debugged and the process has
	// SeDebugPrivileges privileges then this call
	// will be successful, note that this only works
	// with PROCESS_ALL_ACCESS.
	Csrss = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCsrssProcessId());

	if (Csrss != NULL)
	{
		CloseHandle(Csrss);
		return true;
	}
	else
		return false;
}

// This function returns true if the parent process of
// the current running process is Explorer.exe
bool IsParentExplorerExe()
{
	DWORD PPID = GetParentProcessId();
	if (PPID == GetExplorerPIDbyShellWindow())
		return true;
	else
		return false;
}

// Debug self is a function that uses CreateProcess
// to create an identical copy of the current process
// and debugs it
void DebugSelf()
{
	HANDLE hProcess = NULL;
	DEBUG_EVENT de;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&de, sizeof(DEBUG_EVENT));

	GetStartupInfo(&si);

	// Create the copy of ourself
	CreateProcess(NULL, GetCommandLine(), NULL, NULL, FALSE,
		DEBUG_PROCESS, NULL, NULL, &si, &pi);

	// Continue execution
	ContinueDebugEvent(pi.dwProcessId, pi.dwThreadId, DBG_CONTINUE);

	// Wait for an event
	WaitForDebugEvent(&de, INFINITE);
}

// HideThread will attempt to use
// NtSetInformationThread to hide a thread
// from the debugger, this is essentially the same
// as HideThreadFromDebugger. Passing NULL for
// hThread will cause the function to hide the thread
// the function is running in. Also, the function returns
// false on failure and true on success
inline bool HideThread(HANDLE hThread)
{
	typedef NTSTATUS(NTAPI *pNtSetInformationThread)
		(HANDLE, UINT, PVOID, ULONG);

	NTSTATUS Status;

	// Get NtSetInformationThread
	pNtSetInformationThread NtSIT = (pNtSetInformationThread)
		GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtSetInformationThread");
	// Shouldn't fail
	if (NtSIT == NULL)
		return false;

	// Set the thread info
	if (hThread == NULL)
		Status = NtSIT(GetCurrentThread(),
			0x11, //ThreadHideFromDebugger
			0, 0);
	else
		Status = NtSIT(hThread, 0x11, 0, 0);

	if (Status != 0x00000000)
		return false;
	else
		return true;
}


// This function uses NtQuerySystemInformation
// to try to retrieve a handle to the current
// process's debug object handle. If the function
// is successful it'll return true which means we're
// being debugged or it'll return false if it fails
// or the process isn't being debugged
inline bool DebugObjectCheck()
{
	// Much easier in ASM but C/C++ looks so much better
	typedef NTSTATUS(WINAPI *pNtQueryInformationProcess)
		(HANDLE, UINT, PVOID, ULONG, PULONG);

	HANDLE hDebugObject = NULL;
	NTSTATUS Status;

	// Get NtQueryInformationProcess
	pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)
		GetProcAddress(
			GetModuleHandle(TEXT("ntdll.dll")), "NtQueryInformationProcess");

	Status = NtQIP(GetCurrentProcess(),
		0x1e, // ProcessDebugObjectHandle
		&hDebugObject, 4, NULL);

	if (Status != 0x00000000)
		return false;

	if (hDebugObject)
		return true;
	else
		return false;
}

// CheckProcessDebugFlags will return true if 
// the EPROCESS->NoDebugInherit is == FALSE, 
// the reason we check for false is because 
// the NtQueryProcessInformation function returns the
// inverse of EPROCESS->NoDebugInherit so (!TRUE == FALSE)
inline bool CheckProcessDebugFlags()
{
	// Much easier in ASM but C/C++ looks so much better
	typedef NTSTATUS(WINAPI *pNtQueryInformationProcess)
		(HANDLE, UINT, PVOID, ULONG, PULONG);

	DWORD NoDebugInherit = 0;
	NTSTATUS Status;

	// Get NtQueryInformationProcess
	pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)
		GetProcAddress(
			GetModuleHandle(TEXT("ntdll.dll")), "NtQueryInformationProcess");

	Status = NtQIP(GetCurrentProcess(),
		0x1f, // ProcessDebugFlags
		&NoDebugInherit, 4, NULL);

	if (Status != 0x00000000)
		return false;

	if (NoDebugInherit == FALSE)
		return true;
	else
		return false;
}


// CheckOutputDebugString checks whether or 
// OutputDebugString causes an error to occur
// and if the error does occur then we know 
// there's no debugger, otherwise if there IS
// a debugger no error will occur
inline bool CheckOutputDebugString(LPCTSTR String)
{
	OutputDebugString(String);
	if (GetLastError() == 0)
		return true;
	else
		return false;
}

// The Int2DCheck function will check to see if a debugger
// is attached to the current process. It does this by setting up
// SEH and using the Int 2D instruction which will only cause an
// exception if there is no debugger. Also when used in OllyDBG
// it will skip a byte in the disassembly and will create
// some havoc.
inline bool Int2DCheck()
{
	__try
	{
		__asm
		{
			int 0x2d
			xor eax, eax
			add eax, 2
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

inline void PushPopSS()
{

	__asm
	{
		push ss
		pop ss
		mov eax, 9 // This line executes but is stepped over
		xor edx, edx // This is where the debugger will step to
	}
}


// ObjectListCheck uses NtQueryObject to check the environments
// list of objects and more specifically for the number of
// debug objects. This function can cause an exception (although rarely)
// so either surround it in a try catch or __try __except block
// but that shouldn't happen unless one tinkers with the function
inline bool ObjectListCheck()
{
	typedef NTSTATUS(NTAPI *pNtQueryObject)
		(HANDLE, UINT, PVOID, ULONG, PULONG);

	POBJECT_ALL_INFORMATION pObjectAllInfo = NULL;
	void *pMemory = NULL;
	NTSTATUS Status;
	unsigned long Size = 0;

	// Get NtQueryObject
	pNtQueryObject NtQO = (pNtQueryObject)GetProcAddress(
		GetModuleHandle(TEXT("ntdll.dll")), "NtQueryObject");

	// Get the size of the list
	Status = NtQO(NULL, 3, //ObjectAllTypesInformation
		&Size, 4, &Size);

	// Allocate room for the list
	pMemory = VirtualAlloc(NULL, Size, MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);

	if (pMemory == NULL)
		return false;

	// Now we can actually retrieve the list
	Status = NtQO((HANDLE)-1, 3, pMemory, Size, NULL);

	// Status != STATUS_SUCCESS
	if (Status != 0x00000000)
	{
		VirtualFree(pMemory, 0, MEM_RELEASE);
		return false;
	}

	// We have the information we need
	pObjectAllInfo = (POBJECT_ALL_INFORMATION)pMemory;

	unsigned char *pObjInfoLocation =
		(unsigned char*)pObjectAllInfo->ObjectTypeInformation;

	ULONG NumObjects = pObjectAllInfo->NumberOfObjects;

	for (UINT i = 0; i < NumObjects; i++)
	{

		POBJECT_TYPE_INFORMATION pObjectTypeInfo =
			(POBJECT_TYPE_INFORMATION)pObjInfoLocation;

		// The debug object will always be present
		if (wcscmp(L"DebugObject", pObjectTypeInfo->TypeName.Buffer) == 0)
		{
			// Are there any objects?
			if (pObjectTypeInfo->TotalNumberOfObjects > 0)
			{
				VirtualFree(pMemory, 0, MEM_RELEASE);
				return true;
			}
			else
			{
				VirtualFree(pMemory, 0, MEM_RELEASE);
				return false;
			}
		}

		// Get the address of the current entries
		// string so we can find the end
		pObjInfoLocation =
			(unsigned char*)pObjectTypeInfo->TypeName.Buffer;

		// Add the size
		pObjInfoLocation +=
			pObjectTypeInfo->TypeName.Length;

		// Skip the trailing null and alignment bytes
		ULONG tmp = ((ULONG)pObjInfoLocation) & -4;

		// Not pretty but it works
		pObjInfoLocation = ((unsigned char*)tmp) +
			sizeof(unsigned long);
	}

	VirtualFree(pMemory, 0, MEM_RELEASE);
	return true;
}


// The IsDbgPresentPrefixCheck works in at least two debuggers
// OllyDBG and VS 2008, by utilizing the way the debuggers handle
// prefixes we can determine their presence. Specifically if this code
// is ran under a debugger it will simply be stepped over;
// however, if there is no debugger SEH will fire :D
inline bool IsDbgPresentPrefixCheck()
{
	__try
	{
		__asm __emit 0xF3 // 0xF3 0x64 disassembles as PREFIX REP:
		__asm __emit 0x64
		__asm __emit 0xF1 // One byte INT 1
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}
void CheckForDebugger()
{

	HANDLE hOlly = FindWindow(TEXT("OLLYDBG"), NULL);
	HANDLE WireShark = FindWindow(TEXT("Wireshark"), NULL);
	HANDLE CommomView = FindWindow(TEXT("cv"), NULL);
	HANDLE hWinDbg = FindWindow(TEXT("WinDbgFrameClass"), NULL);
	HANDLE hScylla1 = FindWindow(NULL, TEXT("Scylla x86 v0.9.7c"));
	HANDLE hScylla2 = FindWindow(NULL, TEXT("Scylla x64 v0.9.7c"));
	HANDLE x32_dbg = FindWindow(NULL, TEXT("x32_dbg"));
	HANDLE x64_dbg = FindWindow(NULL, TEXT("x64_dbg"));
	HANDLE IDA = FindWindow(NULL, TEXT("IDA"));


	if (IsDbgPresentPrefixCheck())
	{
		DelMe();
		ExitProcess(0);
	}

	if (hOlly)
	{
		DelMe();
		ExitProcess(0);
	}
	if (hWinDbg)
	{
		DelMe();
		ExitProcess(0);
	}
	if (hScylla1)
	{
		DelMe();
		ExitProcess(0);
	}
	if (hScylla2)
	{
		DelMe();
		ExitProcess(0);
	}
	if (x32_dbg)
	{
		DelMe();
		ExitProcess(0);
	}
	if (x64_dbg)
	{
		DelMe();
		ExitProcess(0);
	}
	if (WireShark)
	{
		DelMe();
		ExitProcess(0);
	}
	if (CommomView)
	{
		DelMe();
		ExitProcess(0);
	}
}
WORD GetVersionWord()
{
	OSVERSIONINFO verInfo = { sizeof(OSVERSIONINFO) };
	GetVersionEx(&verInfo);
	return MAKEWORD(verInfo.dwMinorVersion, verInfo.dwMajorVersion);
}
BOOL IsWin8OrHigher() { return GetVersionWord() >= _WIN32_WINNT_WIN8; }
BOOL IsVistaOrHigher() { return GetVersionWord() >= _WIN32_WINNT_VISTA; }
// Get PEB for WOW64 Process
PVOID GetPEB64()
{
	PVOID pPeb = 0;
#ifndef _WIN64
	// 1. There are two copies of PEB - PEB64 and PEB32 in WOW64 process
	// 2. PEB64 follows after PEB32
	// 3. This is true for versions lower than Windows 8, else __readfsdword returns address of real PEB64
	if (IsWin8OrHigher())
	{
		BOOL isWow64 = FALSE;
		typedef BOOL(WINAPI *pfnIsWow64Process)(HANDLE hProcess, PBOOL isWow64);
		pfnIsWow64Process fnIsWow64Process = (pfnIsWow64Process)
			GetProcAddress(GetModuleHandleA("Kernel32.dll"), "IsWow64Process");
		if (fnIsWow64Process(GetCurrentProcess(), &isWow64))
		{
			if (isWow64)
			{
				pPeb = (PVOID)__readfsdword(0x0C * sizeof(PVOID));
				pPeb = (PVOID)((PBYTE)pPeb + 0x1000);
			}
		}
	}
#endif
	return pPeb;
}
// Current PEB for 64bit and 32bit processes accordingly
PVOID GetPEB()
{
#ifdef _WIN64
	return (PVOID)__readgsqword(0x0C * sizeof(PVOID));
#else
	return (PVOID)__readfsdword(0x0C * sizeof(PVOID));
#endif
}
#define FLG_HEAP_ENABLE_TAIL_CHECK   0x10
#define FLG_HEAP_ENABLE_FREE_CHECK   0x20
#define FLG_HEAP_VALIDATE_PARAMETERS 0x40
#define NT_GLOBAL_FLAG_DEBUGGED (FLG_HEAP_ENABLE_TAIL_CHECK | FLG_HEAP_ENABLE_FREE_CHECK | FLG_HEAP_VALIDATE_PARAMETERS)
void CheckNtGlobalFlag()
{
	PVOID pPeb = GetPEB();
	PVOID pPeb64 = GetPEB64();
	DWORD offsetNtGlobalFlag = 0;
#ifdef _WIN64
	offsetNtGlobalFlag = 0xBC;
#else
	offsetNtGlobalFlag = 0x68;
#endif
	DWORD NtGlobalFlag = *(PDWORD)((PBYTE)pPeb + offsetNtGlobalFlag);
	if (NtGlobalFlag & NT_GLOBAL_FLAG_DEBUGGED)
	{
		exit(-1);
	}
	if (pPeb64)
	{
		DWORD NtGlobalFlagWow64 = *(PDWORD)((PBYTE)pPeb64 + 0xBC);
		if (NtGlobalFlagWow64 & NT_GLOBAL_FLAG_DEBUGGED)
		{
			exit(-1);
		}
	}
}
int GetHeapFlagsOffset(bool x64)
{
	return x64 ?
		IsVistaOrHigher() ? 0x70 : 0x14 : //x64 offsets
		IsVistaOrHigher() ? 0x40 : 0x0C; //x86 offsets
}
int GetHeapForceFlagsOffset(bool x64)
{
	return x64 ?
		IsVistaOrHigher() ? 0x74 : 0x18 : //x64 offsets
		IsVistaOrHigher() ? 0x44 : 0x10; //x86 offsets
}
void CheckHeap()
{
	PVOID pPeb = GetPEB();
	PVOID pPeb64 = GetPEB64();
	PVOID heap = 0;
	DWORD offsetProcessHeap = 0;
	PDWORD heapFlagsPtr = 0, heapForceFlagsPtr = 0;
	BOOL x64 = FALSE;
#ifdef _WIN64
	x64 = TRUE;
	offsetProcessHeap = 0x30;
#else
	offsetProcessHeap = 0x18;
#endif
	heap = (PVOID)*(PDWORD_PTR)((PBYTE)pPeb + offsetProcessHeap);
	heapFlagsPtr = (PDWORD)((PBYTE)heap + GetHeapFlagsOffset(x64));
	heapForceFlagsPtr = (PDWORD)((PBYTE)heap + GetHeapForceFlagsOffset(x64));
	if (*heapFlagsPtr & ~HEAP_GROWABLE || *heapForceFlagsPtr != 0)
	{
		exit(-1);
	}
	if (pPeb64)
	{
		heap = (PVOID)*(PDWORD_PTR)((PBYTE)pPeb64 + 0x30);
		heapFlagsPtr = (PDWORD)((PBYTE)heap + GetHeapFlagsOffset(true));
		heapForceFlagsPtr = (PDWORD)((PBYTE)heap + GetHeapForceFlagsOffset(true));
		if (*heapFlagsPtr & ~HEAP_GROWABLE || *heapForceFlagsPtr != 0)
		{
			exit(-1);
		}
	}
}
bool MemoryBreakpointDebuggerCheck()
{
	unsigned char *pMem = NULL;
	SYSTEM_INFO sysinfo = { 0 };
	DWORD OldProtect = 0;
	void *pAllocation = NULL; // Get the page size for the system 

	GetSystemInfo(&sysinfo); // Allocate memory 

	pAllocation = VirtualAlloc(NULL, sysinfo.dwPageSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	if (pAllocation == NULL)
		return false;

	// Write a ret to the buffer (opcode 0xc3)
	pMem = (unsigned char*)pAllocation;
	*pMem = 0xc3;

	// Make the page a guard page         
	if (VirtualProtect(pAllocation, sysinfo.dwPageSize,
		PAGE_EXECUTE_READWRITE | PAGE_GUARD,
		&OldProtect) == 0)
	{
		return false;
	}

	__try
	{
		__asm
		{
			mov eax, pAllocation
			// This is the address we'll return to if we're under a debugger
			push MemBpBeingDebugged
			jmp eax // Exception or execution, which shall it be :D?
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// The exception occured and no debugger was detected
		VirtualFree(pAllocation, NULL, MEM_RELEASE);
		return false;
	}

	__asm {MemBpBeingDebugged:}
	VirtualFree(pAllocation, NULL, MEM_RELEASE);
	return true;
}

void CheckForDebuggers()
{
	CheckNtGlobalFlag();
	CheckHeap();
	if (DebuggerDriversPresent())
	{
		DelMe();
		ErasePEHeaderFromMemory();
		recurse1();
		SelfDestruct();
		OutputDebugString("%s%s%s%s");
		exit(1);
	}
	if (HasHardwareBreakpoints())
	{
		DelMe();
		ErasePEHeaderFromMemory();
		recurse1();
		SelfDestruct();
		OutputDebugString("%s%s%s%s");
		exit(1);
	}
	if (IsDbgPresentPrefixCheck())
	{
		DelMe();
		ErasePEHeaderFromMemory();
		recurse1();
		SelfDestruct();
		OutputDebugString("%s%s%s%s");
		exit(1);
	}
	if (IsDebuggerPresent())
	{
		DelMe();
		ErasePEHeaderFromMemory();
		recurse1();
		SelfDestruct();
		OutputDebugString("%s%s%s%s");
		exit(1);
	}
}
class CAntiLeak
{
public:
	void ErasePE();
}; CAntiLeak *AntiLeak = new CAntiLeak;

void CAntiLeak::ErasePE()
{
	char *pBaseAddr = (char*)GetModuleHandle(NULL);
	DWORD dwOldProtect = 0;
	VirtualProtect(pBaseAddr, 4096, PAGE_READWRITE, &dwOldProtect);
	ZeroMemory(pBaseAddr, 4096);
	VirtualProtect(pBaseAddr, 4096, dwOldProtect, &dwOldProtect);
}

