/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
typedef int32_t LONG;
typedef uint64_t ULONGLONG;
typedef void *HMODULE;
static inline LONG InterlockedCompareExchange(volatile LONG *p,LONG n,LONG old) {
 __atomic_compare_exchange_n(p,&old,n,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);return old;
}
static inline LONG InterlockedExchange(volatile LONG *p,LONG n){return __atomic_exchange_n(p,n,__ATOMIC_SEQ_CST);}
static inline LONG InterlockedIncrement(volatile LONG *p){return __atomic_add_fetch(p,1,__ATOMIC_SEQ_CST);}
static inline void *InterlockedCompareExchangePointer(void *volatile *p,void *n,void *old){
 __atomic_compare_exchange_n(p,&old,n,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);return old;
}
static inline ULONGLONG GetTickCount64(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
