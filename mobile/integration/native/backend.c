/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "backend.h"
#include "dobby.h"
#include <link.h>
#include <stdlib.h>
#include "signatures.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>
volatile LONG feature_flags[FEATURE_COUNT],capabilities[FEATURE_COUNT];
volatile LONG multi_count=2,vacuum_distance=3;
LONG read_atomic(volatile LONG *value){return InterlockedCompareExchange(value,0,0);}
static bool on(enum Feature f){return read_atomic(&capabilities[f])&&read_atomic(&feature_flags[f]);}

enum ABI { A_HIT, A_VOID, A_INTBOOL, A_INT, A_VALUE8, A_TOUGHNESS, A_DAMAGE, A_BOOL0, A_OBJECT };
enum TargetID { HIT_ACTOR,HIT_AREA,HIT_COMPONENT,HIT_WEAPON,HIT_BULLET,REDUCE,RESET,CLEAR,SET_ENERGY,DRIVE,TOUGHNESS,REAL_DAMAGE,UPDATE_CD,TARGET_COUNT };
struct Target {const char *type,*name,*ret;int count,abi;const char *params[9];const void *method;int group;volatile LONG observed;};
static struct Target targets[TARGET_COUNT]={
#define HIT_PARAMS {"HitBox","System.Int32","System.Int32","LogicEntity","DeterministicRaycastHit&","System.Boolean&","UnityEngine.GameObject","System.Boolean","System.Boolean"}
 {"AdventureActor","OnHitActor","System.Boolean",9,A_HIT,HIT_PARAMS,0,-1,0},
 {"AreaEffectEntity","OnHitActor","System.Boolean",9,A_HIT,HIT_PARAMS,0,-1,0},
 {"AreaEffect_AttackComponent","OnHitActor","System.Boolean",9,A_HIT,HIT_PARAMS,0,-1,0},
 {"AdventureWeapon","OnHitActor","System.Boolean",9,A_HIT,HIT_PARAMS,0,-1,0},
 {"AdventureBulletBase","OnHitActor","System.Boolean",9,A_HIT,HIT_PARAMS,0,-1,0},
#undef HIT_PARAMS
 {"PlayerSkillCd","ReduceSkillSection","System.Void",2,A_INTBOOL,{"System.Int32","System.Boolean"},0,-1,0},
 {"PlayerSkillCd","ResetUseInterval","System.Void",1,A_INT,{"System.Int32"},0,-1,0},
 {"PlayerSkillCd","ClearEnergy","System.Void",0,A_VOID,{0},0,-1,0},
 {"SkillInfo","set_currentEnergy","System.Void",1,A_VALUE8,{"TrueSync.FDP"},0,-1,0},
 {"AdventureModuleController","DriveUpdate","System.Void",0,A_VOID,{0},0,-1,0},
 {"MonsterToughnessInfo","DamageToToughness","System.Boolean",3,A_TOUGHNESS,{"AdventureActor","System.Int64","System.Collections.Generic.IList<System.Int32>"},0,-1,0},
 {"AdventureActor","RealDamage","System.Void",3,A_DAMAGE,{"System.Int64","System.Boolean","Nova.Client.Recorder.Types.Damage"},0,-1,0},
 {"PlayerSkillCd","UpdateSkillCD","System.Void",1,A_VALUE8,{"TrueSync.FP"},0,-1,0}
};
#define MAX_GROUPS 24
struct Group {void *entry;dobby_dummy_func_t original;const void *shape;int abi;bool conflict,enabled;volatile LONG entry_calls,null_methods;void *volatile sample_method;};
static struct Group groups[MAX_GROUPS];static int group_count;
static const void *total_energy,*toughness_value,*weapon_attack_id,*reset_all_cd;
static bool frame_vacuum_ready;
static volatile LONG frame_actions[FEATURE_COUNT];
static void *from_weapon_field;
static _Thread_local int hit_depth;

static void record_entry(int slot,const void *method) {
 InterlockedIncrement(&groups[slot].entry_calls);
 if(!method)InterlockedIncrement(&groups[slot].null_methods);
 else InterlockedCompareExchangePointer(&groups[slot].sample_method,(void*)method,NULL);
}

static bool matches(enum TargetID id,int slot,const void *method) {
 if(method&&targets[id].method==method&&targets[id].group==slot) {
  InterlockedIncrement(&targets[id].observed);return true;
 }
 return false;
}
static bool ready(enum TargetID id) {
 int n=targets[id].group;return targets[id].method&&n>=0&&groups[n].enabled;
}
static bool read_box(void *box,void *out,size_t n) {
 if(!box)return false;
 uint32_t align=0;int32_t size=api.class_value_size(api.object_get_class(box),&align);
 if(size!=(int32_t)n)return false;
 void *p=api.object_unbox(box);if(!p)return false;memcpy(out,p,n);return true;
}
static const void *method0(void *klass,const char *name,const char *ret) {
 return find_method(klass,name,ret,0,NULL,false);
}
static const void *method1(void *klass,const char *name,const char *ret,const char *p) {
 const char *params[]={p};return find_method(klass,name,ret,1,params,false);
}

/* All vector/value arguments below are boxed/unboxed by the runtime. No legacy
 * object layouts or fixed-point shifts are used. Executed on DriveUpdate's thread. */
