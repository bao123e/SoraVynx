/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include "daily_report_policy.h"

struct DailyReportType {void *klass;const char *name,*ns,*image;int group,rank;};
static const char *(*daily_report_param_name)(const void*,uint32_t);
static int daily_report_compare(const void *left,const void *right) {
 const struct DailyReportType *a=left,*b=right;
 int ag=a->group==DAILY_BRIDGE_GROUP?-1:a->group,bg=b->group==DAILY_BRIDGE_GROUP?-1:b->group;
 if(ag!=bg)return ag-bg;
 if(a->rank!=b->rank)return a->rank-b->rank;
 int order=strcmp(a->image,b->image);if(order)return order;order=strcmp(a->ns,b->ns);return order?order:strcmp(a->name,b->name);
}
static bool daily_report_is_protocol(void *klass) {
 void *iterator=NULL;const void *method;unsigned scanned=0;
 while(scanned++<256&&(method=api.class_get_methods(klass,&iterator))!=NULL) {
  const char *name=api.method_get_name(method);
  if(!name||strcmp(name,"get_Parser"))continue;
  const void *type=api.method_get_return_type(method);char *result=type?api.type_get_name(type):NULL;
  bool protocol=daily_report_proto_return(result);if(result)api.free(result);
  if(protocol)return true;
 }
 return false;
}
static void daily_report_signature(char *text,size_t limit,size_t *used,const void *method) {
 const char *name=api.method_get_name(method);uint32_t count=api.method_get_param_count(method),impl=0;
 const void *return_type=api.method_get_return_type(method);char *ret=return_type?api.type_get_name(return_type):NULL;
 daily_append(text,limit,used,"  %s%.180s %.180s(",(api.method_get_flags(method,&impl)&0x10)?"static ":"",ret?ret:"?",name?name:"?");if(ret)api.free(ret);
 for(uint32_t p=0;p<count&&p<12;p++){const void *type=api.method_get_param(method,p);char *value=type?api.type_get_name(type):NULL;const char *label=daily_report_param_name?daily_report_param_name(method,p):NULL;daily_append(text,limit,used,"%s%.180s%s%.80s",p?", ":"",value?value:"?",label&&label[0]?" ":"",label?label:"");if(value)api.free(value);}
 if(count>12)daily_append(text,limit,used,", ... [%u parameters]",count);
 daily_append(text,limit,used,")\r\n");
}
static void daily_report_members(char *text,size_t limit,size_t *used,void *klass,
 void *(*get_fields)(void*,void**),const char *(*field_name)(void*),size_t field_budget) {
 size_t fields_limit=*used+field_budget;if(fields_limit>limit)fields_limit=limit;
 if(get_fields&&field_name)for(int pass=0;pass<2;pass++) {
  void *iterator=NULL,*field;unsigned scanned=0;
  while(scanned++<512&&*used+192<fields_limit&&(field=get_fields(klass,&iterator))!=NULL) {
   const char *name=field_name(field);bool important=(api.field_get_flags(field)&0x10)!=0||daily_name_matches(name,"selected")||daily_name_matches(name,"current")||daily_name_matches(name,"instance")||daily_name_matches(name,"progress");
   if((pass==0)!=important)continue;
   const void *type=api.field_get_type(field);char *value=type?api.type_get_name(type):NULL;
   daily_append(text,fields_limit,used,"  FIELD %s%.120s %.120s\r\n",(api.field_get_flags(field)&0x10)?"static ":"",value?value:"?",name?name:"?");if(value)api.free(value);
  }
 }
 /* Search all declared methods before deciding what fits, so late UI handlers survive. */
 for(int rank=0;rank<3;rank++) {
  void *iterator=NULL;const void *method;unsigned scanned=0;
  while(scanned++<1024&&*used+512<limit&&(method=api.class_get_methods(klass,&iterator))!=NULL)
   if(daily_report_method_rank(api.method_get_name(method))==rank)daily_report_signature(text,limit,used,method);
 }
}

