/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <commdlg.h>
#include <objidl.h>
#include <gdiplus.h>

static ULONG_PTR image_decoder;
static HBITMAP custom_image;
static wchar_t background_path[32768],ui_config_path[32768];
static int background_opacity=35;
static int panel_opacity=35,button_opacity=55;
static bool background_fill=true;

static HBITMAP decode_background(const wchar_t *path) {
 if(!image_decoder||!path[0])return NULL;
 GpBitmap *bitmap=NULL;HBITMAP result=NULL;UINT width=0,height=0;
 if(GdipCreateBitmapFromFile(path,&bitmap)!=Ok||!bitmap)return NULL;
 if(GdipGetImageWidth((GpImage*)bitmap,&width)==Ok&&GdipGetImageHeight((GpImage*)bitmap,&height)==Ok&&
    width&&height&&width<=8192&&height<=8192&&(uint64_t)width*height<=32000000)
  GdipCreateHBITMAPFromBitmap(bitmap,&result,0xff09111e);
 GdipDisposeImage((GpImage*)bitmap);return result;
}
static void load_background_settings(HMODULE module) {
 GdiplusStartupInput input={0};input.GdiplusVersion=1;input.SuppressExternalCodecs=TRUE;
 if(GdiplusStartup(&image_decoder,&input,NULL)!=Ok)image_decoder=0;
 DWORD length=GetModuleFileNameW(module,ui_config_path,32768);
 if(!length||length>=32768){ui_config_path[0]=0;return;}
 wchar_t *tail=wcsrchr(ui_config_path,L'\\');
 if(!tail||(size_t)(tail-ui_config_path)+12>=32768){ui_config_path[0]=0;return;}
 wcscpy(tail+1,L"config.ini");
 GetPrivateProfileStringW(L"Appearance",L"Background",L"",background_path,32768,ui_config_path);
 UINT opacity=GetPrivateProfileIntW(L"Appearance",L"Opacity",35,ui_config_path);
 background_opacity=opacity<=100?(int)opacity:35;
 opacity=GetPrivateProfileIntW(L"Appearance",L"PanelOpacity",35,ui_config_path);panel_opacity=opacity<=100?(int)opacity:35;
 opacity=GetPrivateProfileIntW(L"Appearance",L"ButtonOpacity",55,ui_config_path);button_opacity=opacity<=100?(int)opacity:55;
 background_fill=GetPrivateProfileIntW(L"Appearance",L"Fill",1,ui_config_path)!=0;
 custom_image=decode_background(background_path);
}
static void save_background_settings(HWND owner) {
 wchar_t value[16],panel[16],button[16];swprintf(value,16,L"%d",background_opacity);
 swprintf(panel,16,L"%d",panel_opacity);swprintf(button,16,L"%d",button_opacity);
 if(!ui_config_path[0]||
    !WritePrivateProfileStringW(L"Appearance",L"Background",background_path,ui_config_path)||
    !WritePrivateProfileStringW(L"Appearance",L"Opacity",value,ui_config_path)||
    !WritePrivateProfileStringW(L"Appearance",L"PanelOpacity",panel,ui_config_path)||
    !WritePrivateProfileStringW(L"Appearance",L"ButtonOpacity",button,ui_config_path)||
    !WritePrivateProfileStringW(L"Appearance",L"Fill",background_fill?L"1":L"0",ui_config_path))
  MessageBoxW(owner,L"Appearance changed for this session, but config.ini could not be saved. Move SoraVynx to a writable folder.",L"SoraVynx",MB_OK|MB_ICONWARNING);
}
static bool choose_background(HWND owner) {
 if(!image_decoder){MessageBoxW(owner,L"Windows could not initialize the image decoder.",L"SoraVynx",MB_OK|MB_ICONERROR);return false;}
 wchar_t path[32768]=L"";
 OPENFILENAMEW dialog={0};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=owner;
 dialog.lpstrTitle=L"SoraVynx - Choose background";
 dialog.lpstrFilter=L"Images (PNG, JPG, BMP)\0*.png;*.jpg;*.jpeg;*.bmp\0\0";
 dialog.lpstrFile=path;dialog.nMaxFile=32768;
 dialog.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&dialog)) {
  if(CommDlgExtendedError())MessageBoxW(owner,L"The image picker could not be opened.",L"SoraVynx",MB_OK|MB_ICONERROR);
  return false;
 }
 HBITMAP next=decode_background(path);
 if(!next){MessageBoxW(owner,L"Cannot read this image. Choose a PNG, JPG or BMP up to 8192 pixels per side and 32 megapixels.",L"SoraVynx",MB_OK|MB_ICONERROR);return false;}
 if(custom_image)DeleteObject(custom_image);custom_image=next;wcscpy(background_path,path);
 save_background_settings(owner);return true;
}
/* Alpha-blend a color without changing text opacity or the game window. */
static void tint_rect(HDC target,RECT rect,COLORREF color,BYTE alpha) {
 HDC source=CreateCompatibleDC(target);HBITMAP pixel=CreateCompatibleBitmap(target,1,1);
 if(source&&pixel){HGDIOBJ old=SelectObject(source,pixel);SetPixel(source,0,0,color);
  BLENDFUNCTION blend={AC_SRC_OVER,0,alpha,0};
  AlphaBlend(target,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,source,0,0,1,1,blend);
  SelectObject(source,old);
 }
 if(pixel)DeleteObject(pixel);if(source)DeleteDC(source);
}
