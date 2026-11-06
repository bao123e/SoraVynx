#include <assert.h>
#include <stdio.h>
#include "../integration/native/bootstrap.h"
struct RuntimeAPI api;
const void *hotfix_image;
static const void *assemblies[2]={(void*)1,(void*)2};
static size_t assembly_count;
static bool game_loaded;
static const void **enumerate(void *domain,size_t *count){assert(domain==NULL);*count=assembly_count;return assembly_count?assemblies:NULL;}
static const void *image(const void *assembly){return assembly;}
static const char *name(const void *image){return image==(void*)2&&game_loaded?"Hotfix.dll":"mscorlib.dll";}
static void *forbidden_domain_get(void){assert(!"Domain getter must not run while probing startup readiness");return NULL;}
int main(void){
 api.domain_get=forbidden_domain_get;api.domain_get_assemblies=enumerate;
 api.assembly_get_image=image;api.image_get_name=name;
 assert(!bootstrap_hotfix_loaded()&&!hotfix_image);
 assembly_count=2;assert(!bootstrap_hotfix_loaded()&&!hotfix_image);
 game_loaded=true;assert(bootstrap_hotfix_loaded()&&hotfix_image==(void*)2);
 hotfix_image=NULL;assembly_count=4097;assert(!bootstrap_hotfix_loaded()&&!hotfix_image);
 puts("PASS bootstrap waits for managed game metadata without calling the unsafe early domain getter");
}
