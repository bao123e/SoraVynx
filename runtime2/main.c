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
static HWND window,checks[FEATURE_COUNT],status_label,multiplier_hint;
static bool global_hotkey;
static HBRUSH background_brush,card_brush;
static HFONT body_font,title_font;
static FILE *log_file;
static CRITICAL_SECTION log_lock;
static volatile LONG init_state;
static const wchar_t *labels[FEATURE_COUNT]={L"Instant Break",L"God Mode",L"Damage Multiplier",L"No Cooldown",L"Unlimited Energy",L"Mob Vacuum"};
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
 if(!install_frame_backend(game))goto failed;
 api.thread_detach(thread);InterlockedExchange(&init_state,1);PostMessageW(window,WM_APP+1,0,0);return 0;
 failed:
 if(thread)api.thread_detach(thread);
 InterlockedExchange(&init_state,-1);PostMessageW(window,WM_APP+1,0,0);return 1;
}
static void disable_all(void) {
 for(int i=0;i<FEATURE_COUNT;i++){InterlockedExchange(&feature_flags[i],0);if(checks[i]){SendMessageW(checks[i],BM_SETCHECK,BST_UNCHECKED,0);InvalidateRect(checks[i],NULL,TRUE);}}
 log_line("All features disabled; installed hooks are pass-through.");
}
static void refresh_status(void) {
 LONG state=read_atomic(&init_state);
 if(state<0){SetWindowTextW(status_label,L"Startup stopped. Read SSTool2.log beside the DLL.\r\nNo features are enabled.");return;}
 if(!state){SetWindowTextW(status_label,L"Waiting for the game and Hotfix runtime...\r\nEnter the main menu. Features remain off until ready.");return;}
 int available=0;
 for(int i=0;i<FEATURE_COUNT;i++){BOOL enabled=read_atomic(&capabilities[i])!=0;EnableWindow(checks[i],enabled);if(enabled)available++;}
 wchar_t text[256];swprintf(text,256,L"Ready  |  %d features available  |  F1 show / hide",available);
 SetWindowTextW(status_label,text);
 wchar_t hint[64];swprintf(hint,64,L"Multiplier 2-50: x%ld",(long)read_atomic(&multi_count));SetWindowTextW(multiplier_hint,hint);
 report_observed_calls();
}
static void toggle_panel(HWND hwnd) {
 if(IsWindowVisible(hwnd)&&!IsIconic(hwnd))ShowWindow(hwnd,SW_HIDE);
 else {ShowWindow(hwnd,SW_RESTORE);SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);SetForegroundWindow(hwnd);}
}
static void draw_button(DRAWITEMSTRUCT *item) {
 HDC dc=item->hDC;RECT rect=item->rcItem;
 FillRect(dc,&rect,card_brush);SetBkMode(dc,TRANSPARENT);SelectObject(dc,body_font);
 int id=(int)item->CtlID;bool feature=id>=100&&id<100+FEATURE_COUNT;
 bool available=!(item->itemState&ODS_DISABLED),active=feature&&read_atomic(&feature_flags[id-100]);
 SetTextColor(dc,available?RGB(229,234,245):RGB(103,114,135));
 RECT text=rect;text.left+=14;text.right-=feature?70:0;
 wchar_t label[100];GetWindowTextW(item->hwndItem,label,100);DrawTextW(dc,label,-1,&text,DT_SINGLELINE|DT_VCENTER|DT_LEFT);
 if(feature) {
  RECT badge={rect.right-61,rect.top+10,rect.right-10,rect.bottom-10};
  HBRUSH brush=CreateSolidBrush(active?RGB(51,196,159):RGB(57,67,87));FillRect(dc,&badge,brush);DeleteObject(brush);
  SetTextColor(dc,active?RGB(10,30,30):RGB(172,183,204));DrawTextW(dc,active?L"ON":L"OFF",-1,&badge,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
 }
 if(item->itemState&ODS_FOCUS)DrawFocusRect(dc,&rect);
}
static LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
 switch(message) {
 case WM_COMMAND: {
  int id=LOWORD(wp);
  if(id>=100&&id<100+FEATURE_COUNT&&HIWORD(wp)==BN_CLICKED) {
   int f=id-100;
   if(!read_atomic(&capabilities[f])){SendMessageW(checks[f],BM_SETCHECK,BST_UNCHECKED,0);break;}
   LONG value=!read_atomic(&feature_flags[f]);
   InvalidateRect(checks[f],NULL,TRUE);
   InterlockedExchange(&feature_flags[f],value);log_line("Feature %d -> %s",f,value?"ON":"OFF");
  } else if(id==200)disable_all();
  else if((id==201||id==202)&&(HIWORD(wp)==EN_CHANGE||HIWORD(wp)==EN_KILLFOCUS)) {
   BOOL valid=FALSE;UINT value=GetDlgItemInt(hwnd,id,&valid,FALSE);
   UINT minimum=id==201?2:1,maximum=id==201?50:10;
   volatile LONG *setting=id==201?&multi_count:&vacuum_distance;
   if(HIWORD(wp)==EN_KILLFOCUS) {
    if(!valid)value=(UINT)read_atomic(setting);
    if(value<minimum)value=minimum;if(value>maximum)value=maximum;
    SetDlgItemInt(hwnd,id,value,FALSE);
   }
   if(value>=minimum&&value<=maximum&&(valid||HIWORD(wp)==EN_KILLFOCUS)) {
    LONG previous=InterlockedExchange(setting,(LONG)value);
    if(previous!=(LONG)value)log_line("SETTING %s=%u",id==201?"damage_multiplier":"vacuum_distance",value);
   }
  }
  break;
 }
 case WM_HOTKEY:if(wp==1){toggle_panel(hwnd);return 0;}break;
 case WM_DRAWITEM:if(((DRAWITEMSTRUCT*)lp)->CtlType==ODT_BUTTON){draw_button((DRAWITEMSTRUCT*)lp);return TRUE;}break;
 case WM_CTLCOLORSTATIC:
  SetTextColor((HDC)wp,RGB(181,193,215));SetBkColor((HDC)wp,RGB(18,24,36));return (LRESULT)background_brush;
 case WM_CTLCOLOREDIT:
  SetTextColor((HDC)wp,RGB(237,243,253));SetBkColor((HDC)wp,RGB(30,39,56));return (LRESULT)card_brush;
 case WM_TIMER: {
  if(!global_hotkey){static bool was_down;bool down=(GetAsyncKeyState(VK_F1)&0x8000)!=0;if(down&&!was_down)toggle_panel(hwnd);was_down=down;}
  refresh_status();break;
 }
 case WM_APP+1:refresh_status();break;
 case WM_CLOSE:disable_all();ShowWindow(hwnd,SW_HIDE);return 0;
 default:return DefWindowProcW(hwnd,message,wp,lp);
 }
 return 0;
}
static HWND control(const wchar_t *kind,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id) {
 HWND c=CreateWindowW(kind,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,(HMENU)(intptr_t)id,self_module,NULL);
 if(c)SendMessageW(c,WM_SETFONT,(WPARAM)body_font,TRUE);
 return c;
}
static DWORD WINAPI run(void *unused) {
 (void)unused;InitializeCriticalSection(&log_lock);bool logging=open_log();
 WNDCLASSW wc={0};wc.lpfnWndProc=procedure;wc.hInstance=self_module;wc.lpszClassName=L"SSToolRuntime2";
 wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));background_brush=CreateSolidBrush(RGB(18,24,36));card_brush=CreateSolidBrush(RGB(30,39,56));
 body_font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 title_font=CreateFontW(-25,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 wc.hbrBackground=background_brush;
 if(!RegisterClassW(&wc))return 1;
 window=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,wc.lpszClassName,L"Stella Sora | SSTool",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,600,490,NULL,NULL,self_module,NULL);
 if(!window)return 1;
 HWND heading=control(L"STATIC",L"STELLA SORA",0,24,18,300,34,0);SendMessageW(heading,WM_SETFONT,(WPARAM)title_font,TRUE);
 control(L"STATIC",L"SSTOOL  /  COMBAT CONTROLS",0,24,55,400,24,0);
 control(L"STATIC",L"F1  Show / Hide",0,390,28,170,24,0);
 const int visible[]={GOD_MODE,MULTI_HIT,NO_COOLDOWN,UNLIMITED_ENERGY,MOB_VACUUM};
 for(int row=0;row<5;row++) {int i=visible[row];checks[i]=control(L"BUTTON",labels[i],BS_OWNERDRAW|WS_TABSTOP,24,94+48*row,280,40,100+i);EnableWindow(checks[i],FALSE);}
 multiplier_hint=control(L"STATIC",L"Multiplier 2-50: x2",0,330,103,220,24,0);
 control(L"EDIT",L"2",WS_BORDER|ES_NUMBER|WS_TABSTOP,330,136,214,32,201);
 control(L"STATIC",L"Vacuum distance (1-10)",0,330,207,220,24,0);
 control(L"EDIT",L"3",WS_BORDER|ES_NUMBER|WS_TABSTOP,330,240,214,32,202);
 control(L"BUTTON",L"Disable all",BS_OWNERDRAW|WS_TABSTOP,24,350,280,38,200);
 control(L"STATIC",L"Features start OFF.\r\nF1 works while this panel is hidden.",0,330,335,220,50,0);
 status_label=control(L"STATIC",L"Starting...",0,24,410,530,30,0);
 global_hotkey=RegisterHotKey(window,1,MOD_NOREPEAT,VK_F1)!=0;
 if(logging)log_line("F1 global hotkey: %s",global_hotkey?"registered":"unavailable; using focus-independent key polling");
 SetTimer(window,1,global_hotkey?500:50,NULL);ShowWindow(window,SW_SHOWNOACTIVATE);
 if(logging) {
  log_line("=== Stella Sora SSTool UI 1 ===");
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