/* Metadata only. Never calls a game action or writes a diagnostic file. */
static void daily_copy_setup(HWND owner) {
 if(!api.domain_get||!api.domain_get_assemblies||!api.assembly_get_image||!api.image_get_name||!hotfix_image){MessageBoxW(owner,L"Wait until Player/Event report READY and the game reaches its main menu.",L"SoraVynx",MB_OK|MB_ICONINFORMATION);return;}
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
 const size_t capacity=524288;char *text=calloc(capacity,1);size_t used=0;
 if(!text){api.thread_detach(thread);return;}
 daily_append(text,capacity,&used,"SoraVynx Auto Daily setup - report v3\r\nMode: keep current game selections\r\nStatus: task actions not bound; no game actions executed\r\nHotfix classes: %zu\r\n",class_count(hotfix_image));
 size_t assembly_count=0;const void **assemblies=api.domain_get_assemblies(domain,&assembly_count);
 if(!assemblies||assembly_count>4096){free(text);api.thread_detach(thread);MessageBoxW(owner,L"Unexpected assembly list; report stopped.",L"SoraVynx",MB_OK|MB_ICONERROR);return;}
 struct ReportImage {const void *image;const char *name;size_t count;bool scan;};
 struct ReportImage *images=calloc(assembly_count?assembly_count:1,sizeof(*images));size_t total=0;
 if(!images){free(text);api.thread_detach(thread);return;}
 daily_append(text,capacity,&used,"\r\nASSEMBLIES (%zu loaded)\r\n",assembly_count);
 for(size_t i=0;i<assembly_count;i++) {
  const void *image=api.assembly_get_image(assemblies[i]);const char *name=image?api.image_get_name(image):NULL;
  size_t count=image?class_count(image):0;bool wanted=image&&daily_report_scan_image(name),scan=wanted&&count<=100000&&total<=100000-count;
  images[i]=(struct ReportImage){image,name?name:"?",count,scan};if(scan)total+=count;
  daily_append(text,capacity,&used,"ASSEMBLY %.160s classes=%zu scan=%s%s\r\n",name?name:"?",count,scan?"yes":"no",wanted&&!scan?" (class-count budget)":"");
 }
 struct DailyReportType *types=calloc(total?total:1,sizeof(*types));size_t found=0,protocol=0,excluded=0,groups[DAILY_REPORT_GROUPS]={0};
 if(!types){free(images);free(text);api.thread_detach(thread);return;}
 for(size_t image_index=0;image_index<assembly_count;image_index++)if(images[image_index].scan) {
  struct ReportImage *entry=&images[image_index];
  for(size_t i=0;i<entry->count;i++) {
   void *klass=get_class(entry->image,i);const char *name=klass?class_name(klass):NULL,*ns=klass?class_namespace(klass):NULL;
   if(!name||!name[0]||name[0]=='<'||!strncmp(name,"table_",6)||daily_name_matches(ns,"csobjectwrap")){excluded++;continue;}
   int group=daily_report_group(name);
   if(group<0&&daily_name_matches(ns,"xlua"))group=DAILY_BRIDGE_GROUP;
   if(group<0){excluded++;continue;}
   if(daily_report_is_protocol(klass)){protocol++;continue;}
   types[found++]=(struct DailyReportType){klass,name,ns?ns:"",entry->name,group,daily_report_type_rank(name)};groups[group]++;
  }
 }
 free(images);
 daily_append(text,capacity,&used,"Scanned application classes: %zu\r\n",total);
 qsort(types,found,sizeof(*types),daily_report_compare);
 daily_append(text,capacity,&used,"Candidate runtime types: %zu\r\nSkipped generated protocol types: %zu\r\nOther/table/generated names: %zu\r\n",found,protocol,excluded);
 /* Preserve a broad type inventory independently of the per-group signature budgets. */
 size_t indexed=0,index_limit=196608;
 daily_append(text,index_limit,&used,"\r\nCLASS INDEX (runtime candidates only)\r\n");
 for(size_t i=0;i<found&&used+512<index_limit;i++) {
  daily_append(text,index_limit,&used,"TYPE [%d] %.120s:%.160s.%.160s\r\n",types[i].group+1,types[i].image,types[i].ns,types[i].name);indexed++;
 }
 daily_append(text,capacity,&used,"\r\nIndex coverage: %zu/%zu (omitted %zu)\r\n",indexed,found,found-indexed);
 void *(*get_fields)(void*,void**)=NULL;const char *(*field_name)(void*)=NULL;
 proc=GetProcAddress(game,"il2cpp_class_get_fields");memcpy(&get_fields,&proc,sizeof(get_fields));
 proc=GetProcAddress(game,"il2cpp_field_get_name");memcpy(&field_name,&proc,sizeof(field_name));
 proc=GetProcAddress(game,"il2cpp_method_get_param_name");memcpy(&daily_report_param_name,&proc,sizeof(daily_report_param_name));
 daily_append(text,capacity,&used,"Field metadata APIs: %s\r\n",get_fields&&field_name?"available":"unavailable (method signatures only)");
 daily_append(text,capacity,&used,"Parameter names API: %s\r\n",daily_report_param_name?"available":"unavailable (types only)");
 for(int pass=0;pass<DAILY_REPORT_GROUPS;pass++) {
  int group=pass==0?DAILY_BRIDGE_GROUP:pass-1;size_t group_budget=group==DAILY_BRIDGE_GROUP?65536:24576,class_budget=group==DAILY_BRIDGE_GROUP?16384:4096;
  size_t limit=used+group_budget;if(limit>capacity-2048)limit=capacity-2048;size_t emitted=0;
  daily_append(text,limit,&used,"\r\nGROUP %s (%zu candidates)\r\n",daily_group_labels[group],groups[group]);
  for(size_t i=0;i<found&&used+768<limit;i++)if(types[i].group==group) {
   size_t class_limit=used+class_budget;if(class_limit>limit)class_limit=limit;
   daily_append(text,class_limit,&used,"\r\nAPI %.120s:%.160s.%.160s\r\n",types[i].image,types[i].ns,types[i].name);emitted++;
   /* Inherited singleton accessors matter even if absent from the derived type. */
   void *parent=api.class_get_parent?api.class_get_parent(types[i].klass):NULL;
   for(int depth=0;parent&&depth<4&&used+512<class_limit;depth++) {
    const char *name=class_name(parent);daily_append(text,class_limit,&used,"  BASE %.160s\r\n",name?name:"?");
    if(get_fields&&field_name){void *field_iterator=NULL,*field;unsigned scanned_fields=0;
     while(scanned_fields++<128&&used+512<class_limit&&(field=get_fields(parent,&field_iterator))!=NULL){
      const char *field_label=field_name(field);uint32_t flags=api.field_get_flags(field);
      if(!(flags&0x10)&&!daily_name_matches(field_label,"instance")&&!daily_name_matches(field_label,"current")&&!daily_name_matches(field_label,"selected"))continue;
      const void *type=api.field_get_type(field);char *field_type=type?api.type_get_name(type):NULL;
      daily_append(text,class_limit,&used,"  INHERITED FIELD %s%.120s %.120s\r\n",(flags&0x10)?"static ":"",field_type?field_type:"?",field_label?field_label:"?");if(field_type)api.free(field_type);
     }
    }
    void *iterator=NULL;const void *method;unsigned scanned=0;
    while(scanned++<256&&used+512<class_limit&&(method=api.class_get_methods(parent,&iterator))!=NULL) {
     const char *method_name=api.method_get_name(method);
     if(daily_name_matches(method_name,"instance")||daily_name_matches(method_name,"current"))daily_report_signature(text,class_limit,&used,method);
    }
    parent=api.class_get_parent(parent);
   }
   daily_report_members(text,class_limit,&used,types[i].klass,get_fields,field_name,group==DAILY_BRIDGE_GROUP?4096:1024);
  }
  daily_append(text,capacity,&used,"\r\nGroup coverage: %zu/%zu types detailed. Budgets: %zu KiB/group, %zu KiB/type; long member lists may be shortened.\r\n",emitted,groups[group],group_budget/1024,class_budget/1024);
 }
 daily_append(text,capacity,&used,"\r\nReport v3 complete. Index: %zu/%zu. Metadata names/signatures only; no field values, account data, keys, network requests or game actions.\r\n",indexed,found);
 free(types);
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
