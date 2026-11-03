/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <string.h>
#include "daily_report_text.h"

enum { DAILY_REPORT_GROUPS=7, DAILY_BRIDGE_GROUP=6 };
static const char *const daily_group_labels[DAILY_REPORT_GROUPS]={
 "Missions and daily progress","Home, Heartlink and login","Dispatch commissions",
 "Trekker and Disc upgrades","Battles, trials and towers","Shop and rewards","Lua and UI infrastructure"
};
static bool daily_report_scan_image(const char *name) {
 if(!name)return false;
 if(daily_name_matches(name,"game")||daily_name_matches(name,"hotfix")||daily_name_matches(name,"assembly-csharp")||daily_name_matches(name,"xlua"))return true;
 const char *const framework[]={"System","Unity","mscorlib","netstandard","Google","Newtonsoft","Mono","Microsoft","Cysharp","UniTask","Spine","TMPro","TextMeshPro","Accessibility","HybridCLR",NULL};
 for(int i=0;framework[i];i++)if(!strncmp(name,framework[i],strlen(framework[i])))return false;
 return true;
}
static int daily_report_group(const char *name) {
 const char *const words[][12]={
  {"daily","mission","quest","task","affair",NULL},
  {"heartlink","affinity","gift","friend","home","lobby","maininterface","interact","login","signin","checkin",NULL},
  {"dispatch","commission","expedition",NULL},
  {"trekker","disc","character","upgrade","enhance","train","record",NULL},
  {"trial","ascension","stage","dungeon","battle","tower","vigor",NULL},
  {"shop","reward","mail","purchase",NULL}
 };
 if(!name||!name[0]||name[0]=='<'||!strncmp(name,"table_",6))return -1;
 if(daily_name_matches(name,"lua")||daily_name_matches(name,"uimanager")||daily_name_matches(name,"uimgr")||daily_name_matches(name,"uiroot")||daily_name_matches(name,"windowmanager")||daily_name_matches(name,"panelmanager"))return DAILY_BRIDGE_GROUP;
 if(!strncmp(name,"UI",2)&&(daily_name_matches(name,"panel")||daily_name_matches(name,"window")||daily_name_matches(name,"controller")||daily_name_matches(name,"module")))return DAILY_BRIDGE_GROUP;
 for(int g=0;g<DAILY_BRIDGE_GROUP;g++)for(int w=0;words[g][w];w++)if(daily_name_matches(name,words[g][w]))return g;
 return -1;
}
static int daily_report_type_rank(const char *name) {
 if(!strcmp(name,"LuaEnv"))return -8;
 if(!strcmp(name,"LuaTable"))return -7;
 if(!strcmp(name,"LuaFunction"))return -6;
 if(!strcmp(name,"LuaBase"))return -5;
 if(!strcmp(name,"Lua"))return -4;
 if(daily_name_matches(name,"lua")&&!daily_name_matches(name,"lua2csharp")&&!daily_name_matches(name,"csharp2lua")&&!daily_name_matches(name,"wrap")&&!daily_name_matches(name,"delegate"))return -1;
 int extra=daily_name_matches(name,"activity")&&!daily_name_matches(name,"daily")?4:0;
 const char *const logic[]={"controller","manager","module","service","system","proxy",NULL};
 const char *const ui[]={"view","panel","window","wnd","page","screen","dialog","mediator",NULL};
 for(int i=0;logic[i];i++)if(daily_name_matches(name,logic[i]))return extra;
 for(int i=0;ui[i];i++)if(daily_name_matches(name,ui[i]))return extra+1;
 if(daily_name_matches(name,"model")||daily_name_matches(name,"state")||daily_name_matches(name,"data"))return extra+2;
 return extra+3;
}
static bool daily_report_proto_return(const char *type) {
 return daily_name_matches(type,"google.protobuf.");
}
static int daily_report_method_rank(const char *name) {
 const char *const ignored[]={".ctor",".cctor","Clone","Equals","GetHashCode","ToString","WriteTo","CalculateSize","MergeFrom","get_Parser","get_Descriptor","pb::Google.Protobuf.IMessage.get_Descriptor",NULL};
 const char *const actions[]={"click","claim","receive","request","send","start","enter","open","show","sweep","upgrade","enhance","dispatch","commission","select","reward","login","signin","checkin","getmission","getquest","getdaily","gettask",NULL};
 if(!name||!name[0]||name[0]=='<')return -1;
 const char *const bridge[]={"get_Global","get_L","get_RawL","GetKeys","Get","ForEach","DoString","LoadString",NULL};
 for(int i=0;bridge[i];i++)if(!strcmp(name,bridge[i]))return 0;
 for(int i=0;ignored[i];i++)if(!strcmp(name,ignored[i]))return -1;
 for(int i=0;actions[i];i++)if(daily_name_matches(name,actions[i]))return 0;
 if(!strncmp(name,"get_",4)||daily_name_matches(name,"instance")||daily_name_matches(name,"current")||daily_name_matches(name,"progress"))return 1;
 return 2;
}
