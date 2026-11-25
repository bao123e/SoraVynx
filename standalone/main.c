/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Standalone UI extracted from SoraVynx v1.3.5-custom-keys-preview.1.
 * This executable owns its window and local state; there is no game backend.
 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef UNICODE
#define UNICODE 1
#endif
#ifndef _UNICODE
#define _UNICODE 1
#endif
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "ui_layout.h"
#include "ui_background.h"
#include "process_monitor.h"

static bool shutting_down, test_mode;
#include "keybinds.h"

#define APP_CLASS L"WuwaWorkbenchStandalone"
#define APP_TITLE L"Wuwa Workbench - Process Monitor"
#define TRAY_MESSAGE (WM_APP+7)
enum { TAB_OVERVIEW=101, TAB_APPEARANCE, EDIT_KEYS, SHOW_TOP, SHOW_CLOCK,
 SHOW_STATUS, START_PAUSE, RESET_TIMER, MARK_TIMER, PICK_IMAGE, CLEAR_IMAGE,
 SLIDER_IMAGE, SLIDER_PANEL, SLIDER_BUTTON };
enum { TAB_TOOLS=160, SELECT_GAME, CLEAR_GAME, RESCAN_GAME, GAME_PATH };
static HINSTANCE instance;
static HWND window, overview[12], appearance[16], clock_label, status_label;
static HWND game_controls[16],process_state,process_pid,process_version,process_note;
static int game_count;
static wchar_t game_executable[MONITOR_PATH_CAP],game_version[128]=L"Not provided";
static int overview_count, appearance_count, page;
static HFONT normal_font, title_font, number_font;
static HBRUSH brush;
static bool topmost, show_clock=true, show_status=true, timer_running;
static ULONGLONG timer_anchor, elapsed_ms;
static wchar_t last_action[256]=L"Ready. Local UI only.";
static NOTIFYICONDATAW tray;
static bool tray_added;
static UINT taskbar_created;

