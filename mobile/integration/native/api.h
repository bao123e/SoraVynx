/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include "compat.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define IL2CPP_API(X) \
 X(void*,domain_get,(void)) \
 X(const void**,domain_get_assemblies,(void*,size_t*)) \
 X(const void*,assembly_get_image,(const void*)) \
 X(const char*,image_get_name,(const void*)) \
 X(void*,thread_attach,(void*)) \
 X(void,thread_detach,(void*)) \
 X(void*,class_from_name,(const void*,const char*,const char*)) \
 X(void*,class_from_type,(const void*)) \
 X(const void*,class_get_methods,(void*,void**)) \
 X(void*,class_get_parent,(void*)) \
 X(const char*,method_get_name,(const void*)) \
 X(uint32_t,method_get_param_count,(const void*)) \
 X(const void*,method_get_param,(const void*,uint32_t)) \
 X(const void*,method_get_return_type,(const void*)) \
 X(uint32_t,method_get_flags,(const void*,uint32_t*)) \
 X(char*,type_get_name,(const void*)) \
 X(void,free,(void*)) \
 X(void*,object_get_class,(void*)) \
 X(void*,object_unbox,(void*)) \
 X(bool,class_is_assignable_from,(void*,void*)) \
 X(int32_t,class_value_size,(void*,uint32_t*)) \
 X(void*,class_get_field_from_name,(void*,const char*)) \
 X(uint32_t,field_get_flags,(void*)) \
 X(const void*,field_get_type,(void*)) \
 X(void,field_get_value,(void*,void*,void*)) \
 X(void,field_set_value,(void*,void*,void*)) \
 X(void*,array_new,(void*,uintptr_t)) \
 X(void,field_static_get_value,(void*,void*)) \
 X(void*,runtime_invoke,(const void*,void*,void**,void**))

struct RuntimeAPI {
#define DECLARE_API(ret,name,args) ret (*name) args;
 IL2CPP_API(DECLARE_API)
#undef DECLARE_API
};
extern struct RuntimeAPI api;
extern const void *hotfix_image;
extern void *player_class, *monster_class;
void log_line(const char *format,...);
bool bind_api(HMODULE module);
void *find_class(const char *ns,const char *name);
bool type_matches(const void *type,const char *expected);
const void *find_method(void *klass,const char *name,const char *ret,int count,
                       const char *const *params,bool is_static);
void *find_field(void *klass,const char *name,const char *type,bool is_static);
void *invoke(const void *method,void *object,void **args,bool *ok);
bool is_player(void *object);
bool is_monster(void *object);
void log_candidate_methods(void *klass,const char *name);
