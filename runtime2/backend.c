/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "backend.h"
#include "vendor/minhook/include/MinHook.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>
volatile LONG feature_flags[FEATURE_COUNT],capabilities[FEATURE_COUNT];
volatile LONG multi_count=2,vacuum_distance=3;
LONG read_atomic(volatile LONG *value){return InterlockedCompareExchange(value,0,0);}
static bool on(enum Feature f){return read_atomic(&capabilities[f])&&read_atomic(&feature_flags[f]);}

enum ABI { A_HIT, A_VOID, A_INTBOOL, A_INT, A_VALUE8, A_TOUGHNESS, A_DAMAGE };
enum TargetID { HIT_ACTOR,HIT_AREA,HIT_COMPONENT,HIT_WEAPON,HIT_BULLET,REDUCE,RESET,CLEAR,SET_ENERGY,DRIVE,TOUGHNESS,REAL_DAMAGE,TARGET_COUNT };
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
 {"AdventureActor","RealDamage","System.Void",3,A_DAMAGE,{"System.Int64","System.Boolean","Damage"},0,-1,0}
};
#define MAX_GROUPS 16
struct Group {void *entry,*original;int abi;bool conflict,enabled;};
static struct Group groups[MAX_GROUPS];static int group_count;
static const void *total_energy,*toughness_value,*weapon_attack_id;
static void *from_weapon_field;
static _Thread_local int hit_depth;

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
  if(!health)continue;api.field_get_value(health,vacuum.alive_field,&alive);if(!alive)continue;
  void *tr=invoke(vacuum.transform,monster,NULL,&ok);if(!ok||!tr)continue;
  void *old_y=invoke(vacuum.position_y,tr,NULL,&ok);if(!ok||!old_y)continue;
  void *height_args[]={api.object_unbox(target),api.object_unbox(old_y)};
  void *height=invoke(vacuum.terrain,NULL,height_args,&ok);if(!ok||!height)continue;
  void *pos_args[]={api.object_unbox(target)};invoke(vacuum.set_position,tr,pos_args,&ok);if(!ok)continue;
  void *y_args[]={api.object_unbox(height)};invoke(vacuum.set_y,tr,y_args,&ok);if(!ok)continue;
  bool clear=true;void *sync_args[]={&clear};invoke(vacuum.sync,monster,sync_args,&ok);
 }
}

typedef bool (*HitFn)(void*,void*,int32_t,int32_t,void*,void*,bool*,void*,bool,bool,const void*);
typedef void (*VoidFn)(void*,const void*);
typedef void (*IntBoolFn)(void*,int32_t,bool,const void*);
typedef void (*IntFn)(void*,int32_t,const void*);
typedef void (*ValueFn)(void*,uint64_t,const void*);
typedef bool (*ToughFn)(void*,void*,int64_t,void*,const void*);
typedef void (*DamageFn)(void*,int64_t,bool,void*,const void*);
static bool hit_dispatch(int slot,void *self,void *box,int32_t attack,int32_t count,void *actor,void *ray,bool *damaged,void *prefab,bool scaled,bool ignore,const void *method) {
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
 bool clear=matches(CLEAR,slot,method),drive=matches(DRIVE,slot,method);
 if(clear&&on(UNLIMITED_ENERGY))return;
 ((VoidFn)groups[slot].original)(self,method);
 if(drive&&on(MOB_VACUUM))vacuum_update(self);
}
static void intbool_dispatch(int slot,void *self,int32_t id,bool resume,const void *method) {
 if(matches(REDUCE,slot,method)&&on(NO_COOLDOWN))return;
 ((IntBoolFn)groups[slot].original)(self,id,resume,method);
}
static void int_dispatch(int slot,void *self,int32_t id,const void *method) {
 if(matches(RESET,slot,method)&&on(NO_COOLDOWN))return;
 ((IntFn)groups[slot].original)(self,id,method);
}
static void value_dispatch(int slot,void *self,uint64_t value,const void *method) {
 if(matches(SET_ENERGY,slot,method)&&on(UNLIMITED_ENERGY)) {
  bool ok;void *box=invoke(total_energy,self,NULL,&ok);uint64_t full;
  if(ok&&read_box(box,&full,sizeof(full)))value=full;
 }
 ((ValueFn)groups[slot].original)(self,value,method);
}
static bool toughness_dispatch(int slot,void *self,void *from,int64_t damage,void *tags,const void *method) {
 if(matches(TOUGHNESS,slot,method)&&on(INSTANT_BREAK)&&damage>0&&is_player(from)) {
  bool ok;void *box=invoke(toughness_value,self,NULL,&ok);int64_t current;
  if(ok&&read_box(box,&current,sizeof(current))&&current>damage)damage=current;
 }
 return ((ToughFn)groups[slot].original)(self,from,damage,tags,method);
}
static void damage_dispatch(int slot,void *self,int64_t damage,bool flag,void *data,const void *method) {
 if(matches(REAL_DAMAGE,slot,method)&&on(GOD_MODE)&&is_player(self))return;
 ((DamageFn)groups[slot].original)(self,damage,flag,data,method);
}

