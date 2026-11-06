/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "api.h"
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
struct RuntimeAPI api;
const void *hotfix_image;
void *player_class,*monster_class;

bool bind_api(HMODULE module) {
#define BIND_API(ret,name,args) do { \
 void *p=dlsym(module,"il2cpp_" #name); \
 if(!p){log_line("Missing export il2cpp_%s",#name);return false;} \
 memcpy(&api.name,&p,sizeof(p)); \
} while(0);
 IL2CPP_API(BIND_API)
#undef BIND_API
 return true;
}

static void canonical(char *out,size_t capacity,const char *source) {
 size_t n=0;
 if(!source){out[0]=0;return;}
 for(size_t i=0;source[i]&&n+1<capacity;i++) {
  if(source[i]=='`') { while(source[i+1]>='0'&&source[i+1]<='9')i++;continue; }
  if(source[i]!=' ')out[n++]=source[i];
 }
 out[n]=0;
}
bool type_matches(const void *type,const char *expected) {
 if(!expected)return true;
 if(!type)return false;
 char *name=api.type_get_name(type);if(!name)return false;
 char a[512],b[512];canonical(a,sizeof(a),name);canonical(b,sizeof(b),expected);
 bool match=!strcmp(a,b);api.free(name);return match;
}
void *find_class(const char *ns,const char *name) {
 void *klass=hotfix_image?api.class_from_name(hotfix_image,ns,name):NULL;
 if(klass)return klass;
 size_t count=0;const void **assemblies=api.domain_get_assemblies(api.domain_get(),&count);
 if(!assemblies||count>4096)return NULL;
 for(size_t i=0;i<count;i++) {
  const void *image=api.assembly_get_image(assemblies[i]);
  if(image&&(klass=api.class_from_name(image,ns,name)))return klass;
 }
 return NULL;
}
const void *find_method(void *klass,const char *name,const char *ret,int count,
                       const char *const *params,bool is_static) {
 for(void *current=klass;current;current=api.class_get_parent(current)) {
  void *iterator=NULL;const void *method;
  while((method=api.class_get_methods(current,&iterator))) {
   const char *actual=api.method_get_name(method);
   if(!actual||strcmp(actual,name)||api.method_get_param_count(method)!=(uint32_t)count)continue;
   uint32_t flags=api.method_get_flags(method,NULL);
   if(((flags&0x10)!=0)!=is_static||!type_matches(api.method_get_return_type(method),ret))continue;
   bool match=true;
   for(int j=0;j<count&&match;j++)if(params&&params[j])
    match=type_matches(api.method_get_param(method,(uint32_t)j),params[j]);
   if(match)return method;
  }
 }
 return NULL;
}
void *find_field(void *klass,const char *name,const char *type,bool is_static) {
 for(void *current=klass;current;current=api.class_get_parent(current)) {
  void *field=api.class_get_field_from_name(current,name);
  if(field&&(((api.field_get_flags(field)&0x10)!=0)==is_static)&&type_matches(api.field_get_type(field),type))return field;
 }
 return NULL;
}
void *invoke(const void *method,void *object,void **args,bool *ok) {
 *ok=false;if(!method)return NULL;
 void *exception=NULL;void *result=api.runtime_invoke(method,object,args,&exception);
 if(exception) {
  static volatile LONG errors;
  if(InterlockedIncrement(&errors)<=5)log_line("Managed invocation raised an exception; operation skipped.");
  return NULL;
 }
 *ok=true;return result;
}
bool is_player(void *object) {
 return object&&player_class&&api.class_is_assignable_from(player_class,api.object_get_class(object));
}
bool is_monster(void *object) {
 return object&&monster_class&&api.class_is_assignable_from(monster_class,api.object_get_class(object));
}
void log_candidate_methods(void *klass,const char *name) {
 unsigned emitted=0;
 for(void *current=klass;current&&emitted<8;current=api.class_get_parent(current)) {
  void *iterator=NULL;const void *method;
  while(emitted<8&&(method=api.class_get_methods(current,&iterator))) {
   const char *actual=api.method_get_name(method);
   if(!actual||strcmp(actual,name))continue;
   uint32_t count=api.method_get_param_count(method);
   char parameters[2048]={0};
   for(uint32_t i=0;i<count&&i<32;i++) {
    char *type=api.type_get_name(api.method_get_param(method,i));
    size_t used=strlen(parameters);
    if(used+3<sizeof(parameters))snprintf(parameters+used,sizeof(parameters)-used,"%s%s",i?", ":"",type?type:"<null>");
    if(type)api.free(type);
   }
   char *ret=api.type_get_name(api.method_get_return_type(method));
   log_line("ACTUAL CANDIDATE %s: %s (%s), params=%u static=%u MethodInfo=%p",name,ret?ret:"<null>",parameters,count,(api.method_get_flags(method,NULL)&0x10)!=0,method);
   if(ret)api.free(ret);emitted++;
  }
 }
 if(!emitted)log_line("No same-name candidate found for %s",name);
}
