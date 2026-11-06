/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include "backend.h"
static bool apply_control(int kind,int value,bool connected){
 if(kind==-1){for(int i=0;i<FEATURE_COUNT;i++)InterlockedExchange(&feature_flags[i],0);for(int i=0;i<3;i++)InterlockedExchange(&event_flags[i],0);return true;}
 if(!connected)return false;
 if(kind>=0&&kind<9){
  if(value!=0&&value!=1)return false;
  volatile LONG *ready_flag=kind<6?&capabilities[kind]:&event_ready[kind-6];
  if(value&&!read_atomic(ready_flag))return false;
  InterlockedExchange(kind<6?&feature_flags[kind]:&event_flags[kind-6],value);return true;
 }
 if(kind==10&&value>=2&&value<=50){InterlockedExchange(&multi_count,value);return true;}
 if(kind==11&&value>=1&&value<=10){InterlockedExchange(&vacuum_distance,value);return true;}
 if(kind==12&&value>=2&&value<=100){InterlockedExchange(&event_multiplier,value);return true;}
 return false;
}
