#pragma once
#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
typedef int32_t LONG;
typedef uint32_t DWORD;
typedef uint64_t ULONGLONG;
typedef void *HMODULE,*LPVOID;
typedef const wchar_t *LPCWSTR;
typedef const char *LPCSTR;
typedef void VOID;
#define WINAPI
#define MEM_COMMIT 0x1000
#define PAGE_GUARD 0x100
#define PAGE_EXECUTE 0x10
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
struct mbi {DWORD State,Protect;};
typedef struct mbi MEMORY_BASIC_INFORMATION;
static inline LONG InterlockedCompareExchange(volatile LONG *p,LONG value,LONG expected){__atomic_compare_exchange_n(p,&expected,value,0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);return expected;}
static inline LONG InterlockedExchange(volatile LONG *p,LONG value){return __atomic_exchange_n(p,value,__ATOMIC_SEQ_CST);}
static inline LONG InterlockedIncrement(volatile LONG *p){return __atomic_add_fetch(p,1,__ATOMIC_SEQ_CST);}
static inline void *InterlockedCompareExchangePointer(void *volatile*p,void *value,void *expected){__atomic_compare_exchange_n(p,&expected,value,0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);return expected;}
static inline ULONGLONG GetTickCount64(void){return 1000;}
static inline size_t VirtualQuery(void *entry,MEMORY_BASIC_INFORMATION *m,size_t size){(void)entry;m->State=MEM_COMMIT;m->Protect=PAGE_EXECUTE_READ;return size;}
