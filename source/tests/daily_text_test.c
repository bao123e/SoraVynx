/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include <assert.h>
#include <string.h>
#include "../daily_report_text.h"
#include "../daily_report_policy.h"
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
 assert(daily_report_group("table_DailyQuest")==-1);
 assert(daily_report_group("<DailyQuest>d__12")==-1);
 assert(daily_report_group("DailyMissionController")==0);
 assert(daily_report_group("HeartlinkManager")==1);
 assert(daily_report_group("DispatchController")==2);
 assert(daily_report_group("DiscUpgradeController")==3);
 assert(daily_report_group("TrialStageController")==4);
 assert(daily_report_group("ShopRewardManager")==5);
 assert(daily_report_proto_return("Google.Protobuf.MessageParser<Nova.Hotfix.DailyQuest>"));
 assert(!daily_report_proto_return("Nova.Hotfix.DailyMissionController"));
 assert(daily_report_type_rank("DailyMissionController")<daily_report_type_rank("DailyInstanceGameplayRoom"));
 assert(daily_report_method_rank("get_Parser")==-1);
 assert(daily_report_method_rank("OnClickClaim")==0);
 assert(daily_report_method_rank("get_CurrentMission")==1);
 assert(daily_report_scan_image("Game.dll"));
 assert(daily_report_scan_image("Hotfix.dll"));
 assert(daily_report_scan_image("Assembly-CSharp.dll"));
 assert(!daily_report_scan_image("System.Net.Http.dll"));
 assert(!daily_report_scan_image("UnityEngine.CoreModule.dll"));
 assert(daily_report_group("LuaEnv")==DAILY_BRIDGE_GROUP);
 assert(daily_report_group("LuaManager")==DAILY_BRIDGE_GROUP);
 assert(daily_report_group("UIDailyMissionPanel")==DAILY_BRIDGE_GROUP);
 assert(daily_report_type_rank("LuaEnv")<daily_report_type_rank("Lua2CSharpInfo_DiscInfo"));
 assert(daily_report_method_rank("get_Global")==0);
 assert(daily_report_method_rank("DoString")==0);
 assert(daily_report_group("ObjectTranslator")==-1);
 return 0;
}
