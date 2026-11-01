/* SPDX-License-Identifier: AGPL-3.0-or-later */
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#include <windows.h>
#include <tlhelp32.h>
#include <commdlg.h>
#include <bcrypt.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <stdint.h>
#include "launcher_path.h"
#include "launcher_capture.h"

static wchar_t loader_log_path[32768];
static void loader_log(const wchar_t *format,...) {
 DWORD saved=GetLastError();
 FILE *file=loader_log_path[0]?_wfopen(loader_log_path,L"a, ccs=UTF-8"):NULL;
 if(file){SYSTEMTIME now;GetLocalTime(&now);fwprintf(file,L"[%02u:%02u:%02u] ",now.wHour,now.wMinute,now.wSecond);
 va_list args;va_start(args,format);vfwprintf(file,format,args);va_end(args);fputws(L"\n",file);fclose(file);}
 SetLastError(saved);
}
static int fail(const wchar_t *message) {
 DWORD error=GetLastError();loader_log(L"ERROR %ls; last_Win32_error=%lu",message,error);
 wchar_t detail[2048];swprintf(detail,2048,L"%ls\n\nDetails: SSTool-launcher.log beside SSTool.exe.",message);
 MessageBoxW(NULL,detail,L"SSTool launcher",MB_OK|MB_ICONERROR);return 1;
}
static int verify_tool_file(const wchar_t *path) {
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(file==INVALID_HANDLE_VALUE){fail(L"Cannot read MG_cheat.dll beside SSTool.exe.");return 0;}
 LARGE_INTEGER size={0};GetFileSizeEx(file,&size);loader_log(L"DLL path=%ls; bytes=%lld",path,(long long)size.QuadPart);
 BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char buffer[65536],digest[32];DWORD count;int ok=0;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0)goto done;
 for(;;){if(!ReadFile(file,buffer,sizeof(buffer),&count,NULL))goto done;if(!count)break;if(BCryptHashData(hash,buffer,count,0)<0)goto done;}
 if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)goto done;
 char hex[65];for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
 wchar_t wide[65];for(int i=0;i<65;i++)wide[i]=(wchar_t)hex[i];loader_log(L"DLL SHA256=%ls",wide);
 ok=!strcmp(hex,EXPECTED_DLL_SHA256);
 done:
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(file);
 if(!ok)fail(L"MG_cheat.dll is not the matching SSTool build, or could not be verified. Download the DLL supplied with this launcher; do not use the original key-locked DLL.");
 return ok;
}
static int startup_poll;
static DWORD find_game(void) {
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(snapshot==INVALID_HANDLE_VALUE)return 0;
 PROCESSENTRY32W entry={0};entry.dwSize=sizeof(entry);DWORD pid=0;
 if(Process32FirstW(snapshot,&entry))do {
  if(!_wcsicmp(entry.szExeFile,L"StellaSora.exe")) {
   if(pid){pid=MAXDWORD;break;} /* Ambiguous: never choose between two games. */
   pid=entry.th32ProcessID;
  }
 }while(Process32NextW(snapshot,&entry));
 CloseHandle(snapshot);return pid;
}
static uintptr_t remote_module(DWORD pid,const wchar_t *name) {
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
 if(snapshot==INVALID_HANDLE_VALUE){if(!startup_poll)loader_log(L"Module snapshot failed pid=%lu error=%lu",pid,GetLastError());return 0;}
 MODULEENTRY32W entry={0};entry.dwSize=sizeof(entry);uintptr_t base=0;
 if(Module32FirstW(snapshot,&entry))do {
  if(!_wcsicmp(entry.szModule,name)){base=(uintptr_t)entry.modBaseAddr;break;}
 }while(Module32NextW(snapshot,&entry));
 CloseHandle(snapshot);return base;
}
static uintptr_t remote_export(DWORD pid,const char *name) {
 FARPROC address=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),name);HMODULE owner=NULL;wchar_t path[32768];
 if(!address||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)(uintptr_t)address,&owner))return 0;
 DWORD n=GetModuleFileNameW(owner,path,32768);if(!n||n>=32768)return 0;
 wchar_t *tail=wcsrchr(path,L'\\');if(!tail)return 0;
 uintptr_t base=remote_module(pid,tail+1);if(!base)return 0;
 return base+(uintptr_t)address-(uintptr_t)owner;
}
static int load_tool(DWORD pid) {
 wchar_t dll[32768];
 DWORD length=GetModuleFileNameW(NULL,dll,32768);
 if(!length||length>=32768)return fail(L"Cannot locate launcher folder.");
 wchar_t *tail=wcsrchr(dll,L'\\');if(!tail)return fail(L"Invalid launcher path.");
 if((size_t)(tail-dll)+14>=32768)return fail(L"Path is too long.");
 wcscpy(tail+1,L"MG_cheat.dll");
 if(GetFileAttributesW(dll)==INVALID_FILE_ATTRIBUTES)return fail(L"Put MG_cheat.dll beside SSTool.exe.");

 loader_log(L"Loading into pid=%lu",pid);
 if(!verify_tool_file(dll))return 1;
 if(remote_module(pid,L"MG_cheat.dll"))return fail(L"A tool DLL is already loaded. Restart the game before loading another copy.");
 HANDLE process=OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid);
 if(!process)return fail(L"Cannot open the game. Run this launcher at the same privilege level as the game.");
 BOOL wow64=FALSE;
 if(!IsWow64Process(process,&wow64)||wow64){CloseHandle(process);return fail(L"The game must be a Windows x64 process.");}
 uintptr_t load=remote_export(pid,"LoadLibraryW"),last_error=remote_export(pid,"GetLastError");
 if(!load||!last_error){CloseHandle(process);return fail(L"Cannot resolve Windows loader functions.");}
 loader_log(L"Remote LoadLibraryW=%p GetLastError=%p",(void*)load,(void*)last_error);
 SIZE_T size=(wcslen(dll)+1)*sizeof(wchar_t),written=0;
 void *remote=VirtualAllocEx(process,NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!remote){CloseHandle(process);return fail(L"Cannot allocate DLL path.");}
 if(!WriteProcessMemory(process,remote,dll,size,&written)||written!=size) {
  VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);return fail(L"Cannot write DLL path.");
 }
 struct LoaderResult result={load,last_error,(uintptr_t)remote,0,0,0};
 void *remote_result=VirtualAllocEx(process,NULL,sizeof(result),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 void *remote_code=VirtualAllocEx(process,NULL,sizeof(loader_code),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 DWORD old_protect=0;
 if(!remote_result||!remote_code||!WriteProcessMemory(process,remote_result,&result,sizeof(result),&written)||written!=sizeof(result)||
    !WriteProcessMemory(process,remote_code,loader_code,sizeof(loader_code),&written)||written!=sizeof(loader_code)||
    !VirtualProtectEx(process,remote_code,sizeof(loader_code),PAGE_EXECUTE_READ,&old_protect)||!FlushInstructionCache(process,remote_code,sizeof(loader_code))) {
  if(remote_result)VirtualFreeEx(process,remote_result,0,MEM_RELEASE);if(remote_code)VirtualFreeEx(process,remote_code,0,MEM_RELEASE);
  VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);return fail(L"Cannot prepare Windows loader result capture.");
 }
 HANDLE thread=CreateRemoteThread(process,NULL,0,(LPTHREAD_START_ROUTINE)remote_code,remote_result,0,NULL);
 if(!thread){VirtualFreeEx(process,remote_result,0,MEM_RELEASE);VirtualFreeEx(process,remote_code,0,MEM_RELEASE);VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);return fail(L"Windows could not start the DLL loader.");}
 DWORD wait=WaitForSingleObject(thread,30000),exit_code=0;
 BOOL got_code=GetExitCodeThread(thread,&exit_code);
 loader_log(L"Remote thread wait=0x%08lx GetExitCode=%d exit=0x%08lx",wait,(int)got_code,exit_code);
 CloseHandle(thread);
 if(wait!=WAIT_OBJECT_0) {
  /* Keep the path allocated while the remote loader might still use it. */
  CloseHandle(process);return fail(L"Loader did not finish in 30 seconds. Do not load again; restart the game and check SSTool2.log.");
 }
 SIZE_T received=0;
 BOOL read_ok=ReadProcessMemory(process,remote_result,&result,sizeof(result),&received);
 loader_log(L"Loader capture read=%d bytes=%llu stage=%lu module=%p immediate_Win32_error=%lu",(int)read_ok,(unsigned long long)received,result.stage,(void*)(uintptr_t)result.module,result.error);
 VirtualFreeEx(process,remote_result,0,MEM_RELEASE);VirtualFreeEx(process,remote_code,0,MEM_RELEASE);
 VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);
 if(read_ok&&received==sizeof(result)&&result.stage==2&&!result.module) {
  wchar_t reason[512];swprintf(reason,512,L"LoadLibraryW failed with Windows error %lu. See SSTool-launcher.log for the exact DLL path and hash.",result.error);SetLastError(result.error);return fail(reason);
 }
 uintptr_t loaded=(read_ok&&received==sizeof(result)&&result.stage==2)?(uintptr_t)result.module:0;
 for(unsigned attempt=0;attempt<10&&!loaded;attempt++){loaded=remote_module(pid,L"MG_cheat.dll");if(!loaded)Sleep(100);}
 if(!loaded) {
  wchar_t reason[512];swprintf(reason,512,L"Windows loader did not leave the DLL loaded (thread exit 0x%08lx). The launcher log now records the DLL hash and loader result. Restart the game before retrying.",exit_code);
  return fail(reason);
 }
 loader_log(L"DLL module confirmed at %p",(void*)loaded);

 return 0;
}

