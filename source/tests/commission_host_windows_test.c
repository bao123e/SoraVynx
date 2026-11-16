/* Windows host protocol/pinning regression; no game, capture or desktop input. */
#include <assert.h>
#include "../commission_auto.h"
int main(void){
 commission_code("READY");assert(!commission_auto.terminal);
 commission_code("HOLD_MOUSE");assert(commission_auto.mouse_held);commission_code("UP_MOUSE");assert(!commission_auto.mouse_held);
 commission_code("HOLD_ESCAPE");assert(commission_auto.escape_held);commission_code("UP_ESCAPE");assert(!commission_auto.escape_held);
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
 puts("PASS OCR host: enumerated protocol only, held-input tracking, terminal outcome, rejection of unknown output, exact script size/content pinning");return 0;
}