struct Vacuum {
 void *controller_class,*health_field,*alive_field;
 const void *instance,*active_player,*monsters,*transform,*position,*forward,*set_position;
 const void *position_y,*set_y,*sync,*terrain,*scalar,*multiply,*addition;
};
static struct Vacuum vacuum;
static bool setup_vacuum(void) {
 void *module=find_class("","AdventureModuleController");
 vacuum.controller_class=find_class("","AdventurePlayerController");
 vacuum.instance=find_method(vacuum.controller_class,"get_Instance",NULL,0,NULL,true);
 vacuum.active_player=method0(vacuum.controller_class,"get_activedPlayerActor","LogicEntity");
 vacuum.monsters=method0(module,"get_monsterActors","System.Collections.Generic.List<LogicEntity>");
 vacuum.transform=method0(find_class("","LogicEntity"),"get_tsTransform","TrueSyncTransform");
 void *transform=find_class("","TrueSyncTransform");
 vacuum.position=method0(transform,"get_Position","TrueSync.TSVector2");
 vacuum.forward=method0(transform,"get_Forward","TrueSync.TSVector2");
 vacuum.set_position=method1(transform,"set_Position","System.Void","TrueSync.TSVector2");
 vacuum.position_y=method0(transform,"get_PositionY","TrueSync.FP");
 vacuum.set_y=method1(transform,"set_PositionY","System.Void","TrueSync.FP");
 vacuum.sync=method1(find_class("","AdventureActor"),"SyncMovementLocation","System.Void","System.Boolean");
 const char *vp[]={"TrueSync.TSVector2","TrueSync.FP"};
 vacuum.terrain=find_method(find_class("","CommonHelper"),"GetTerrainHeight","TrueSync.FP",2,vp,true);
 void *vector=find_class("TrueSync","TSVector2"),*fp=find_class("TrueSync","FP");
 vacuum.multiply=find_method(vector,"op_Multiply","TrueSync.TSVector2",2,vp,true);
 const char *vv[]={"TrueSync.TSVector2","TrueSync.TSVector2"};
 vacuum.addition=find_method(vector,"op_Addition","TrueSync.TSVector2",2,vv,true);
 const char *integer[]={"System.Int32"};
 vacuum.scalar=find_method(fp,"op_Implicit","TrueSync.FP",1,integer,true);
 vacuum.health_field=find_field(find_class("","AdventureActor"),"actorHealthInfo","ActorHealthInfo",false);
 vacuum.alive_field=find_field(find_class("","ActorHealthInfo"),"_isAlive","System.Boolean",false);
 return vacuum.controller_class&&vacuum.instance&&vacuum.active_player&&vacuum.monsters&&vacuum.transform&&
  vacuum.position&&vacuum.forward&&vacuum.set_position&&vacuum.position_y&&vacuum.set_y&&vacuum.sync&&
  vacuum.terrain&&vacuum.multiply&&vacuum.addition&&vacuum.scalar&&vacuum.health_field&&vacuum.alive_field;
}
static void vacuum_update(void *module) {
 static ULONGLONG last;ULONGLONG now=GetTickCount64();if(now-last<100)return;last=now;
 bool ok;void *controller=invoke(vacuum.instance,NULL,NULL,&ok);
 if(!ok||!controller||!api.class_is_assignable_from(vacuum.controller_class,api.object_get_class(controller)))return;
 void *player=invoke(vacuum.active_player,controller,NULL,&ok);if(!ok||!is_player(player))return;
 void *player_transform=invoke(vacuum.transform,player,NULL,&ok);if(!ok||!player_transform)return;
 void *position=invoke(vacuum.position,player_transform,NULL,&ok);if(!ok||!position)return;
 void *forward=invoke(vacuum.forward,player_transform,NULL,&ok);if(!ok||!forward)return;
 int32_t distance=(int32_t)read_atomic(&vacuum_distance);if(distance<1||distance>10)return;
 void *distance_args[]={&distance};void *scale=invoke(vacuum.scalar,NULL,distance_args,&ok);if(!ok||!scale)return;
 void *mul_args[]={api.object_unbox(forward),api.object_unbox(scale)};
 void *scaled=invoke(vacuum.multiply,NULL,mul_args,&ok);if(!ok||!scaled)return;
 void *add_args[]={api.object_unbox(position),api.object_unbox(scaled)};
 void *target=invoke(vacuum.addition,NULL,add_args,&ok);if(!ok||!target)return;
 void *list=invoke(vacuum.monsters,module,NULL,&ok);if(!ok||!list)return;
 void *list_class=api.object_get_class(list);
 const void *count_method=method0(list_class,"get_Count","System.Int32");
 const void *item_method=method1(list_class,"get_Item","LogicEntity","System.Int32");
 void *count_box=invoke(count_method,list,NULL,&ok);int32_t count;
 if(!ok||!read_box(count_box,&count,sizeof(count))||count<0||count>512||!item_method)return;
 for(int32_t i=0;i<count;i++) {
  void *args[]={&i};void *monster=invoke(item_method,list,args,&ok);if(!ok||!is_monster(monster))continue;
  void *health=NULL;bool alive=false;api.field_get_value(monster,vacuum.health_field,&health);
  if(!health)continue;
  api.field_get_value(health,vacuum.alive_field,&alive);if(!alive)continue;
  void *tr=invoke(vacuum.transform,monster,NULL,&ok);if(!ok||!tr)continue;
  void *old_y=invoke(vacuum.position_y,tr,NULL,&ok);if(!ok||!old_y)continue;
  void *height_args[]={api.object_unbox(target),api.object_unbox(old_y)};
  void *height=invoke(vacuum.terrain,NULL,height_args,&ok);if(!ok||!height)continue;
  void *pos_args[]={api.object_unbox(target)};invoke(vacuum.set_position,tr,pos_args,&ok);if(!ok)continue;
  void *y_args[]={api.object_unbox(height)};invoke(vacuum.set_y,tr,y_args,&ok);if(!ok)continue;
  bool clear=true;void *sync_args[]={&clear};invoke(vacuum.sync,monster,sync_args,&ok);
  if(ok)InterlockedIncrement(&frame_actions[MOB_VACUUM]);
 }
}

#include "event.inc"

