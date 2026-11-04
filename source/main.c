/* SPDX-License-Identifier: AGPL-3.0-or-later */
#define WIN32_LEAN_AND_MEAN
#include "backend.h"
#include <bcrypt.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "ui_layout.h"
#include "ui_background.h"
#include "ui_fonts.h"
#include "daily_setup.h"
#include "daily_lua_probe.h"

/* Image, panel and button opacity are independent; text stays fully visible. */
#define UI_BG RGB(9,17,30)
#define UI_RAIL RGB(12,22,37)
#define UI_CARD RGB(18,30,47)
#define UI_BUTTON RGB(30,46,65)
#define UI_TEXT RGB(238,245,255)
#define UI_MUTED RGB(179,197,219)
#define UI_ACCENT RGB(94,234,212)

static HMODULE self_module;
static HWND window,checks[FEATURE_COUNT],event_checks[3],tabs[4],status_label,daily_report;
static unsigned daily_selection=4095;
static HICON brand_icon;
static HWND multiplier_hint,event_hint,section_title,section_caption,footer;
static HWND background_name,background_mode;
static HWND page_controls[4][16];static int page_count[4],active_page;
static bool global_hotkey;
static HBRUSH background_brush,background_pattern;
static HFONT body_font,button_font,title_font,section_font,daily_font;
static HDC background_dc;static HBITMAP background_bitmap;
static HGDIOBJ background_original;
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
 HMODULE other=GetModuleHandleW(L"SoraVynx.Runtime.dll");
 if(other&&other!=self_module){log_line("Another SoraVynx.Runtime.dll is loaded. Restart with only this DLL.");goto failed;}
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
 (void)daily_lua_install(game,self_module);
 api.thread_detach(thread);InterlockedExchange(&init_state,1);PostMessageW(window,WM_APP+1,0,0);return 0;
 failed:
 if(thread)api.thread_detach(thread);
 InterlockedExchange(&init_state,-1);PostMessageW(window,WM_APP+1,0,0);return 1;
}

