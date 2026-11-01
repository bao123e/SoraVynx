/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <wchar.h>
static void normalize_path(wchar_t *path) {
 size_t n=wcslen(path),start=0;
 while(start<n&&(path[start]==L' '||path[start]==L'"'))start++;
 while(n>start&&(path[n-1]==L' '||path[n-1]==L'"'))n--;
 size_t out=0;
 for(size_t i=start;i<n;i++) {
  wchar_t c=path[i]==L'/'?L'\\':path[i];
  if(c==L'\\'&&out&&path[out-1]==L'\\'&&out!=1)continue;
  /* A doubled slash after a drive colon is also normalized. */
  if(c==L'\\'&&out==3&&path[1]==L':'&&path[2]==L'\\')continue;
  path[out++]=c;
 }
 path[out]=0;
}
