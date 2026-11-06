/* Test double: no actual process code modification. */
#pragma once
static inline int DobbyHook(void *entry,void *hook,void **original){(void)entry;(void)hook;(void)original;return -1;}
