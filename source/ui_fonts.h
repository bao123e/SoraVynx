/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once

static const wchar_t *ui_font_faces[3]={L"Segoe UI",L"Segoe UI",L"Segoe UI"};
static HANDLE ui_font_resources[3];
static void load_ui_fonts(HMODULE module) {
 bool loaded=true;
 for(int i=0;i<3;i++) {
  HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(401+i),MAKEINTRESOURCEW(10));
  HGLOBAL data=resource?LoadResource(module,resource):NULL;
  void *bytes=data?LockResource(data):NULL;DWORD count=0;
  if(bytes)ui_font_resources[i]=AddFontMemResourceEx(bytes,SizeofResource(module,resource),NULL,&count);
  if(!ui_font_resources[i]||!count)loaded=false;
 }
 if(loaded){ui_font_faces[0]=L"SSTool UI";ui_font_faces[1]=L"SSTool UI SemiBold";ui_font_faces[2]=L"SSTool UI Bold";}
}
