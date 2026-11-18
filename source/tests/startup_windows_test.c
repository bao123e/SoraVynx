/* Windows-only startup report UI/clipboard regression; no game or key needed. */
#include <windows.h>
#include <assert.h>
#include <string.h>
#include <wchar.h>
static int dialogs;
static int WINAPI report_message(HWND owner,LPCWSTR text,LPCWSTR title,UINT flags){
 (void)owner;(void)title;(void)flags;
 assert(wcsstr(text,L"Compatibility report copied"));dialogs++;return IDOK;
}
#define MessageBoxW report_message
#include "../main.c"
#undef MessageBoxW
int main(void){
 window=CreateWindowExW(0,L"STATIC",L"Startup fixture",WS_OVERLAPPEDWINDOW,0,0,1000,600,NULL,NULL,NULL,NULL);assert(window);
 status_label=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,752,24,window,NULL,NULL,NULL);assert(status_label);
 HWND button=CreateWindowExW(0,L"BUTTON",L"Copy startup report",WS_CHILD,0,30,208,32,window,(HMENU)433,NULL,NULL);assert(button);
 init_state=-1;startup_code=SV_FRAME_PROFILE_MISMATCH;binary_check=1;hotfix_found=1;
 /* Incomplete setup must never advertise partial capabilities as ready. */
 capabilities[GOD_MODE]=1;event_ready[0]=1;
 refresh_status();wchar_t label[256];GetWindowTextW(status_label,label,256);
 assert(wcsstr(label,L"FRAME_PROFILE_MISMATCH"));assert(IsWindowEnabled(button));
 copy_startup_report(window);assert(dialogs==1);assert(OpenClipboard(window));
 HGLOBAL memory=GetClipboardData(CF_TEXT);assert(memory);const char *text=GlobalLock(memory);assert(text);
 assert(strstr(text,"Startup: FRAME_PROFILE_MISMATCH"));assert(strstr(text,"allowlist: matched"));
 assert(strstr(text,"Player signatures ready: 0/5"));assert(strstr(text,"Event signatures ready: 0/3"));
 assert(!strstr(text,"issuer-private")&&!strstr(text,"C:\\"));
 GlobalUnlock(memory);CloseClipboard();
 assert(feature_hotkey_command(20)==100+GOD_MODE);
 assert(feature_hotkey_command(24)==100+MOB_VACUUM);
 assert(feature_hotkey_command(25)==437&&feature_hotkey_command(26)==200);
 assert(feature_hotkey_command(19)==0&&feature_hotkey_command(27)==0);
 assert(procedure(window,WM_NCHITTEST,0,0)==HTCLIENT);
 WNDCLASSW gameClass={0};gameClass.lpfnWndProc=DefWindowProcW;gameClass.lpszClassName=L"UnityWndClass";gameClass.hInstance=GetModuleHandleW(NULL);assert(RegisterClassW(&gameClass));
 HWND game=CreateWindowExW(0,L"UnityWndClass",L"Synthetic game",WS_POPUP|WS_VISIBLE,40,60,1600,900,NULL,NULL,gameClass.hInstance,NULL);assert(game);
 int x,y;panel_position(window,1000,600,&x,&y);assert(x==340&&y==210);
 center_panel(window);RECT rect;assert(GetWindowRect(window,&rect));assert(rect.left==340&&rect.top==210);
 SetWindowPos(game,NULL,90,100,1400,800,SWP_NOZORDER|SWP_NOACTIVATE);
 center_panel(window);assert(GetWindowRect(window,&rect));assert(rect.left==290&&rect.top==200);
 assert(!IsWindowVisible(window)); /* Positioning never shows the hidden menu. */
 DestroyWindow(game);UnregisterClassW(L"UnityWndClass",gameClass.hInstance);DestroyWindow(window);
 puts("PASS actual startup status label and clipboard report on Windows, with failed readiness suppressed");return 0;
}