/* Per-entry wrappers preserve a known trampoline even if MethodInfo is NULL or
 * the intercepted entry is a shared interpreter thunk for unrelated methods. */
#define SLOT(n) \
 static bool hit##n(void*a,void*b,int32_t c,int32_t d,void*e,void*f,bool*g,void*h,bool i,bool j,const void*k){return hit_dispatch(n,a,b,c,d,e,f,g,h,i,j,k);} \
 static void vo##n(void*a,const void*b){void_dispatch(n,a,b);} \
 static void ib##n(void*a,int32_t b,bool c,const void*d){intbool_dispatch(n,a,b,c,d);} \
 static void in##n(void*a,int32_t b,const void*c){int_dispatch(n,a,b,c);} \
 static void va##n(void*a,uint64_t b,const void*c){value_dispatch(n,a,b,c);} \
 static bool to##n(void*a,void*b,int64_t c,void*d,const void*e){return toughness_dispatch(n,a,b,c,d,e);} \
 static void da##n(void*a,int64_t b,bool c,void*d,const void*e){damage_dispatch(n,a,b,c,d,e);}
SLOT(0) SLOT(1) SLOT(2) SLOT(3) SLOT(4) SLOT(5) SLOT(6) SLOT(7)
SLOT(8) SLOT(9) SLOT(10) SLOT(11) SLOT(12) SLOT(13) SLOT(14) SLOT(15)
#undef SLOT
#define HANDLERS(prefix) {(void*)prefix##0,(void*)prefix##1,(void*)prefix##2,(void*)prefix##3,(void*)prefix##4,(void*)prefix##5,(void*)prefix##6,(void*)prefix##7,(void*)prefix##8,(void*)prefix##9,(void*)prefix##10,(void*)prefix##11,(void*)prefix##12,(void*)prefix##13,(void*)prefix##14,(void*)prefix##15}
static void *handlers[7][MAX_GROUPS]={HANDLERS(hit),HANDLERS(vo),HANDLERS(ib),HANDLERS(in),HANDLERS(va),HANDLERS(to),HANDLERS(da)};
#undef HANDLERS
static bool executable(void *entry) {
 MEMORY_BASIC_INFORMATION m;
 if(!entry||!VirtualQuery(entry,&m,sizeof(m))||m.State!=MEM_COMMIT||m.Protect&PAGE_GUARD)return false;
 return (m.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
}
bool initialize_backend(void) {
 player_class=find_class("","PlayerAdventureActor");monster_class=find_class("","MonsterAdventureActor");
 if(!player_class||!monster_class){log_line("Player/monster class not found; no hooks installed.");return false;}
 void *skill=find_class("","SkillInfo"),*tough=find_class("","MonsterToughnessInfo");
 total_energy=method0(skill,"get_totalEnergy","TrueSync.FDP");
 toughness_value=method0(tough,"get_toughness","System.Int64");
 weapon_attack_id=method0(find_class("","AdventureWeapon"),"get_uniqueAttackId","System.Int32");
 from_weapon_field=find_field(find_class("","AdventureActor"),"fromWeaponTemp","AdventureWeapon",true);
 bool vacuum_supported=setup_vacuum();
 for(int i=0;i<TARGET_COUNT;i++) {
  struct Target *t=&targets[i];void *klass=find_class("",t->type);
  t->method=find_method(klass,t->name,t->ret,t->count,t->params,false);
  if(!t->method){log_line("UNAVAILABLE %s.%s: signature not found",t->type,t->name);continue;}
  if(i==SET_ENERGY) {
   const void *value_type=api.method_get_param(t->method,0);uint32_t alignment=0;
   void *value_class=api.class_from_type(value_type);
   if(!value_class||api.class_value_size(value_class,&alignment)!=8) {
    log_line("UNAVAILABLE energy: FDP is not an 8-byte value; no setter hook installed.");t->method=NULL;continue;
   }
  }
  void *entry=*(void *const *)t->method;
  if(!executable(entry)){log_line("UNAVAILABLE %s.%s: no executable native/interpreter entry",t->type,t->name);t->method=NULL;continue;}
  int g;
  for(g=0;g<group_count;g++)if(groups[g].entry==entry)break;
  if(g==group_count) {
   if(g>=MAX_GROUPS){t->method=NULL;continue;}
   groups[g].entry=entry;groups[g].abi=t->abi;group_count++;
  } else if(groups[g].abi!=t->abi)groups[g].conflict=true;
  t->group=g;log_line("RESOLVED %s.%s: MethodInfo=%p entry=%p group=%d",t->type,t->name,t->method,entry,g);
 }
 MH_STATUS status=MH_Initialize();if(status!=MH_OK){log_line("MinHook init failed: %s",MH_StatusToString(status));return false;}
 for(int g=0;g<group_count;g++) {
  if(groups[g].conflict){log_line("Group %d has incompatible signatures sharing one entry; refusing hook.",g);continue;}
  status=MH_CreateHook(groups[g].entry,handlers[groups[g].abi][g],&groups[g].original);
  if(status!=MH_OK){log_line("CreateHook group %d failed: %s",g,MH_StatusToString(status));continue;}
  /* Publish every trampoline before any entry can be enabled. */
  status=MH_QueueEnableHook(groups[g].entry);
  if(status==MH_OK)groups[g].enabled=true;
  else log_line("Queue group %d failed: %s",g,MH_StatusToString(status));
 }
 status=MH_ApplyQueued();
 if(status!=MH_OK) {
  log_line("Apply hooks failed: %s; rollback to pass-through.",MH_StatusToString(status));
  MH_DisableHook(MH_ALL_HOOKS);
  for(int g=0;g<group_count;g++)groups[g].enabled=false;
  return false;
 }
 InterlockedExchange(&capabilities[NO_COOLDOWN],ready(REDUCE)&&ready(RESET));
 InterlockedExchange(&capabilities[UNLIMITED_ENERGY],ready(CLEAR)&&ready(SET_ENERGY)&&total_energy);
 InterlockedExchange(&capabilities[GOD_MODE],ready(REAL_DAMAGE));
 InterlockedExchange(&capabilities[INSTANT_BREAK],ready(TOUGHNESS)&&toughness_value);
 bool all_hits=true;for(int i=HIT_ACTOR;i<=HIT_BULLET;i++)if(!ready((enum TargetID)i))all_hits=false;
 InterlockedExchange(&capabilities[MULTI_HIT],all_hits&&from_weapon_field&&weapon_attack_id);
 InterlockedExchange(&capabilities[MOB_VACUUM],ready(DRIVE)&&vacuum_supported);
 for(int i=0;i<FEATURE_COUNT;i++)log_line("Capability %d: %s",i,read_atomic(&capabilities[i])?"hooks/resources ready (gameplay not yet verified)":"unavailable");
 return true;
}
void report_observed_calls(void) {
 static bool reported[TARGET_COUNT];
 for(int i=0;i<TARGET_COUNT;i++)if(!reported[i]&&read_atomic(&targets[i].observed)) {
  reported[i]=true;log_line("OBSERVED target call: %s.%s",targets[i].type,targets[i].name);
 }
}
int observed_target_count(void) {
 int count=0;for(int i=0;i<TARGET_COUNT;i++)if(read_atomic(&targets[i].observed))count++;
 return count;
}