static COLORREF bg_color(void){return RGB(16,24,40);}
static COLORREF text_color(void){return RGB(238,242,255);}
static void note(const wchar_t *text){wcsncpy(last_action,text,255);last_action[255]=0;}
static ULONGLONG timer_value(void){return elapsed_ms+(timer_running?GetTickCount64()-timer_anchor:0);}
static void timer_pause(void){if(timer_running){elapsed_ms=timer_value();timer_running=false;}}
static void timer_start(void){if(!timer_running){timer_anchor=GetTickCount64();timer_running=true;}}
static void timer_reset(void){timer_running=false;elapsed_ms=0;}
static void timer_text(wchar_t *out,size_t cap,ULONGLONG value){
 swprintf(out,cap,L"%02llu:%02llu:%02llu.%01llu",value/3600000,(value/60000)%60,(value/1000)%60,(value/100)%10);
}
static bool save_settings(void){
 return WritePrivateProfileStringW(L"Window",L"AlwaysOnTop",topmost?L"1":L"0",ui_config_path)&&
  WritePrivateProfileStringW(L"Window",L"ShowClock",show_clock?L"1":L"0",ui_config_path)&&
  WritePrivateProfileStringW(L"Window",L"ShowStatus",show_status?L"1":L"0",ui_config_path);
}
static void load_settings(void){
 topmost=GetPrivateProfileIntW(L"Window",L"AlwaysOnTop",0,ui_config_path)!=0;
 show_clock=GetPrivateProfileIntW(L"Window",L"ShowClock",1,ui_config_path)!=0;
 show_status=GetPrivateProfileIntW(L"Window",L"ShowStatus",1,ui_config_path)!=0;
 sv_key_defaults(sv_keys);
 struct SvKey pending[SV_KEY_COUNT];
 if(sv_key_load(ui_config_path,pending))memcpy(sv_keys,pending,sizeof(pending));
 GetPrivateProfileStringW(L"Game",L"Executable",L"",game_executable,MONITOR_PATH_CAP,ui_config_path);
}
static bool config_path(wchar_t *out,size_t cap){
 wchar_t base[MAX_PATH];
 if(SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,NULL,SHGFP_TYPE_CURRENT,base)!=S_OK)return false;
 if(swprintf(out,cap,L"%ls\\WuwaWorkbench",base)<0)return false;
 if(!CreateDirectoryW(out,NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)return false;
 size_t used=wcslen(out);if(used+12>=cap)return false;
 wcscpy(out+used,L"\\config.ini");return true;
}
static HWND control(const wchar_t *kind,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id){
 HWND c=CreateWindowExW(0,kind,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,(HMENU)(INT_PTR)id,instance,NULL);
 if(c)SendMessageW(c,WM_SETFONT,(WPARAM)normal_font,TRUE);return c;
}
static HWND label(const wchar_t *text,int x,int y,int w,int h){return control(L"STATIC",text,SS_LEFT,x,y,w,h,0);}
static HWND button(const wchar_t *text,int x,int y,int w,int id){return control(L"BUTTON",text,BS_OWNERDRAW|WS_TABSTOP,x,y,w,36,id);}
static void add_overview(HWND c){overview[overview_count++]=c;}
static void add_appearance(HWND c){appearance[appearance_count++]=c;}
static void add_game(HWND c){game_controls[game_count++]=c;}
static void switch_page(int next){
 page=next;
 for(int i=0;i<game_count;i++)ShowWindow(game_controls[i],next==0?SW_SHOW:SW_HIDE);
 for(int i=0;i<overview_count;i++)ShowWindow(overview[i],next==2?SW_SHOW:SW_HIDE);
 for(int i=0;i<appearance_count;i++)ShowWindow(appearance[i],next==1?SW_SHOW:SW_HIDE);
 if(next==2){ShowWindow(clock_label,show_clock?SW_SHOW:SW_HIDE);ShowWindow(status_label,show_status?SW_SHOW:SW_HIDE);}
 if(next==0)ShowWindow(process_note,show_status?SW_SHOW:SW_HIDE);
 InvalidateRect(window,NULL,TRUE);
}
static void refresh_process_view(void){
 struct ProcessProbe result=monitor_result();wchar_t text[256];const wchar_t *state;
 switch(result.status){
 case MON_SELECT:state=L"Choose your game executable to begin.";break;
 case MON_CHECKING:state=L"Checking the selected executable...";break;
 case MON_STOPPED:state=L"Selected executable is not running.";break;
 case MON_RUNNING:state=L"Running process found - full path verified.";break;
 case MON_RESTRICTED:state=L"Matching filename found; path verification is restricted.";break;
 default:state=L"Process scan unavailable. Use Rescan to retry.";break;
 }
 SetWindowTextW(process_state,state);
 if(result.status==MON_RUNNING)swprintf(text,256,L"PID: %lu   |   Instances: %lu",(unsigned long)result.pid,(unsigned long)result.count);
 else if(result.error)swprintf(text,256,L"PID: unavailable   |   Windows error: %lu",(unsigned long)result.error);
 else wcscpy(text,L"PID: --");SetWindowTextW(process_pid,text);
 swprintf(text,256,L"EXE file version: %ls",game_version);SetWindowTextW(process_version,text);
 SetWindowTextW(process_note,last_action);
}
static void refresh(void){
 if(!window)return;
 wchar_t value[128];timer_text(value,128,timer_value());SetWindowTextW(GetDlgItem(window,450),value);
 SYSTEMTIME now;GetLocalTime(&now);swprintf(value,128,L"Local time  %02u:%02u:%02u",now.wHour,now.wMinute,now.wSecond);SetWindowTextW(clock_label,value);
 SetWindowTextW(status_label,last_action);
 SetWindowTextW(GetDlgItem(window,START_PAUSE),timer_running?L"Pause timer":L"Start timer");
 SetWindowTextW(GetDlgItem(window,SHOW_TOP),topmost?L"Always on top: ON":L"Always on top: OFF");
 SetWindowTextW(GetDlgItem(window,SHOW_CLOCK),show_clock?L"Clock: ON":L"Clock: OFF");
 SetWindowTextW(GetDlgItem(window,SHOW_STATUS),show_status?L"Status: ON":L"Status: OFF");
 refresh_process_view();
}
static bool save_game_selection(const wchar_t *input){
 wchar_t canonical[MONITOR_PATH_CAP]=L"";
 if(input[0]&&!executable_path(input,canonical)){note(L"Choose an existing .exe file.");return false;}
 if(!WritePrivateProfileStringW(L"Game",L"Executable",canonical,ui_config_path)){note(L"Could not save the executable selection. Previous selection kept.");return false;}
 wcscpy(game_executable,canonical);wcscpy(game_version,L"Not provided");
 if(canonical[0])executable_version(canonical,game_version,128);
 SetWindowTextW(GetDlgItem(window,GAME_PATH),game_executable);monitor_target(game_executable);
 note(canonical[0]?L"Selection saved. Process discovery does not enable gameplay features.":L"Selection cleared.");refresh();return true;
}
static void choose_game_executable(void){
 wchar_t path[MONITOR_PATH_CAP]=L"";OPENFILENAMEW dialog={0};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
 dialog.lpstrTitle=L"Choose the WuWa game executable (not the launcher)";dialog.lpstrFilter=L"Windows executable (*.exe)\0*.exe\0\0";
 dialog.lpstrFile=path;dialog.nMaxFile=MONITOR_PATH_CAP;dialog.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(GetOpenFileNameW(&dialog))save_game_selection(path);
 else if(CommDlgExtendedError())note(L"The executable picker could not be opened.");
 refresh();
}
/* Only the restore shortcut is global. Other bindings are reserved while this
 * app has focus, so unrelated applications retain their keyboard shortcuts. */
