/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <string.h>
/* AArch64 wrapper shapes: X = GPR64/reference, I = GPR32, B = bool, V = void.
 * FP registers and unverified value aggregates cannot use these wrappers. */
static char native_type_abi(const void *type){
 char *name=api.type_get_name(type);if(!name)return '?';
 char result='?';
 if(!strcmp(name,"System.Void"))result='V';
 else if(!strcmp(name,"System.Boolean"))result='B';
 else if(!strcmp(name,"System.Int32")||!strcmp(name,"System.UInt32"))result='I';
 else if(!strcmp(name,"System.Int64")||!strcmp(name,"System.UInt64")||!strcmp(name,"System.IntPtr")||!strcmp(name,"System.UIntPtr")||strchr(name,'&'))result='X';
 else {
  bool fixed8=!strcmp(name,"TrueSync.FP")||!strcmp(name,"TrueSync.FDP");
  void *klass=api.class_from_type(type);
  if(klass){
   bool value=api.class_is_valuetype(klass);uint32_t align=0;
   if(!value&&!fixed8)result='X';
   else if(value&&fixed8&&api.class_value_size(klass,&align)==8&&align<=8)result='X';
  }
 }
 api.free(name);return result;
}
static bool native_shape(const void *method,char shape[16]){
 uint32_t n=api.method_get_param_count(method);if(n>9)return false;
 size_t out=0;shape[out++]=native_type_abi(api.method_get_return_type(method));
 if(shape[0]!='V'&&shape[0]!='B')return false;
 if(!(api.method_get_flags(method,NULL)&0x10))shape[out++]='X';
 for(uint32_t i=0;i<n;i++){
  char argument=native_type_abi(api.method_get_param(method,i));
  if(argument=='?'||argument=='V')return false;
  shape[out++]=argument;
 }
 shape[out++]='X'; /* trailing MethodInfo */
 shape[out]='\0';return true;
}
static bool native_parameters_safe(const void *method,int abi){
 static const char *expected[]={"BXXIIXXXXBBX","VXX","VXIBX","VXIX","VXXX","BXXXXX","VXXBXX","BXX","VXXX"};
 char shape[16];return abi>=0&&abi<9&&native_shape(method,shape)&&!strcmp(shape,expected[abi]);
}
static bool signature_equal(const void *a,const void *b){
 if(a==b)return true;
 char x[16],y[16];return native_shape(a,x)&&native_shape(b,y)&&!strcmp(x,y);
}
