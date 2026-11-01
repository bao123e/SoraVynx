/* SPDX-License-Identifier: AGPL-3.0-or-later */
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <wchar.h>
#include <stdint.h>

static int fail(const wchar_t *message) {
 fwprintf(stderr,L"%ls (Windows error %lu)\n",message,GetLastError());
 MessageBoxW(NULL,message,L"SSTool launcher",MB_OK|MB_ICONERROR);return 1;
}
static DWORD find_game(void) {
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(snapshot==INVALID_HANDLE_VALUE)return 0;
 PROCESSENTRY32W entry={0};entry.dwSize=sizeof(entry);DWORD pid=0;
 if(Process32FirstW(snapshot,&entry))do {
  if(!_wcsicmp(entry.szExeFile,L"StellaSora.exe")) {
   if(pid){pid=0;break;} /* Ambiguous: never choose between two games. */
   pid=entry.th32ProcessID;
  }
 }while(Process32NextW(snapshot,&entry));
 CloseHandle(snapshot);return pid;
}
static uintptr_t remote_module(DWORD pid,const wchar_t *name) {
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
 if(snapshot==INVALID_HANDLE_VALUE)return 0;
 MODULEENTRY32W entry={0};entry.dwSize=sizeof(entry);uintptr_t base=0;
 if(Module32FirstW(snapshot,&entry))do {
  if(!_wcsicmp(entry.szModule,name)){base=(uintptr_t)entry.modBaseAddr;break;}
 }while(Module32NextW(snapshot,&entry));
 CloseHandle(snapshot);return base;
}
int main(void) {
 wchar_t dll[32768],owner_path[32768];
 DWORD length=GetModuleFileNameW(NULL,dll,32768);
 if(!length||length>=32768)return fail(L"Cannot locate launcher folder.");
 wchar_t *tail=wcsrchr(dll,L'\\');if(!tail)return fail(L"Invalid launcher path.");
 if((size_t)(tail-dll)+14>=32768)return fail(L"Path is too long.");
 wcscpy(tail+1,L"MG_cheat.dll");
 if(GetFileAttributesW(dll)==INVALID_FILE_ATTRIBUTES)return fail(L"Put MG_cheat.dll beside SSTool.exe.");
 DWORD pid=find_game();
 if(!pid)return fail(L"Start exactly one StellaSora.exe, enter its main menu, then run SSTool.exe again.");
 if(remote_module(pid,L"MG_cheat.dll"))return fail(L"A tool DLL is already loaded. Restart the game before loading another copy.");
 HANDLE process=OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid);
 if(!process)return fail(L"Cannot open the game. Run this launcher at the same privilege level as the game.");
 BOOL wow64=FALSE;
 if(!IsWow64Process(process,&wow64)||wow64){CloseHandle(process);return fail(L"The game must be a Windows x64 process.");}
 FARPROC load=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
 HMODULE owner=NULL;
 if(!load||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)(uintptr_t)load,&owner)) {
  CloseHandle(process);return fail(L"Cannot resolve Windows DLL loader.");
 }
 length=GetModuleFileNameW(owner,owner_path,32768);tail=wcsrchr(owner_path,L'\\');
 if(!length||length>=32768||!tail){CloseHandle(process);return fail(L"Cannot identify loader module.");}
 uintptr_t base=remote_module(pid,tail+1);
 if(!base){CloseHandle(process);return fail(L"Cannot locate Windows loader module in game.");}
 /* Rebase the exported address by its actual owning module, including forwards. */
 LPTHREAD_START_ROUTINE remote_load=(LPTHREAD_START_ROUTINE)(base+(uintptr_t)load-(uintptr_t)owner);
 SIZE_T size=(wcslen(dll)+1)*sizeof(wchar_t),written=0;
 void *remote=VirtualAllocEx(process,NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!remote){CloseHandle(process);return fail(L"Cannot allocate DLL path.");}
 if(!WriteProcessMemory(process,remote,dll,size,&written)||written!=size) {
  VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);return fail(L"Cannot write DLL path.");
 }
 HANDLE thread=CreateRemoteThread(process,NULL,0,remote_load,remote,0,NULL);
 if(!thread){VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);return fail(L"Windows could not start the DLL loader.");}
 DWORD wait=WaitForSingleObject(thread,30000);
 CloseHandle(thread);
 if(wait!=WAIT_OBJECT_0) {
  /* Keep the path allocated while the remote loader might still use it. */
  CloseHandle(process);return fail(L"Loader did not finish in 30 seconds. Do not load again; restart the game and check SSTool2.log.");
 }
 VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);
 if(!remote_module(pid,L"MG_cheat.dll"))return fail(L"DLL was not loaded. Check that MG_cheat.dll is a compatible x64 build.");
 MessageBoxW(NULL,L"DLL loaded. Wait for the tool window. F1 shows/hides it. If startup stops, read SSTool2.log beside the DLL.",L"SSTool launcher",MB_OK|MB_ICONINFORMATION);
 return 0;
}
