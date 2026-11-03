/* SPDX-License-Identifier: AGPL-3.0-or-later */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include "../backend.h"
#include "../daily_lua_probe.h"
#undef NDEBUG
#include <assert.h>
struct RuntimeAPI api;
const void *hotfix_image;
static int originals,inspections,utility_calls,handles,freed;
static bool on_main=true,raise_exception,manager_ready=true,bad_result,reenter;
static const wchar_t report_text[]=L"SoraVynx Auto Daily setup - Lua report v5\r\nFUNCTION loaded/Daily.Claim\r\n";
static void *manager_object=(void*)10,*env_object=(void*)11;
static unsigned char array_storage[40];
static void *string_object=(void*)12,*array_object=array_storage,*script_object=(void*)14;
static void *mock_class(void *object){return object==manager_object?(void*)20:object==env_object?(void*)21:object==string_object?(void*)22:(void*)23;}
static bool mock_assignable(void *a,void *b){return a==b;}
static void mock_static(void *field,void *value){assert(field==(void*)30);*(void**)value=manager_ready?manager_object:NULL;}
static void mock_field(void *object,void *field,void *value){assert(object==manager_object&&field==(void*)31);*(void**)value=env_object;}
static void *mock_string(const char *text){assert(!strcmp(text,"fixed inspection script"));return script_object;}
static int32_t mock_length(void *value){assert(value==string_object);return (int32_t)wcslen(report_text);}
static const uint16_t *mock_chars(void *value){assert(value==string_object);return (const uint16_t*)report_text;}
static uintptr_t mock_count(void *value){assert(value==array_object);return bad_result?2:1;}
static uint32_t mock_header(void){return 32;}
static int32_t mock_element_size(void *klass){assert(klass==(void*)23);return 8;}
static uint32_t mock_handle(void *value,bool pinned){assert(!pinned&&(value==script_object||value==array_object));return (uint32_t)++handles;}
static void mock_free(uint32_t handle){assert(handle>0);freed++;}
static void mock_original(void *self,const void *method){(void)self;(void)method;originals++;}
static void *mock_unbox(void *object){assert(object==(void*)90);return &on_main;}
static void *mock_invoke(const void *method,void *object,void **args,void **exception){
 if(method==(void*)91){assert(!object&&!args);utility_calls++;return (void*)90;}
 assert(method==(void*)77&&object==env_object&&args[0]==script_object&&!args[1]&&!args[2]);
 assert(originals>0);inspections++;
 if(reenter)daily_lua_update_hook(manager_object,daily_lua_update_method);
 if(raise_exception){*exception=(void*)99;return NULL;}
 return array_object;
}
void *find_class(const char *ns,const char *name){return !strcmp(ns,"System")&&!strcmp(name,"String")?(void*)22:NULL;}
const void *find_method(void *k,const char *n,const char *r,int count,const char *const *p,bool s){(void)k;(void)n;(void)r;(void)count;(void)p;(void)s;return NULL;}
void *find_field(void *k,const char *n,const char *t,bool s){(void)k;(void)n;(void)t;(void)s;return NULL;}
MH_STATUS WINAPI MH_CreateHook(LPVOID target,LPVOID detour,LPVOID *original){(void)target;(void)detour;(void)original;abort();}
MH_STATUS WINAPI MH_EnableHook(LPVOID target){(void)target;abort();}
MH_STATUS WINAPI MH_RemoveHook(LPVOID target){(void)target;abort();}
static BOOL CALLBACK close_box(HWND hwnd,LPARAM unused){(void)unused;wchar_t name[32];GetClassNameW(hwnd,name,32);if(!wcscmp(name,L"#32770"))PostMessageW(hwnd,WM_COMMAND,IDOK,0);return TRUE;}
static void CALLBACK dismiss(HWND hwnd,UINT message,UINT_PTR timer,DWORD time){(void)hwnd;(void)message;(void)timer;(void)time;EnumThreadWindows(GetCurrentThreadId(),close_box,0);}
static void discard(void){free(daily_lua_result);daily_lua_result=NULL;daily_lua_state=0;}
int main(void){
 memcpy(array_storage+32,&string_object,sizeof(string_object));
 api.object_get_class=mock_class;api.class_is_assignable_from=mock_assignable;api.field_static_get_value=mock_static;api.field_get_value=mock_field;api.runtime_invoke=mock_invoke;api.object_unbox=mock_unbox;
 daily_lua_manager_class=(void*)20;daily_lua_env_class=(void*)21;daily_lua_instance_field=(void*)30;daily_lua_env_field=(void*)31;
 daily_lua_string_new=mock_string;daily_lua_string_length=mock_length;daily_lua_string_chars=mock_chars;daily_lua_array_length=mock_count;daily_lua_array_header_size=mock_header;daily_lua_array_element_size=mock_element_size;daily_lua_handle_new=mock_handle;daily_lua_handle_free=mock_free;
 daily_lua_update_original=mock_original;daily_lua_update_method=(void*)76;daily_lua_inspect_method=(void*)77;daily_lua_main_thread_method=(void*)91;daily_lua_script="fixed inspection script";daily_lua_available=true;
 assert(!daily_lua_install(NULL,NULL)); /* Optional probe failure never alters core startup. */
 daily_lua_update_hook(manager_object,daily_lua_update_method);assert(originals==1&&!inspections&&!utility_calls);
 daily_lua_request(NULL);daily_lua_request(NULL);assert(daily_lua_state==1);
 daily_lua_update_hook(manager_object,(void*)75);assert(daily_lua_state==1&&!utility_calls);
 on_main=false;daily_lua_update_hook(manager_object,daily_lua_update_method);assert(daily_lua_state==1&&!inspections);
 on_main=true;reenter=true;daily_lua_update_hook(manager_object,daily_lua_update_method);
 assert(daily_lua_state==3&&inspections==1&&handles==freed&&!wcscmp(daily_lua_result,report_text));
 daily_lua_update_hook(manager_object,daily_lua_update_method);assert(inspections==1);
 assert(SetTimer(NULL,1,50,dismiss));daily_lua_poll(NULL);assert(!daily_lua_state);
 assert(OpenClipboard(NULL));HANDLE memory=GetClipboardData(CF_UNICODETEXT);assert(memory);const wchar_t *copied=GlobalLock(memory);assert(copied&&!wcscmp(copied,report_text));GlobalUnlock(memory);CloseClipboard();
 reenter=false;raise_exception=true;daily_lua_request(NULL);daily_lua_update_hook(manager_object,daily_lua_update_method);assert(daily_lua_state==3&&wcsstr(daily_lua_result,L"inspection failed")&&handles==freed);discard();
 raise_exception=false;bad_result=true;daily_lua_request(NULL);daily_lua_update_hook(manager_object,daily_lua_update_method);assert(wcsstr(daily_lua_result,L"Unexpected or oversized")&&handles==freed);discard();
 manager_ready=false;int before=inspections;daily_lua_request(NULL);daily_lua_update_hook(manager_object,daily_lua_update_method);assert(inspections==before&&wcsstr(daily_lua_result,L"not ready"));discard();
 daily_lua_request(NULL);daily_lua_requested_at=GetTickCount64()-16000;daily_lua_poll(NULL);assert(!daily_lua_state);KillTimer(NULL,1);
 puts("PASS Lua probe: original Update, idle/shared/off-main guards, one queued scan, reentrancy, GC handles, exceptions, clipboard and timeout");return 0;
}
