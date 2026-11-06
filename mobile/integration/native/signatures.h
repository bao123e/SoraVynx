/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <string.h>
/* Native wrappers are allowed only for the exact scalar/reference shapes they
 * implement. An unrelated by-value struct must never be forwarded as a pointer. */
static bool native_parameters_safe(const void *method){
 uint32_t count=api.method_get_param_count(method);if(count>9)return false;
 for(uint32_t i=0;i<count;i++){
  const void *type=api.method_get_param(method,i);char *name=api.type_get_name(type);
  if(!name)return false;
  bool scalar=!strcmp(name,"System.Int32")||!strcmp(name,"System.Int64")||!strcmp(name,"System.Boolean");
  bool byref=strchr(name,'&')!=NULL;
  bool fixed8=!strcmp(name,"TrueSync.FP")||!strcmp(name,"TrueSync.FDP");
  api.free(name);
  if(scalar||byref)continue;
  void *klass=api.class_from_type(type);if(!klass)return false;
  bool value_type=api.class_is_valuetype(klass);
  if(fixed8&&!value_type)return false;
  if(value_type){
   uint32_t align=0;if(!fixed8||api.class_value_size(klass,&align)!=8)return false;
  }
 }
 return true;
}
static bool type_equal(const void *a,const void *b){
 if(a==b)return true;
 char *x=api.type_get_name(a),*y=api.type_get_name(b);
 bool equal=x&&y&&!strcmp(x,y);if(x)api.free(x);if(y)api.free(y);return equal;
}
static bool signature_equal(const void *a,const void *b){
 if(a==b)return true;
 uint32_t n=api.method_get_param_count(a);
 if(n>9||n!=api.method_get_param_count(b)||((api.method_get_flags(a,NULL)^api.method_get_flags(b,NULL))&0x10))return false;
 if(!type_equal(api.method_get_return_type(a),api.method_get_return_type(b)))return false;
 for(uint32_t i=0;i<n;i++)if(!type_equal(api.method_get_param(a,i),api.method_get_param(b,i)))return false;
 return true;
}
