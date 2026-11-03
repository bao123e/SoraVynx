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

/* Release build: do not create launcher logs. */
static void loader_log(const wchar_t *format,...){(void)format;}
static int fail(const wchar_t *message) {
 MessageBoxW(NULL,message,L"SoraVynx v1.1",MB_OK|MB_ICONERROR);return 1;
}
static int verify_tool_file(const wchar_t *path) {
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(file==INVALID_HANDLE_VALUE){fail(L"Cannot read SoraVynx.Runtime.dll beside SoraVynx.exe.");return 0;}
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
 if(!ok)fail(L"SoraVynx.Runtime.dll is not the matching SoraVynx build, or could not be verified. Download the DLL supplied with this launcher; do not use the original key-locked DLL.");
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
 if((size_t)(tail-dll)+wcslen(L"SoraVynx.Runtime.dll")+2>=32768)return fail(L"Path is too long.");
 wcscpy(tail+1,L"SoraVynx.Runtime.dll");
 if(GetFileAttributesW(dll)==INVALID_FILE_ATTRIBUTES)return fail(L"Put SoraVynx.Runtime.dll beside SoraVynx.exe.");

 loader_log(L"Loading into pid=%lu",pid);
 if(!verify_tool_file(dll))return 1;
 if(remote_module(pid,L"SoraVynx.Runtime.dll"))return fail(L"A tool DLL is already loaded. Restart the game before loading another copy.");
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
  CloseHandle(process);return fail(L"Loader did not finish in 30 seconds. Do not load again; restart the game before retrying.");
 }
 SIZE_T received=0;
 BOOL read_ok=ReadProcessMemory(process,remote_result,&result,sizeof(result),&received);
 loader_log(L"Loader capture read=%d bytes=%llu stage=%lu module=%p immediate_Win32_error=%lu",(int)read_ok,(unsigned long long)received,result.stage,(void*)(uintptr_t)result.module,result.error);
 VirtualFreeEx(process,remote_result,0,MEM_RELEASE);VirtualFreeEx(process,remote_code,0,MEM_RELEASE);
 VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);
 if(read_ok&&received==sizeof(result)&&result.stage==2&&!result.module) {
  wchar_t reason[512];swprintf(reason,512,L"LoadLibraryW failed with Windows error %lu. Check the DLL placement and restart the game before retrying.",result.error);SetLastError(result.error);return fail(reason);
 }
 uintptr_t loaded=(read_ok&&received==sizeof(result)&&result.stage==2)?(uintptr_t)result.module:0;
 for(unsigned attempt=0;attempt<10&&!loaded;attempt++){loaded=remote_module(pid,L"SoraVynx.Runtime.dll");if(!loaded)Sleep(100);}
 if(!loaded) {
  wchar_t reason[512];swprintf(reason,512,L"Windows loader did not leave the DLL loaded (thread exit 0x%08lx). Restart the game before retrying.",exit_code);
  return fail(reason);
 }
 loader_log(L"DLL module confirmed at %p",(void*)loaded);

 return 0;
}

static HWND launcher_window;
static wchar_t ini_path[32768],selected_path[32768];


