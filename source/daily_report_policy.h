/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <string.h>
#include "daily_report_text.h"

enum { DAILY_REPORT_GROUPS=6 };
static const char *const daily_group_labels[DAILY_REPORT_GROUPS]={
 "Missions and daily progress","Home, Heartlink and login","Dispatch commissions",
 "Trekker and Disc upgrades","Battles, trials and towers","Shop and rewards"
};
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
 for(int g=0;g<DAILY_REPORT_GROUPS;g++)for(int w=0;words[g][w];w++)if(daily_name_matches(name,words[g][w]))return g;
 return -1;
}
static int daily_report_type_rank(const char *name) {
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
 for(int i=0;ignored[i];i++)if(!strcmp(name,ignored[i]))return -1;
 for(int i=0;actions[i];i++)if(daily_name_matches(name,actions[i]))return 0;
 if(!strncmp(name,"get_",4)||daily_name_matches(name,"instance")||daily_name_matches(name,"current")||daily_name_matches(name,"progress"))return 1;
 return 2;
}
