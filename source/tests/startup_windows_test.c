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
 assert(feature_hotkey_command(21)==100+GOD_MODE);
 assert(feature_hotkey_command(25)==100+MOB_VACUUM);
 assert(feature_hotkey_command(29)==437&&feature_hotkey_command(31)==200);
 assert(feature_hotkey_command(19)==0&&feature_hotkey_command(32)==0);
 /* Custom combinations round-trip through native hotkey controls and one INI value. */
 struct SvKey keys[SV_KEY_COUNT],loaded[SV_KEY_COUNT];sv_key_defaults(keys);
 assert(sv_key_validate(keys)==-1);keys[2]=keys[1];assert(sv_key_validate(keys)==2);sv_key_defaults(keys);
 keys[0].key=0;assert(sv_key_validate(keys)==0);sv_key_defaults(keys);
 keys[1].key=VK_F24;keys[1].mod=MOD_CONTROL|MOD_SHIFT;
 struct SvKey decoded=sv_key_from_word(sv_key_word(keys[1]));assert(decoded.key==VK_F24&&decoded.mod==(MOD_CONTROL|MOD_SHIFT));
 INITCOMMONCONTROLSEX common={sizeof(common),ICC_HOTKEY_CLASS};assert(InitCommonControlsEx(&common));
 HWND field=CreateWindowExW(0,HOTKEY_CLASSW,L"",WS_CHILD,0,0,200,30,window,NULL,NULL,NULL);assert(field);
 SendMessageW(field,HKM_SETHOTKEY,sv_key_word(keys[1]),0);decoded=sv_key_from_word((WORD)SendMessageW(field,HKM_GETHOTKEY,0,0));assert(decoded.key==keys[1].key&&decoded.mod==keys[1].mod);
 wchar_t temporary[MAX_PATH],config[MAX_PATH];assert(GetTempPathW(MAX_PATH,temporary));assert(GetTempFileNameW(temporary,L"svk",0,config));
 assert(sv_key_save(config,keys)&&sv_key_load(config,loaded)&&!memcmp(keys,loaded,sizeof(keys)));
 /* An external registration blocks Save, restores old choices, and does not change disk. */
 memcpy(sv_keys,keys,sizeof(keys));struct SvKey pending[SV_KEY_COUNT];memcpy(pending,keys,sizeof(pending));pending[1].mod=MOD_ALT|MOD_CONTROL|MOD_SHIFT;
 assert(RegisterHotKey(window,777,pending[1].mod|MOD_NOREPEAT,pending[1].key));
 assert(sv_key_apply(window,config,pending)==1);assert(!memcmp(sv_keys,keys,sizeof(keys)));assert(sv_key_load(config,loaded)&&!memcmp(keys,loaded,sizeof(keys)));UnregisterHotKey(window,777);
 assert(sv_key_apply(window,config,pending)==-1);assert(sv_key_load(config,loaded)&&!memcmp(pending,loaded,sizeof(pending)));DeleteFileW(config);DestroyWindow(field);
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
