/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include "daily_report_text.h"

/* Metadata only. Never calls a game action or writes a diagnostic file. */
static void daily_copy_setup(HWND owner) {
 if(!api.domain_get||!hotfix_image){MessageBoxW(owner,L"Wait until Player/Event report READY and the game reaches its main menu.",L"SoraVynx",MB_OK|MB_ICONINFORMATION);return;}
 HMODULE game=GetModuleHandleW(L"GameAssembly.dll");
 size_t (*class_count)(const void*)=NULL;void *(*get_class)(const void*,size_t)=NULL;
 const char *(*class_name)(void*)=NULL;const char *(*class_namespace)(void*)=NULL;
 FARPROC proc=game?GetProcAddress(game,"il2cpp_image_get_class_count"):NULL;memcpy(&class_count,&proc,sizeof(class_count));
 proc=game?GetProcAddress(game,"il2cpp_image_get_class"):NULL;memcpy(&get_class,&proc,sizeof(get_class));
 proc=game?GetProcAddress(game,"il2cpp_class_get_name"):NULL;memcpy(&class_name,&proc,sizeof(class_name));
 proc=game?GetProcAddress(game,"il2cpp_class_get_namespace"):NULL;memcpy(&class_namespace,&proc,sizeof(class_namespace));
 if(!class_count||!get_class||!class_name||!class_namespace){MessageBoxW(owner,L"The runtime does not expose the metadata APIs needed for Auto Daily setup.",L"SoraVynx",MB_OK|MB_ICONERROR);return;}
 void *domain=api.domain_get();void *thread=domain?api.thread_attach(domain):NULL;
 if(!thread){MessageBoxW(owner,L"The game runtime is not ready for the setup report.",L"SoraVynx",MB_OK|MB_ICONERROR);return;}
 const size_t capacity=98304;char *text=calloc(capacity,1);size_t used=0,listed=0,total=class_count(hotfix_image);
 if(!text){api.thread_detach(thread);return;}
 daily_append(text,capacity,&used,"SoraVynx Auto Daily setup\r\nMode: keep current game selections\r\nStatus: task actions not bound; no game actions executed\r\nHotfix classes: %zu\r\n",total);
 if(total>100000){daily_append(text,capacity,&used,"Unexpected metadata count; enumeration stopped.\r\n");total=0;}
 const char *primary[]={"daily","mission","quest"};
 const char *secondary[]={"trial","ascension","tower","trekker","disc","vigor","stage","reward"};
 for(int pass=0;pass<2;pass++)for(size_t i=0;i<total&&listed<160&&used<capacity-2048;i++) {
  void *klass=get_class(hotfix_image,i);const char *name=klass?class_name(klass):NULL;
  if(!name||name[0]=='<')continue;
  bool first=false,second=false;
  for(size_t j=0;j<sizeof(primary)/sizeof(primary[0]);j++)if(daily_name_matches(name,primary[j]))first=true;
  if(pass==1)for(size_t j=0;j<sizeof(secondary)/sizeof(secondary[0]);j++)if(daily_name_matches(name,secondary[j]))second=true;
  if((pass==0&&!first)||(pass==1&&(first||!second)))continue;
  const char *ns=class_namespace(klass);daily_append(text,capacity,&used,"\r\nTYPE %s.%s\r\n",ns?ns:"",name);listed++;
  void *iterator=NULL;const void *method=NULL;unsigned methods=0;
  while(methods++<32&&(method=api.class_get_methods(klass,&iterator))!=NULL&&used<capacity-1024) {
   const char *method_name=api.method_get_name(method);uint32_t count=api.method_get_param_count(method),impl=0;
   char *ret=api.type_get_name(api.method_get_return_type(method));
   daily_append(text,capacity,&used,"  %s%s %s(",(api.method_get_flags(method,&impl)&0x10)?"static ":"",ret?ret:"?",method_name?method_name:"?");if(ret)api.free(ret);
   for(uint32_t p=0;p<count&&p<12;p++){char *type=api.type_get_name(api.method_get_param(method,p));daily_append(text,capacity,&used,"%s%s",p?", ":"",type?type:"?");if(type)api.free(type);}
   daily_append(text,capacity,&used,")\r\n");
  }
 }
 daily_append(text,capacity,&used,"\r\nListed types: %zu. Bounded metadata sample; no account values, keys, network requests or game actions.\r\n",listed);
 api.thread_detach(thread);
 int length=MultiByteToWideChar(CP_UTF8,0,text,-1,NULL,0);HGLOBAL memory=length>0?GlobalAlloc(GMEM_MOVEABLE,(SIZE_T)length*sizeof(wchar_t)):NULL;
 bool copied=false;
 if(memory){wchar_t *wide=GlobalLock(memory);if(wide){MultiByteToWideChar(CP_UTF8,0,text,-1,wide,length);GlobalUnlock(memory);
   if(OpenClipboard(owner)){EmptyClipboard();copied=SetClipboardData(CF_UNICODETEXT,memory)!=NULL;CloseClipboard();}
  }if(!copied)GlobalFree(memory);
 }
 free(text);
 MessageBoxW(owner,copied?L"Setup report copied. Send this text to finish binding the daily actions. No file was created.":L"Could not copy the report. Try again when the clipboard is available.",L"SoraVynx",MB_OK|(copied?MB_ICONINFORMATION:MB_ICONERROR));
}
