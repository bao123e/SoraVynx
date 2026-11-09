/* Native Windows CNG + DPAPI + gate checks. Test-only issuer, no game process. */
#include <stdint.h>
#include <stdio.h>
#include "fixture_public.h"
#define SV_LICENSE_CUSTOM_FETCH 1
#include "../license_gate.h"
#include "fixture_cases.h"
static char ui_record[SV_KEY_LENGTH+1];
static int sv_fetch(const char hash[65],char receipt[SV_KEY_LENGTH+1]){if(memcmp(ui_record+26,hash,64))return 0;memcpy(receipt,ui_record,SV_KEY_LENGTH+1);return 1;}
static DWORD WINAPI gate_thread(void *unused){(void)unused;return (DWORD)sv_license_require(GetModuleHandleW(NULL));}
int main(int argc,char **argv) {
 if(argc==4&&!strcmp(argv[1],"--ui")){
  char key[SV_SHORT_LENGTH+1];FILE *file=fopen(argv[2],"rb");if(!file)return 1;size_t n=fread(key,1,SV_SHORT_LENGTH,file);fclose(file);if(n!=SV_SHORT_LENGTH)return 2;key[n]=0;file=fopen(argv[3],"rb");if(!file)return 10;if(fread(ui_record,1,SV_KEY_LENGTH,file)!=SV_KEY_LENGTH){fclose(file);return 11;}fclose(file);ui_record[SV_KEY_LENGTH]=0;
  HANDLE thread=CreateThread(NULL,0,gate_thread,NULL,0,NULL);if(!thread)return 3;HWND window=NULL;
  for(int i=0;i<100&&!window;i++){window=FindWindowW(L"SoraVynxLicenseGate",NULL);if(!window)Sleep(50);}if(!window)return 4;
  SetWindowTextW(GetDlgItem(window,11),L"not-a-license");SendMessageW(window,WM_COMMAND,1,0);if(InterlockedCompareExchange(&sv_license_authorized,0,0))return 5;
  wchar_t wide[SV_SHORT_LENGTH+1];for(size_t i=0;i<=n;i++)wide[i]=(wchar_t)key[i];SetWindowTextW(GetDlgItem(window,11),wide);SendMessageW(window,WM_COMMAND,1,0);
  if(WaitForSingleObject(thread,10000)!=WAIT_OBJECT_0)return 6;DWORD result=0;GetExitCodeThread(thread,&result);CloseHandle(thread);if(result!=1||!sv_license_alive())return 7;
  InterlockedExchange(&sv_license_authorized,0);if(!sv_license_require(GetModuleHandleW(NULL)))return 8;puts("PASS: invalid key stays locked; signed key unlocks; DPAPI activation reloads");return 0;
 }
 for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
  uint64_t issued=0,expires=0;int result=sv_license_check(cases[i].key,cases[i].machine,cases[i].now,&issued,&expires);
  if(result!=cases[i].valid){fprintf(stderr,"FAIL: %s\n",cases[i].name);return 1;}
 }
 sv_license_issued=sv_now();sv_license_clock=sv_now();sv_license_expires=sv_now()-1;sv_license_started=GetTickCount64();InterlockedExchange(&sv_license_authorized,1);
 if(sv_license_alive())return 2;puts("PASS: native CNG protocol, expiry, wrong machine and tamper checks");return 0;
}