static void update_text(HWND hwnd,const wchar_t *text) {
 if(!hwnd)return;wchar_t old[512]={0};GetWindowTextW(hwnd,old,512);if(wcscmp(old,text))SetWindowTextW(hwnd,text);
}
static void disable_all(void) {
 daily_lua_cancel_queued();
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
 if(daily_report){LONG pending=InterlockedCompareExchange(&daily_lua_state,0,0);EnableWindow(daily_report,state==1&&!pending);update_text(daily_report,pending&&!daily_lua_kind?L"Inspecting Lua names...":L"Copy Lua name report");}
 LONG pending_action=InterlockedCompareExchange(&daily_lua_state,0,0);
 EnableWindow(GetDlgItem(window,432),state==1&&!pending_action&&(daily_selection&(1u<<7)));
 update_text(GetDlgItem(window,432),pending_action&&daily_lua_kind?L"Return to the game...":L"Tower reward / status");
 swprintf(text,256,L"Damage Multiplier  (x%ld / 2 - 50)",(long)read_atomic(&multi_count));update_text(multiplier_hint,text);
 swprintf(text,256,L"Score Multiplier  (x%ld / 2 - 100)",(long)read_atomic(&event_multiplier));update_text(event_hint,text);
 swprintf(text,256,L"Mob Vacuum  (distance %ld / 1 - 10)",(long)read_atomic(&vacuum_distance));update_text(checks[MOB_VACUUM],text);
}
static void toggle_panel(HWND hwnd) {
 if(IsWindowVisible(hwnd)&&!IsIconic(hwnd))ShowWindow(hwnd,SW_HIDE);
 else {ShowWindow(hwnd,SW_RESTORE);SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);SetForegroundWindow(hwnd);}
}
static void make_background(HWND hwnd) {
 if(background_pattern){DeleteObject(background_pattern);background_pattern=NULL;}
 if(background_dc){SelectObject(background_dc,background_original);DeleteObject(background_bitmap);DeleteDC(background_dc);background_dc=NULL;}
 RECT rect;GetClientRect(hwnd,&rect);int w=rect.right,h=rect.bottom;HDC dc=GetDC(hwnd);
 background_dc=CreateCompatibleDC(dc);background_bitmap=CreateCompatibleBitmap(dc,w,h);background_original=SelectObject(background_dc,background_bitmap);
 FillRect(background_dc,&rect,background_brush);
 HBITMAP image=custom_image;
 if(image){
  BITMAP info;GetObjectW(image,sizeof(info),&info);HDC src=CreateCompatibleDC(dc);HGDIOBJ old=SelectObject(src,image);
  HDC scaled=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,w,h);HGDIOBJ previous=SelectObject(scaled,bitmap);
  FillRect(scaled,&rect,background_brush);
  struct ImageFit fit=background_fill?cover_image(info.bmWidth,info.bmHeight,0,0,w,h):fit_image(info.bmWidth,info.bmHeight,0,0,w,h);
  SetStretchBltMode(scaled,HALFTONE);SetBrushOrgEx(scaled,0,0,NULL);
  if(fit.width>0&&fit.height>0)StretchBlt(scaled,fit.x,fit.y,fit.width,fit.height,src,0,0,info.bmWidth,info.bmHeight,SRCCOPY);
  BLENDFUNCTION blend={AC_SRC_OVER,0,(BYTE)(background_opacity*255/100),0};
  AlphaBlend(background_dc,0,0,w,h,scaled,0,0,w,h,blend);
  SelectObject(scaled,previous);DeleteObject(bitmap);DeleteDC(scaled);SelectObject(src,old);DeleteDC(src);
 }
 BYTE panel_alpha=(BYTE)(panel_opacity*255/100),button_alpha=(BYTE)(button_opacity*255/100);
 RECT rail={0,0,176,h},header={176,0,w,80},panel={192,88,976,488},status={192,504,976,584};
 tint_rect(background_dc,rail,UI_RAIL,panel_alpha);tint_rect(background_dc,header,UI_BG,panel_alpha);
 tint_rect(background_dc,panel,UI_CARD,panel_alpha);
 tint_rect(background_dc,status,UI_CARD,panel_alpha);
 RECT appearance_inputs[3]={{432,296,520,332},{856,296,944,332},{432,368,520,404}};
 if(active_page==2){for(int i=0;i<3;i++)tint_rect(background_dc,appearance_inputs[i],UI_BUTTON,button_alpha);}
 else if(active_page==3){for(int i=0;i<12;i++){RECT row={208,180+25*i,624,204+25*i};tint_rect(background_dc,row,UI_BUTTON,button_alpha);}}
 else {for(int i=0;i<(active_page==0?5:3);i++){RECT row={208,180+58*i,960,228+58*i};tint_rect(background_dc,row,UI_BUTTON,button_alpha);}}
 if(brand_icon)DrawIconEx(background_dc,24,12,brand_icon,28,28,0,NULL,DI_NORMAL);
 RECT accent={24,88,152,91};HBRUSH brush=CreateSolidBrush(UI_ACCENT);FillRect(background_dc,&accent,brush);DeleteObject(brush);
 background_pattern=CreatePatternBrush(background_bitmap);ReleaseDC(hwnd,dc);
}
static void refresh_background(void) {
 const wchar_t *name=wcsrchr(background_path,L'\\');name=name?name+1:background_path;
 update_text(background_name,custom_image?name:background_path[0]?L"Image unavailable - choose another":L"No background selected");
 update_text(background_mode,background_fill?L"Image fit: Fill window":L"Image fit: Full image");
 make_background(window);RedrawWindow(window,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
}
static void shadow_text(HDC dc,const wchar_t *text,RECT rect,UINT flags,COLORREF color) {
 SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(0,0,0));
 const int offsets[5][2]={{-1,0},{1,0},{0,-1},{0,1},{1,2}};
 for(int i=0;i<5;i++){RECT shadow=rect;OffsetRect(&shadow,offsets[i][0],offsets[i][1]);DrawTextW(dc,text,-1,&shadow,flags|DT_NOPREFIX);}
 SetTextColor(dc,color);DrawTextW(dc,text,-1,&rect,flags|DT_NOPREFIX);
}
static void copy_control_background(DRAWITEMSTRUCT *item) {
 POINT origin={0,0};MapWindowPoints(item->hwndItem,window,&origin,1);
 if(background_dc)BitBlt(item->hDC,0,0,item->rcItem.right,item->rcItem.bottom,background_dc,origin.x,origin.y,SRCCOPY);
}
static void draw_label(DRAWITEMSTRUCT *item) {
 copy_control_background(item);HFONT font=(HFONT)SendMessageW(item->hwndItem,WM_GETFONT,0,0);
 if(font)SelectObject(item->hDC,font);
 wchar_t text[512];GetWindowTextW(item->hwndItem,text,512);
 UINT flags=DT_LEFT|DT_TOP;
 if(GetWindowLongPtrW(item->hwndItem,GWL_STYLE)&SS_PATHELLIPSIS)flags|=DT_PATH_ELLIPSIS|DT_SINGLELINE;
 shadow_text(item->hDC,text,item->rcItem,flags,item->CtlID==500?UI_ACCENT:UI_TEXT);
}
static void draw_button(DRAWITEMSTRUCT *item) {
 HDC dc=item->hDC;RECT rect=item->rcItem;int id=(int)item->CtlID;
 bool player=id>=100&&id<100+FEATURE_COUNT,event_control=id>=300&&id<303,tab=id>=400&&id<=403,daily=id>=600&&id<612;
 bool active=player?read_atomic(&feature_flags[id-100]):event_control?read_atomic(&event_flags[id-300]):daily?(daily_selection&(1u<<(id-600)))!=0:tab&&active_page==id-400;
 bool available=!(item->itemState&ODS_DISABLED);
 bool pressed=(item->itemState&ODS_SELECTED)!=0;
 copy_control_background(item);
 if(!player&&!event_control&&!daily)tint_rect(dc,rect,tab&&active?RGB(30,77,84):UI_BUTTON,(BYTE)(button_opacity*255/100));
 if(pressed)tint_rect(dc,rect,UI_ACCENT,48);HBRUSH brush;
 if(active){RECT line={0,0,3,rect.bottom};brush=CreateSolidBrush(UI_ACCENT);FillRect(dc,&line,brush);DeleteObject(brush);}
 SetBkMode(dc,TRANSPARENT);SelectObject(dc,daily?daily_font:button_font);SetTextColor(dc,available?UI_TEXT:UI_MUTED);
 RECT text=rect;text.left+=16;if(player||event_control||daily)text.right-=70;
 wchar_t label[100];GetWindowTextW(item->hwndItem,label,100);
 if(id==410||id==411)text=rect;
 shadow_text(dc,label,text,DT_SINGLELINE|DT_VCENTER|((id==410||id==411)?DT_CENTER:DT_LEFT),available?UI_TEXT:UI_MUTED);
 if(player||event_control||daily){
  RECT badge={rect.right-59,rect.top+(daily?2:13),rect.right-12,rect.bottom-(daily?2:13)};
  tint_rect(dc,badge,active?RGB(74,220,191):RGB(44,59,79),(BYTE)(button_opacity*255/100));
  shadow_text(dc,daily?(active?L"USE":L"SKIP"):!available?L"N/A":active?L"ON":L"OFF",badge,DT_CENTER|DT_SINGLELINE|DT_VCENTER,active?UI_ACCENT:UI_TEXT);
 }
 if(item->itemState&ODS_FOCUS){InflateRect(&rect,-4,-4);SetTextColor(dc,UI_TEXT);SetBkColor(dc,UI_BG);DrawFocusRect(dc,&rect);}
}
static void switch_page(int page) {
 if(page<0||page>=4)return;
 active_page=page;
 for(int p=0;p<4;p++)for(int i=0;i<page_count[p];i++)ShowWindow(page_controls[p][i],p==page?SW_SHOW:SW_HIDE);
 const wchar_t *titles[]={L"Player",L"Bouncing Bumper Bash",L"Custom Background",L"Auto Daily"};
 const wchar_t *captions[]={L"Your core combat toolkit",L"Independent skill and score controls",L"Image, panel and button appearance",L"Keep current in-game selections"};
 const wchar_t *footers[]={L"Hide the panel with F1. Your toggles stay active.",L"F6 cooldown   /   F7 energy   /   F8 score",L"Appearance is saved automatically. Gameplay settings stay active.",L"Tower reward / status requests Tower daily rewards. Shop is skipped."};
 update_text(section_title,titles[page]);update_text(section_caption,captions[page]);update_text(footer,footers[page]);
 make_background(window);InvalidateRect(window,NULL,TRUE);for(int i=0;i<4;i++)InvalidateRect(tabs[i],NULL,TRUE);
}
static LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
 switch(message){
 case WM_NCHITTEST:{
  POINT point={(short)LOWORD(lp),(short)HIWORD(lp)};ScreenToClient(hwnd,&point);
  if(point.y>=0&&point.y<80&&point.x>=0&&point.x<880)return HTCAPTION;
  return HTCLIENT;
 }
 case WM_COMMAND:{
  int id=LOWORD(wp),notice=HIWORD(wp);
  if(notice==BN_CLICKED){
   if(id>=400&&id<=403){switch_page(id-400);return 0;}
   if(id==430){daily_lua_request(hwnd);return 0;}
   if(id==431){daily_copy_setup(hwnd);return 0;}
   if(id==432){if(read_atomic(&init_state)==1&&(daily_selection&(1u<<7))){daily_lua_tower_request(hwnd);if(daily_lua_kind==1&&InterlockedCompareExchange(&daily_lua_state,0,0)==1)daily_lua_return_to_game(hwnd);}return 0;}
   if(id==610)return 0;
   if(id>=600&&id<612){daily_selection^=1u<<(id-600);if(id==607&&!(daily_selection&(1u<<7))&&daily_lua_kind==1)daily_lua_cancel_queued();wchar_t value[16];swprintf(value,16,L"%u",daily_selection);if(!WritePrivateProfileStringW(L"AutoDaily",L"Tasks",value,ui_config_path)||!WritePrivateProfileStringW(L"AutoDaily",L"TaskCount",L"12",ui_config_path))MessageBoxW(hwnd,L"Daily task selection could not be saved.",L"SoraVynx",MB_OK|MB_ICONWARNING);InvalidateRect(GetDlgItem(hwnd,id),NULL,TRUE);return 0;}
   if(id==410){ShowWindow(hwnd,SW_HIDE);return 0;}
   if(id==411){SendMessageW(hwnd,WM_CLOSE,0,0);return 0;}
   if(id==420){if(choose_background(hwnd))refresh_background();return 0;}
   if(id==421){if(custom_image)DeleteObject(custom_image);custom_image=NULL;background_path[0]=0;save_background_settings(hwnd);refresh_background();return 0;}
   if(id==422){background_fill=!background_fill;save_background_settings(hwnd);refresh_background();return 0;}
   if(id>=100&&id<100+FEATURE_COUNT){int i=id-100;if(read_atomic(&capabilities[i]))InterlockedExchange(&feature_flags[i],!read_atomic(&feature_flags[i]));InvalidateRect(checks[i],NULL,TRUE);}
   else if(id>=300&&id<303){int i=id-300;if(read_atomic(&event_ready[i]))InterlockedExchange(&event_flags[i],!read_atomic(&event_flags[i]));InvalidateRect(event_checks[i],NULL,TRUE);}
   else if(id==200)disable_all();
  }
  if(id>=204&&id<=206&&(notice==EN_CHANGE||notice==EN_KILLFOCUS)){
   int *setting=id==204?&background_opacity:id==205?&panel_opacity:&button_opacity;
   BOOL valid;UINT value=GetDlgItemInt(hwnd,id,&valid,FALSE);
   if(notice==EN_KILLFOCUS){if(!valid)value=(UINT)*setting;if(value>100)value=100;SetDlgItemInt(hwnd,id,value,FALSE);valid=TRUE;}
   if(valid&&value<=100&&(int)value!=*setting){*setting=(int)value;refresh_background();}
   if(notice==EN_KILLFOCUS)save_background_settings(hwnd);
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
 case WM_DRAWITEM:{DRAWITEMSTRUCT *item=(DRAWITEMSTRUCT*)lp;
  if(item->CtlType==ODT_BUTTON){draw_button(item);return TRUE;}
  if(item->CtlType==ODT_STATIC){draw_label(item);return TRUE;}break;
 }
 case WM_CTLCOLORSTATIC:
 case WM_CTLCOLOREDIT:{
  SetTextColor((HDC)wp,GetDlgCtrlID((HWND)lp)==500?UI_ACCENT:UI_TEXT);SetBkMode((HDC)wp,TRANSPARENT);
  POINT origin={0,0};MapWindowPoints((HWND)lp,hwnd,&origin,1);SetBrushOrgEx((HDC)wp,-origin.x,-origin.y,NULL);
  return (LRESULT)(background_pattern?background_pattern:background_brush);
 }
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT paint;HDC dc=BeginPaint(hwnd,&paint);if(background_dc){RECT r;GetClientRect(hwnd,&r);BitBlt(dc,0,0,r.right,r.bottom,background_dc,0,0,SRCCOPY);}EndPaint(hwnd,&paint);return 0;}
 case WM_TIMER:
  daily_lua_poll(hwnd);
  if(!global_hotkey){static bool previous;bool down=(GetAsyncKeyState(VK_F1)&0x8000)!=0;if(down&&!previous)toggle_panel(hwnd);previous=down;}
  refresh_status();return 0;
 case WM_APP+1:refresh_status();return 0;
 case WM_CLOSE:disable_all();ShowWindow(hwnd,SW_HIDE);return 0;
 default:return DefWindowProcW(hwnd,message,wp,lp);
 }
 return DefWindowProcW(hwnd,message,wp,lp);
}
static HWND control(const wchar_t *kind,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id) {
 if(!wcscmp(kind,L"STATIC"))style|=SS_OWNERDRAW;
 HWND c=CreateWindowW(kind,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,(HMENU)(intptr_t)id,self_module,NULL);
 if(c)SendMessageW(c,WM_SETFONT,(WPARAM)(id>=600&&id<612?daily_font:!wcscmp(kind,L"BUTTON")?button_font:body_font),TRUE);return c;
}
static HWND page_control(int page,const wchar_t *kind,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id){
 HWND c=control(kind,text,style,x,y,w,h,id);if(c&&page_count[page]<16)page_controls[page][page_count[page]++]=c;return c;
}
static DWORD WINAPI run(void *unused) {
 (void)unused;WNDCLASSW wc={0};wc.lpfnWndProc=procedure;wc.hInstance=self_module;wc.lpszClassName=L"SoraVynxRuntime";
 wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));
 brand_icon=LoadImageW(self_module,MAKEINTRESOURCEW(101),IMAGE_ICON,32,32,LR_DEFAULTCOLOR);wc.hIcon=brand_icon;
 background_brush=CreateSolidBrush(UI_BG);wc.hbrBackground=background_brush;
 load_ui_fonts(self_module);
 body_font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,ui_font_faces[0]);
 button_font=CreateFontW(-16,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,ui_font_faces[1]);
 daily_font=CreateFontW(-14,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,ui_font_faces[1]);
 title_font=CreateFontW(-26,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,ui_font_faces[2]);
 section_font=CreateFontW(-23,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,ui_font_faces[1]);
 if(!RegisterClassW(&wc))return 1;
 DWORD style=WS_POPUP|WS_SYSMENU;
 window=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,wc.lpszClassName,L"SoraVynx v1.1 | Stella Sora",style,CW_USEDEFAULT,CW_USEDEFAULT,1000,600,NULL,NULL,self_module,NULL);
 if(!window)return 1;
 HRGN corners=CreateRoundRectRgn(0,0,1001,601,24,24);if(corners&&!SetWindowRgn(window,corners,FALSE))DeleteObject(corners);
 HMODULE dwm=LoadLibraryExW(L"dwmapi.dll",NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(dwm){typedef HRESULT (WINAPI *SetAttribute)(HWND,DWORD,LPCVOID,DWORD);FARPROC proc=GetProcAddress(dwm,"DwmSetWindowAttribute");SetAttribute set=NULL;memcpy(&set,&proc,sizeof(set));BOOL dark=TRUE;if(set)set(window,20,&dark,sizeof(dark));FreeLibrary(dwm);}
 load_background_settings(self_module);make_background(window);
 daily_selection=GetPrivateProfileIntW(L"AutoDaily",L"Tasks",4095,ui_config_path)&4095u;
 if(GetPrivateProfileIntW(L"AutoDaily",L"TaskCount",8,ui_config_path)<12)daily_selection|=0xf00u;
 daily_selection&=~(1u<<10);wchar_t daily_value[16];swprintf(daily_value,16,L"%u",daily_selection);WritePrivateProfileStringW(L"AutoDaily",L"Tasks",daily_value,ui_config_path);
 HWND h=control(L"STATIC",L"SoraVynx",0,24,44,145,36,500);SendMessageW(h,WM_SETFONT,(WPARAM)title_font,TRUE);
 control(L"STATIC",L"STELLA SORA  /  CONTROL PANEL",0,208,34,480,24,0);control(L"STATIC",L"F1 SHOW / HIDE",0,720,34,152,24,0);
 control(L"BUTTON",L"Hide",BS_OWNERDRAW|WS_TABSTOP,888,24,56,40,410);
 control(L"BUTTON",L"X",BS_OWNERDRAW|WS_TABSTOP,952,24,32,40,411);
 control(L"STATIC",L"WORKSPACE",0,24,104,145,24,0);
 tabs[0]=control(L"BUTTON",L"Player",BS_OWNERDRAW|WS_TABSTOP,16,140,146,48,400);
 tabs[1]=control(L"BUTTON",L"Event",BS_OWNERDRAW|WS_TABSTOP,16,200,146,48,401);
 tabs[2]=control(L"BUTTON",L"Custom BG",BS_OWNERDRAW|WS_TABSTOP,16,260,146,48,402);
 tabs[3]=control(L"BUTTON",L"Auto Daily",BS_OWNERDRAW|WS_TABSTOP,16,320,146,48,403);
 section_title=control(L"STATIC",L"Player",0,208,104,344,32,0);SendMessageW(section_title,WM_SETFONT,(WPARAM)section_font,TRUE);
 section_caption=control(L"STATIC",L"Your core combat toolkit",0,208,140,344,24,0);
 const int visible[]={GOD_MODE,MULTI_HIT,NO_COOLDOWN,UNLIMITED_ENERGY,MOB_VACUUM};
 for(int row=0;row<5;row++){int i=visible[row];int width=i==MULTI_HIT||i==MOB_VACUUM?656:752;checks[i]=page_control(0,L"BUTTON",labels[i],BS_OWNERDRAW|WS_TABSTOP,208,180+58*row,width,48,100+i);EnableWindow(checks[i],FALSE);}
 page_control(2,L"BUTTON",L"Choose image...",BS_OWNERDRAW|WS_TABSTOP,208,180,248,44,420);
 page_control(2,L"BUTTON",L"Remove",BS_OWNERDRAW|WS_TABSTOP,472,180,144,44,421);
 background_name=page_control(2,L"STATIC",L"No background selected",SS_PATHELLIPSIS,208,240,752,24,0);
 page_control(2,L"STATIC",L"Image opacity (%)",0,208,304,216,24,0);
 page_control(2,L"EDIT",L"35",WS_BORDER|ES_NUMBER|WS_TABSTOP,432,296,88,36,204);SetDlgItemInt(window,204,(UINT)background_opacity,FALSE);
 page_control(2,L"STATIC",L"Panel opacity (%)",0,568,304,272,24,0);
 page_control(2,L"EDIT",L"35",WS_BORDER|ES_NUMBER|WS_TABSTOP,856,296,88,36,205);SetDlgItemInt(window,205,(UINT)panel_opacity,FALSE);
 page_control(2,L"STATIC",L"Button opacity (%)",0,208,376,216,24,0);
 page_control(2,L"EDIT",L"55",WS_BORDER|ES_NUMBER|WS_TABSTOP,432,368,88,36,206);SetDlgItemInt(window,206,(UINT)button_opacity,FALSE);
 background_mode=page_control(2,L"BUTTON",L"Image fit: Full image",BS_OWNERDRAW|WS_TABSTOP,568,368,392,40,422);
 page_control(2,L"STATIC",L"0% = transparent    /    100% = solid. Text stays visible.",0,208,440,752,24,0);
 multiplier_hint=checks[MULTI_HIT];
 page_control(0,L"EDIT",L"2",WS_BORDER|ES_NUMBER|WS_TABSTOP,872,244,88,36,201);
 page_control(0,L"EDIT",L"3",WS_BORDER|ES_NUMBER|WS_TABSTOP,872,418,88,36,202);
 const wchar_t *event_labels[]={L"No Cooldown (R)",L"Unlimited Energy (R)",L"Score Multiplier"};
 for(int i=0;i<3;i++){event_checks[i]=page_control(1,L"BUTTON",event_labels[i],BS_OWNERDRAW|WS_TABSTOP,208,180+58*i,i==2?656:752,48,300+i);EnableWindow(event_checks[i],FALSE);}
 event_hint=event_checks[2];
 page_control(1,L"EDIT",L"2",WS_BORDER|ES_NUMBER|WS_TABSTOP,872,302,88,36,203);
 page_control(1,L"STATIC",L"Higher score factors reach the target sooner. Start low and increase.",0,208,424,752,24,0);
 page_control(1,L"STATIC",L"Event settings are independent from Player.",0,208,384,752,24,0);
 const wchar_t *daily_labels[]={L"Tier-up Trial: 1 battle",L"Ascension: 1 run",L"Upgrade Disc once",L"Upgrade Trekker once",L"Battle stages: 5 clears",L"Spend 100 Vigor",L"Interact with home Trekker",L"Claim Tyrant's Tower rewards",L"Send 1 Heartlink gift",L"Dispatch 1 commission",L"Shop daily gift (skipped)",L"Daily login"};
 for(int i=0;i<12;i++)page_control(3,L"BUTTON",daily_labels[i],BS_OWNERDRAW|WS_TABSTOP,208,180+25*i,416,24,600+i);
 EnableWindow(GetDlgItem(window,610),FALSE);
 page_control(3,L"STATIC",L"PARTIAL INTEGRATION\r\n\r\nTower reward request available.\r\nShop is skipped.\r\nOther 10 tasks require setup.\r\n\r\nOne attempt per game session.\r\nF1 restores the hidden tool.",0,648,180,312,170,0);
 daily_report=page_control(3,L"BUTTON",L"Copy Lua name report",BS_OWNERDRAW|WS_TABSTOP,648,368,312,44,430);
 page_control(3,L"BUTTON",L"Copy API metadata",BS_OWNERDRAW|WS_TABSTOP,648,420,312,32,431);
 page_control(3,L"BUTTON",L"Tower reward / status",BS_OWNERDRAW|WS_TABSTOP,648,460,312,32,432);
 control(L"BUTTON",L"Disable all",BS_OWNERDRAW|WS_TABSTOP,16,440,146,48,200);
 status_label=control(L"STATIC",L"Connecting to game...",0,208,516,752,24,0);
 footer=control(L"STATIC",L"",0,208,552,752,24,0);
 for(int i=6;i<=8;i++)RegisterHotKey(window,i,MOD_NOREPEAT,VK_F6+i-6);
 global_hotkey=RegisterHotKey(window,1,MOD_NOREPEAT,VK_F1)!=0;
 switch_page(0);refresh_background();SetTimer(window,1,global_hotkey?500:50,NULL);ShowWindow(window,SW_SHOWNOACTIVATE);
 HANDLE thread=CreateThread(NULL,0,initialize,NULL,0,NULL);if(thread)CloseHandle(thread);else InterlockedExchange(&init_state,-1);
 MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
 (void)reserved;if(reason==DLL_PROCESS_ATTACH){self_module=module;HANDLE thread=CreateThread(NULL,0,run,NULL,0,NULL);if(thread)CloseHandle(thread);}return TRUE;
}
