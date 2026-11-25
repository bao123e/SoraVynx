/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <commctrl.h>
#define SV_KEY_COUNT 12
#define SV_KEY_FIRST 20
struct SvKey {UINT key,mod;};
static struct SvKey sv_keys[SV_KEY_COUNT];
static unsigned sv_key_registered;
static bool sv_keys_editing;
static const wchar_t *sv_key_labels[SV_KEY_COUNT]={L"Show / hide window",L"Overview tab",L"Appearance tab",L"Always on top",L"Show clock",L"Show status",L"Start timer",L"Pause timer",L"Reset timer",L"Start / pause timer",L"Mark timer",L"Stop and reset"};
static void sv_key_defaults(struct SvKey *keys){
 const UINT codes[]={VK_F1,'1','2','3','4','5',VK_F6,VK_F7,VK_F8,'9',VK_F9,'0'};
 for(int i=0;i<SV_KEY_COUNT;i++){keys[i].key=codes[i];keys[i].mod=(i>=1&&i<=5)||i==9||i==11?MOD_CONTROL|MOD_ALT:0;}
}
/* Returns the offending row, or -1. The window must always have an accessible shortcut. */
static int sv_key_validate(const struct SvKey *keys){
 if(!keys[0].key)return 0;
 for(int i=0;i<SV_KEY_COUNT;i++){
  UINT k=keys[i].key,m=keys[i].mod;
  if(m&~(MOD_CONTROL|MOD_ALT|MOD_SHIFT))return i;
  if(k&&(k<8||k>254||k==VK_SHIFT||k==VK_CONTROL||k==VK_MENU||k==VK_LWIN||k==VK_RWIN))return i;
  for(int j=0;j<i;j++)if(k&&k==keys[j].key&&m==keys[j].mod)return i;
 }return -1;
}
static void sv_key_unregister(HWND owner){for(int i=0;i<SV_KEY_COUNT;i++)UnregisterHotKey(owner,SV_KEY_FIRST+i);sv_key_registered=0;}
static int sv_key_register(HWND owner){
 int failed=-1;sv_key_registered=0;
 for(int i=0;i<SV_KEY_COUNT;i++)if(sv_keys[i].key){
  if(RegisterHotKey(owner,SV_KEY_FIRST+i,sv_keys[i].mod|MOD_NOREPEAT,sv_keys[i].key))sv_key_registered|=1u<<i;
  else if(failed<0)failed=i;
 }
 return failed;
}
static bool sv_key_load(const wchar_t *path,struct SvKey *keys){
 struct SvKey parsed[SV_KEY_COUNT];
 wchar_t raw[512];GetPrivateProfileStringW(L"Hotkeys",L"Bindings",L"",raw,512,path);
 if(!raw[0])return false;
 wchar_t *cursor=raw;
 for(int i=0;i<SV_KEY_COUNT;i++){
  wchar_t *end;unsigned long key=wcstoul(cursor,&end,10);if(end==cursor||*end!=L':')return false;cursor=end+1;
  unsigned long mod=wcstoul(cursor,&end,10);if(end==cursor||*end!=(i==SV_KEY_COUNT-1?0:L','))return false;
  if(key>254||mod>7)return false;parsed[i].key=(UINT)key;parsed[i].mod=(UINT)mod;cursor=*end?end+1:end;
 }if(sv_key_validate(parsed)>=0)return false;memcpy(keys,parsed,sizeof(parsed));return true;
}
static bool sv_key_save(const wchar_t *path,const struct SvKey *keys){
 wchar_t raw[512];size_t used=0;raw[0]=0;
 for(int i=0;i<SV_KEY_COUNT;i++)used+=(size_t)swprintf(raw+used,512-used,L"%ls%u:%u",i?L",":L"",keys[i].key,keys[i].mod);
 return path[0]&&WritePrivateProfileStringW(L"Hotkeys",L"Bindings",raw,path)!=0;
}
/* Called with bindings suspended while editing. Failure leaves old choices intact. */
static int sv_key_apply(HWND owner,const wchar_t *path,const struct SvKey *pending){
 struct SvKey old[SV_KEY_COUNT];int bad=sv_key_validate(pending);if(bad>=0)return bad;
 memcpy(old,sv_keys,sizeof(old));memcpy(sv_keys,pending,sizeof(old));bad=sv_key_register(owner);
 if(bad>=0||!sv_key_save(path,pending)){
  sv_key_unregister(owner);memcpy(sv_keys,old,sizeof(old));return bad>=0?bad:SV_KEY_COUNT;
 }
 sv_key_unregister(owner);return -1;
}
static bool sv_key_down(const struct SvKey *binding){
 if(!binding->key||!(GetAsyncKeyState((int)binding->key)&0x8000))return false;
 UINT mod=0;if(GetAsyncKeyState(VK_CONTROL)&0x8000)mod|=MOD_CONTROL;if(GetAsyncKeyState(VK_MENU)&0x8000)mod|=MOD_ALT;if(GetAsyncKeyState(VK_SHIFT)&0x8000)mod|=MOD_SHIFT;
 return mod==binding->mod;
}
struct SvKeyDialog {HWND owner,controls[SV_KEY_COUNT];const wchar_t *path;bool done;};
static WORD sv_key_word(struct SvKey k){return MAKEWORD(k.key,((k.mod&MOD_CONTROL)?HOTKEYF_CONTROL:0)|((k.mod&MOD_ALT)?HOTKEYF_ALT:0)|((k.mod&MOD_SHIFT)?HOTKEYF_SHIFT:0));}
static struct SvKey sv_key_from_word(WORD word){struct SvKey k={LOBYTE(word),0};BYTE f=HIBYTE(word);if(f&HOTKEYF_CONTROL)k.mod|=MOD_CONTROL;if(f&HOTKEYF_ALT)k.mod|=MOD_ALT;if(f&HOTKEYF_SHIFT)k.mod|=MOD_SHIFT;return k;}
static void sv_key_dialog_fill(struct SvKeyDialog *d,const struct SvKey *keys){for(int i=0;i<SV_KEY_COUNT;i++)SendMessageW(d->controls[i],HKM_SETHOTKEY,sv_key_word(keys[i]),0);}
static LRESULT CALLBACK sv_key_dialog_proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
 struct SvKeyDialog *d=(struct SvKeyDialog*)GetWindowLongPtrW(hwnd,GWLP_USERDATA);
 if(message==WM_NCCREATE){d=((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(hwnd,GWLP_USERDATA,(LONG_PTR)d);}
 if(!d)return DefWindowProcW(hwnd,message,wp,lp);
 if(message==WM_COMMAND){
  int id=LOWORD(wp);
  if(id==2){DestroyWindow(hwnd);return 0;}
  if(id==3){struct SvKey defaults[SV_KEY_COUNT];sv_key_defaults(defaults);sv_key_dialog_fill(d,defaults);return 0;}
  if(id==1){
   struct SvKey pending[SV_KEY_COUNT];for(int i=0;i<SV_KEY_COUNT;i++)pending[i]=sv_key_from_word((WORD)SendMessageW(d->controls[i],HKM_GETHOTKEY,0,0));
   int bad=sv_key_validate(pending);wchar_t text[320];
   if(bad>=0){swprintf(text,320,L"Invalid or duplicate shortcut: %ls. The menu shortcut cannot be empty.",sv_key_labels[bad]);MessageBoxW(hwnd,text,L"Keybinds",MB_OK|MB_ICONWARNING);SetFocus(d->controls[bad]);return 0;}
   bad=sv_key_apply(d->owner,d->path,pending);
   if(bad>=0){
    if(bad<SV_KEY_COUNT)swprintf(text,320,L"Shortcut for %ls is unavailable or used by another app. Previous bindings are kept; choose another key.",sv_key_labels[bad]);
    else wcscpy(text,L"Could not save config.ini. Previous bindings are kept.");
    MessageBoxW(hwnd,text,L"Keybinds",MB_OK|MB_ICONWARNING);return 0;
   }
   /* Keep input suspended until the editor closes. */
   sv_key_unregister(d->owner);DestroyWindow(hwnd);return 0;
  }
 }
 if(message==WM_CLOSE){DestroyWindow(hwnd);return 0;}
 if(message==WM_DESTROY){d->done=true;return 0;}
 return DefWindowProcW(hwnd,message,wp,lp);
}
static void sv_key_show(HWND owner,HMODULE module,const wchar_t *path){
 INITCOMMONCONTROLSEX init={sizeof(init),ICC_HOTKEY_CLASS};if(!InitCommonControlsEx(&init))return;
 WNDCLASSW wc={0};wc.hInstance=module;wc.lpfnWndProc=sv_key_dialog_proc;wc.lpszClassName=L"WuwaWorkbenchKeybinds";wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&wc);
 struct SvKeyDialog d={0};d.owner=owner;d.path=path;RECT r;GetWindowRect(owner,&r);
 HWND dialog=CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_TOPMOST,wc.lpszClassName,L"Wuwa Workbench - Custom keybinds",WS_POPUP|WS_CAPTION|WS_SYSMENU,r.left+180,r.top+5,640,590,owner,NULL,module,&d);if(!dialog)return;
 HWND note=CreateWindowW(L"STATIC",L"Click a shortcut field, then press your key or Ctrl/Alt/Shift combination.\nBackspace clears a field. Save applies and remembers your choices.",WS_CHILD|WS_VISIBLE,18,16,600,40,dialog,NULL,module,NULL);SendMessageW(note,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
 for(int i=0;i<SV_KEY_COUNT;i++){
  HWND label=CreateWindowW(L"STATIC",sv_key_labels[i],WS_CHILD|WS_VISIBLE,18,68+34*i,300,26,dialog,NULL,module,NULL);SendMessageW(label,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
  d.controls[i]=CreateWindowExW(WS_EX_CLIENTEDGE,HOTKEY_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP,326,64+34*i,270,27,dialog,(HMENU)(INT_PTR)(100+i),module,NULL);SendMessageW(d.controls[i],WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
 }
 const wchar_t *names[]={L"Save",L"Cancel",L"Reset defaults"};for(int i=0;i<3;i++){HWND b=CreateWindowW(L"BUTTON",names[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP,18+198*i,500,182,34,dialog,(HMENU)(INT_PTR)(i+1),module,NULL);SendMessageW(b,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);}
 sv_key_dialog_fill(&d,sv_keys);sv_keys_editing=true;sv_key_unregister(owner);EnableWindow(owner,FALSE);ShowWindow(dialog,SW_SHOW);SetForegroundWindow(dialog);SetFocus(d.controls[0]);
 MSG msg;while(!d.done&&IsWindow(owner)){BOOL result=GetMessageW(&msg,NULL,0,0);if(result<=0){if(result==0)PostQuitMessage((int)msg.wParam);break;}if(!IsDialogMessageW(dialog,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 if(IsWindow(dialog))DestroyWindow(dialog);EnableWindow(owner,TRUE);sv_keys_editing=false;
 if(IsWindow(owner)&&!shutting_down){sv_key_register(owner);SetForegroundWindow(owner);}
}