static HWND launcher_window,path_edit,start_button,browse_button,status_text;
static wchar_t ini_path[32768],selected_path[32768];


static DWORD WINAPI start_worker(void *unused) {
 (void)unused;DWORD pid=find_game();int result=1;
 if(pid){loader_log(L"Fresh-start request refused: existing pid=%lu",pid);fail(L"Close all StellaSora.exe instances, then use Start game + tool. This build starts a fresh game instead of attaching to an existing session.");goto done;}
 HANDLE launched=NULL;
 if(!pid) {
  if(!selected_path[0]||GetFileAttributesW(selected_path)==INVALID_FILE_ATTRIBUTES){fail(L"Choose the game's StellaSora.exe first.");goto done;}
  const wchar_t *name=wcsrchr(selected_path,L'\\');
  if(!name||_wcsicmp(name+1,L"StellaSora.exe")){fail(L"Select StellaSora.exe from the game installation folder.");goto done;}
  wchar_t directory[32768];wcscpy(directory,selected_path);directory[name-selected_path]=0;
  STARTUPINFOW startup={0};startup.cb=sizeof(startup);PROCESS_INFORMATION info={0};
  if(!CreateProcessW(selected_path,NULL,NULL,NULL,FALSE,0,NULL,directory,&startup,&info)) {fail(L"Could not start the game. Check its path and access permissions.");goto done;}
  CloseHandle(info.hThread);launched=info.hProcess;pid=info.dwProcessId;
  loader_log(L"Created fresh game pid=%lu; waiting for Windows loader exports, not GameAssembly",pid);
 }
 startup_poll=1;
 ULONGLONG deadline=GetTickCount64()+120000;
 for(;;) {
  if(remote_export(pid,"LoadLibraryW")&&remote_export(pid,"GetLastError")) {
   startup_poll=0;loader_log(L"Windows loader exports ready in fresh pid=%lu; attempting DLL load immediately",pid);
   result=load_tool(pid);break;
  }
  if(launched&&WaitForSingleObject(launched,0)==WAIT_OBJECT_0){fail(L"The newly started game exited before its Windows loader became ready.");break;}
  if(GetTickCount64()>=deadline){fail(L"Timed out waiting for Windows loader exports in the fresh game.");break;}
  Sleep(10);
 }
 startup_poll=0;
 if(launched)CloseHandle(launched);
 done:PostMessageW(launcher_window,WM_APP+1,(WPARAM)result,0);return 0;
}
static LRESULT CALLBACK launcher_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
 switch(msg) {
 case WM_COMMAND:
  if(LOWORD(wp)==101) {
   wchar_t path[32768]=L"";OPENFILENAMEW ofn={0};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=hwnd;
   ofn.lpstrFilter=L"StellaSora.exe\0StellaSora.exe\0Executable files\0*.exe\0\0";ofn.lpstrFile=path;ofn.nMaxFile=32768;
   ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
   if(GetOpenFileNameW(&ofn))SetWindowTextW(path_edit,path);
  } else if(LOWORD(wp)==102) {
   GetWindowTextW(path_edit,selected_path,32768);normalize_path(selected_path);SetWindowTextW(path_edit,selected_path);
   /* Create Unicode INI for new settings, preserving non-ASCII game paths. */
   if(selected_path[0]&&GetFileAttributesW(ini_path)==INVALID_FILE_ATTRIBUTES) {
    HANDLE file=CreateFileW(ini_path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file!=INVALID_HANDLE_VALUE){WORD bom=0xfeff;DWORD written;WriteFile(file,&bom,sizeof(bom),&written,NULL);CloseHandle(file);}
   }
   if(selected_path[0]&&!WritePrivateProfileStringW(L"Config",L"GamePath",selected_path,ini_path)) {fail(L"Could not save config.ini beside SSTool.exe. Move the tool to a writable folder.");return 0;}
   EnableWindow(start_button,FALSE);EnableWindow(browse_button,FALSE);EnableWindow(path_edit,FALSE);
   SetWindowTextW(status_text,L"Starting / waiting for game, then loading tool...");
   HANDLE thread=CreateThread(NULL,0,start_worker,NULL,0,NULL);
   if(thread)CloseHandle(thread);else PostMessageW(hwnd,WM_APP+1,1,0);
  }
  return 0;
 case WM_APP+1:
  EnableWindow(start_button,TRUE);EnableWindow(browse_button,TRUE);EnableWindow(path_edit,TRUE);
  SetWindowTextW(status_text,wp?L"Could not finish. See error message; check game and retry.":L"Tool loaded. Wait for its menu; F1 shows/hides it.");return 0;
 case WM_CLOSE:
  if(!IsWindowEnabled(start_button)){MessageBoxW(hwnd,L"Please wait for startup to finish.",L"SSTool",MB_OK);return 0;}
  DestroyWindow(hwnd);return 0;
 case WM_DESTROY:PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(hwnd,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR command,int show) {
 (void)previous;(void)command;
 DWORD n=GetModuleFileNameW(NULL,ini_path,32768);
 if(!n||n>=32768)return fail(L"Cannot locate launcher folder.");
 wchar_t *tail=wcsrchr(ini_path,L'\\');if(!tail||(size_t)(tail-ini_path)+24>=32768)return 1;
 wcscpy(tail+1,L"SSTool-launcher.log");wcscpy(loader_log_path,ini_path);
 loader_log(L"=== SSTool fresh-start launcher 5 ===");
 wcscpy(tail+1,L"config.ini");
 GetPrivateProfileStringW(L"Config",L"GamePath",L"",selected_path,32768,ini_path);normalize_path(selected_path);
 WNDCLASSW cls={0};cls.lpfnWndProc=launcher_proc;cls.hInstance=instance;cls.lpszClassName=L"SSToolLauncher";
 cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
 if(!RegisterClassW(&cls))return 1;
 launcher_window=CreateWindowW(cls.lpszClassName,L"SSTool - Fresh start",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,680,190,NULL,NULL,instance,NULL);
 if(!launcher_window)return 1;
 CreateWindowW(L"STATIC",L"Game path (StellaSora.exe):",WS_CHILD|WS_VISIBLE,15,12,620,20,launcher_window,NULL,instance,NULL);
 path_edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",selected_path,WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,15,35,530,25,launcher_window,NULL,instance,NULL);
 browse_button=CreateWindowW(L"BUTTON",L"Browse...",WS_CHILD|WS_VISIBLE,555,35,95,25,launcher_window,(HMENU)101,instance,NULL);
 start_button=CreateWindowW(L"BUTTON",L"Start game + tool",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,15,75,175,28,launcher_window,(HMENU)102,instance,NULL);
 status_text=CreateWindowW(L"STATIC",L"Close the game first. Choose its path, then Start game + tool.",WS_CHILD|WS_VISIBLE,15,115,640,25,launcher_window,NULL,instance,NULL);
 SendMessageW(path_edit,EM_SETLIMITTEXT,32766,0);
 ShowWindow(launcher_window,show);MSG msg;
 while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
 return 0;
}
