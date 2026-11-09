/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Short random codes resolve to P-256 signed records over HTTPS. No embedded signing secret or log files.
 * Codes are bearer credentials, not machine-bound. No signing secret in the client. */
#ifndef SORAVYNX_LICENSE_GATE_H
#define SORAVYNX_LICENSE_GATE_H
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <winhttp.h>
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
#define SV_SHORT_LENGTH 15
static int sv_short_hash(const char *key,char hex[65]) {
 if(strlen(key)!=SV_SHORT_LENGTH)return 0;
 for(int i=0;i<SV_SHORT_LENGTH;i++)if(!strchr("ABCDEFGHJKLMNPQRSTUVWXYZ23456789",key[i]))return 0;
 unsigned char digest[32];if(!sv_sha256(key,SV_SHORT_LENGTH,digest))return 0;
 for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);return 1;
}
#ifndef SV_LICENSE_CUSTOM_FETCH
static int sv_fetch_host(const wchar_t *host,const wchar_t *prefix,const char hash[65],char receipt[SV_KEY_LENGTH+1]) {
 HINTERNET session=NULL,connection=NULL,request=NULL;int ok=0;DWORD status=0,size=sizeof(status),read=0,total=0,error=ERROR_INVALID_DATA;char bytes[SV_KEY_LENGTH+8];
 wchar_t path[180];wcscpy(path,prefix);size_t offset=wcslen(path);for(int i=0;i<64;i++)path[offset+i]=(wchar_t)hash[i];wcscpy(path+offset+64,L".lic");
 session=WinHttpOpen(L"SoraVynx/1.2",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!session)goto network_error;
 WinHttpSetTimeouts(session,5000,10000,10000,10000);
 connection=WinHttpConnect(session,host,INTERNET_DEFAULT_HTTPS_PORT,0);if(!connection)goto network_error;
 request=WinHttpOpenRequest(connection,L"GET",path,NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE|WINHTTP_FLAG_REFRESH);if(!request)goto network_error;
 if(!WinHttpSendRequest(request,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request,NULL))goto network_error;
 if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX))goto network_error;
 if(status!=200){error=status==404?ERROR_FILE_NOT_FOUND:ERROR_BAD_NET_RESP;goto done;}
 do {if(total>=sizeof(bytes))goto done;if(!WinHttpReadData(request,bytes+total,(DWORD)sizeof(bytes)-total,&read))goto network_error;total+=read;}while(read);
 while(total&&(bytes[total-1]=='\n'||bytes[total-1]=='\r'))total--;
 if(total!=SV_KEY_LENGTH)goto done;memcpy(receipt,bytes,total);receipt[total]=0;ok=1;goto done;
 network_error:error=GetLastError();
 done:if(request)WinHttpCloseHandle(request);if(connection)WinHttpCloseHandle(connection);if(session)WinHttpCloseHandle(session);SetLastError(ok?ERROR_SUCCESS:error);return ok;
}
static int sv_fetch(const char hash[65],char receipt[SV_KEY_LENGTH+1]) {
 if(sv_fetch_host(L"soravynx.online",L"/licenses/",hash,receipt))return 1;
 /* Same signed registry through GitHub if the custom domain cannot be reached. */
 return sv_fetch_host(L"raw.githubusercontent.com",L"/bao123e/soravynx.github.io/main/licenses/",hash,receipt);
}
#else
static int sv_fetch(const char hash[65],char receipt[SV_KEY_LENGTH+1]);
#endif
/* Strict fixed-format parser; never trust expiry or machine fields before signature verification. */
static int sv_license_check(const char *key,const char *device,uint64_t now,uint64_t *issued_out,uint64_t *expires_out) {
 SetLastError(ERROR_INVALID_DATA);
 if(!SV_LICENSE_CONFIGURED||strlen(key)!=SV_KEY_LENGTH||strncmp(key,"SV2.",4)||key[14]!='.'||key[25]!='.'||key[90]!='.'||key[123]!='.')return 0;
 uint64_t issued,expires;unsigned char signature[64],digest[32];
 if(!sv_decimal(key+4,&issued)||!sv_decimal(key+15,&expires)||!sv_hex(key+26,32,NULL)||!sv_hex(key+91,16,NULL)||!sv_hex(key+124,64,signature))return 0;
 struct {BCRYPT_ECCKEY_BLOB header;unsigned char xy[64];} blob={{BCRYPT_ECDSA_PUBLIC_P256_MAGIC,32},{0}};
 memcpy(blob.xy,sv_license_public_xy,64);
 BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_KEY_HANDLE public_key=NULL;int valid=0,time_error=0;
 if(!sv_sha256(key,123,digest)||BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_ECDSA_P256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptImportKeyPair(algorithm,NULL,BCRYPT_ECCPUBLIC_BLOB,&public_key,(PUCHAR)&blob,sizeof(blob),0)<0)goto done;
 if(BCryptVerifySignature(public_key,NULL,digest,32,signature,64,0)<0)goto done;
 if(memcmp(key+26,device,64)||(expires&&expires<=issued))goto done;
 if(issued>now+300){time_error=1;goto done;}
 if(expires&&now>=expires){time_error=2;goto done;}
 *issued_out=issued;*expires_out=expires;valid=1;
 done:if(public_key)BCryptDestroyKey(public_key);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);SetLastError(valid?ERROR_SUCCESS:(time_error==1?ERROR_TIME_SKEW:(time_error==2?ERROR_PASSWORD_EXPIRED:ERROR_INVALID_DATA)));return valid;
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
 wcscat(path,L"\\license-short.dat");return 1;
}
static int sv_cache(const char *key,int save,char output[SV_SHORT_LENGTH+1]) {
 wchar_t path[MAX_PATH];if(!sv_cache_path(path))return 0;
 DATA_BLOB in={0},out={0};int ok=0;HANDLE file=INVALID_HANDLE_VALUE;DWORD read=0;
 if(save){
  in.pbData=(BYTE*)key;in.cbData=SV_SHORT_LENGTH;
  if(!CryptProtectData(&in,L"SoraVynx license",NULL,NULL,NULL,CRYPTPROTECT_UI_FORBIDDEN,&out))goto done;
  file=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)goto done;
  ok=WriteFile(file,out.pbData,out.cbData,&read,NULL)&&read==out.cbData;
 }else{
  BYTE bytes[4096];file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)goto done;
  LARGE_INTEGER size;if(!GetFileSizeEx(file,&size)||size.QuadPart<=0||size.QuadPart>(LONGLONG)sizeof(bytes))goto done;
  if(!ReadFile(file,bytes,(DWORD)size.QuadPart,&read,NULL)||read!=(DWORD)size.QuadPart)goto done;
  in.pbData=bytes;in.cbData=read;if(!CryptUnprotectData(&in,NULL,NULL,NULL,NULL,CRYPTPROTECT_UI_FORBIDDEN,&out))goto done;
  if(out.cbData!=SV_SHORT_LENGTH)goto done;memcpy(output,out.pbData,SV_SHORT_LENGTH);output[SV_SHORT_LENGTH]=0;ok=1;
 }
 done:if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);if(out.pbData){SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);}return ok;
}
static HWND sv_key_edit,sv_gate_button;static HFONT sv_gate_font;static HINSTANCE sv_gate_module;static int sv_gate_accepted,sv_gate_busy;
static HBRUSH sv_gate_bg,sv_gate_field;
struct SvGateResult {HWND window;uint64_t issued,expires;int valid,stage;DWORD error;char key[SV_SHORT_LENGTH+1];};
static DWORD WINAPI sv_gate_worker(void *argument) {
 struct SvGateResult *r=argument;char hash[65],receipt[SV_KEY_LENGTH+1];
 if(sv_short_hash(r->key,hash)){
  r->stage=1;
  if(sv_fetch(hash,receipt)){r->stage=2;r->valid=sv_license_check(receipt,hash,sv_now(),&r->issued,&r->expires);}
  r->error=GetLastError();
 }
 if(!PostMessageW(r->window,WM_APP+42,0,(LPARAM)r)){SecureZeroMemory(r,sizeof(*r));HeapFree(GetProcessHeap(),0,r);}return 0;
}
static LRESULT CALLBACK sv_gate_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
 if(msg==WM_COMMAND&&LOWORD(wp)==1&&!sv_gate_busy){
  wchar_t wide[32];GetWindowTextW(sv_key_edit,wide,32);char key[SV_SHORT_LENGTH+1]={0};size_t n=wcslen(wide);
  if(n!=SV_SHORT_LENGTH){SetWindowTextW(sv_gate_button,L"Key phải có 15 ký tự");return 0;}
  for(size_t i=0;i<n;i++){wchar_t c=wide[i];if(c>=L'a'&&c<=L'z')c-=L'a'-L'A';if(c>127){SetWindowTextW(sv_gate_button,L"Key không hợp lệ");return 0;}key[i]=(char)c;}
  char hash[65];if(!sv_short_hash(key,hash)){SetWindowTextW(sv_gate_button,L"Key không hợp lệ");return 0;}
  struct SvGateResult *r=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*r));if(!r)return 0;r->window=hwnd;memcpy(r->key,key,sizeof(key));
  sv_gate_busy=1;EnableWindow(sv_gate_button,FALSE);SetWindowTextW(sv_gate_button,L"Đang xác thực...");
  HANDLE thread=CreateThread(NULL,0,sv_gate_worker,r,0,NULL);if(thread)CloseHandle(thread);else{HeapFree(GetProcessHeap(),0,r);sv_gate_busy=0;EnableWindow(sv_gate_button,TRUE);SetWindowTextW(sv_gate_button,L"Thử lại");}return 0;
 }
 if(msg==WM_APP+42){
  struct SvGateResult *r=(struct SvGateResult*)lp;sv_gate_busy=0;EnableWindow(sv_gate_button,TRUE);
  if(r->valid&&sv_cache(r->key,1,NULL)){sv_license_issued=r->issued;sv_license_expires=r->expires;sv_license_clock=sv_now();sv_license_started=GetTickCount64();InterlockedExchange(&sv_license_authorized,1);sv_gate_accepted=1;}
  wchar_t message[100];
  if(r->valid)wcscpy(message,L"Không lưu được key — kiểm tra quyền ghi");
  else if(r->stage==1&&r->error==ERROR_FILE_NOT_FOUND)wcscpy(message,L"Key chưa đăng ký hoặc nhập sai");
  else if(r->stage==1)swprintf(message,100,L"Lỗi kết nối %lu — thử lại",(unsigned long)r->error);
  else if(r->error==ERROR_TIME_SKEW)wcscpy(message,L"Sai ngày giờ máy — bật đồng bộ giờ");
  else if(r->error==ERROR_PASSWORD_EXPIRED)wcscpy(message,L"Key đã hết hạn");
  else wcscpy(message,L"Key chưa khớp — tải bản tool mới");
  SecureZeroMemory(r,sizeof(*r));HeapFree(GetProcessHeap(),0,r);
  if(sv_gate_accepted)DestroyWindow(hwnd);else SetWindowTextW(sv_gate_button,message);return 0;
 }
 if(msg==WM_DRAWITEM){
  DRAWITEMSTRUCT *d=(DRAWITEMSTRUCT*)lp;if(d->CtlID==1){HBRUSH brush=CreateSolidBrush((d->itemState&ODS_DISABLED)?RGB(63,110,107):RGB(94,234,212));FillRect(d->hDC,&d->rcItem,brush);DeleteObject(brush);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,RGB(9,30,35));SelectObject(d->hDC,sv_gate_font);wchar_t text[100];GetWindowTextW(sv_gate_button,text,100);DrawTextW(d->hDC,text,-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(d->itemState&ODS_FOCUS){RECT focus=d->rcItem;InflateRect(&focus,-4,-4);DrawFocusRect(d->hDC,&focus);}return TRUE;}
 }
 if(msg==WM_CTLCOLOREDIT){SetTextColor((HDC)wp,RGB(238,245,255));SetBkColor((HDC)wp,RGB(22,36,52));return(LRESULT)sv_gate_field;}
 if(msg==WM_NCHITTEST){POINT point={(short)LOWORD(lp),(short)HIWORD(lp)};ScreenToClient(hwnd,&point);if(point.x>=24&&point.x<406&&((point.y>=25&&point.y<69)||(point.y>=91&&point.y<136)))return HTCLIENT;return HTCAPTION;}
 if(msg==WM_CLOSE||(msg==WM_COMMAND&&LOWORD(wp)==IDCANCEL)){DestroyWindow(hwnd);return 0;}
 if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
 return DefWindowProcW(hwnd,msg,wp,lp);
}
static int sv_license_require(HINSTANCE module) {
 char key[SV_SHORT_LENGTH+1],hash[65],receipt[SV_KEY_LENGTH+1];uint64_t issued=0,expires=0;
 if(sv_cache(NULL,0,key)&&sv_short_hash(key,hash)&&sv_fetch(hash,receipt)&&sv_license_check(receipt,hash,sv_now(),&issued,&expires)){
  SecureZeroMemory(key,sizeof(key));sv_license_issued=issued;sv_license_expires=expires;sv_license_clock=sv_now();sv_license_started=GetTickCount64();InterlockedExchange(&sv_license_authorized,1);return 1;
 }
 SecureZeroMemory(key,sizeof(key));sv_gate_module=module;sv_gate_accepted=0;sv_gate_busy=0;
 sv_gate_bg=CreateSolidBrush(RGB(9,17,30));sv_gate_field=CreateSolidBrush(RGB(22,36,52));
 WNDCLASSW wc={0};wc.lpfnWndProc=sv_gate_proc;wc.hInstance=module;wc.lpszClassName=L"SoraVynxLicenseGate";wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hbrBackground=sv_gate_bg;
 if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return 0;
 sv_gate_font=CreateFontW(-17,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 int width=430,height=164;int x=(GetSystemMetrics(SM_CXSCREEN)-width)/2,y=(GetSystemMetrics(SM_CYSCREEN)-height)/2;
 HWND window=CreateWindowExW(WS_EX_APPWINDOW,wc.lpszClassName,L"SoraVynx",WS_POPUP|WS_SYSMENU,x,y,width,height,NULL,NULL,module,NULL);if(!window)return 0;
 HRGN shape=CreateRoundRectRgn(0,0,width+1,height+1,20,20);if(shape&&!SetWindowRgn(window,shape,FALSE))DeleteObject(shape);
 sv_key_edit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_CENTER|ES_AUTOHSCROLL|WS_TABSTOP,24,25,382,44,window,(HMENU)(intptr_t)11,module,NULL);
 SendMessageW(sv_key_edit,WM_SETFONT,(WPARAM)sv_gate_font,TRUE);SendMessageW(sv_key_edit,EM_SETLIMITTEXT,15,0);SendMessageW(sv_key_edit,0x1501,TRUE,(LPARAM)L"Nhập key truy cập");
 sv_gate_button=CreateWindowW(L"BUTTON",L"Vào tool",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,24,91,382,45,window,(HMENU)(intptr_t)1,module,NULL);SendMessageW(sv_gate_button,WM_SETFONT,(WPARAM)sv_gate_font,TRUE);
 ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(sv_key_edit);MSG msg;
 while(GetMessageW(&msg,NULL,0,0)>0){if(msg.message==WM_KEYDOWN&&msg.wParam==VK_RETURN){SendMessageW(window,WM_COMMAND,1,0);continue;}if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 DeleteObject(sv_gate_font);sv_gate_font=NULL;return sv_gate_accepted;
}
#endif