static void register_active_keys(void){
 sv_key_unregister(window);if(shutting_down||test_mode||sv_keys_editing)return;
 if(GetForegroundWindow()==window){
  int bad=sv_key_register(window);
  if(bad>=0){wchar_t text[256];swprintf(text,256,L"Shortcut unavailable: %ls. Open Keybinds to change it.",sv_key_labels[bad]);note(text);}
 }else if(sv_keys[0].key&&RegisterHotKey(window,SV_KEY_FIRST,sv_keys[0].mod|MOD_NOREPEAT,sv_keys[0].key))sv_key_registered=1;
}
static void toggle_window(void){
 if(IsWindowVisible(window)){ShowWindow(window,SW_HIDE);note(L"Window hidden. Restore with its shortcut or tray icon.");}
 else{ShowWindow(window,SW_RESTORE);SetForegroundWindow(window);}
 register_active_keys();
}
static void apply_topmost(void){SetWindowPos(window,topmost?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);}
static void do_action(int action){
 switch(action){
 case 0:toggle_window();break;
 case 1:switch_page(0);note(L"Overview selected.");break;
 case 2:switch_page(1);note(L"Appearance selected.");break;
 case 3:topmost=!topmost;apply_topmost();note(save_settings()?L"Window preference saved.":L"Could not save window preference.");break;
 case 4:show_clock=!show_clock;switch_page(page);note(save_settings()?L"Clock preference saved.":L"Could not save clock preference.");break;
 case 5:show_status=!show_status;switch_page(page);note(save_settings()?L"Status preference saved.":L"Could not save status preference.");break;
 case 6:timer_start();note(L"Local timer started.");break;
 case 7:timer_pause();note(L"Local timer paused.");break;
 case 8:timer_reset();note(L"Local timer reset.");break;
 case 9:if(timer_running)timer_pause();else timer_start();note(timer_running?L"Local timer started.":L"Local timer paused.");break;
 case 10:{wchar_t value[80];timer_text(value,80,timer_value());swprintf(last_action,256,L"Timer mark: %ls",value);break;}
 case 11:timer_reset();note(L"Local timer stopped and reset.");break;
 }
 refresh();
}
static void tray_add(void){
 memset(&tray,0,sizeof(tray));tray.cbSize=sizeof(tray);tray.hWnd=window;tray.uID=1;
 tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=TRAY_MESSAGE;
 tray.hIcon=LoadIconW(NULL,MAKEINTRESOURCEW(32512));wcscpy(tray.szTip,L"Wuwa Workbench - Standalone UI");
 tray_added=Shell_NotifyIconW(NIM_ADD,&tray)!=FALSE;
}
static void create_controls(void){
 HWND title=label(L"WUWA WORKBENCH",28,24,650,42);SendMessageW(title,WM_SETFONT,(WPARAM)title_font,TRUE);
 label(L"Process monitor preview  |  Gameplay integration: not implemented",30,72,880,26);
 button(L"Overview",28,115,160,TAB_OVERVIEW);button(L"Appearance",200,115,160,TAB_APPEARANCE);button(L"Keybinds",372,115,160,EDIT_KEYS);
 button(L"Local tools",544,115,160,TAB_TOOLS);
 add_game(label(L"GAME PROCESS",48,194,800,28));
 process_state=label(L"",48,234,850,60);add_game(process_state);
 add_game(button(L"Select game EXE",48,304,230,SELECT_GAME));add_game(button(L"Rescan",294,304,180,RESCAN_GAME));add_game(button(L"Clear selection",490,304,210,CLEAR_GAME));
 process_pid=label(L"PID: --",48,366,330,30);add_game(process_pid);
 process_version=label(L"",400,366,470,30);add_game(process_version);
 add_game(label(L"Selected executable (select the game binary, not its launcher):",48,410,830,26));
 add_game(control(L"EDIT",game_executable,ES_READONLY|ES_AUTOHSCROLL|WS_BORDER|WS_TABSTOP,48,442,820,30,GAME_PATH));
 process_note=label(last_action,48,488,820,40);add_game(process_note);
 add_overview(label(L"Local timer",48,194,800,28));
 HWND digits=control(L"STATIC",L"00:00:00.0",SS_LEFT,44,230,810,70,450);SendMessageW(digits,WM_SETFONT,(WPARAM)number_font,TRUE);add_overview(digits);
 add_overview(button(L"Start timer",48,328,180,START_PAUSE));add_overview(button(L"Reset",244,328,180,RESET_TIMER));add_overview(button(L"Mark",440,328,180,MARK_TIMER));
 clock_label=label(L"",48,402,800,30);add_overview(clock_label);
 status_label=label(last_action,48,448,800,66);add_overview(status_label);
 add_appearance(button(L"Choose background",48,190,220,PICK_IMAGE));add_appearance(button(L"Clear background",284,190,220,CLEAR_IMAGE));
 const wchar_t *names[]={L"Background opacity",L"Panel opacity",L"Button opacity"};int vals[]={background_opacity,panel_opacity,button_opacity};
 for(int i=0;i<3;i++){
  add_appearance(label(names[i],48,248+64*i,220,28));
  HWND track=control(TRACKBAR_CLASSW,L"",TBS_HORZ|WS_TABSTOP,284,242+64*i,470,36,SLIDER_IMAGE+i);
  SendMessageW(track,TBM_SETRANGE,TRUE,MAKELONG(0,100));SendMessageW(track,TBM_SETPOS,TRUE,vals[i]);add_appearance(track);
 }
 add_appearance(button(L"Always on top",48,452,220,SHOW_TOP));add_appearance(button(L"Clock",284,452,220,SHOW_CLOCK));add_appearance(button(L"Status",520,452,220,SHOW_STATUS));
 label(L"F1 restores the window by default. Other shortcuts work while this app has focus.",28,566,900,26);
 switch_page(0);refresh();
}
static void paint_window(HWND hwnd){
 PAINTSTRUCT ps;HDC target=BeginPaint(hwnd,&ps);RECT client;GetClientRect(hwnd,&client);
 HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,client.right,client.bottom);
 if(!dc||!bitmap){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);FillRect(target,&client,brush);EndPaint(hwnd,&ps);return;}
 HGDIOBJ old=SelectObject(dc,bitmap);FillRect(dc,&client,brush);
 if(custom_image){BITMAP source;GetObjectW(custom_image,sizeof(source),&source);HDC image=CreateCompatibleDC(dc);
  if(image){HGDIOBJ before=SelectObject(image,custom_image);struct ImageFit fit=fit_image(source.bmWidth,source.bmHeight,0,0,client.right,client.bottom);
   BLENDFUNCTION blend={AC_SRC_OVER,0,(BYTE)(background_opacity*255/100),0};
   AlphaBlend(dc,fit.x,fit.y,fit.width,fit.height,image,0,0,source.bmWidth,source.bmHeight,blend);SelectObject(image,before);DeleteDC(image);}
 }
 RECT panel={28,174,client.right-28,538};tint_rect(dc,panel,RGB(40,56,82),(BYTE)(panel_opacity*255/100));
 BitBlt(target,0,0,client.right,client.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);EndPaint(hwnd,&ps);
}
static void draw_button(const DRAWITEMSTRUCT *d){
 COLORREF color=(d->itemState&ODS_SELECTED)?RGB(47,89,140):RGB(32+button_opacity/3,49+button_opacity/3,73+button_opacity/3);
 HBRUSH fill=CreateSolidBrush(color);FillRect(d->hDC,&d->rcItem,fill);DeleteObject(fill);
 wchar_t text[160];GetWindowTextW(d->hwndItem,text,160);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,text_color());SelectObject(d->hDC,normal_font);
 RECT r=d->rcItem;DrawTextW(d->hDC,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 if(d->itemState&ODS_FOCUS){InflateRect(&r,-4,-4);DrawFocusRect(d->hDC,&r);}
}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 if(taskbar_created&&msg==taskbar_created){if(!test_mode)tray_add();return 0;}
 switch(msg){
 case WM_CREATE:window=hwnd;create_controls();SetTimer(hwnd,1,200,NULL);SetTimer(hwnd,2,2000,NULL);return 0;
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:paint_window(hwnd);return 0;
 case WM_CTLCOLORSTATIC:SetBkMode((HDC)wp,OPAQUE);SetBkColor((HDC)wp,bg_color());SetTextColor((HDC)wp,text_color());return (LRESULT)brush;
 case WM_DRAWITEM:draw_button((DRAWITEMSTRUCT*)lp);return TRUE;
 case WM_ACTIVATE:if(!sv_keys_editing)register_active_keys();return 0;
 case WM_TIMER:if(wp==2)monitor_refresh();refresh();return 0;
 case WM_HOTKEY:
  if(!sv_keys_editing&&(test_mode||wp==SV_KEY_FIRST||GetForegroundWindow()==hwnd))do_action((int)wp-SV_KEY_FIRST);
  return 0;
 case WM_HSCROLL:{
  int id=GetDlgCtrlID((HWND)lp);int value=(int)SendMessageW((HWND)lp,TBM_GETPOS,0,0);
  if(id==SLIDER_IMAGE)background_opacity=value;else if(id==SLIDER_PANEL)panel_opacity=value;else if(id==SLIDER_BUTTON)button_opacity=value;
  if(LOWORD(wp)==TB_ENDTRACK){save_background_settings(hwnd);note(L"Appearance saved.");}
  InvalidateRect(hwnd,NULL,TRUE);return 0;
 }
 case WM_COMMAND:switch(LOWORD(wp)){
  case TAB_OVERVIEW:do_action(1);break;case TAB_APPEARANCE:do_action(2);break;
  case TAB_TOOLS:switch_page(2);note(L"Local tools selected.");refresh();break;
  case SELECT_GAME:choose_game_executable();break;
  case CLEAR_GAME:save_game_selection(L"");break;
  case RESCAN_GAME:monitor_refresh();note(L"Rescan requested.");refresh();break;
  case EDIT_KEYS:sv_key_show(hwnd,instance,ui_config_path);register_active_keys();note(L"Shortcut editor closed.");refresh();break;
  case SHOW_TOP:do_action(3);break;case SHOW_CLOCK:do_action(4);break;case SHOW_STATUS:do_action(5);break;
  case START_PAUSE:do_action(9);break;case RESET_TIMER:do_action(8);break;case MARK_TIMER:do_action(10);break;
  case PICK_IMAGE:if(choose_background(hwnd)){note(L"Background saved.");InvalidateRect(hwnd,NULL,TRUE);}break;
  case CLEAR_IMAGE:if(custom_image){DeleteObject(custom_image);custom_image=NULL;}background_path[0]=0;save_background_settings(hwnd);InvalidateRect(hwnd,NULL,TRUE);break;
  }return 0;
 case TRAY_MESSAGE:
  if(lp==WM_LBUTTONDBLCLK){ShowWindow(hwnd,SW_RESTORE);SetForegroundWindow(hwnd);register_active_keys();}
  if(lp==WM_RBUTTONUP){HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"Show window");AppendMenuW(menu,MF_STRING,2,L"Exit");POINT p;GetCursorPos(&p);SetForegroundWindow(hwnd);
   int id=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,hwnd,NULL);DestroyMenu(menu);
   if(id==1){ShowWindow(hwnd,SW_RESTORE);SetForegroundWindow(hwnd);}if(id==2)DestroyWindow(hwnd);
  }return 0;
 case WM_CLOSE:DestroyWindow(hwnd);return 0;
 case WM_DESTROY:shutting_down=true;KillTimer(hwnd,1);KillTimer(hwnd,2);monitor_shutdown();sv_key_unregister(hwnd);if(tray_added)Shell_NotifyIconW(NIM_DELETE,&tray);PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(hwnd,msg,wp,lp);
}

