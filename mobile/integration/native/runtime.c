/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "backend.h"
#include "controls.h"
#include <jni.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static volatile LONG phase;
static pthread_once_t once=PTHREAD_ONCE_INIT;
/* Deliberately no file, logcat, telemetry or clipboard output. */
void log_line(const char *format,...){(void)format;}
static void *initialize(void *unused){
 (void)unused;InterlockedExchange(&phase,1);
 void *module=NULL,*thread=NULL;
 for(int attempt=0;attempt<600;attempt++){
  if(!module)module=dlopen("libil2cpp.so",RTLD_NOW|RTLD_NOLOAD);
  if(module&&bind_api(module)){
   void *domain=api.domain_get();
   if(domain){
    if(!thread)thread=api.thread_attach(domain);
    if(thread){
     InterlockedExchange(&phase,2);
     if(!hotfix_image){
      size_t count=0;const void **assemblies=api.domain_get_assemblies(domain,&count);
      if(assemblies&&count<4096)for(size_t i=0;i<count;i++){
       const void *image=api.assembly_get_image(assemblies[i]);const char *name=image?api.image_get_name(image):NULL;
       if(name&&(!strcmp(name,"Hotfix.dll")||!strcmp(name,"Hotfix"))){hotfix_image=image;break;}
      }
     }
     if(hotfix_image&&initialize_backend()){
      InterlockedExchange(&phase,enabled_hook_count()?3:4);
      api.thread_detach(thread);return NULL;
     }
    }
   }
  }
  sleep(1);
 }
 if(thread)api.thread_detach(thread);
 InterlockedExchange(&phase,5);return NULL;
}
static void launch(void){pthread_t worker;if(pthread_create(&worker,NULL,initialize,NULL)==0)pthread_detach(worker);else InterlockedExchange(&phase,5);}
JNIEXPORT void JNICALL Java_com_soravynx_bridge_GameBridge_startNative(JNIEnv *env,jclass type){(void)env;(void)type;pthread_once(&once,launch);}
JNIEXPORT jstring JNICALL Java_com_soravynx_bridge_GameBridge_statusNative(JNIEnv *env,jclass type){
 (void)type;char json[2048];
 snprintf(json,sizeof(json),"{\"phase\":%d,\"hooks\":%d,\"capabilities\":[%d,%d,%d,%d,%d,%d,%d,%d,%d],\"enabled\":[%d,%d,%d,%d,%d,%d,%d,%d,%d],\"damage\":%d,\"vacuum\":%d,\"score\":%d}",
 read_atomic(&phase),read_atomic(&phase)==3?enabled_hook_count():0,
 read_atomic(&capabilities[0]),read_atomic(&capabilities[1]),read_atomic(&capabilities[2]),read_atomic(&capabilities[3]),read_atomic(&capabilities[4]),read_atomic(&capabilities[5]),
 read_atomic(&event_ready[0]),read_atomic(&event_ready[1]),read_atomic(&event_ready[2]),
 read_atomic(&feature_flags[0]),read_atomic(&feature_flags[1]),read_atomic(&feature_flags[2]),read_atomic(&feature_flags[3]),read_atomic(&feature_flags[4]),read_atomic(&feature_flags[5]),
 read_atomic(&event_flags[0]),read_atomic(&event_flags[1]),read_atomic(&event_flags[2]),
 read_atomic(&multi_count),read_atomic(&vacuum_distance),read_atomic(&event_multiplier));
 return (*env)->NewStringUTF(env,json);
}
JNIEXPORT jboolean JNICALL Java_com_soravynx_bridge_GameBridge_commandNative(JNIEnv *env,jclass type,jint kind,jint value){
 (void)env;(void)type;
 return apply_control(kind,value,read_atomic(&phase)==3)?JNI_TRUE:JNI_FALSE;
}
