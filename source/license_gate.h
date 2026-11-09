/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Offline, per-machine P-256 signatures. No embedded signing secret or log files.
 * This is a first-stage local license gate, not online payment/revocation enforcement. */
#ifndef SORAVYNX_LICENSE_GATE_H
#define SORAVYNX_LICENSE_GATE_H
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#ifndef SORAVYNX_ISSUER_HEADER
#define SORAVYNX_ISSUER_HEADER "license_public_key.h"
#endif
#include SORAVYNX_ISSUER_HEADER
#define SV_KEY_LENGTH 252
static volatile LONG sv_license_authorized;
static uint64_t sv_license_issued,sv_license_expires,sv_license_started,sv_license_clock;
static char sv_machine[65];
static int sv_sha256(const void *data,ULONG length,unsigned char result[32]) {
 BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;int ok=0;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)<0)goto done;
 if(BCryptHashData(hash,(PUCHAR)data,length,0)<0)goto done;
 ok=BCryptFinishHash(hash,result,32,0)>=0;
 done:if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);return ok;
}
static uint64_t sv_now(void){FILETIME t;ULARGE_INTEGER n;GetSystemTimeAsFileTime(&t);n.LowPart=t.dwLowDateTime;n.HighPart=t.dwHighDateTime;return n.QuadPart/10000000ULL-11644473600ULL;}
static int sv_hex_value(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
static int sv_hex(const char *src,size_t count,unsigned char *dst){for(size_t i=0;i<count;i++){int a=sv_hex_value(src[2*i]),b=sv_hex_value(src[2*i+1]);if(a<0||b<0)return 0;if(dst)dst[i]=(unsigned char)((a<<4)|b);}return 1;}
static int sv_decimal(const char *s,uint64_t *n){*n=0;for(int i=0;i<10;i++){if(s[i]<'0'||s[i]>'9')return 0;*n=*n*10+(unsigned)(s[i]-'0');}return 1;}
static int sv_device(char out[65]) {
 wchar_t guid[128];DWORD bytes=sizeof(guid);unsigned char digest[32];
 if(RegGetValueW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Cryptography",L"MachineGuid",RRF_RT_REG_SZ|RRF_SUBKEY_WOW6464KEY,NULL,guid,&bytes)!=ERROR_SUCCESS)return 0;
 size_t n=wcsnlen(guid,128);if(!n||n>=128)return 0;
 wchar_t salted[180]=L"SoraVynx-License-v1|";wcscat(salted,guid);
 if(!sv_sha256(salted,(ULONG)(wcslen(salted)*sizeof(wchar_t)),digest))return 0;
 for(int i=0;i<32;i++)snprintf(out+2*i,3,"%02x",digest[i]);SecureZeroMemory(guid,sizeof(guid));return 1;
}
/* Strict fixed-format parser; never trust expiry or machine fields before signature verification. */
static int sv_license_check(const char *key,const char *device,uint64_t now,uint64_t *issued_out,uint64_t *expires_out) {
 if(!SV_LICENSE_CONFIGURED||strlen(key)!=SV_KEY_LENGTH||strncmp(key,"SV1.",4)||key[14]!='.'||key[25]!='.'||key[90]!='.'||key[123]!='.')return 0;
 uint64_t issued,expires;unsigned char signature[64],digest[32];
 if(!sv_decimal(key+4,&issued)||!sv_decimal(key+15,&expires)||!sv_hex(key+26,32,NULL)||!sv_hex(key+91,16,NULL)||!sv_hex(key+124,64,signature))return 0;
 struct {BCRYPT_ECCKEY_BLOB header;unsigned char xy[64];} blob={{BCRYPT_ECDSA_PUBLIC_P256_MAGIC,32},{0}};
 memcpy(blob.xy,sv_license_public_xy,64);
 BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_KEY_HANDLE public_key=NULL;int valid=0;
 if(!sv_sha256(key,123,digest)||BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_ECDSA_P256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptImportKeyPair(algorithm,NULL,BCRYPT_ECCPUBLIC_BLOB,&public_key,(PUCHAR)&blob,sizeof(blob),0)<0)goto done;
 if(BCryptVerifySignature(public_key,NULL,digest,32,signature,64,0)<0)goto done;
 if(memcmp(key+26,device,64)||issued>now+300||(expires&&expires<=issued)||(expires&&now>=expires))goto done;
 *issued_out=issued;*expires_out=expires;valid=1;
 done:if(public_key)BCryptDestroyKey(public_key);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);return valid;
}
static int sv_license_alive(void) {
 if(!InterlockedCompareExchange(&sv_license_authorized,0,0))return 0;
 uint64_t now=sv_now(),elapsed=(GetTickCount64()-sv_license_started)/1000;
 if(now+300<sv_license_issued||now+300<sv_license_clock||(sv_license_expires&&(now>=sv_license_expires||elapsed>=sv_license_expires-sv_license_clock))) {InterlockedExchange(&sv_license_authorized,0);return 0;}
 return 1;
}
static int sv_cache_path(wchar_t path[MAX_PATH]) {
 DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",path,MAX_PATH);if(!n||n+32>=MAX_PATH)return 0;
 wcscat(path,L"\\SoraVynx");if(!CreateDirectoryW(path,NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
 wcscat(path,L"\\license.dat");return 1;
}
static int sv_cache(const char *key,int save,char output[SV_KEY_LENGTH+1]) {
 wchar_t path[MAX_PATH];if(!sv_cache_path(path))return 0;
 DATA_BLOB in={0},out={0};int ok=0;HANDLE file=INVALID_HANDLE_VALUE;DWORD read=0;
 if(save){
  in.pbData=(BYTE*)key;in.cbData=SV_KEY_LENGTH;
  if(!CryptProtectData(&in,L"SoraVynx license",NULL,NULL,NULL,CRYPTPROTECT_UI_FORBIDDEN,&out))goto done;
  file=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)goto done;
  ok=WriteFile(file,out.pbData,out.cbData,&read,NULL)&&read==out.cbData;
 }else{
  BYTE bytes[4096];file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)goto done;
  LARGE_INTEGER size;if(!GetFileSizeEx(file,&size)||size.QuadPart<=0||size.QuadPart>(LONGLONG)sizeof(bytes))goto done;
  if(!ReadFile(file,bytes,(DWORD)size.QuadPart,&read,NULL)||read!=(DWORD)size.QuadPart)goto done;
  in.pbData=bytes;in.cbData=read;if(!CryptUnprotectData(&in,NULL,NULL,NULL,NULL,CRYPTPROTECT_UI_FORBIDDEN,&out))goto done;
  if(out.cbData!=SV_KEY_LENGTH)goto done;memcpy(output,out.pbData,SV_KEY_LENGTH);output[SV_KEY_LENGTH]=0;ok=1;
 }
 done:if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);if(out.pbData){SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);}return ok;
}
static HWND sv_key_edit,sv_license_status;static HFONT sv_gate_font;static HINSTANCE sv_gate_module;static int sv_gate_accepted;
static void sv_copy_machine(HWND hwnd) {
 if(!OpenClipboard(hwnd))return;HGLOBAL data=GlobalAlloc(GMEM_MOVEABLE,65*sizeof(wchar_t));
 if(data){wchar_t *p=GlobalLock(data);if(p){for(int i=0;i<65;i++)p[i]=(wchar_t)sv_machine[i];GlobalUnlock(data);if(!EmptyClipboard()||!SetClipboardData(CF_UNICODETEXT,data))GlobalFree(data);}else GlobalFree(data);}CloseClipboard();
}
static LRESULT CALLBACK sv_gate_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
 if(msg==WM_COMMAND&&LOWORD(wp)==1){
  wchar_t wide[1024];char key[SV_KEY_LENGTH+1];size_t at=0;GetWindowTextW(sv_key_edit,wide,1024);
  for(size_t i=0;wide[i];i++){if(wide[i]==L' '||wide[i]==L'\r'||wide[i]==L'\n'||wide[i]==L'\t')continue;if(wide[i]>127||at>=SV_KEY_LENGTH){at=0;break;}key[at++]=(char)wide[i];}key[at]=0;
  uint64_t issued=0,expires=0,now=sv_now();
  if(!sv_license_check(key,sv_machine,now,&issued,&expires)){SetWindowTextW(sv_license_status,SV_LICENSE_CONFIGURED?L"Key invalid, expired, or issued for another machine.":L"Owner setup required: configure the issuer public key and rebuild.");SecureZeroMemory(key,sizeof(key));return 0;}
  if(!sv_cache(key,1,NULL)){SetWindowTextW(sv_license_status,L"Cannot save activation in LocalAppData. Tool stays locked.");SecureZeroMemory(key,sizeof(key));return 0;}
  SecureZeroMemory(key,sizeof(key));sv_license_issued=issued;sv_license_expires=expires;sv_license_clock=now;sv_license_started=GetTickCount64();InterlockedExchange(&sv_license_authorized,1);sv_gate_accepted=1;DestroyWindow(hwnd);return 0;
 }
 if(msg==WM_COMMAND&&LOWORD(wp)==2){sv_copy_machine(hwnd);return 0;}
 if(msg==WM_CLOSE){DestroyWindow(hwnd);return 0;}
 if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
 return DefWindowProcW(hwnd,msg,wp,lp);
}
static HWND sv_gate_control(HWND parent,const wchar_t *kind,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id) {
 HWND c=CreateWindowW(kind,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,(HMENU)(intptr_t)id,sv_gate_module,NULL);if(c)SendMessageW(c,WM_SETFONT,(WPARAM)sv_gate_font,TRUE);return c;
}
static int sv_license_require(HINSTANCE module) {
 if(!sv_device(sv_machine)){MessageBoxW(NULL,L"Cannot obtain this machine's license code. Tool stays locked.",L"SoraVynx access",MB_OK|MB_ICONERROR);return 0;}
 char cached[SV_KEY_LENGTH+1];uint64_t issued=0,expires=0,now=sv_now();
 if(sv_cache(NULL,0,cached)&&sv_license_check(cached,sv_machine,now,&issued,&expires)){
  SecureZeroMemory(cached,sizeof(cached));sv_license_issued=issued;sv_license_expires=expires;sv_license_clock=now;sv_license_started=GetTickCount64();InterlockedExchange(&sv_license_authorized,1);return 1;
 }
 SecureZeroMemory(cached,sizeof(cached));sv_gate_module=module;sv_gate_accepted=0;
 WNDCLASSW wc={0};wc.lpfnWndProc=sv_gate_proc;wc.hInstance=module;wc.lpszClassName=L"SoraVynxLicenseGate";wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
 if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return 0;
 sv_gate_font=CreateFontW(-16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 RECT r={0,0,630,365};AdjustWindowRect(&r,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE);
 HWND window=CreateWindowW(wc.lpszClassName,L"SoraVynx - Access key",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,NULL,NULL,module,NULL);
 if(!window){DeleteObject(sv_gate_font);return 0;}
 sv_gate_control(window,L"STATIC",L"SoraVynx  /  Activate your access",0,24,20,580,28,0);
 sv_gate_control(window,L"STATIC",L"Copy your machine code and send it to Discord @vsc.plus.",0,24,58,580,25,0);
 wchar_t machine[65];for(int i=0;i<65;i++)machine[i]=(wchar_t)sv_machine[i];
 sv_gate_control(window,L"EDIT",machine,WS_BORDER|ES_READONLY|ES_AUTOHSCROLL|WS_TABSTOP,24,92,442,34,10);
 sv_gate_control(window,L"BUTTON",L"Copy code",WS_TABSTOP,480,92,125,34,2);
 sv_gate_control(window,L"STATIC",L"Paste the signed access key supplied by the owner:",0,24,144,580,25,0);
 sv_key_edit=sv_gate_control(window,L"EDIT",L"",WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL|WS_TABSTOP,24,176,580,66,11);SendMessageW(sv_key_edit,EM_SETLIMITTEXT,1023,0);
 sv_license_status=sv_gate_control(window,L"STATIC",SV_LICENSE_CONFIGURED?L"No valid key: Player/Event controls remain locked.":L"Owner setup required: this build has no issuer public key.",0,24,257,580,42,0);
 sv_gate_control(window,L"BUTTON",L"Activate",WS_TABSTOP|BS_DEFPUSHBUTTON,424,310,180,38,1);
 ShowWindow(window,SW_SHOW);SetFocus(sv_key_edit);MSG msg;
 while(GetMessageW(&msg,NULL,0,0)>0){if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 DeleteObject(sv_gate_font);sv_gate_font=NULL;return sv_gate_accepted;
}
#endif
