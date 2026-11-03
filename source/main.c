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
static HWND window,checks[FEATURE_COUNT],event_checks[3],tabs[2],status_label;
static HWND multiplier_hint,event_hint,section_title,section_caption,footer;
static HWND page_controls[2][16];static int page_count[2],active_page;
static bool global_hotkey;
static HBRUSH background_brush,card_brush,background_pattern;
static HFONT body_font,title_font,section_font;
static HDC background_dc;static HBITMAP background_bitmap;
static volatile LONG init_state;
static const wchar_t *labels[FEATURE_COUNT]={L"Instant Break",L"God Mode",L"Damage Multiplier",L"No Cooldown",L"Unlimited Energy",L"Mob Vacuum"};
/* Release build: diagnostics do not create files. */
void log_line(const char *format,...){(void)format;}
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
 HMODULE other=GetModuleHandleW(L"Mastercheat.dll");
 if(other&&other!=self_module){log_line("Another Mastercheat.dll is loaded. Restart with only this DLL.");goto failed;}
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

static void update_text(HWND hwnd,const wchar_t *text) {
 if(!hwnd)return;wchar_t old[512]={0};GetWindowTextW(hwnd,old,512);if(wcscmp(old,text))SetWindowTextW(hwnd,text);
}
static void disable_all(void) {
 for(int i=0;i<3;i++){InterlockedExchange(&event_flags[i],0);if(event_checks[i])InvalidateRect(event_checks[i],NULL,TRUE);}
 for(int i=0;i<FEATURE_COUNT;i++){InterlockedExchange(&feature_flags[i],0);if(checks[i])InvalidateRect(checks[i],NULL,TRUE);}
}
static void refresh_status(void) {
 LONG state=read_atomic(&init_state);int player=0,events=0;
 for(int i=0;i<FEATURE_COUNT;i++)if(checks[i]){BOOL on=state==1&&read_atomic(&capabilities[i]);EnableWindow(checks[i],on);if(on)player++;}
 for(int i=0;i<3;i++){BOOL on=state==1&&read_atomic(&event_ready[i]);if(event_checks[i]){EnableWindow(event_checks[i],on);InvalidateRect(event_checks[i],NULL,FALSE);}if(on)events++;}
 wchar_t text[256];
 if(state<0)wcscpy(text,L"Startup stopped. Game version unsupported or runtime unavailable.");
 else if(!state)wcscpy(text,L"Connecting to the game... enter the main menu.");
 else swprintf(text,256,L"READY   /   Player %d of 5   /   Event %d of 3",player,events);
 update_text(status_label,text);
 swprintf(text,256,L"Damage  x%ld  /  2 - 50",(long)read_atomic(&multi_count));update_text(multiplier_hint,text);
 swprintf(text,256,L"Score  x%ld  /  2 - 100",(long)read_atomic(&event_multiplier));update_text(event_hint,text);
}
static void toggle_panel(HWND hwnd) {
 if(IsWindowVisible(hwnd)&&!IsIconic(hwnd))ShowWindow(hwnd,SW_HIDE);
 else {ShowWindow(hwnd,SW_RESTORE);SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);SetForegroundWindow(hwnd);}
}
static void make_background(HWND hwnd) {
 RECT rect;GetClientRect(hwnd,&rect);int w=rect.right,h=rect.bottom;HDC dc=GetDC(hwnd);
 background_dc=CreateCompatibleDC(dc);background_bitmap=CreateCompatibleBitmap(dc,w,h);SelectObject(background_dc,background_bitmap);
 FillRect(background_dc,&rect,background_brush);
 HBITMAP image=LoadImageW(self_module,MAKEINTRESOURCEW(301),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
 if(image){
  BITMAP info;GetObjectW(image,sizeof(info),&info);HDC src=CreateCompatibleDC(dc);HGDIOBJ old=SelectObject(src,image);
  int sw=info.bmWidth,sh=info.bmHeight;
  if((int64_t)sw*h>(int64_t)sh*w)sw=sh*w/h;else sh=sw*h/w;
  SetStretchBltMode(background_dc,HALFTONE);SetBrushOrgEx(background_dc,0,0,NULL);
  StretchBlt(background_dc,0,0,w,h,src,(info.bmWidth-sw)/2,(info.bmHeight-sh)/2,sw,sh,SRCCOPY);
  SelectObject(src,old);DeleteDC(src);DeleteObject(image);
  HDC tint=CreateCompatibleDC(dc);HBITMAP pixel=CreateCompatibleBitmap(dc,1,1);HGDIOBJ previous=SelectObject(tint,pixel);
  SetPixel(tint,0,0,RGB(6,13,27));BLENDFUNCTION blend={AC_SRC_OVER,0,105,0};
  AlphaBlend(background_dc,0,0,w,h,tint,0,0,1,1,blend);
  SelectObject(tint,previous);DeleteObject(pixel);DeleteDC(tint);
 }
 RECT rail={0,0,178,h};HBRUSH brush=CreateSolidBrush(RGB(12,19,32));FillRect(background_dc,&rail,brush);DeleteObject(brush);
 background_pattern=CreatePatternBrush(background_bitmap);ReleaseDC(hwnd,dc);
}
static void draw_button(DRAWITEMSTRUCT *item) {
 HDC dc=item->hDC;RECT rect=item->rcItem;int id=(int)item->CtlID;
 bool player=id>=100&&id<100+FEATURE_COUNT,event_control=id>=300&&id<303,tab=id==400||id==401;
 bool active=player?read_atomic(&feature_flags[id-100]):event_control?read_atomic(&event_flags[id-300]):tab&&active_page==id-400;
 bool available=!(item->itemState&ODS_DISABLED);
 COLORREF fill=tab?(active?RGB(32,68,81):RGB(18,28,44)):RGB(21,32,49);
 HBRUSH brush=CreateSolidBrush(fill);FillRect(dc,&rect,brush);DeleteObject(brush);
 if(active){RECT line={0,0,3,rect.bottom};brush=CreateSolidBrush(RGB(80,226,201));FillRect(dc,&line,brush);DeleteObject(brush);}
 SetBkMode(dc,TRANSPARENT);SelectObject(dc,body_font);SetTextColor(dc,available?RGB(232,240,252):RGB(111,127,148));
 RECT text=rect;text.left+=16;if(player||event_control)text.right-=70;
 wchar_t label[100];GetWindowTextW(item->hwndItem,label,100);DrawTextW(dc,label,-1,&text,DT_SINGLELINE|DT_VCENTER|DT_LEFT);
 if(player||event_control){
  RECT badge={rect.right-59,rect.top+13,rect.right-12,rect.bottom-13};brush=CreateSolidBrush(active?RGB(74,220,191):RGB(44,59,79));FillRect(dc,&badge,brush);DeleteObject(brush);
  SetTextColor(dc,active?RGB(7,31,30):RGB(175,191,213));DrawTextW(dc,active?L"ON":L"OFF",-1,&badge,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
 }
 if(item->itemState&ODS_FOCUS)DrawFocusRect(dc,&rect);
}
static void switch_page(int page) {
 active_page=page;
 for(int p=0;p<2;p++)for(int i=0;i<page_count[p];i++)ShowWindow(page_controls[p][i],p==page?SW_SHOW:SW_HIDE);
 update_text(section_title,page?L"Bouncing Bumper Bash":L"Player");
 update_text(section_caption,page?L"Independent skill and score controls":L"Your core combat toolkit");
 update_text(footer,page?L"F6 cooldown   /   F7 energy   /   F8 score":L"Hide the panel with F1. Your toggles stay active.");
 InvalidateRect(window,NULL,TRUE);InvalidateRect(tabs[0],NULL,TRUE);InvalidateRect(tabs[1],NULL,TRUE);
}
static LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
 switch(message){
 case WM_COMMAND:{
  int id=LOWORD(wp),notice=HIWORD(wp);
  if(notice==BN_CLICKED){
   if(id==400||id==401){switch_page(id-400);return 0;}
   if(id>=100&&id<100+FEATURE_COUNT){int i=id-100;if(read_atomic(&capabilities[i]))InterlockedExchange(&feature_flags[i],!read_atomic(&feature_flags[i]));InvalidateRect(checks[i],NULL,TRUE);}
   else if(id>=300&&id<303){int i=id-300;if(read_atomic(&event_ready[i]))InterlockedExchange(&event_flags[i],!read_atomic(&event_flags[i]));InvalidateRect(event_checks[i],NULL,TRUE);}
   else if(id==200)disable_all();
  }
  if((id==201||id==202||id==203)&&(notice==EN_CHANGE||notice==EN_KILLFOCUS)){
   BOOL valid;UINT value=GetDlgItemInt(hwnd,id,&valid,FALSE);UINT minimum=id==202?1:2,maximum=id==202?10:id==203?100:50;
   volatile LONG *setting=id==201?&multi_count:id==202?&vacuum_distance:&event_multiplier;
   if(notice==EN_KILLFOCUS){if(!valid)value=(UINT)read_atomic(setting);if(value<minimum)value=minimum;if(value>maximum)value=maximum;SetDlgItemInt(hwnd,id,value,FALSE);valid=TRUE;}
   if(valid&&value>=minimum&&value<=maximum)InterlockedExchange(setting,(LONG)value);
   refresh_status();
  }
  return 0;
 }
 case WM_HOTKEY:
  if(wp==1)toggle_panel(hwnd);
  else if(wp>=6&&wp<=8){int i=(int)wp-6;if(read_atomic(&event_ready[i]))InterlockedExchange(&event_flags[i],!read_atomic(&event_flags[i]));refresh_status();}
  return 0;
 case WM_DRAWITEM:if(((DRAWITEMSTRUCT*)lp)->CtlType==ODT_BUTTON){draw_button((DRAWITEMSTRUCT*)lp);return TRUE;}break;
 case WM_CTLCOLORSTATIC:{
  SetTextColor((HDC)wp,RGB(214,228,246));SetBkMode((HDC)wp,TRANSPARENT);
  POINT origin={0,0};MapWindowPoints((HWND)lp,hwnd,&origin,1);SetBrushOrgEx((HDC)wp,-origin.x,-origin.y,NULL);
  return (LRESULT)(background_pattern?background_pattern:background_brush);
 }
 case WM_CTLCOLOREDIT:SetTextColor((HDC)wp,RGB(240,248,255));SetBkColor((HDC)wp,RGB(21,32,49));return (LRESULT)card_brush;
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT paint;HDC dc=BeginPaint(hwnd,&paint);if(background_dc){RECT r;GetClientRect(hwnd,&r);BitBlt(dc,0,0,r.right,r.bottom,background_dc,0,0,SRCCOPY);}EndPaint(hwnd,&paint);return 0;}
 case WM_TIMER:
  if(!global_hotkey){static bool previous;bool down=(GetAsyncKeyState(VK_F1)&0x8000)!=0;if(down&&!previous)toggle_panel(hwnd);previous=down;}
  refresh_status();return 0;
 case WM_APP+1:refresh_status();return 0;
 case WM_CLOSE:disable_all();ShowWindow(hwnd,SW_HIDE);return 0;
 default:return DefWindowProcW(hwnd,message,wp,lp);
 }
 return DefWindowProcW(hwnd,message,wp,lp);
}
static HWND control(const wchar_t *kind,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id) {
 HWND c=CreateWindowW(kind,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,(HMENU)(intptr_t)id,self_module,NULL);
 if(c)SendMessageW(c,WM_SETFONT,(WPARAM)body_font,TRUE);return c;
}
static HWND page_control(int page,const wchar_t *kind,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id){
 HWND c=control(kind,text,style,x,y,w,h,id);if(c&&page_count[page]<16)page_controls[page][page_count[page]++]=c;return c;
}
static DWORD WINAPI run(void *unused) {
 (void)unused;WNDCLASSW wc={0};wc.lpfnWndProc=procedure;wc.hInstance=self_module;wc.lpszClassName=L"SSToolRuntime2";
 wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));
 background_brush=CreateSolidBrush(RGB(9,17,30));card_brush=CreateSolidBrush(RGB(21,32,49));wc.hbrBackground=background_brush;
 body_font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 title_font=CreateFontW(-29,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 section_font=CreateFontW(-23,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 if(!RegisterClassW(&wc))return 1;
 window=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,wc.lpszClassName,L"SSTool v1.0 | Stella Sora",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,820,620,NULL,NULL,self_module,NULL);
 if(!window)return 1;
 HMODULE dwm=LoadLibraryExW(L"dwmapi.dll",NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(dwm){typedef HRESULT (WINAPI *SetAttribute)(HWND,DWORD,LPCVOID,DWORD);FARPROC proc=GetProcAddress(dwm,"DwmSetWindowAttribute");SetAttribute set=NULL;memcpy(&set,&proc,sizeof(set));BOOL dark=TRUE;if(set)set(window,20,&dark,sizeof(dark));FreeLibrary(dwm);}
 make_background(window);
 HWND h=control(L"STATIC",L"SSTOOL",0,24,25,145,40,0);SendMessageW(h,WM_SETFONT,(WPARAM)title_font,TRUE);
 control(L"STATIC",L"STELLA SORA",0,200,32,280,28,0);control(L"STATIC",L"F1  SHOW / HIDE",0,640,32,160,24,0);
 control(L"STATIC",L"WORKSPACE",0,24,100,145,24,0);
 tabs[0]=control(L"BUTTON",L"01  Player",BS_OWNERDRAW|WS_TABSTOP,16,140,146,48,400);
 tabs[1]=control(L"BUTTON",L"02  Event",BS_OWNERDRAW|WS_TABSTOP,16,200,146,48,401);
 section_title=control(L"STATIC",L"Player",0,200,96,560,32,0);SendMessageW(section_title,WM_SETFONT,(WPARAM)section_font,TRUE);
 section_caption=control(L"STATIC",L"Your core combat toolkit",0,200,132,560,24,0);
 const int visible[]={GOD_MODE,MULTI_HIT,NO_COOLDOWN,UNLIMITED_ENERGY,MOB_VACUUM};
 for(int row=0;row<5;row++){int i=visible[row];checks[i]=page_control(0,L"BUTTON",labels[i],BS_OWNERDRAW|WS_TABSTOP,200,176+54*row,350,46,100+i);EnableWindow(checks[i],FALSE);}
 multiplier_hint=page_control(0,L"STATIC",L"Damage  x2  /  2 - 50",0,578,181,198,26,0);
 page_control(0,L"EDIT",L"2",WS_BORDER|ES_NUMBER|WS_TABSTOP,578,216,192,34,201);
 page_control(0,L"STATIC",L"Vacuum distance  /  1 - 10",0,578,292,200,26,0);
 page_control(0,L"EDIT",L"3",WS_BORDER|ES_NUMBER|WS_TABSTOP,578,326,192,34,202);
 const wchar_t *event_labels[]={L"No Cooldown (R)",L"Unlimited Energy (R)",L"Score Multiplier"};
 for(int i=0;i<3;i++){event_checks[i]=page_control(1,L"BUTTON",event_labels[i],BS_OWNERDRAW|WS_TABSTOP,200,176+54*i,350,46,300+i);EnableWindow(event_checks[i],FALSE);}
 event_hint=page_control(1,L"STATIC",L"Score  x2  /  2 - 100",0,578,181,198,26,0);
 page_control(1,L"EDIT",L"2",WS_BORDER|ES_NUMBER|WS_TABSTOP,578,216,192,34,203);
 page_control(1,L"STATIC",L"Higher factors reach the\r\ntarget score sooner.\r\nStart low and increase.",0,578,280,196,80,0);
 page_control(1,L"STATIC",L"Event settings are separate from Player.",0,200,360,560,26,0);
 control(L"BUTTON",L"Disable all",BS_OWNERDRAW|WS_TABSTOP,16,440,146,42,200);
 control(L"STATIC",L"good luck\r\nhave fun!",0,24,502,140,52,0);
 status_label=control(L"STATIC",L"Connecting to game...",0,200,478,570,28,0);
 footer=control(L"STATIC",L"",0,200,518,570,28,0);
 for(int i=6;i<=8;i++)RegisterHotKey(window,i,MOD_NOREPEAT,VK_F6+i-6);
 global_hotkey=RegisterHotKey(window,1,MOD_NOREPEAT,VK_F1)!=0;
 switch_page(0);SetTimer(window,1,global_hotkey?500:50,NULL);ShowWindow(window,SW_SHOWNOACTIVATE);
 HANDLE thread=CreateThread(NULL,0,initialize,NULL,0,NULL);if(thread)CloseHandle(thread);else InterlockedExchange(&init_state,-1);
 MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
 return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
 (void)reserved;if(reason==DLL_PROCESS_ATTACH){self_module=module;HANDLE thread=CreateThread(NULL,0,run,NULL,0,NULL);if(thread)CloseHandle(thread);}return TRUE;
}