static int self_test(const wchar_t *report_path){
 FILE *out=_wfopen(report_path,L"wb");if(!out)return 2;int failures=0;
#define CHECK(condition,label) do{bool ok=(condition);fprintf(out,"%s %s\n",ok?"PASS":"FAIL",label);if(!ok)failures++;}while(0)
 struct SvKey defaults[SV_KEY_COUNT],pending[SV_KEY_COUNT],loaded[SV_KEY_COUNT];sv_key_defaults(defaults);
 CHECK(sv_key_validate(defaults)==-1,"default keybindings are valid");
 memcpy(pending,defaults,sizeof(pending));pending[1]=pending[0];CHECK(sv_key_validate(pending)==1,"duplicate bindings rejected");
 memcpy(pending,defaults,sizeof(pending));pending[0].key=0;CHECK(sv_key_validate(pending)==0,"restore binding cannot be empty");
 CHECK(sv_key_save(ui_config_path,defaults),"save keybindings");
 CHECK(sv_key_load(ui_config_path,loaded)&&memcmp(defaults,loaded,sizeof(defaults))==0,"reload keybindings without changes");
 WritePrivateProfileStringW(L"Hotkeys",L"Bindings",L"112:0,broken",ui_config_path);
 CHECK(!sv_key_load(ui_config_path,loaded),"malformed stored bindings rejected");
 CHECK(memcmp(loaded,defaults,sizeof(defaults))==0,"malformed configuration does not partially overwrite bindings");sv_key_save(ui_config_path,defaults);
 HWND blocker=CreateWindowW(L"STATIC",L"test",0,0,0,0,0,HWND_MESSAGE,NULL,instance,NULL);
 bool reserved=RegisterHotKey(blocker,123,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F24)!=0;
 CHECK(reserved,"reserve a shortcut in another window");
 if(reserved){
  memcpy(pending,defaults,sizeof(pending));pending[0].key=VK_F24;pending[0].mod=MOD_CONTROL|MOD_ALT;
  CHECK(sv_key_apply(window,ui_config_path,pending)==0,"OS shortcut conflict rejects save");
  CHECK(memcmp(sv_keys,defaults,sizeof(defaults))==0,"conflict preserves in-memory bindings");
  CHECK(sv_key_load(ui_config_path,loaded)&&memcmp(loaded,defaults,sizeof(defaults))==0,"conflict preserves saved bindings");
  UnregisterHotKey(blocker,123);
 }
 if(blocker)DestroyWindow(blocker);
 wchar_t impossible[32768];swprintf(impossible,32768,L"%ls.missing\\config.ini",ui_config_path);
 memcpy(pending,defaults,sizeof(pending));for(int i=0;i<SV_KEY_COUNT;i++){pending[i].key=VK_F13+i;pending[i].mod=MOD_CONTROL|MOD_ALT|MOD_SHIFT;}
 CHECK(sv_key_apply(window,impossible,pending)==SV_KEY_COUNT,"unwritable configuration rejects save");
 CHECK(memcmp(sv_keys,defaults,sizeof(defaults))==0,"save failure preserves in-memory bindings");
 CHECK(sv_key_load(ui_config_path,loaded)&&memcmp(loaded,defaults,sizeof(defaults))==0,"save failure preserves existing configuration");
 SendMessageW(window,WM_COMMAND,TAB_APPEARANCE,0);CHECK(page==1&&(GetWindowLongW(appearance[0],GWL_STYLE)&WS_VISIBLE)&&!(GetWindowLongW(overview[0],GWL_STYLE)&WS_VISIBLE),"appearance page selection");
 SendMessageW(window,WM_COMMAND,TAB_OVERVIEW,0);CHECK(page==0,"overview page selection");
 SendMessageW(window,WM_HOTKEY,SV_KEY_FIRST+6,0);CHECK(timer_running,"hotkey starts timer");
 timer_anchor-=1250;SendMessageW(window,WM_HOTKEY,SV_KEY_FIRST+7,0);CHECK(!timer_running&&elapsed_ms>=1250,"hotkey pauses and retains elapsed time");
 SendMessageW(window,WM_COMMAND,MARK_TIMER,0);CHECK(wcsstr(last_action,L"Timer mark:")!=NULL,"button marks elapsed time");
 SendMessageW(window,WM_COMMAND,RESET_TIMER,0);CHECK(!timer_running&&elapsed_ms==0,"button resets timer");
 SendMessageW(window,WM_HOTKEY,SV_KEY_FIRST+4,0);CHECK(!show_clock,"clock toggle updates local state");
 show_clock=true;load_settings();CHECK(!show_clock,"window preferences persist");
 WritePrivateProfileStringW(L"Appearance",L"Opacity",L"999",ui_config_path);
 if(image_decoder){GdiplusShutdown(image_decoder);image_decoder=0;}load_background_settings(ui_config_path);CHECK(background_opacity==35,"invalid opacity falls back to default");
 CHECK(GetModuleHandleW(L"GameAssembly.dll")==NULL,"self-test does not load Unity runtime");
 wchar_t own[MONITOR_PATH_CAP],canonical[MONITOR_PATH_CAP],stored[MONITOR_PATH_CAP];GetModuleFileNameW(NULL,own,MONITOR_PATH_CAP);
 CHECK(executable_path(own,canonical),"existing executable accepted");
 CHECK(!executable_path(L"C:\\missing-workbench-file.exe",canonical),"missing executable rejected");
 CHECK(!executable_path(ui_config_path,canonical),"non-executable file rejected");
 CHECK(probe_process_path(L"").status==MON_SELECT,"empty process selection stays unconfigured");
 executable_path(own,canonical);struct ProcessProbe detected=probe_process_path(canonical);
 CHECK(detected.status==MON_RUNNING&&detected.count>=1&&detected.pid==GetCurrentProcessId(),"exact path discovers the current real process");
 wchar_t wrong[MONITOR_PATH_CAP];swprintf(wrong,MONITOR_PATH_CAP,L"C:\\Different-Wuwa-Folder\\%ls",path_filename(canonical));
 CHECK(probe_process_path(wrong).status==MON_STOPPED,"same filename with a different path is not accepted");
 CHECK(save_game_selection(own),"executable selection saved");
 GetPrivateProfileStringW(L"Game",L"Executable",L"",stored,MONITOR_PATH_CAP,ui_config_path);
 CHECK(!_wcsicmp(stored,game_executable),"executable selection roundtrips through configuration");
 CHECK(!save_game_selection(ui_config_path)&&!_wcsicmp(stored,game_executable),"invalid selection preserves the previous executable");
 ULONGLONG deadline=GetTickCount64()+5000;
 while(monitor_result().status==MON_CHECKING&&GetTickCount64()<deadline)Sleep(10);
 CHECK(monitor_result().status==MON_RUNNING,"background scanner publishes a verified result");
 SendMessageW(window,WM_COMMAND,TAB_TOOLS,0);CHECK(page==2&&(GetWindowLongW(overview[0],GWL_STYLE)&WS_VISIBLE),"local tools has its own page");
 SendMessageW(window,WM_COMMAND,TAB_OVERVIEW,0);CHECK(page==0&&(GetWindowLongW(game_controls[0],GWL_STYLE)&WS_VISIBLE)&&!(GetWindowLongW(overview[0],GWL_STYLE)&WS_VISIBLE),"overview shows process monitor instead of local timer");
 CHECK(save_game_selection(L""),"executable selection can be cleared");
 deadline=GetTickCount64()+5000;while(monitor_result().status==MON_CHECKING&&GetTickCount64()<deadline)Sleep(10);
 CHECK(monitor_result().status==MON_SELECT,"cleared target does not retain a running PID");
 executable_path(own,canonical);monitor_target(canonical);monitor_target(L"");
 deadline=GetTickCount64()+5000;while(monitor_busy()&&GetTickCount64()<deadline)Sleep(10);
 CHECK(!monitor_busy()&&monitor_result().status==MON_SELECT&&monitor_result().pid==0,"rapid target changes cannot publish a stale PID");
 /* A benign copy of this executable exercises actual start/exit discovery.
  * The helper opens no game and uses a parent-owned event to exit. */
 wchar_t helper[MONITOR_PATH_CAP],stop_name[128],ready_name[128],command[MONITOR_PATH_CAP];
 swprintf(helper,MONITOR_PATH_CAP,L"%ls.helper.exe",ui_config_path);
 bool copied=CopyFileW(own,helper,FALSE)!=FALSE;CHECK(copied,"create isolated lifecycle test executable");
 if(copied){
  executable_path(helper,canonical);CHECK(probe_process_path(canonical).status==MON_STOPPED,"helper is absent before launch");
  swprintf(stop_name,128,L"Local\\WuwaWorkbenchStop_%lu",(unsigned long)GetCurrentProcessId());
  swprintf(ready_name,128,L"Local\\WuwaWorkbenchReady_%lu",(unsigned long)GetCurrentProcessId());
  HANDLE stop=CreateEventW(NULL,TRUE,FALSE,stop_name),ready=CreateEventW(NULL,TRUE,FALSE,ready_name);
  swprintf(command,MONITOR_PATH_CAP,L"\"%ls\" --probe-helper %ls %ls",canonical,stop_name,ready_name);
  STARTUPINFOW startup={0};startup.cb=sizeof(startup);PROCESS_INFORMATION child={0};
  bool launched=stop&&ready&&CreateProcessW(canonical,command,NULL,NULL,FALSE,0,NULL,NULL,&startup,&child);
  CHECK(launched,"launch benign probe helper");
  if(launched){
   CHECK(WaitForSingleObject(ready,5000)==WAIT_OBJECT_0,"helper is ready for discovery");
   detected=probe_process_path(canonical);CHECK(detected.status==MON_RUNNING&&detected.pid==child.dwProcessId,"scanner detects started helper by full path");
   SetEvent(stop);CHECK(WaitForSingleObject(child.hProcess,20000)==WAIT_OBJECT_0,"helper exits on its stop event");
   CHECK(probe_process_path(canonical).status==MON_STOPPED,"scanner removes PID after helper exits");
   CloseHandle(child.hThread);CloseHandle(child.hProcess);
  }
  if(stop)CloseHandle(stop);if(ready)CloseHandle(ready);DeleteFileW(helper);
 }
 fprintf(out,"RESULT failures=%d\n",failures);fclose(out);return failures?1:0;
#undef CHECK
}

