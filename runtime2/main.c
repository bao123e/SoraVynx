/* SPDX-License-Identifier: AGPL-3.0-or-later */
#define WIN32_LEAN_AND_MEAN
#include "backend.h"
#include <bcrypt.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>

static HMODULE self_module;
static HWND window,checks[FEATURE_COUNT],status_label;
static FILE *log_file;
static CRITICAL_SECTION log_lock;
static volatile LONG init_state;
static const wchar_t *labels[FEATURE_COUNT]={L"Instant Break",L"God Mode",L"Multi Hit",L"No Cooldown",L"Unlimited Energy",L"Mob Vacuum"};
void log_line(const char *format,...) {
 EnterCriticalSection(&log_lock);
 if(log_file) {
  SYSTEMTIME now;GetLocalTime(&now);
  fprintf(log_file,"[%02u:%02u:%02u] ",now.wHour,now.wMinute,now.wSecond);
  va_list args;va_start(args,format);vfprintf(log_file,format,args);va_end(args);
  fputc('\n',log_file);fflush(log_file);
 }
 LeaveCriticalSection(&log_lock);
}
static bool open_log(void) {
 wchar_t path[1024];DWORD n=GetModuleFileNameW(self_module,path,1024);
 if(!n||n>=1024)return false;
 wchar_t *slash=wcsrchr(path,L'\\');if(!slash)return false;
 if((size_t)(slash-path)+wcslen(L"\\SSTool2.log")>=1024)return false;
 wcscpy(slash,L"\\SSTool2.log");log_file=_wfopen(path,L"ab");return log_file!=NULL;
}
static bool verify_game(HMODULE game) {
 static const unsigned char expected[32]={0x6a,0xdc,0x5d,0xe8,0xd2,0xed,0x85,0xee,0x75,0x2b,0xe8,0x0b,0x38,0xaf,0xf0,0xe5,0xe8,0x5d,0x21,0xe6,0xef,0xb6,0x13,0x3c,0x26,0x2c,0x9f,0x26,0x32,0xe0,0x6d,0xd8};
 wchar_t path[1024];DWORD n=GetModuleFileNameW(game,path,1024);if(!n||n>=1024)return false;
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(file==INVALID_HANDLE_VALUE)return false;
 BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char digest[32];
 unsigned char *buffer=malloc(65536);bool ok=false;DWORD read;
 if(!buffer)goto done;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0)goto done;
 for(;;) {
  if(!ReadFile(file,buffer,65536,&read,NULL))goto done;
  if(!read)break;
  if(BCryptHashData(hash,buffer,read,0)<0)goto done;
 }
 if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)goto done;
 char hex[65];for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
 log_line("GameAssembly SHA256: %s",hex);
 ok=!memcmp(digest,expected,32);
 if(!ok)log_line("Unsupported game binary: no hooks will be installed. Do not edit the expected hash to force startup.");
 done:
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
 free(buffer);CloseHandle(file);return ok;
}
static DWORD WINAPI initialize(void *unused) {
 (void)unused;void *thread=NULL;HMODULE game=NULL;
 HMODULE other=GetModuleHandleW(L"MG_cheat.dll");
 if(other&&other!=self_module){log_line("Another MG_cheat.dll is loaded. Restart with only this DLL.");goto failed;}
 for(int i=0;i<120;i++){game=GetModuleHandleW(L"GameAssembly.dll");if(game)break;Sleep(1000);}
 if(!game||!verify_game(game)||!bind_api(game))goto failed;
 log_line("GameAssembly loaded base: %p",(void*)game);
 void *domain=NULL;
 for(int i=0;i<120;i++){domain=api.domain_get();if(domain)break;Sleep(1000);}
 if(!domain){log_line("IL2CPP domain timeout.");goto failed;}
 thread=api.thread_attach(domain);if(!thread)goto failed;
 for(int attempt=0;attempt<180;attempt++) {
  size_t count=0;const void **assemblies=api.domain_get_assemblies(domain,&count);
  if(assemblies&&count<4096)for(size_t i=0;i<count;i++) {
   const void *image=api.assembly_get_image(assemblies[i]);const char *name=image?api.image_get_name(image):NULL;
   if(name&&(!strcmp(name,"Hotfix.dll")||!strcmp(name,"Hotfix"))){hotfix_image=image;break;}
  }
  if(hotfix_image)break;Sleep(1000);
 }
 if(!hotfix_image){log_line("Hotfix image not visible after 180 seconds; retry after the game reaches the main menu.");goto failed;}
 log_line("Hotfix image found. Binding verified signatures.");
 if(!initialize_backend())goto failed;
 install_interpreter_probe(game);
 api.thread_detach(thread);InterlockedExchange(&init_state,1);PostMessageW(window,WM_APP+1,0,0);return 0;
 failed:
 if(thread)api.thread_detach(thread);
 InterlockedExchange(&init_state,-1);PostMessageW(window,WM_APP+1,0,0);return 1;
}
static void disable_all(void) {
 for(int i=0;i<FEATURE_COUNT;i++){InterlockedExchange(&feature_flags[i],0);SendMessageW(checks[i],BM_SETCHECK,BST_UNCHECKED,0);}
 log_line("All features disabled; installed hooks are pass-through.");
}
static void refresh_status(void) {
 LONG state=read_atomic(&init_state);
 if(state<0){SetWindowTextW(status_label,L"Startup stopped. Read SSTool2.log beside the DLL.\r\nNo features are enabled.");return;}
 if(!state){SetWindowTextW(status_label,L"Waiting for the game and Hotfix runtime...\r\nEnter the main menu. Features remain off until ready.");return;}
 int available=0;
 for(int i=0;i<FEATURE_COUNT;i++){BOOL enabled=read_atomic(&capabilities[i])!=0;EnableWindow(checks[i],enabled);if(enabled)available++;}
 wchar_t text[256];swprintf(text,256,L"Hooks: %d/6; entry calls: %ld; matching targets: %d/12.\r\nF1 shows/hides. Counts distinguish entry activity from target matches.",available,(long)total_entry_calls(),observed_target_count());
 SetWindowTextW(status_label,text);report_observed_calls();
}
static LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
 switch(message) {
 case WM_COMMAND: {
  int id=LOWORD(wp);
  if(id>=100&&id<100+FEATURE_COUNT&&HIWORD(wp)==BN_CLICKED) {
   int f=id-100;
   if(!read_atomic(&capabilities[f])){SendMessageW(checks[f],BM_SETCHECK,BST_UNCHECKED,0);break;}
   LONG value=SendMessageW(checks[f],BM_GETCHECK,0,0)==BST_CHECKED;
   InterlockedExchange(&feature_flags[f],value);log_line("Feature %d -> %s",f,value?"ON":"OFF");
  } else if(id==200)disable_all();
  else if((id==201||id==202)&&HIWORD(wp)==EN_CHANGE) {
   BOOL valid=FALSE;UINT value=GetDlgItemInt(hwnd,id,&valid,FALSE);
   if(valid&&id==201&&value>=2&&value<=50)InterlockedExchange(&multi_count,(LONG)value);
   if(valid&&id==202&&value>=1&&value<=10)InterlockedExchange(&vacuum_distance,(LONG)value);
  }
  break;
 }
 case WM_TIMER: {
  static bool was_down;bool down=(GetAsyncKeyState(VK_F1)&0x8000)!=0;DWORD process=0;
  GetWindowThreadProcessId(GetForegroundWindow(),&process);
  if(down&&!was_down&&process==GetCurrentProcessId()) {
   if(IsWindowVisible(hwnd))ShowWindow(hwnd,SW_HIDE);
   else {ShowWindow(hwnd,SW_SHOW);SetForegroundWindow(hwnd);}
  }
  was_down=down;refresh_status();break;
 }
 case WM_APP+1:refresh_status();break;
 case WM_CLOSE:disable_all();ShowWindow(hwnd,SW_HIDE);return 0;
 default:return DefWindowProcW(hwnd,message,wp,lp);
 }
 return 0;
}
static HWND control(const wchar_t *kind,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id) {
 HWND c=CreateWindowW(kind,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,(HMENU)(intptr_t)id,self_module,NULL);
 if(c)SendMessageW(c,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
 return c;
}
static DWORD WINAPI run(void *unused) {
 (void)unused;InitializeCriticalSection(&log_lock);bool logging=open_log();
 WNDCLASSW wc={0};wc.lpfnWndProc=procedure;wc.hInstance=self_module;wc.lpszClassName=L"SSToolRuntime2";
 wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
 if(!RegisterClassW(&wc))return 1;
 window=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,wc.lpszClassName,L"Stella Sora Runtime 2 - diagnostics 3",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,570,405,NULL,NULL,self_module,NULL);
 if(!window)return 1;
 control(L"STATIC",L"All features start OFF. F1 shows / hides this window.",0,20,16,510,24,0);
 for(int i=0;i<FEATURE_COUNT;i++) {
  checks[i]=control(L"BUTTON",labels[i],BS_AUTOCHECKBOX|WS_TABSTOP,24,49+31*i,260,26,100+i);EnableWindow(checks[i],FALSE);
 }
 control(L"STATIC",L"Multiplier (2-50)",0,310,114,150,20,0);
 control(L"EDIT",L"2",WS_BORDER|ES_NUMBER|WS_TABSTOP,465,110,60,24,201);
 control(L"STATIC",L"Vacuum distance (1-10)",0,310,207,150,22,0);
 control(L"EDIT",L"3",WS_BORDER|ES_NUMBER|WS_TABSTOP,465,203,60,24,202);
 control(L"BUTTON",L"Disable all",BS_PUSHBUTTON|WS_TABSTOP,24,245,130,28,200);
 status_label=control(L"STATIC",L"Starting...",0,24,289,510,60,0);
 SetTimer(window,1,500,NULL);ShowWindow(window,SW_SHOWNOACTIVATE);
 if(logging) {
  log_line("=== Stella Sora Runtime 2 diagnostics 3 ===");
  log_line("Source: TheReVeaLz/StellaSora-Tool 30f67811411dca754f1b876b4d4dbf6a001e0a20; new runtime backend.");
  log_line("No network, key handling, anti-cheat changes or game file writes. Closing the menu disables every feature.");
  HANDLE thread=CreateThread(NULL,0,initialize,NULL,0,NULL);if(thread)CloseHandle(thread);
  else InterlockedExchange(&init_state,-1);
 } else {
  InterlockedExchange(&init_state,-1);SetWindowTextW(status_label,L"Cannot create SSTool2.log. Put the DLL in a writable folder.\r\nStartup stopped.");
 }
 MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
 return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
 (void)reserved;
 if(reason==DLL_PROCESS_ATTACH) {
  self_module=module;
  /* Static TLS is used by the reentrancy guard, so keep thread notifications. */
  HANDLE thread=CreateThread(NULL,0,run,NULL,0,NULL);if(thread)CloseHandle(thread);
 }
 return TRUE;
}