typedef bool (*HitFn)(void*,void*,int32_t,int32_t,void*,void*,bool*,void*,bool,bool,const void*);
typedef void (*VoidFn)(void*,const void*);
typedef void (*IntBoolFn)(void*,int32_t,bool,const void*);
typedef void (*IntFn)(void*,int32_t,const void*);
typedef void (*ValueFn)(void*,uint64_t,const void*);
typedef bool (*ToughFn)(void*,void*,int64_t,void*,const void*);
typedef void (*DamageFn)(void*,int64_t,bool,void*,const void*);
static bool hit_dispatch(int slot,void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method) {
 record_entry(slot,method);
 HitFn origin=(HitFn)groups[slot].original;bool target=false;
 for(int i=HIT_ACTOR;i<=HIT_BULLET;i++)if(matches((enum TargetID)i,slot,method))target=true;
 if(target&&on(GOD_MODE)&&is_player(actor)){if(damaged)*damaged=false;return false;}
 if(!target||!on(MULTI_HIT)||hit_depth||!is_monster(actor))return origin(self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);
 int repeats=(int)read_atomic(&multi_count);if(repeats<2||repeats>50)repeats=2;
 bool result=false,any_damage=false;hit_depth++;
 for(int i=0;i<repeats;i++) {
  int32_t id=attack;
  if(i) {
   void *weapon=NULL;api.field_static_get_value(from_weapon_field,&weapon);if(!weapon)break;
   bool ok;void *id_box=invoke(weapon_attack_id,weapon,NULL,&ok);if(!ok||!read_box(id_box,&id,sizeof(id)))break;
  }
  bool current=origin(self,box,id,count,actor,ray,damaged,prefab,scaled,ignore,method);
  result=result||current;if(damaged)any_damage=any_damage||*damaged;
 }
 hit_depth--;if(damaged)*damaged=any_damage;return result;
}
static void void_dispatch(int slot,void *self,const void *method) {
 record_entry(slot,method);
 bool clear=matches(CLEAR,slot,method),drive=matches(DRIVE,slot,method);
 if(clear&&on(UNLIMITED_ENERGY))return;
 ((VoidFn)groups[slot].original)(self,method);
 if(drive&&on(MOB_VACUUM))vacuum_update(self);
}
static void intbool_dispatch(int slot,void *self,int32_t id,bool resume,const void *method) {
 record_entry(slot,method);
 if(matches(REDUCE,slot,method)&&on(NO_COOLDOWN))return;
 ((IntBoolFn)groups[slot].original)(self,id,resume,method);
}
static void event_native(const void *,void *,uint64_t *);
static void int_dispatch(int slot,void *self,int32_t id,const void *method) {
 uint64_t argument=(uint32_t)id;event_native(method,self,&argument);id=(int32_t)argument;
 record_entry(slot,method);
 if(matches(RESET,slot,method)&&on(NO_COOLDOWN))return;
 ((IntFn)groups[slot].original)(self,id,method);
}
static _Thread_local int cooldown_depth;
static void value_dispatch(int slot,void *self,uint64_t value,const void *method) {
 event_native(method,self,&value);
 record_entry(slot,method);
 if(matches(SET_ENERGY,slot,method)&&on(UNLIMITED_ENERGY)) {
  bool ok;void *box=invoke(total_energy,self,NULL,&ok);uint64_t full;
  if(ok&&read_box(box,&full,sizeof(full)))value=full;
 }
 ((ValueFn)groups[slot].original)(self,value,method);
 if(matches(UPDATE_CD,slot,method)&&on(NO_COOLDOWN)&&!cooldown_depth){bool ok;cooldown_depth++;invoke(reset_all_cd,self,NULL,&ok);cooldown_depth--;}
}
static bool toughness_dispatch(int slot,void *self,void *from,int64_t damage,void *tags,const void *method) {
 record_entry(slot,method);
 if(matches(TOUGHNESS,slot,method)&&on(INSTANT_BREAK)&&damage>0&&is_player(from)) {
  bool ok;void *box=invoke(toughness_value,self,NULL,&ok);int64_t current;
  if(ok&&read_box(box,&current,sizeof(current))&&current>damage)damage=current;
 }
 return ((ToughFn)groups[slot].original)(self,from,damage,tags,method);
}
static void damage_dispatch(int slot,void *self,int64_t damage,bool flag,void *data,const void *method) {
 record_entry(slot,method);
 if(matches(REAL_DAMAGE,slot,method)&&on(GOD_MODE)&&is_player(self))return;
 ((DamageFn)groups[slot].original)(self,damage,flag,data,method);
}