int WINAPI wWinMain(HINSTANCE module,HINSTANCE previous,LPWSTR command,int show){
 (void)previous;(void)command;instance=module;int argc=0;wchar_t **argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 if(argc==4&&!wcscmp(argv[1],L"--probe-helper")){
  HANDLE stop=OpenEventW(SYNCHRONIZE,FALSE,argv[2]),ready=OpenEventW(EVENT_MODIFY_STATE,FALSE,argv[3]);int code=2;
  if(stop&&ready){SetEvent(ready);code=WaitForSingleObject(stop,15000)==WAIT_OBJECT_0?0:2;}
  if(stop)CloseHandle(stop);if(ready)CloseHandle(ready);LocalFree(argv);return code;
 }
 test_mode=argc>=3&&!wcscmp(argv[1],L"--self-test");
 wchar_t path[32768],temp_dir[MAX_PATH]=L"";
 if(test_mode){wchar_t temp[MAX_PATH];if(!GetTempPathW(MAX_PATH,temp)||!GetTempFileNameW(temp,L"wwb",0,temp_dir)){if(argv)LocalFree(argv);return 2;}
  DeleteFileW(temp_dir);if(!CreateDirectoryW(temp_dir,NULL)){LocalFree(argv);return 2;}swprintf(path,32768,L"%ls\\config.ini",temp_dir);
 }else if(!config_path(path,32768)){MessageBoxW(NULL,L"Cannot create the configuration folder in LocalAppData.",APP_TITLE,MB_ICONERROR);if(argv)LocalFree(argv);return 2;}
 INITCOMMONCONTROLSEX init={sizeof(init),ICC_HOTKEY_CLASS|ICC_BAR_CLASSES};if(!InitCommonControlsEx(&init)){LocalFree(argv);return 2;}
 load_background_settings(path);load_settings();
 executable_version(game_executable,game_version,128);monitor_init(game_executable);
 normal_font=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 title_font=CreateFontW(-30,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 number_font=CreateFontW(-52,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Consolas");
 brush=CreateSolidBrush(bg_color());WNDCLASSW wc={0};wc.hInstance=module;wc.lpfnWndProc=window_proc;wc.lpszClassName=APP_CLASS;
 wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hIcon=LoadIconW(NULL,MAKEINTRESOURCEW(32512));wc.hbrBackground=brush;
 if(!RegisterClassW(&wc)){LocalFree(argv);return 2;}
 RECT bounds={0,0,960,614};DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;AdjustWindowRect(&bounds,style,FALSE);
 window=CreateWindowExW(0,APP_CLASS,APP_TITLE,style,CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,NULL,NULL,module,NULL);
 if(!window){MessageBoxW(NULL,L"Cannot create the application window.",APP_TITLE,MB_ICONERROR);LocalFree(argv);return 2;}
 int result=0;
 if(test_mode){result=self_test(argv[2]);DestroyWindow(window);}
 else{
  if(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES){sv_key_save(path,sv_keys);save_settings();}
  taskbar_created=RegisterWindowMessageW(L"TaskbarCreated");tray_add();apply_topmost();ShowWindow(window,show);UpdateWindow(window);register_active_keys();
  MSG msg;BOOL status;while((status=GetMessageW(&msg,NULL,0,0))>0){if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}if(status<0)result=2;
 }
 if(custom_image)DeleteObject(custom_image);if(image_decoder)GdiplusShutdown(image_decoder);
 DeleteObject(normal_font);DeleteObject(title_font);DeleteObject(number_font);DeleteObject(brush);
 if(temp_dir[0]){DeleteFileW(path);RemoveDirectoryW(temp_dir);}if(argv)LocalFree(argv);return result;
}
