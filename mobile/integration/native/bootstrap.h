/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include "api.h"
#include <string.h>
/* The Android domain getter lazily creates managed domain objects. Calling it
 * before il2cpp_init completes dereferences uninitialized runtime defaults.
 * Enumerating the native assembly vector does not create managed objects;
 * a loaded Hotfix image proves managed game startup has progressed past init.
 * The profiled IL2CPP domain_get_assemblies API ignores its domain argument. */
static bool bootstrap_hotfix_loaded(void){
 size_t count=0;
 const void **assemblies=api.domain_get_assemblies(NULL,&count);
 if(!assemblies||!count||count>4096)return false;
 for(size_t i=0;i<count;i++){
  const void *image=api.assembly_get_image(assemblies[i]);
  const char *name=image?api.image_get_name(image):NULL;
  if(name&&(!strcmp(name,"Hotfix.dll")||!strcmp(name,"Hotfix"))){hotfix_image=image;return true;}
 }
 return false;
}
