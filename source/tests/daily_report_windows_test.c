/* SPDX-License-Identifier: AGPL-3.0-or-later */
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "../backend.h"
#include "../daily_setup.h"
#undef NDEBUG
#include <assert.h>
struct RuntimeAPI api;
const void *hotfix_image=(void*)1;
static int report_detaches,report_actions;
static void *report_domain(void){return (void*)1;}
static void *report_attach(void *domain){return domain;}
static void report_detach(void *thread){(void)thread;report_detaches++;}
static uintptr_t method_class(const void *method){return (uintptr_t)method>>10;}
static const void *report_method(void *klass,void **iterator){uintptr_t n=(uintptr_t)*iterator+1,max=(uintptr_t)klass==101?41:1;if(n>max)return NULL;*iterator=(void*)n;return (void*)(((uintptr_t)klass<<10)|n);}
static const char *report_method_name(const void *m){
 uintptr_t c=method_class(m),n=(uintptr_t)m&1023;
 if(c<=100)return "get_Parser";if(c==1000)return "get_Instance";
 if(c==101)return n==41?"OnClickClaimDaily":"Warmup";
 const char *names[]={"SendGift","StartCommission","Upgrade","EnterStage","ClaimRandomReward","get_Progress","OnClickClaimAll","levelEnd","Other"};return names[c-102];
}
static uint32_t report_param_count(const void *m){return method_class(m)==103?2:0;}
static const void *report_param(const void *m,uint32_t i){(void)m;return (void*)(uintptr_t)(i?5:3);}
static const void *report_return_type(const void *m){uintptr_t c=method_class(m);return (void*)(uintptr_t)(c<=100?2:c==1000?4:c==107?3:1);}
static char *report_type_name(const void *t){const char *names[]={"?","System.Void","Google.Protobuf.MessageParser<Nova.Hotfix.DailyQuest>","System.Int32","DailyMissionController","System.Boolean"};const char *value=names[(uintptr_t)t];char *copy=malloc(strlen(value)+1);strcpy(copy,value);return copy;}
static uint32_t report_flags(const void *m,uint32_t *flags){(void)m;if(flags)*flags=0;return 0x10;}
static void *report_parent(void *klass){return (uintptr_t)klass==101?(void*)1000:NULL;}
static const void *report_field_type(void *field){return (void*)(uintptr_t)(((uintptr_t)field>>4)==1000?4:3);}
static uint32_t report_field_flags(void *field){return ((uintptr_t)field>>4)==1000?0x10:0;}
static void *report_invoke(const void *m,void *o,void **args,void **error){(void)m;(void)o;(void)args;(void)error;report_actions++;return NULL;}
static BOOL CALLBACK close_test_dialog(HWND hwnd,LPARAM id) {
 wchar_t kind[32];GetClassNameW(hwnd,kind,32);
 if(!wcscmp(kind,L"#32770")){PostMessageW(hwnd,WM_COMMAND,IDOK,0);KillTimer(NULL,(UINT_PTR)id);return FALSE;}
 return TRUE;
}
static void CALLBACK close_report_box(HWND h,UINT m,UINT_PTR id,DWORD tick) {
 (void)h;(void)m;(void)tick;EnumThreadWindows(GetCurrentThreadId(),close_test_dialog,(LPARAM)id);
}
int main(void) {
 HMODULE fixture=LoadLibraryW(L"GameAssembly.dll");assert(fixture);
 HWND owner=CreateWindowW(L"STATIC",L"Report regression",WS_OVERLAPPED,0,0,100,100,NULL,NULL,GetModuleHandleW(NULL),NULL);assert(owner);
 api.domain_get=report_domain;api.thread_attach=report_attach;api.thread_detach=report_detach;
 api.class_get_methods=report_method;api.method_get_name=report_method_name;api.method_get_param_count=report_param_count;
 api.method_get_return_type=report_return_type;api.type_get_name=report_type_name;api.free=free;
 api.method_get_flags=report_flags;api.runtime_invoke=report_invoke;api.method_get_param=report_param;
 api.class_get_parent=report_parent;api.field_get_type=report_field_type;api.field_get_flags=report_field_flags;
 assert(SetTimer(NULL,105,100,close_report_box));daily_copy_setup(owner);
 assert(report_detaches==1&&report_actions==0);assert(OpenClipboard(owner));
 HANDLE data=GetClipboardData(CF_UNICODETEXT);assert(data);const wchar_t *text=GlobalLock(data);assert(text);
 const wchar_t *required[]={L"report v2",L"Candidate runtime types: 9",L"Skipped generated protocol types: 100",L"DailyMissionController",L"HeartlinkManager",L"DispatchController",L"DiscUpgradeController",L"TrialStageController",L"ShopRewardManager",L"OnClickClaimDaily",L"StartCommission(System.Int32 commissionId, System.Boolean useCurrentSelection)",L"get_Instance",L"_instance",L"SelectedId",L"no game actions executed"};
 for(size_t i=0;i<sizeof(required)/sizeof(required[0]);i++)assert(wcsstr(text,required[i]));
 assert(!wcsstr(text,L"TYPE [1] Nova.Hotfix.DailyMissionProtocol"));
 GlobalUnlock(data);CloseClipboard();DestroyWindow(owner);FreeLibrary(fixture);
 puts("PASS report v2: 100 leading protobuf types skipped, six groups, late handlers, inherited singleton fields, clipboard and zero actions");
 return 0;
}
