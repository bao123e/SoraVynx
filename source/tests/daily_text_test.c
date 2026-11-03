/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include <assert.h>
#include <string.h>
#include "../daily_report_text.h"
int main(void) {
 assert(daily_name_matches("DailyMissionController","daily"));
 assert(daily_name_matches("DAILY_MISSIONS","mission"));
 assert(!daily_name_matches("DiscController","daily"));
 assert(!daily_name_matches(NULL,"daily"));
 assert(!daily_name_matches("","daily"));
 assert(!daily_name_matches("Daily",""));
 struct {char before;char text[8];char after;} output={'A',"",'Z'};
 size_t used=0;daily_append(output.text,sizeof(output.text),&used,"%s","123");assert(used==3);
 daily_append(output.text,sizeof(output.text),&used,"%s","456789long");assert(used==7&&!strcmp(output.text,"1234567"));
 daily_append(output.text,sizeof(output.text),&used,"overflow");assert(used==7);
 assert(output.before=='A'&&output.after=='Z');
 char tiny[1]={0};used=0;daily_append(tiny,1,&used,"abc");assert(!tiny[0]&&used==0);
 daily_append(tiny,0,&used,"abc");assert(!tiny[0]&&used==0);
 return 0;
}
