/* Test a published signed record with the actual owner public key and HTTPS transport. */
#include "../license_gate.h"
int main(int argc,char **argv) {
 int reject_old=argc==3&&!strcmp(argv[1],"--reject-old");
 const char *hash=reject_old?argv[2]:argc==2?argv[1]:NULL;
 if(!hash||strlen(hash)!=64||!sv_hex(hash,32,NULL))return 1;
 char receipt[SV_KEY_LENGTH+1];uint64_t issued=0,expires=0;
 if(!sv_fetch(hash,receipt)){puts("FAIL: HTTPS record lookup");return 2;}
 if(reject_old){
  if(sv_license_check(receipt,hash,sv_now(),&issued,&expires)){puts("FAIL: previous issuer still accepted");return 6;}
  if(!sv_fetch_host(L"raw.githubusercontent.com",L"/bao123e/soravynx.github.io/main/licenses/",hash,receipt)||sv_license_check(receipt,hash,sv_now(),&issued,&expires)){puts("FAIL: alternate registry or previous issuer rejection");return 7;}
  puts("PASS: HTTPS transport and previous issuer rejection. Current owner-signed record not yet supplied.");return 0;
 }
 if(!sv_license_check(receipt,hash,sv_now(),&issued,&expires)){puts("FAIL: owner signature or license time");return 3;}
 if(!sv_fetch_host(L"raw.githubusercontent.com",L"/bao123e/soravynx.github.io/main/licenses/",hash,receipt)||!sv_license_check(receipt,hash,sv_now(),&issued,&expires)){puts("FAIL: alternate HTTPS registry");return 4;}
 if(sv_license_check(receipt,hash,issued-301,&issued,&expires)||GetLastError()!=ERROR_TIME_SKEW){puts("FAIL: clock error must be reported explicitly");return 5;}
 puts("PASS: owner signature, production and alternate HTTPS registries, and clock diagnostics");return 0;
}