/* Typed method-entry hooks only. No Windows offsets or assumed interpreter frame ABI. */
static _Thread_local int event_depth;
static int event_groups[4]={-1,-1,-1,-1};
static const void *event_methods[4];
static void event_native(const void *method,void *self,uint64_t *value) {
 if(!method||event_depth)return;
 bool matched=false;
 for(int i=0;i<4;i++)if(event_methods[i]==method&&event_groups[i]>=0&&groups[event_groups[i]].enabled)matched=true;
 if(!matched)return;
 uint64_t slots[2]={(uintptr_t)self,value?*value:0};
 event_depth++;event_frame(method,slots);event_depth--;
 if(value)*value=slots[1];
}
static bool bool0_dispatch(int slot,void *self,const void *method){
 event_native(method,self,NULL);return ((bool(*)(void*,const void*))groups[slot].original)(self,method);
}
static void object_dispatch(int slot,void *self,void *object,const void *method){
 uint64_t value=(uintptr_t)object;event_native(method,self,&value);
 ((void(*)(void*,void*,const void*))groups[slot].original)(self,object,method);
}
static bool hit_0(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(0,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_1(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(1,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_2(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(2,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_3(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(3,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_4(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(4,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_5(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(5,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_6(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(6,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_7(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(7,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_8(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(8,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_9(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(9,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_10(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(10,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_11(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(11,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_12(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(12,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_13(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(13,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_14(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(14,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_15(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(15,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_16(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(16,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_17(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(17,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_18(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(18,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_19(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(19,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_20(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(20,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_21(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(21,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_22(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(22,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static bool hit_23(void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method){return hit_dispatch(23,self,box,attack,count,actor,ray,damaged,prefab,scaled,ignore,method);}
static void void_0(void *self,const void *method){void_dispatch(0,self,method);}
static void void_1(void *self,const void *method){void_dispatch(1,self,method);}
static void void_2(void *self,const void *method){void_dispatch(2,self,method);}
static void void_3(void *self,const void *method){void_dispatch(3,self,method);}
static void void_4(void *self,const void *method){void_dispatch(4,self,method);}
static void void_5(void *self,const void *method){void_dispatch(5,self,method);}
static void void_6(void *self,const void *method){void_dispatch(6,self,method);}
static void void_7(void *self,const void *method){void_dispatch(7,self,method);}
static void void_8(void *self,const void *method){void_dispatch(8,self,method);}
static void void_9(void *self,const void *method){void_dispatch(9,self,method);}
static void void_10(void *self,const void *method){void_dispatch(10,self,method);}
static void void_11(void *self,const void *method){void_dispatch(11,self,method);}
static void void_12(void *self,const void *method){void_dispatch(12,self,method);}
static void void_13(void *self,const void *method){void_dispatch(13,self,method);}
static void void_14(void *self,const void *method){void_dispatch(14,self,method);}
static void void_15(void *self,const void *method){void_dispatch(15,self,method);}
static void void_16(void *self,const void *method){void_dispatch(16,self,method);}
static void void_17(void *self,const void *method){void_dispatch(17,self,method);}
static void void_18(void *self,const void *method){void_dispatch(18,self,method);}
static void void_19(void *self,const void *method){void_dispatch(19,self,method);}
static void void_20(void *self,const void *method){void_dispatch(20,self,method);}
static void void_21(void *self,const void *method){void_dispatch(21,self,method);}
static void void_22(void *self,const void *method){void_dispatch(22,self,method);}
static void void_23(void *self,const void *method){void_dispatch(23,self,method);}
static void intbool_0(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(0,self,id,resume,method);}
static void intbool_1(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(1,self,id,resume,method);}
static void intbool_2(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(2,self,id,resume,method);}
static void intbool_3(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(3,self,id,resume,method);}
static void intbool_4(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(4,self,id,resume,method);}
static void intbool_5(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(5,self,id,resume,method);}
static void intbool_6(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(6,self,id,resume,method);}
static void intbool_7(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(7,self,id,resume,method);}
static void intbool_8(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(8,self,id,resume,method);}
static void intbool_9(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(9,self,id,resume,method);}
static void intbool_10(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(10,self,id,resume,method);}
static void intbool_11(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(11,self,id,resume,method);}
static void intbool_12(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(12,self,id,resume,method);}
static void intbool_13(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(13,self,id,resume,method);}
static void intbool_14(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(14,self,id,resume,method);}
static void intbool_15(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(15,self,id,resume,method);}
static void intbool_16(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(16,self,id,resume,method);}
static void intbool_17(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(17,self,id,resume,method);}
static void intbool_18(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(18,self,id,resume,method);}
static void intbool_19(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(19,self,id,resume,method);}
static void intbool_20(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(20,self,id,resume,method);}
static void intbool_21(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(21,self,id,resume,method);}
static void intbool_22(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(22,self,id,resume,method);}
static void intbool_23(void *self,int32_t id,bool resume,const void *method){intbool_dispatch(23,self,id,resume,method);}
static void int_0(void *self,int32_t id,const void *method){int_dispatch(0,self,id,method);}
static void int_1(void *self,int32_t id,const void *method){int_dispatch(1,self,id,method);}
static void int_2(void *self,int32_t id,const void *method){int_dispatch(2,self,id,method);}
static void int_3(void *self,int32_t id,const void *method){int_dispatch(3,self,id,method);}
static void int_4(void *self,int32_t id,const void *method){int_dispatch(4,self,id,method);}
static void int_5(void *self,int32_t id,const void *method){int_dispatch(5,self,id,method);}
static void int_6(void *self,int32_t id,const void *method){int_dispatch(6,self,id,method);}
static void int_7(void *self,int32_t id,const void *method){int_dispatch(7,self,id,method);}
static void int_8(void *self,int32_t id,const void *method){int_dispatch(8,self,id,method);}
static void int_9(void *self,int32_t id,const void *method){int_dispatch(9,self,id,method);}
static void int_10(void *self,int32_t id,const void *method){int_dispatch(10,self,id,method);}
static void int_11(void *self,int32_t id,const void *method){int_dispatch(11,self,id,method);}
static void int_12(void *self,int32_t id,const void *method){int_dispatch(12,self,id,method);}
static void int_13(void *self,int32_t id,const void *method){int_dispatch(13,self,id,method);}
static void int_14(void *self,int32_t id,const void *method){int_dispatch(14,self,id,method);}
static void int_15(void *self,int32_t id,const void *method){int_dispatch(15,self,id,method);}
static void int_16(void *self,int32_t id,const void *method){int_dispatch(16,self,id,method);}
static void int_17(void *self,int32_t id,const void *method){int_dispatch(17,self,id,method);}
static void int_18(void *self,int32_t id,const void *method){int_dispatch(18,self,id,method);}
static void int_19(void *self,int32_t id,const void *method){int_dispatch(19,self,id,method);}
static void int_20(void *self,int32_t id,const void *method){int_dispatch(20,self,id,method);}
static void int_21(void *self,int32_t id,const void *method){int_dispatch(21,self,id,method);}
static void int_22(void *self,int32_t id,const void *method){int_dispatch(22,self,id,method);}
static void int_23(void *self,int32_t id,const void *method){int_dispatch(23,self,id,method);}
static void value_0(void *self,uint64_t value,const void *method){value_dispatch(0,self,value,method);}
static void value_1(void *self,uint64_t value,const void *method){value_dispatch(1,self,value,method);}
static void value_2(void *self,uint64_t value,const void *method){value_dispatch(2,self,value,method);}
static void value_3(void *self,uint64_t value,const void *method){value_dispatch(3,self,value,method);}
static void value_4(void *self,uint64_t value,const void *method){value_dispatch(4,self,value,method);}
static void value_5(void *self,uint64_t value,const void *method){value_dispatch(5,self,value,method);}
static void value_6(void *self,uint64_t value,const void *method){value_dispatch(6,self,value,method);}
static void value_7(void *self,uint64_t value,const void *method){value_dispatch(7,self,value,method);}
static void value_8(void *self,uint64_t value,const void *method){value_dispatch(8,self,value,method);}
static void value_9(void *self,uint64_t value,const void *method){value_dispatch(9,self,value,method);}
static void value_10(void *self,uint64_t value,const void *method){value_dispatch(10,self,value,method);}
static void value_11(void *self,uint64_t value,const void *method){value_dispatch(11,self,value,method);}
static void value_12(void *self,uint64_t value,const void *method){value_dispatch(12,self,value,method);}
static void value_13(void *self,uint64_t value,const void *method){value_dispatch(13,self,value,method);}
static void value_14(void *self,uint64_t value,const void *method){value_dispatch(14,self,value,method);}
static void value_15(void *self,uint64_t value,const void *method){value_dispatch(15,self,value,method);}
static void value_16(void *self,uint64_t value,const void *method){value_dispatch(16,self,value,method);}
static void value_17(void *self,uint64_t value,const void *method){value_dispatch(17,self,value,method);}
static void value_18(void *self,uint64_t value,const void *method){value_dispatch(18,self,value,method);}
static void value_19(void *self,uint64_t value,const void *method){value_dispatch(19,self,value,method);}
static void value_20(void *self,uint64_t value,const void *method){value_dispatch(20,self,value,method);}
static void value_21(void *self,uint64_t value,const void *method){value_dispatch(21,self,value,method);}
static void value_22(void *self,uint64_t value,const void *method){value_dispatch(22,self,value,method);}
static void value_23(void *self,uint64_t value,const void *method){value_dispatch(23,self,value,method);}
static bool toughness_0(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(0,self,from,damage,tags,method);}
static bool toughness_1(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(1,self,from,damage,tags,method);}
static bool toughness_2(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(2,self,from,damage,tags,method);}
static bool toughness_3(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(3,self,from,damage,tags,method);}
static bool toughness_4(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(4,self,from,damage,tags,method);}
static bool toughness_5(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(5,self,from,damage,tags,method);}
static bool toughness_6(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(6,self,from,damage,tags,method);}
static bool toughness_7(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(7,self,from,damage,tags,method);}
static bool toughness_8(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(8,self,from,damage,tags,method);}
static bool toughness_9(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(9,self,from,damage,tags,method);}
static bool toughness_10(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(10,self,from,damage,tags,method);}
static bool toughness_11(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(11,self,from,damage,tags,method);}
static bool toughness_12(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(12,self,from,damage,tags,method);}
static bool toughness_13(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(13,self,from,damage,tags,method);}
static bool toughness_14(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(14,self,from,damage,tags,method);}
static bool toughness_15(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(15,self,from,damage,tags,method);}
static bool toughness_16(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(16,self,from,damage,tags,method);}
static bool toughness_17(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(17,self,from,damage,tags,method);}
static bool toughness_18(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(18,self,from,damage,tags,method);}
static bool toughness_19(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(19,self,from,damage,tags,method);}
static bool toughness_20(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(20,self,from,damage,tags,method);}
static bool toughness_21(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(21,self,from,damage,tags,method);}
static bool toughness_22(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(22,self,from,damage,tags,method);}
static bool toughness_23(void *self,void *from,int64_t damage,void *tags,const void *method){return toughness_dispatch(23,self,from,damage,tags,method);}
static void damage_0(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(0,self,damage,flag,data,method);}
static void damage_1(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(1,self,damage,flag,data,method);}
static void damage_2(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(2,self,damage,flag,data,method);}
static void damage_3(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(3,self,damage,flag,data,method);}
static void damage_4(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(4,self,damage,flag,data,method);}
static void damage_5(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(5,self,damage,flag,data,method);}
static void damage_6(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(6,self,damage,flag,data,method);}
static void damage_7(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(7,self,damage,flag,data,method);}
static void damage_8(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(8,self,damage,flag,data,method);}
static void damage_9(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(9,self,damage,flag,data,method);}
static void damage_10(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(10,self,damage,flag,data,method);}
static void damage_11(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(11,self,damage,flag,data,method);}
static void damage_12(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(12,self,damage,flag,data,method);}
static void damage_13(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(13,self,damage,flag,data,method);}
static void damage_14(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(14,self,damage,flag,data,method);}
static void damage_15(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(15,self,damage,flag,data,method);}
static void damage_16(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(16,self,damage,flag,data,method);}
static void damage_17(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(17,self,damage,flag,data,method);}
static void damage_18(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(18,self,damage,flag,data,method);}
static void damage_19(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(19,self,damage,flag,data,method);}
static void damage_20(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(20,self,damage,flag,data,method);}
static void damage_21(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(21,self,damage,flag,data,method);}
static void damage_22(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(22,self,damage,flag,data,method);}
static void damage_23(void *self,int64_t damage,bool flag,void *data,const void *method){damage_dispatch(23,self,damage,flag,data,method);}
static bool bool0_0(void *self,const void *method){return bool0_dispatch(0,self,method);}
static bool bool0_1(void *self,const void *method){return bool0_dispatch(1,self,method);}
static bool bool0_2(void *self,const void *method){return bool0_dispatch(2,self,method);}
static bool bool0_3(void *self,const void *method){return bool0_dispatch(3,self,method);}
static bool bool0_4(void *self,const void *method){return bool0_dispatch(4,self,method);}
static bool bool0_5(void *self,const void *method){return bool0_dispatch(5,self,method);}
static bool bool0_6(void *self,const void *method){return bool0_dispatch(6,self,method);}
static bool bool0_7(void *self,const void *method){return bool0_dispatch(7,self,method);}
static bool bool0_8(void *self,const void *method){return bool0_dispatch(8,self,method);}
static bool bool0_9(void *self,const void *method){return bool0_dispatch(9,self,method);}
static bool bool0_10(void *self,const void *method){return bool0_dispatch(10,self,method);}
static bool bool0_11(void *self,const void *method){return bool0_dispatch(11,self,method);}
static bool bool0_12(void *self,const void *method){return bool0_dispatch(12,self,method);}
static bool bool0_13(void *self,const void *method){return bool0_dispatch(13,self,method);}
static bool bool0_14(void *self,const void *method){return bool0_dispatch(14,self,method);}
static bool bool0_15(void *self,const void *method){return bool0_dispatch(15,self,method);}
static bool bool0_16(void *self,const void *method){return bool0_dispatch(16,self,method);}
static bool bool0_17(void *self,const void *method){return bool0_dispatch(17,self,method);}
static bool bool0_18(void *self,const void *method){return bool0_dispatch(18,self,method);}
static bool bool0_19(void *self,const void *method){return bool0_dispatch(19,self,method);}
static bool bool0_20(void *self,const void *method){return bool0_dispatch(20,self,method);}
static bool bool0_21(void *self,const void *method){return bool0_dispatch(21,self,method);}
static bool bool0_22(void *self,const void *method){return bool0_dispatch(22,self,method);}
static bool bool0_23(void *self,const void *method){return bool0_dispatch(23,self,method);}
static void object_0(void *self,void *object,const void *method){object_dispatch(0,self,object,method);}
static void object_1(void *self,void *object,const void *method){object_dispatch(1,self,object,method);}
static void object_2(void *self,void *object,const void *method){object_dispatch(2,self,object,method);}
static void object_3(void *self,void *object,const void *method){object_dispatch(3,self,object,method);}
static void object_4(void *self,void *object,const void *method){object_dispatch(4,self,object,method);}
static void object_5(void *self,void *object,const void *method){object_dispatch(5,self,object,method);}
static void object_6(void *self,void *object,const void *method){object_dispatch(6,self,object,method);}
static void object_7(void *self,void *object,const void *method){object_dispatch(7,self,object,method);}
static void object_8(void *self,void *object,const void *method){object_dispatch(8,self,object,method);}
static void object_9(void *self,void *object,const void *method){object_dispatch(9,self,object,method);}
static void object_10(void *self,void *object,const void *method){object_dispatch(10,self,object,method);}
static void object_11(void *self,void *object,const void *method){object_dispatch(11,self,object,method);}
static void object_12(void *self,void *object,const void *method){object_dispatch(12,self,object,method);}
static void object_13(void *self,void *object,const void *method){object_dispatch(13,self,object,method);}
static void object_14(void *self,void *object,const void *method){object_dispatch(14,self,object,method);}
static void object_15(void *self,void *object,const void *method){object_dispatch(15,self,object,method);}
static void object_16(void *self,void *object,const void *method){object_dispatch(16,self,object,method);}
static void object_17(void *self,void *object,const void *method){object_dispatch(17,self,object,method);}
static void object_18(void *self,void *object,const void *method){object_dispatch(18,self,object,method);}
static void object_19(void *self,void *object,const void *method){object_dispatch(19,self,object,method);}
static void object_20(void *self,void *object,const void *method){object_dispatch(20,self,object,method);}
static void object_21(void *self,void *object,const void *method){object_dispatch(21,self,object,method);}
static void object_22(void *self,void *object,const void *method){object_dispatch(22,self,object,method);}
static void object_23(void *self,void *object,const void *method){object_dispatch(23,self,object,method);}
static dobby_dummy_func_t wrappers[9][MAX_GROUPS]={
{(dobby_dummy_func_t)hit_0,(dobby_dummy_func_t)hit_1,(dobby_dummy_func_t)hit_2,(dobby_dummy_func_t)hit_3,(dobby_dummy_func_t)hit_4,(dobby_dummy_func_t)hit_5,(dobby_dummy_func_t)hit_6,(dobby_dummy_func_t)hit_7,(dobby_dummy_func_t)hit_8,(dobby_dummy_func_t)hit_9,(dobby_dummy_func_t)hit_10,(dobby_dummy_func_t)hit_11,(dobby_dummy_func_t)hit_12,(dobby_dummy_func_t)hit_13,(dobby_dummy_func_t)hit_14,(dobby_dummy_func_t)hit_15,(dobby_dummy_func_t)hit_16,(dobby_dummy_func_t)hit_17,(dobby_dummy_func_t)hit_18,(dobby_dummy_func_t)hit_19,(dobby_dummy_func_t)hit_20,(dobby_dummy_func_t)hit_21,(dobby_dummy_func_t)hit_22,(dobby_dummy_func_t)hit_23},
{(dobby_dummy_func_t)void_0,(dobby_dummy_func_t)void_1,(dobby_dummy_func_t)void_2,(dobby_dummy_func_t)void_3,(dobby_dummy_func_t)void_4,(dobby_dummy_func_t)void_5,(dobby_dummy_func_t)void_6,(dobby_dummy_func_t)void_7,(dobby_dummy_func_t)void_8,(dobby_dummy_func_t)void_9,(dobby_dummy_func_t)void_10,(dobby_dummy_func_t)void_11,(dobby_dummy_func_t)void_12,(dobby_dummy_func_t)void_13,(dobby_dummy_func_t)void_14,(dobby_dummy_func_t)void_15,(dobby_dummy_func_t)void_16,(dobby_dummy_func_t)void_17,(dobby_dummy_func_t)void_18,(dobby_dummy_func_t)void_19,(dobby_dummy_func_t)void_20,(dobby_dummy_func_t)void_21,(dobby_dummy_func_t)void_22,(dobby_dummy_func_t)void_23},
{(dobby_dummy_func_t)intbool_0,(dobby_dummy_func_t)intbool_1,(dobby_dummy_func_t)intbool_2,(dobby_dummy_func_t)intbool_3,(dobby_dummy_func_t)intbool_4,(dobby_dummy_func_t)intbool_5,(dobby_dummy_func_t)intbool_6,(dobby_dummy_func_t)intbool_7,(dobby_dummy_func_t)intbool_8,(dobby_dummy_func_t)intbool_9,(dobby_dummy_func_t)intbool_10,(dobby_dummy_func_t)intbool_11,(dobby_dummy_func_t)intbool_12,(dobby_dummy_func_t)intbool_13,(dobby_dummy_func_t)intbool_14,(dobby_dummy_func_t)intbool_15,(dobby_dummy_func_t)intbool_16,(dobby_dummy_func_t)intbool_17,(dobby_dummy_func_t)intbool_18,(dobby_dummy_func_t)intbool_19,(dobby_dummy_func_t)intbool_20,(dobby_dummy_func_t)intbool_21,(dobby_dummy_func_t)intbool_22,(dobby_dummy_func_t)intbool_23},
{(dobby_dummy_func_t)int_0,(dobby_dummy_func_t)int_1,(dobby_dummy_func_t)int_2,(dobby_dummy_func_t)int_3,(dobby_dummy_func_t)int_4,(dobby_dummy_func_t)int_5,(dobby_dummy_func_t)int_6,(dobby_dummy_func_t)int_7,(dobby_dummy_func_t)int_8,(dobby_dummy_func_t)int_9,(dobby_dummy_func_t)int_10,(dobby_dummy_func_t)int_11,(dobby_dummy_func_t)int_12,(dobby_dummy_func_t)int_13,(dobby_dummy_func_t)int_14,(dobby_dummy_func_t)int_15,(dobby_dummy_func_t)int_16,(dobby_dummy_func_t)int_17,(dobby_dummy_func_t)int_18,(dobby_dummy_func_t)int_19,(dobby_dummy_func_t)int_20,(dobby_dummy_func_t)int_21,(dobby_dummy_func_t)int_22,(dobby_dummy_func_t)int_23},
{(dobby_dummy_func_t)value_0,(dobby_dummy_func_t)value_1,(dobby_dummy_func_t)value_2,(dobby_dummy_func_t)value_3,(dobby_dummy_func_t)value_4,(dobby_dummy_func_t)value_5,(dobby_dummy_func_t)value_6,(dobby_dummy_func_t)value_7,(dobby_dummy_func_t)value_8,(dobby_dummy_func_t)value_9,(dobby_dummy_func_t)value_10,(dobby_dummy_func_t)value_11,(dobby_dummy_func_t)value_12,(dobby_dummy_func_t)value_13,(dobby_dummy_func_t)value_14,(dobby_dummy_func_t)value_15,(dobby_dummy_func_t)value_16,(dobby_dummy_func_t)value_17,(dobby_dummy_func_t)value_18,(dobby_dummy_func_t)value_19,(dobby_dummy_func_t)value_20,(dobby_dummy_func_t)value_21,(dobby_dummy_func_t)value_22,(dobby_dummy_func_t)value_23},
{(dobby_dummy_func_t)toughness_0,(dobby_dummy_func_t)toughness_1,(dobby_dummy_func_t)toughness_2,(dobby_dummy_func_t)toughness_3,(dobby_dummy_func_t)toughness_4,(dobby_dummy_func_t)toughness_5,(dobby_dummy_func_t)toughness_6,(dobby_dummy_func_t)toughness_7,(dobby_dummy_func_t)toughness_8,(dobby_dummy_func_t)toughness_9,(dobby_dummy_func_t)toughness_10,(dobby_dummy_func_t)toughness_11,(dobby_dummy_func_t)toughness_12,(dobby_dummy_func_t)toughness_13,(dobby_dummy_func_t)toughness_14,(dobby_dummy_func_t)toughness_15,(dobby_dummy_func_t)toughness_16,(dobby_dummy_func_t)toughness_17,(dobby_dummy_func_t)toughness_18,(dobby_dummy_func_t)toughness_19,(dobby_dummy_func_t)toughness_20,(dobby_dummy_func_t)toughness_21,(dobby_dummy_func_t)toughness_22,(dobby_dummy_func_t)toughness_23},
{(dobby_dummy_func_t)damage_0,(dobby_dummy_func_t)damage_1,(dobby_dummy_func_t)damage_2,(dobby_dummy_func_t)damage_3,(dobby_dummy_func_t)damage_4,(dobby_dummy_func_t)damage_5,(dobby_dummy_func_t)damage_6,(dobby_dummy_func_t)damage_7,(dobby_dummy_func_t)damage_8,(dobby_dummy_func_t)damage_9,(dobby_dummy_func_t)damage_10,(dobby_dummy_func_t)damage_11,(dobby_dummy_func_t)damage_12,(dobby_dummy_func_t)damage_13,(dobby_dummy_func_t)damage_14,(dobby_dummy_func_t)damage_15,(dobby_dummy_func_t)damage_16,(dobby_dummy_func_t)damage_17,(dobby_dummy_func_t)damage_18,(dobby_dummy_func_t)damage_19,(dobby_dummy_func_t)damage_20,(dobby_dummy_func_t)damage_21,(dobby_dummy_func_t)damage_22,(dobby_dummy_func_t)damage_23},
{(dobby_dummy_func_t)bool0_0,(dobby_dummy_func_t)bool0_1,(dobby_dummy_func_t)bool0_2,(dobby_dummy_func_t)bool0_3,(dobby_dummy_func_t)bool0_4,(dobby_dummy_func_t)bool0_5,(dobby_dummy_func_t)bool0_6,(dobby_dummy_func_t)bool0_7,(dobby_dummy_func_t)bool0_8,(dobby_dummy_func_t)bool0_9,(dobby_dummy_func_t)bool0_10,(dobby_dummy_func_t)bool0_11,(dobby_dummy_func_t)bool0_12,(dobby_dummy_func_t)bool0_13,(dobby_dummy_func_t)bool0_14,(dobby_dummy_func_t)bool0_15,(dobby_dummy_func_t)bool0_16,(dobby_dummy_func_t)bool0_17,(dobby_dummy_func_t)bool0_18,(dobby_dummy_func_t)bool0_19,(dobby_dummy_func_t)bool0_20,(dobby_dummy_func_t)bool0_21,(dobby_dummy_func_t)bool0_22,(dobby_dummy_func_t)bool0_23},
{(dobby_dummy_func_t)object_0,(dobby_dummy_func_t)object_1,(dobby_dummy_func_t)object_2,(dobby_dummy_func_t)object_3,(dobby_dummy_func_t)object_4,(dobby_dummy_func_t)object_5,(dobby_dummy_func_t)object_6,(dobby_dummy_func_t)object_7,(dobby_dummy_func_t)object_8,(dobby_dummy_func_t)object_9,(dobby_dummy_func_t)object_10,(dobby_dummy_func_t)object_11,(dobby_dummy_func_t)object_12,(dobby_dummy_func_t)object_13,(dobby_dummy_func_t)object_14,(dobby_dummy_func_t)object_15,(dobby_dummy_func_t)object_16,(dobby_dummy_func_t)object_17,(dobby_dummy_func_t)object_18,(dobby_dummy_func_t)object_19,(dobby_dummy_func_t)object_20,(dobby_dummy_func_t)object_21,(dobby_dummy_func_t)object_22,(dobby_dummy_func_t)object_23},
};

struct ExecQuery {uintptr_t address;bool valid;};
static int executable_segment(struct dl_phdr_info *info,size_t size,void *opaque){
 (void)size;struct ExecQuery *q=opaque;
 const char *name=strrchr(info->dlpi_name,'/');name=name?name+1:info->dlpi_name;
 if(strcmp(name,"libil2cpp.so"))return 0;
 for(int i=0;i<info->dlpi_phnum;i++) {
  const ElfW(Phdr) *h=&info->dlpi_phdr[i];uintptr_t begin=info->dlpi_addr+h->p_vaddr;
  if(h->p_type==PT_LOAD&&(h->p_flags&PF_X)&&q->address>=begin&&q->address<begin+h->p_memsz){q->valid=true;return 1;}
 }
 return 0;
}
static bool executable(void *entry){struct ExecQuery q={(uintptr_t)entry,false};dl_iterate_phdr(executable_segment,&q);return q.valid;}
static int add_group(const void *method,int abi) {
 if(!method||!native_parameters_safe(method))return -1;
 void *entry=*(void *const*)method;if(!entry||!executable(entry))return -1;
 int g;
 for(g=0;g<group_count;g++)if(groups[g].entry==entry)break;
 if(g==group_count){if(g>=MAX_GROUPS)return -1;groups[g].entry=entry;groups[g].abi=abi;groups[g].shape=method;group_count++;}
 else if(groups[g].abi!=abi||!signature_equal(groups[g].shape,method))groups[g].conflict=true;
 return g;
}
/* Check all currently loaded metadata for shared entries, not just our target list.
 * Conflicting or unbounded enumeration disables every uncertain hook. */
static bool audit_shared_entries(void){
 size_t count=0,total_classes=0,total_methods=0;
 const void **assemblies=api.domain_get_assemblies(api.domain_get(),&count);
 if(!assemblies||count>4096)return false;
 for(size_t a=0;a<count;a++){
  const void *image=api.assembly_get_image(assemblies[a]);if(!image)return false;
  size_t classes=api.image_get_class_count(image);
  if(classes>200000||classes>200000-total_classes)return false;
  total_classes+=classes;
  for(size_t c=0;c<classes;c++){
   void *klass=api.image_get_class(image,c);if(!klass)return false;
   void *iterator=NULL;const void *method;
   while((method=api.class_get_methods(klass,&iterator))){
    if(++total_methods>2000000)return false;
    void *entry=*(void *const*)method;if(!entry)continue;
    for(int g=0;g<group_count;g++)if(!groups[g].conflict&&groups[g].entry==entry){
     if(!signature_equal(groups[g].shape,method))groups[g].conflict=true;
    }
   }
  }
 }
 return true;
}
static bool event_ready_method(int i){int g=event_groups[i];return g>=0&&groups[g].enabled;}
int enabled_hook_count(void){int n=0;for(int g=0;g<group_count;g++)n+=groups[g].enabled;return n;}
bool initialize_backend(void){
 player_class=find_class("","PlayerAdventureActor");monster_class=find_class("","MonsterAdventureActor");
 if(!player_class||!monster_class)return false;
 total_energy=method0(find_class("","SkillInfo"),"get_totalEnergy","TrueSync.FDP");
 toughness_value=method0(find_class("","MonsterToughnessInfo"),"get_toughness","System.Int64");
 weapon_attack_id=method0(find_class("","AdventureWeapon"),"get_uniqueAttackId","System.Int32");
 from_weapon_field=find_field(find_class("","AdventureActor"),"fromWeaponTemp","AdventureWeapon",true);
 reset_all_cd=method0(find_class("","PlayerSkillCd"),"ResetAllSkillCD","System.Void");
 frame_vacuum_ready=setup_vacuum();setup_event();
 for(int i=0;i<TARGET_COUNT;i++){
  struct Target *t=&targets[i];t->group=-1;
  t->method=find_method(find_class("",t->type),t->name,t->ret,t->count,t->params,false);
  if(i==SET_ENERGY&&t->method){uint32_t alignment;void *k=api.class_from_type(api.method_get_param(t->method,0));if(!k||api.class_value_size(k,&alignment)!=8)t->method=NULL;}
  t->group=add_group(t->method,t->abi);
 }
 event_methods[0]=event.full;event_methods[1]=event.sync;event_methods[2]=event.update;event_methods[3]=event.add_score;
 const int abis[]={A_BOOL0,A_OBJECT,A_VALUE8,A_INT};
 for(int i=0;i<4;i++)event_groups[i]=add_group(event_methods[i],abis[i]);
 if(!audit_shared_entries())for(int g=0;g<group_count;g++)groups[g].conflict=true;
 for(int g=0;g<group_count;g++){
  if(groups[g].conflict)continue;
  if(DobbyHook(groups[g].entry,wrappers[groups[g].abi][g],&groups[g].original)==0&&groups[g].original)groups[g].enabled=true;
 }
 bool hits=true;for(int i=HIT_ACTOR;i<=HIT_BULLET;i++)hits=hits&&ready((enum TargetID)i);
 InterlockedExchange(&capabilities[INSTANT_BREAK],ready(TOUGHNESS)&&toughness_value);
 InterlockedExchange(&capabilities[GOD_MODE],ready(REAL_DAMAGE));
 InterlockedExchange(&capabilities[MULTI_HIT],hits&&weapon_attack_id&&from_weapon_field);
 InterlockedExchange(&capabilities[NO_COOLDOWN],ready(UPDATE_CD)&&reset_all_cd);
 InterlockedExchange(&capabilities[UNLIMITED_ENERGY],ready(SET_ENERGY)&&ready(CLEAR)&&total_energy);
 InterlockedExchange(&capabilities[MOB_VACUUM],ready(DRIVE)&&frame_vacuum_ready);
 bool tick=event_ready_method(0)&&event_ready_method(1)&&event_ready_method(2);
 InterlockedExchange(&event_ready[EV_CD],read_atomic(&event_ready[EV_CD])&&tick);
 InterlockedExchange(&event_ready[EV_ENERGY],read_atomic(&event_ready[EV_ENERGY])&&tick);
 InterlockedExchange(&event_ready[EV_SCORE],read_atomic(&event_ready[EV_SCORE])&&event_ready_method(3));
 return true;
}
