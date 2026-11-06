/* Test double: no actual process code modification. */
#pragma once
typedef void (*dobby_dummy_func_t)(void);
static inline int DobbyHook(void *entry,dobby_dummy_func_t hook,dobby_dummy_func_t *original){(void)entry;(void)hook;(void)original;return -1;}
