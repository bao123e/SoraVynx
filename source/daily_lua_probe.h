/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include "vendor/minhook/include/MinHook.h"

/* Optional discovery bridge. All VM access is deferred to LuaManager.Update;
 * no game task, network request, require(), or arbitrary script is invoked. */
static const void *daily_lua_update_method,*daily_lua_inspect_method,*daily_lua_main_thread_method;
static void *daily_lua_manager_class,*daily_lua_env_class;
static void *daily_lua_instance_field,*daily_lua_env_field;
static void (*daily_lua_update_original)(void*,const void*);
static void *(*daily_lua_string_new)(const char*);
static int32_t (*daily_lua_string_length)(void*);
static const uint16_t *(*daily_lua_string_chars)(void*);
static uintptr_t (*daily_lua_array_length)(void*);
static uint32_t (*daily_lua_array_header_size)(void);
static int32_t (*daily_lua_array_element_size)(void*);
static uint32_t (*daily_lua_handle_new)(void*,bool);
static void (*daily_lua_handle_free)(uint32_t);
static char *daily_lua_script;
static wchar_t *daily_lua_result;
static volatile LONG daily_lua_state; /* 0 idle, 1 queued, 2 executing, 3 result */
static bool daily_lua_available;
static ULONGLONG daily_lua_requested_at;
static const wchar_t *daily_lua_failure=L"Lua inspection is unavailable in this game build. Use Copy API metadata for diagnostics.";