struct PanelSearch {DWORD pid;HWND panel;};
static BOOL CALLBACK find_panel(HWND hwnd,LPARAM argument) {
 struct PanelSearch *search=(struct PanelSearch*)argument;DWORD pid=0;wchar_t name[64];
 GetWindowThreadProcessId(hwnd,&pid);
 if(pid==search->pid&&GetClassNameW(hwnd,name,64)&&!wcscmp(name,L"SoraVynxRuntime")) {search->panel=hwnd;return FALSE;}
 return TRUE;
}
static int restore_panel(DWORD pid) {
 if(!pid||pid==MAXDWORD)return 0;
 struct PanelSearch search={pid,NULL};EnumWindows(find_panel,(LPARAM)&search);
 if(!search.panel)return 0;
 ShowWindowAsync(search.panel,SW_RESTORE);
 if(!SetWindowPos(search.panel,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_ASYNCWINDOWPOS)) {
  loader_log(L"Existing panel restore failed pid=%lu error=%lu",pid,GetLastError());return 0;
 }
 SetForegroundWindow(search.panel);loader_log(L"Restored existing tool panel in pid=%lu; no DLL loaded again",pid);return 1;
}
static DWORD WINAPI start_worker(void *unused) {
 (void)unused;DWORD pid=find_game();int result=1;
 if(pid){if(restore_panel(pid)){result=0;goto done;}loader_log(L"Fresh-start request refused: existing pid=%lu",pid);fail(L"The game is running but its tool panel is not available yet. If startup has not finished, wait. Otherwise close the game and start it through SoraVynx.exe.");goto done;}
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
 case WM_APP+2: {
  if(restore_panel(find_game())){DestroyWindow(hwnd);return 0;}
  wchar_t path[32768]=L"";
  if(GetFileAttributesW(selected_path)!=INVALID_FILE_ATTRIBUTES)wcscpy(path,selected_path);
  OPENFILENAMEW ofn={0};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=hwnd;
  ofn.lpstrTitle=L"SoraVynx v1.1 - Select StellaSora.exe";
  ofn.lpstrFilter=L"StellaSora.exe\0StellaSora.exe\0Executable files\0*.exe\0\0";
  ofn.lpstrFile=path;ofn.nMaxFile=32768;
  ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
  if(!GetOpenFileNameW(&ofn)) {
   DWORD error=CommDlgExtendedError();
   if(error){loader_log(L"File picker failed: 0x%08lx",error);fail(L"Cannot open the game file picker.");}
   DestroyWindow(hwnd);return 0;
  }
  normalize_path(path);
  const wchar_t *name=wcsrchr(path,L'\\');
  if(!name||_wcsicmp(name+1,L"StellaSora.exe")){fail(L"Select the game's StellaSora.exe.");PostMessageW(hwnd,WM_APP+2,0,0);return 0;}
  wcscpy(selected_path,path);
  if(GetFileAttributesW(ini_path)==INVALID_FILE_ATTRIBUTES) {
   HANDLE file=CreateFileW(ini_path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
   if(file!=INVALID_HANDLE_VALUE){WORD bom=0xfeff;DWORD written;WriteFile(file,&bom,sizeof(bom),&written,NULL);CloseHandle(file);}
  }
  if(!WritePrivateProfileStringW(L"Config",L"GamePath",selected_path,ini_path)) {
   fail(L"Cannot save config.ini. Move the tool to a writable folder.");DestroyWindow(hwnd);return 0;
  }
  HANDLE thread=CreateThread(NULL,0,start_worker,NULL,0,NULL);
  if(thread)CloseHandle(thread);
  else {fail(L"Cannot start the game launch worker.");DestroyWindow(hwnd);}
  return 0;
 }
 case WM_APP+1:DestroyWindow(hwnd);return 0;
 case WM_CLOSE:return 0; /* Hidden message owner stays alive until startup completes. */
 case WM_DESTROY:PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(hwnd,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR command,int show) {
 (void)previous;(void)command;(void)show;
 DWORD n=GetModuleFileNameW(NULL,ini_path,32768);
 if(!n||n>=32768)return fail(L"Cannot locate launcher folder.");
 wchar_t *tail=wcsrchr(ini_path,L'\\');if(!tail||(size_t)(tail-ini_path)+24>=32768)return 1;
 wcscpy(tail+1,L"config.ini");
 GetPrivateProfileStringW(L"Config",L"GamePath",L"",selected_path,32768,ini_path);normalize_path(selected_path);
 WNDCLASSW cls={0};cls.lpfnWndProc=launcher_proc;cls.hInstance=instance;cls.lpszClassName=L"SoraVynxLauncher";
 cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
 if(!RegisterClassW(&cls))return 1;
 launcher_window=CreateWindowW(cls.lpszClassName,L"SoraVynx v1.1",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,680,190,NULL,NULL,instance,NULL);
 if(!launcher_window)return 1;
 /* Hidden owner for the modal picker and worker completion messages.
  * Open launches immediately; Cancel exits without starting the game. */
 PostMessageW(launcher_window,WM_APP+2,0,0);MSG msg;
 while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
 return 0;
}
