/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

static bool daily_name_matches(const char *name,const char *word) {
 if(!name||!word||!*word)return false;
 for(;*name;name++){size_t i=0;while(word[i]&&name[i]&&tolower((unsigned char)name[i])==word[i])i++;if(!word[i])return true;}
 return false;
}
static void daily_append(char *text,size_t capacity,size_t *used,const char *format,...) {
 if(!text||!used||!format||!capacity||*used>=capacity-1)return;
 va_list args;va_start(args,format);int n=vsnprintf(text+*used,capacity-*used,format,args);va_end(args);
 if(n>0)*used+=(size_t)n<capacity-*used?(size_t)n:capacity-*used-1;
}
