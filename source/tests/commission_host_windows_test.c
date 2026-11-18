/* Windows host protocol/pinning regression; no game, capture or desktop input. */
#include <assert.h>
#include "../commission_auto.h"
int main(void){
 commission_code("READY");assert(!commission_auto.terminal);
 commission_code("HOLD_MOUSE");assert(commission_auto.mouse_held);commission_code("UP_MOUSE");assert(!commission_auto.mouse_held);
 commission_code("HOLD_ESCAPE");assert(commission_auto.escape_held);commission_code("UP_ESCAPE");assert(!commission_auto.escape_held);
 commission_code("ACTION_CLOSE_POWER_UP");assert(wcsstr(commission_auto_message(),L"No additional upgrade"));
 commission_code("ACTION_REOPEN_UPGRADE");assert(wcsstr(commission_auto_message(),L"navigation once"));
 commission_code("ACTION_COMMIT_UPGRADE");assert(wcsstr(commission_auto_message(),L"never retried"));
 commission_code("STATE_TREKKER_EDIT");assert(wcsstr(commission_auto_message(),L"training"));
 commission_code("ACTION_INTERACT");assert(wcsstr(commission_auto_message(),L"home character"));
 commission_code("STATE_ITEMS");assert(wcsstr(commission_auto_message(),L"Items Obtained"));
 commission_code("DONE");assert(commission_auto.terminal==1);commission_code("STATE_CLAIM");assert(wcsstr(commission_auto_message(),L"not auto-verified"));
 memset(&commission_auto,0,sizeof(commission_auto));commission_code("NO_REWARDS");assert(commission_auto.terminal==2);
 memset(&commission_auto,0,sizeof(commission_auto));commission_code("ERR_OCR_LANGUAGE");assert(commission_auto.terminal==-1&&wcsstr(commission_auto_message(),L"English OCR"));
 memset(&commission_auto,0,sizeof(commission_auto));commission_code("untrusted UI/account text");assert(commission_auto.terminal==-1);
 wchar_t folder[MAX_PATH],file[MAX_PATH];assert(GetTempPathW(MAX_PATH,folder));assert(GetTempFileNameW(folder,L"svx",0,file));
 HANDLE h=CreateFileW(file,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,NULL);assert(h!=INVALID_HANDLE_VALUE);
 const char text[]="fixed local screen reader";DWORD written;assert(WriteFile(h,text,sizeof(text),&written,NULL));SetFilePointer(h,0,NULL,FILE_BEGIN);
 assert(commission_script_matches(h,text,sizeof(text)));SetFilePointer(h,0,NULL,FILE_BEGIN);
 const char wrong[sizeof(text)]="wrong script";assert(!commission_script_matches(h,wrong,sizeof(wrong)));SetFilePointer(h,0,NULL,FILE_BEGIN);assert(!commission_script_matches(h,text,sizeof(text)-1));CloseHandle(h);
 /* Launch the real pinned helper against an undersized fake Unity window.
  * View() rejects its size/focus BEFORE any capture/input. Verify job and pipe cleanup. */
 memset(&commission_auto,0,sizeof(commission_auto));
 HINSTANCE instance=GetModuleHandleW(NULL);WNDCLASSW cls={0};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=instance;cls.lpszClassName=L"UnityWndClass";assert(RegisterClassW(&cls));
 HWND fake=CreateWindowW(L"UnityWndClass",L"No-input fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,10,10,200,120,NULL,NULL,instance,NULL);assert(fake);
 HWND owner=CreateWindowW(L"STATIC",L"Host fixture",WS_OVERLAPPED,300,300,100,100,NULL,NULL,instance,NULL);assert(owner);
 assert(!commission_start(owner,instance,fake,0));
 assert(commission_start(owner,instance,fake,588));BOOL in_job=FALSE;assert(IsProcessInJob(commission_auto.process,commission_auto.job,&in_job)&&in_job);
 ULONGLONG deadline=GetTickCount64()+25000;
 while(commission_auto.running&&GetTickCount64()<deadline){MSG msg;while(PeekMessageW(&msg,NULL,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}commission_poll(owner);Sleep(20);}
 assert(!commission_auto.running&&commission_auto.terminal==-1&&!commission_auto.process&&!commission_auto.job&&!commission_auto.pipe&&!commission_auto.script);
 assert(!commission_auto.mouse_held&&!commission_auto.escape_held);assert(wcsstr(commission_auto_message(),L"ERR_CONTEXT_OR_CAPTURE"));
 DestroyWindow(owner);DestroyWindow(fake);UnregisterClassW(L"UnityWndClass",instance);
 puts("PASS actual helper launch: pinned resource, restricted inherited handles/job, enumerated child error and complete cleanup, no screenshot/input on undersized fake window");
 puts("PASS OCR host: enumerated protocol only, held-input tracking, terminal outcome, rejection of unknown output, exact script size/content pinning");return 0;
}
