/* Test a published signed record with the actual owner public key and HTTPS transport. */
#include "../license_gate.h"
int main(int argc,char **argv) {
 if(argc!=2||strlen(argv[1])!=64||!sv_hex(argv[1],32,NULL))return 1;
 char receipt[SV_KEY_LENGTH+1];uint64_t issued=0,expires=0;
 if(!sv_fetch(argv[1],receipt)){puts("FAIL: HTTPS record lookup");return 2;}
 if(!sv_license_check(receipt,argv[1],sv_now(),&issued,&expires)){puts("FAIL: owner signature or license time");return 3;}
 puts("PASS: production HTTPS lookup and actual owner signature");return 0;
}
