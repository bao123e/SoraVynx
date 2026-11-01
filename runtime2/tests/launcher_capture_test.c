#define _GNU_SOURCE
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include "../launcher_capture.h"
static struct LoaderResult *current;
static uint64_t module;
static uint64_t __attribute__((ms_abi)) load(uint64_t path){assert(current->stage==1&&path==123);return module;}
static uint32_t __attribute__((ms_abi)) error(void){assert(current->module==module);return module?0:126;}
int main(void){
 void *code=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(code!=MAP_FAILED);
 memcpy(code,loader_code,sizeof(loader_code));assert(!mprotect(code,4096,PROT_READ|PROT_EXEC));
 typedef uint32_t (__attribute__((ms_abi)) *Entry)(struct LoaderResult*);Entry entry=(Entry)code;
 struct LoaderResult r={(uintptr_t)load,(uintptr_t)error,123,0,0,0};current=&r;module=0x1234567800000000ULL;
 assert(entry(&r)==0&&r.module==module&&r.stage==2&&r.error==0);
 r.stage=0;r.module=0;module=0;assert(entry(&r)==0&&r.stage==2&&r.error==126);
 assert(!munmap(code,4096));return 0;
}