static wchar_t *daily_lua_error_result(const wchar_t *reason) {
 size_t length=wcslen(reason)+160;wchar_t *result=calloc(length,sizeof(*result));
 if(result)swprintf(result,length,L"SoraVynx Auto Daily setup - Lua report v4\r\nStatus: inspection stopped\r\n%ls\r\nNo daily actions executed.\r\n",reason);
 return result;
}
static wchar_t *daily_lua_collect(void) {
 /* Read the existing singleton field, never get_Instance() which may create it. */
 void *manager=NULL,*env=NULL;api.field_static_get_value(daily_lua_instance_field,&manager);
 if(!manager||!api.class_is_assignable_from(daily_lua_manager_class,api.object_get_class(manager)))return daily_lua_error_result(L"Existing LuaManager is not ready. Enter the game main menu and retry.");
 api.field_get_value(manager,daily_lua_env_field,&env);
 if(!env||!api.class_is_assignable_from(daily_lua_env_class,api.object_get_class(env)))return daily_lua_error_result(L"The existing Lua environment is not ready. No environment was created.");
 void *script=daily_lua_string_new(daily_lua_script);
 if(!script)return daily_lua_error_result(L"Could not allocate the fixed inspection script.");
 uint32_t script_handle=daily_lua_handle_new(script,false);if(!script_handle)return daily_lua_error_result(L"Could not retain the inspection script.");
 /* Exactly the bundled fixed inspection script; no input text is evaluated.
  * Managed reference arguments are passed as objects, not pointer-to-pointer. */
 void *args[]={script,NULL,NULL},*exception=NULL;
 void *array=api.runtime_invoke(daily_lua_inspect_method,env,args,&exception);
 daily_lua_handle_free(script_handle);
 if(exception||!array)return daily_lua_error_result(L"Fixed Lua name inspection failed. Exception contents were not copied.");
 uint32_t array_handle=daily_lua_handle_new(array,false);if(!array_handle)return daily_lua_error_result(L"Could not retain the inspection result.");
 wchar_t *result=NULL;
 uint32_t header=daily_lua_array_header_size();
 if(daily_lua_array_length(array)==1&&header>=3*sizeof(void*)&&header<=128&&header%sizeof(void*)==0&&daily_lua_array_element_size(api.object_get_class(array))==(int32_t)sizeof(void*)) {
  void *value=NULL;memcpy(&value,(const char*)array+header,sizeof(value));
  void *string_class=find_class("System","String");
  if(value&&string_class&&api.object_get_class(value)==string_class) {
   int32_t length=daily_lua_string_length(value);const uint16_t *chars=daily_lua_string_chars(value);
   if(chars&&length>0&&length<=220000){result=calloc((size_t)length+1,sizeof(*result));if(result)memcpy(result,chars,(size_t)length*sizeof(*result));}
  }
 }
 daily_lua_handle_free(array_handle);
 return result?result:daily_lua_error_result(L"Unexpected or oversized Lua inspection result; contents were not copied.");
}
static void daily_lua_update_hook(void *self,const void *method) {
 daily_lua_update_original(self,method);
 if(method!=daily_lua_update_method||InterlockedCompareExchange(&daily_lua_state,0,0)!=1)return;
 void *exception=NULL,*boxed=api.runtime_invoke(daily_lua_main_thread_method,NULL,NULL,&exception);
 bool main_thread=false;if(!exception&&boxed){void *value=api.object_unbox(boxed);if(value)memcpy(&main_thread,value,sizeof(main_thread));}
 if(!main_thread)return;
 /* Pointer identity and CAS prevent shared-entry callbacks and nested Updates
  * from running the collector twice. The original Update always runs first. */
 if(method!=daily_lua_update_method||InterlockedCompareExchange(&daily_lua_state,2,1)!=1)return;
 daily_lua_result=daily_lua_collect();InterlockedExchange(&daily_lua_state,3);
}
static bool daily_lua_install(HMODULE game,HMODULE resources) {
 void *manager=find_class("","LuaManager"),*env=find_class("XLua","LuaEnv"),*unity_object=find_class("UnityEngine","Object");
 if(!manager||!env||!unity_object)return false;
 const void *main_thread=find_method(unity_object,"CurrentThreadIsMainThread","System.Boolean",0,NULL,true);
 if(!main_thread)return false;
 const char *params[]={"System.String","System.String","XLua.LuaTable"};
 const void *update=find_method(manager,"Update","System.Void",0,NULL,false);
 const void *inspect=find_method(env,"DoString","System.Object[]",3,params,false);
 void *instance=find_field(manager,"g_instance","LuaManager",true);
 void *env_field=find_field(manager,"<luaEnv>k__BackingField","XLua.LuaEnv",false);
 if(!update||!inspect||!instance||!env_field)return false;
#define DAILY_LUA_EXPORT(name,export_name) do { FARPROC proc=GetProcAddress(game,export_name);if(!proc)return false;memcpy(&name,&proc,sizeof(name)); }while(0)
 DAILY_LUA_EXPORT(daily_lua_string_new,"il2cpp_string_new");
 DAILY_LUA_EXPORT(daily_lua_string_length,"il2cpp_string_length");
 DAILY_LUA_EXPORT(daily_lua_string_chars,"il2cpp_string_chars");
 DAILY_LUA_EXPORT(daily_lua_array_length,"il2cpp_array_length");
 DAILY_LUA_EXPORT(daily_lua_array_header_size,"il2cpp_array_object_header_size");
 DAILY_LUA_EXPORT(daily_lua_array_element_size,"il2cpp_array_element_size");
 DAILY_LUA_EXPORT(daily_lua_handle_new,"il2cpp_gchandle_new");
 DAILY_LUA_EXPORT(daily_lua_handle_free,"il2cpp_gchandle_free");
#undef DAILY_LUA_EXPORT
 HRSRC resource=FindResourceW(resources,MAKEINTRESOURCEW(404),MAKEINTRESOURCEW(10));
 DWORD bytes=resource?SizeofResource(resources,resource):0;HGLOBAL data=resource?LoadResource(resources,resource):NULL;
 const void *script=data?LockResource(data):NULL;if(!script||!bytes||bytes>65536)return false;
 daily_lua_script=calloc((size_t)bytes+1,1);if(!daily_lua_script)return false;memcpy(daily_lua_script,script,bytes);
 void *entry=*(void *const *)update;MEMORY_BASIC_INFORMATION info;
 if(!entry||!VirtualQuery(entry,&info,sizeof(info))||info.State!=MEM_COMMIT||
    (info.Protect&(PAGE_GUARD|PAGE_NOACCESS))||!(info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))goto failed;
 daily_lua_manager_class=manager;daily_lua_env_class=env;daily_lua_update_method=update;daily_lua_inspect_method=inspect;
 daily_lua_main_thread_method=main_thread;daily_lua_instance_field=instance;daily_lua_env_field=env_field;
 void *original=NULL;if(MH_CreateHook(entry,(void*)daily_lua_update_hook,&original)!=MH_OK)goto failed;
 memcpy(&daily_lua_update_original,&original,sizeof(original));
 if(MH_EnableHook(entry)!=MH_OK){MH_RemoveHook(entry);goto failed;}
 daily_lua_available=true;return true;
 failed:free(daily_lua_script);daily_lua_script=NULL;return false;
}
static void daily_lua_request(HWND owner) {
 if(!daily_lua_available){MessageBoxW(owner,daily_lua_failure,L"SoraVynx",MB_OK|MB_ICONINFORMATION);return;}
 if(InterlockedCompareExchange(&daily_lua_state,1,0)==0)daily_lua_requested_at=GetTickCount64();
}
static void daily_lua_poll(HWND owner) {
 LONG state=InterlockedCompareExchange(&daily_lua_state,0,0);
 if(state==1&&GetTickCount64()-daily_lua_requested_at>15000&&InterlockedCompareExchange(&daily_lua_state,0,1)==1){
  MessageBoxW(owner,L"No Lua Update received within 15 seconds. Return to the game main menu and retry.",L"SoraVynx",MB_OK|MB_ICONINFORMATION);return;
 }
 if(state!=3||InterlockedCompareExchange(&daily_lua_state,2,3)!=3)return;
 wchar_t *result=daily_lua_result;daily_lua_result=NULL;InterlockedExchange(&daily_lua_state,0);
 bool copied=false;
 if(result){SIZE_T bytes=(wcslen(result)+1)*sizeof(*result);HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);
  if(memory){void *wide=GlobalLock(memory);if(wide){memcpy(wide,result,bytes);GlobalUnlock(memory);
    if(OpenClipboard(owner)){EmptyClipboard();copied=SetClipboardData(CF_UNICODETEXT,memory)!=NULL;CloseClipboard();}}
   if(!copied)GlobalFree(memory);
  }free(result);
 }
 MessageBoxW(owner,copied?L"Lua name report copied. Send it to continue Auto Daily integration. No daily tasks were run.":L"Could not copy the Lua report. Please retry.",L"SoraVynx",MB_OK|(copied?MB_ICONINFORMATION:MB_ICONERROR));
}
