/* Native mock integration tests: actual callbacks, no Windows/game execution. */
#include <assert.h>
#include <stdarg.h>
#include "../backend.c"
struct RuntimeAPI api;
const void *hotfix_image;
void *player_class=(void*)1,*monster_class=(void*)2;
static void *player=(void*)10,*monster=(void*)20,*weapon=(void*)30;
static const char mi_clear,mi_drive,mi_reduce,mi_reset,mi_energy,mi_tough,mi_real,mi_hit,mi_other,mi_update;
static void *frame_group,*frame_argument;static const void *frame_method;
static void *frame_original(void*g,const void*m,void*a){frame_group=g;frame_method=m;frame_argument=a;return (void*)123;}
static unsigned calls,cd_resets;
static bool cd_failure,cd_nested;
static const void *probe_method;static void *probe_args,*probe_result;
static void probe_original(const void*m,void*a,void*r){probe_method=m;probe_args=a;probe_result=r;}
static int32_t last_id;
static int64_t last_damage;
static uint64_t last_value;
static int box_size=8,attack_sequence;
static bool nest;
struct Box {uint64_t value;};
static struct Box energy_box={0x123456789abcdef0ULL},tough_box={5000000000ULL},id_box;
static struct Box count_box={3};
static void *dead_monster=(void*)21,*list_object=(void*)70;
static double pos_data[2]={10,20},forward_data[2]={0,1},scaled_data[2],target_data[2];
static double scale_data,old_y=2,new_y=7.5,moved_position[2],moved_y;
static unsigned moves,syncs;
static struct {uint64_t energy,maximum,cooldown;} evskill={5,100,88};
static struct Box evzero={0x42};static int32_t evscore=10;static int evsync;static bool evtest;
static const char evfull,evupdate,evsync_method,evscore_method;
static void ev_read(void *object,void *field,void *out) {
 if(field==(void*)10001)memcpy(out,&evskill.energy,8);
 else if(field==(void*)10002)memcpy(out,&evskill.maximum,8);
 else if(field==(void*)10003)memcpy(out,&evskill.cooldown,8);
 else if(field==(void*)10004)memcpy(out,&evscore,4);
 else if(field==(void*)10005){void *dict=(void*)2111;memcpy(out,&dict,sizeof(dict));}
 else assert(0);(void)object;
}
static void ev_write(void *object,void *field,void *value) {
 assert(object==&evskill);
 if(field==(void*)10001)memcpy(&evskill.energy,value,8);
 else if(field==(void*)10003)memcpy(&evskill.cooldown,value,8);
 else assert(0);
}
static void *ev_array(void *klass,uintptr_t count){assert(klass==(void*)77&&count==1);return (void*)2333;}
void log_line(const char *format,...){(void)format;}
void *find_class(const char *ns,const char *name){(void)ns;(void)name;return NULL;}
bool type_matches(const void *t,const char *expected){(void)t;(void)expected;return false;}
const void *find_method(void *k,const char *n,const char *r,int c,const char *const*p,bool s){(void)r;(void)c;(void)p;(void)s;if(evtest){if(!strcmp(n,"get_Values"))return (void*)901;if(!strcmp(n,"get_Count"))return (void*)902;if(!strcmp(n,"CopyTo"))return (void*)903;if(!strcmp(n,"GetValue"))return (void*)904;}if(k==(void*)5&&!strcmp(n,"get_Count"))return (void*)220;if(k==(void*)5&&!strcmp(n,"get_Item"))return (void*)221;return NULL;}
void *find_field(void*k,const char*n,const char*t,bool s){(void)k;(void)n;(void)t;(void)s;return NULL;}
static void *object_class(void *object){if(object==&evskill)return (void*)77;if(object==player)return player_class;if(object==monster||object==dead_monster)return monster_class;if(object==&id_box||object==&count_box)return (void*)4;if(object==list_object)return (void*)5;return (void*)3;}
static bool assignable(void *a,void*b){return a==b;}
bool is_player(void *object){return object&&api.class_is_assignable_from(player_class,api.object_get_class(object));}
bool is_monster(void *object){return object&&api.class_is_assignable_from(monster_class,api.object_get_class(object));}
static int32_t value_size(void*k,uint32_t*a){*a=8;return k==(void*)4?4:box_size;}
static void *unbox(void *box){return &((struct Box*)box)->value;}
static void static_value(void *field,void*out){(void)field;memcpy(out,&weapon,sizeof(weapon));}
static void field_value(void*object,void*field,void*out){if(field==(void*)400){void*health=object==monster?(void*)90:(void*)91;memcpy(out,&health,sizeof(health));}else{assert(field==(void*)401);bool alive=object==(void*)90;memcpy(out,&alive,sizeof(alive));}}
void *invoke(const void *method,void*object,void**args,bool*ok){
 *ok=true;
 if(method==(void*)900)return &evzero;
 if(evtest&&method==event.sync){assert(object==(void*)2444&&args[0]==&evskill);evsync++;return NULL;}
 if(method==(void*)901)return (void*)2222;
 if(method==(void*)902){count_box.value=1;return &count_box;}
 if(method==(void*)903){assert(args[0]==(void*)2333);return NULL;}
 if(method==(void*)904){assert(*(int32_t*)args[0]==0);return &evskill;}
 if(method==total_energy)return &energy_box;if(method==toughness_value)return &tough_box;if(method==weapon_attack_id){id_box.value=(uint32_t)++attack_sequence;return &id_box;}
 switch((uintptr_t)method){
 case 230:
  cd_resets++;if(cd_nested){uint64_t slots[2]={(uintptr_t)object,0};enter_frame_hook(NULL,&mi_update,slots);}
  *ok=!cd_failure;return NULL;
 case 200:return (void*)40;
 case 201:assert(object==(void*)40);return player;
 case 202:return list_object;
 case 203:return object==player?(void*)50:(void*)60;
 case 204:return pos_data;
 case 205:return forward_data;
 case 206:assert(object==(void*)60);memcpy(moved_position,args[0],sizeof(moved_position));moves++;return NULL;
 case 207:return &old_y;
 case 208:assert(object==(void*)60);memcpy(&moved_y,args[0],sizeof(moved_y));return NULL;
 case 209:assert(object==monster&&*(bool*)args[0]);syncs++;return NULL;
 case 210:return &new_y;
 case 211:scale_data=*(int32_t*)args[0];return &scale_data;
 case 212:for(int i=0;i<2;i++)scaled_data[i]=((double*)args[0])[i]**(double*)args[1];return scaled_data;
 case 213:for(int i=0;i<2;i++)target_data[i]=((double*)args[0])[i]+((double*)args[1])[i];return target_data;
 case 220:return &count_box;
 case 221:{void*objects[]={monster,player,dead_monster};return objects[*(int32_t*)args[0]];}
 default:*ok=false;return NULL;
 }
}
static void orig_void(void*a,const void*b){(void)a;(void)b;calls++;}
static void orig_ib(void*a,int32_t b,bool c,const void*d){(void)a;(void)c;(void)d;calls++;last_id=b;}
static void orig_int(void*a,int32_t b,const void*c){(void)a;(void)c;calls++;last_id=b;}
static void orig_value(void*a,uint64_t b,const void*c){(void)a;(void)c;calls++;last_value=b;}
static bool orig_tough(void*a,void*b,int64_t c,void*d,const void*e){(void)a;(void)b;(void)d;(void)e;calls++;last_damage=c;return true;}
static void orig_damage(void*a,int64_t b,bool c,void*d,const void*e){(void)a;(void)c;(void)d;(void)e;calls++;last_damage=b;}
static bool orig_hit(void*a,void*b,int32_t id,int32_t count,void*actor,void*ray,bool*damaged,void*prefab,bool scaled,bool ignore,const void*method){
 calls++;last_id=id;if(damaged)*damaged=true;
 if(nest){nest=false;hit_dispatch(0,a,b,id,count,actor,ray,damaged,prefab,scaled,ignore,method);}
 return true;
}
static void reset(void){
 for(int i=0;i<FEATURE_COUNT;i++){feature_flags[i]=0;capabilities[i]=1;}
 for(int i=0;i<TARGET_COUNT;i++){targets[i].group=0;targets[i].method=NULL;targets[i].observed=0;}
 targets[CLEAR].method=&mi_clear;targets[DRIVE].method=&mi_drive;targets[REDUCE].method=&mi_reduce;
 targets[RESET].method=&mi_reset;targets[SET_ENERGY].method=&mi_energy;targets[TOUGHNESS].method=&mi_tough;
 targets[UPDATE_CD].method=&mi_update;targets[REAL_DAMAGE].method=&mi_real;targets[HIT_ACTOR].method=&mi_hit;
 total_energy=(void*)101;toughness_value=(void*)102;weapon_attack_id=(void*)103;from_weapon_field=(void*)104;
 calls=0;last_id=0;last_value=0;last_damage=0;box_size=8;attack_sequence=200;nest=false;hit_depth=0;multi_count=3;
 group_count=1;groups[0].entry_calls=groups[0].null_methods=0;groups[0].sample_method=NULL;
 api.object_get_class=object_class;api.class_is_assignable_from=assignable;api.class_value_size=value_size;api.object_unbox=unbox;api.field_static_get_value=static_value;api.field_get_value=field_value;
}
int main(void){
 reset();groups[0].original=(void*)orig_void;
 void_dispatch(0,player,&mi_clear);assert(calls==1);
 feature_flags[UNLIMITED_ENERGY]=1;void_dispatch(0,player,&mi_clear);assert(calls==1);
 void_dispatch(0,player,&mi_other);void_dispatch(0,player,NULL);void_dispatch(0,player,&mi_drive);assert(calls==4);
 assert(total_entry_calls()==5&&groups[0].null_methods==1&&observed_target_count()==2);
 puts("PASS default-off and shared/null MethodInfo passthrough");
 reset();groups[0].original=(void*)orig_ib;feature_flags[NO_COOLDOWN]=1;
 intbool_dispatch(0,player,123,true,&mi_reduce);assert(!calls);
 intbool_dispatch(0,player,456,true,&mi_other);assert(calls==1&&last_id==456);
 groups[0].original=(void*)orig_int;int_dispatch(0,player,1,&mi_reset);assert(calls==1);
 feature_flags[NO_COOLDOWN]=0;int_dispatch(0,player,2,&mi_reset);assert(calls==2&&last_id==2);
 puts("PASS cooldown filtering and disable behavior");
 reset();groups[0].original=(void*)orig_value;feature_flags[UNLIMITED_ENERGY]=1;
 value_dispatch(0,player,7,&mi_energy);assert(calls==1&&last_value==energy_box.value);
 value_dispatch(0,player,9,&mi_other);assert(last_value==9);
 box_size=16;value_dispatch(0,player,11,&mi_energy);assert(last_value==11);
 puts("PASS FDP energy copy and invalid-size fallback");
 reset();groups[0].original=(void*)orig_tough;feature_flags[INSTANT_BREAK]=1;
 assert(toughness_dispatch(0,monster,player,1,NULL,&mi_tough));assert(last_damage==5000000000LL);
 toughness_dispatch(0,monster,monster,2,NULL,&mi_tough);assert(last_damage==2);
 toughness_dispatch(0,monster,player,-1,NULL,&mi_tough);assert(last_damage==-1);
 puts("PASS 64-bit toughness and attacker filtering");
 reset();groups[0].original=(void*)orig_damage;feature_flags[GOD_MODE]=1;
 damage_dispatch(0,player,123,false,NULL,&mi_real);assert(!calls);
 damage_dispatch(0,monster,456,false,NULL,&mi_real);assert(calls==1&&last_damage==456);
 damage_dispatch(0,player,789,false,NULL,NULL);assert(calls==2&&last_damage==789);
 puts("PASS god mode class/MethodInfo filtering");
 reset();groups[0].original=(void*)orig_hit;feature_flags[MULTI_HIT]=1;bool damaged=false;
 assert(hit_dispatch(0,player,NULL,100,1,monster,NULL,&damaged,NULL,false,false,&mi_hit));
 assert(calls==3&&attack_sequence==202&&damaged&&last_id==202);
 calls=0;nest=true;hit_dispatch(0,player,NULL,100,1,monster,NULL,&damaged,NULL,false,false,&mi_hit);
 assert(calls==4&&hit_depth==0);
 calls=0;hit_dispatch(0,player,NULL,100,1,monster,NULL,&damaged,NULL,false,false,&mi_other);assert(calls==1);
 puts("PASS multihit IDs, nested-call guard and unrelated-method passthrough");
 reset();groups[0].original=(void*)orig_void;capabilities[UNLIMITED_ENERGY]=0;feature_flags[UNLIMITED_ENERGY]=1;
 void_dispatch(0,player,&mi_clear);assert(calls==1);
 puts("PASS unavailable feature never alters behavior");
 reset();groups[0].original=(void*)orig_void;feature_flags[MOB_VACUUM]=1;
 vacuum.controller_class=(void*)3;vacuum.instance=(void*)200;vacuum.active_player=(void*)201;vacuum.monsters=(void*)202;
 vacuum.transform=(void*)203;vacuum.position=(void*)204;vacuum.forward=(void*)205;vacuum.set_position=(void*)206;
 vacuum.position_y=(void*)207;vacuum.set_y=(void*)208;vacuum.sync=(void*)209;vacuum.terrain=(void*)210;
 vacuum.scalar=(void*)211;vacuum.multiply=(void*)212;vacuum.addition=(void*)213;vacuum.health_field=(void*)400;vacuum.alive_field=(void*)401;
 void_dispatch(0,(void*)80,&mi_drive);
 assert(calls==1&&moves==1&&syncs==1&&moved_position[0]==10&&moved_position[1]==23&&moved_y==7.5);
 puts("PASS vacuum runtime invocation flow, vector arguments and player/dead-monster filtering");
 interpreter_original=probe_original;
 int probe_input=42,probe_output=17;
 interpreter_probe(&mi_drive,&probe_input,&probe_output);
 assert(probe_method==&mi_drive&&probe_args==&probe_input&&probe_result==&probe_output);
 assert(probe_input==42&&probe_output==17&&interpreter_calls==1&&interpreter_targets[DRIVE]==1);
 interpreter_probe(NULL,NULL,NULL);
 assert(!probe_method&&!probe_args&&!probe_result&&interpreter_calls==2&&interpreter_targets[DRIVE]==1);
 puts("PASS interpreter telemetry exact-argument passthrough and target/null filtering");
 reset();enter_frame_original=frame_original;
 uint64_t frame_args[4]={(uintptr_t)player,7,8,9};
 feature_flags[UNLIMITED_ENERGY]=1;
 assert(enter_frame_hook((void*)111,&mi_energy,frame_args)==(void*)123);
 assert(frame_group==(void*)111&&frame_method==&mi_energy&&frame_argument==frame_args);
 assert(frame_args[0]==(uintptr_t)player&&frame_args[1]==energy_box.value&&frame_args[2]==8&&frame_args[3]==9);
 feature_flags[UNLIMITED_ENERGY]=0;frame_args[1]=42;
 enter_frame_hook(NULL,&mi_energy,frame_args);assert(frame_args[1]==42);
 feature_flags[INSTANT_BREAK]=1;frame_args[0]=(uintptr_t)monster;frame_args[1]=(uintptr_t)player;frame_args[2]=10;
 enter_frame_hook(NULL,&mi_tough,frame_args);assert(frame_args[2]==10&&frame_args[3]==9);
 frame_args[1]=(uintptr_t)monster;frame_args[2]=10;
 enter_frame_hook(NULL,&mi_tough,frame_args);assert(frame_args[2]==10);
 enter_frame_hook(NULL,NULL,NULL);assert(!frame_method&&!frame_argument&&frame_callback_depth==0);
 puts("PASS frame energy slots, disabled break and allocation passthrough");
 reset();enter_frame_original=frame_original;feature_flags[GOD_MODE]=1;feature_flags[MULTI_HIT]=1;multi_count=3;
 uint64_t damage_args[4]={(uintptr_t)player,123,7,8};
 enter_frame_hook(NULL,&mi_real,damage_args);assert(damage_args[1]==0&&damage_args[2]==7&&damage_args[3]==8);
 damage_args[0]=(uintptr_t)monster;damage_args[1]=100;
 enter_frame_hook(NULL,&mi_real,damage_args);assert(damage_args[1]==300);
 damage_args[1]=INT64_MAX;enter_frame_hook(NULL,&mi_real,damage_args);assert(damage_args[1]==INT64_MAX);
 feature_flags[MULTI_HIT]=0;damage_args[1]=12;
 enter_frame_hook(NULL,&mi_real,damage_args);assert(damage_args[1]==12);
 damage_args[0]=(uintptr_t)player;damage_args[1]=(uint64_t)-7;
 enter_frame_hook(NULL,&mi_real,damage_args);assert(damage_args[1]==(uint64_t)-7);
 feature_flags[GOD_MODE]=0;damage_args[1]=12;
 enter_frame_hook(NULL,&mi_real,damage_args);assert(damage_args[1]==12);
 puts("PASS frame god/damage scaling filters, overflow clamp, negative values and disabled state");
 reset_all_cd=(void*)230;feature_flags[NO_COOLDOWN]=1;cd_nested=true;
 uint64_t cd_args[2]={(uintptr_t)player,0x12345678};
 enter_frame_hook(NULL,&mi_update,cd_args);assert(cd_resets==1&&cd_args[1]==0x12345678&&frame_callback_depth==0);
 feature_flags[NO_COOLDOWN]=0;enter_frame_hook(NULL,&mi_update,cd_args);assert(cd_resets==1);
 feature_flags[NO_COOLDOWN]=1;cd_failure=true;enter_frame_hook(NULL,&mi_update,cd_args);assert(cd_resets==2&&frame_method==&mi_update);
 puts("PASS frame cooldown reset invocation, nested recursion guard, disable and exception fallback");
 feature_flags[MOB_VACUUM]=1;mock_ticks=2000;uint64_t drive_args[1]={80};
 enter_frame_hook(NULL,&mi_drive,drive_args);assert(moves==2&&syncs==2&&frame_method==&mi_drive);
 puts("PASS vacuum processing from internal frame before original allocation");
 memset(&event,0,sizeof(event));event.skill_class=(void*)77;
 event.current_energy=(void*)10001;event.maximum_energy=(void*)10002;event.cooldown=(void*)10003;event.score=(void*)10004;event.dictionary=(void*)10005;
 event.full=&evfull;event.update=&evupdate;event.sync=&evsync_method;event.add_score=&evscore_method;event.zero=(void*)900;
 api.field_get_value=ev_read;api.field_set_value=ev_write;api.array_new=ev_array;
 for(int i=0;i<3;i++){event_ready[i]=1;event_flags[i]=0;event_hits[i]=0;}
 uint64_t eslots[3]={(uintptr_t)&evskill,123,456};
 event_frame(&evfull,eslots);assert(evskill.energy==5&&evskill.cooldown==88);
 event_flags[EV_CD]=1;event_frame(&evfull,eslots);assert(evskill.energy==5&&evskill.cooldown==evzero.value);
 event_flags[EV_CD]=0;event_flags[EV_ENERGY]=1;evskill.cooldown=99;
 event_frame(&evfull,eslots);assert(evskill.energy==100&&evskill.cooldown==99);
 assert(event_hits[EV_CD]==1&&event_hits[EV_ENERGY]==1);
 event_ready[EV_ENERGY]=0;evskill.energy=7;event_frame(&evfull,eslots);assert(evskill.energy==7);event_ready[EV_ENERGY]=1;
 uint64_t syncslots[2]={2444,(uintptr_t)&evskill};event_frame(&evsync_method,syncslots);assert(evskill.energy==100);
 evtest=true;event_flags[EV_CD]=1;evskill.energy=7;evskill.cooldown=90;mock_ticks=3000;uint64_t updates[2]={2444,0};
 event_frame(&evupdate,updates);assert(evskill.energy==100&&evskill.cooldown==evzero.value&&evsync==1);
 puts("PASS event independent cooldown/energy, FP zero conversion, disabled resource fallback and UI synchronization");
 event_flags[EV_SCORE]=1;uint64_t scores[3]={2444,25,0x55555555};event_frame(&evscore_method,scores);assert((int32_t)scores[1]==50&&scores[2]==0x55555555);
 scores[1]=(uint32_t)-5;event_frame(&evscore_method,scores);assert((int32_t)scores[1]==-5);
 evscore=INT32_MAX-3;scores[1]=10;event_frame(&evscore_method,scores);assert((int32_t)scores[1]==3);
 scores[1]=9;event_frame(&mi_other,scores);assert(scores[1]==9);
 event_flags[EV_SCORE]=0;event_frame(&evscore_method,scores);assert(scores[1]==9);
 puts("PASS event controller-only score delta x2, negative/disabled filtering, overflow ceiling and unchanged neighboring slots");
 event_flags[EV_SCORE]=1;event_multiplier=100;evscore=0;scores[1]=30;
 event_frame(&evscore_method,scores);assert(scores[1]==3000);
 event_multiplier=101;scores[1]=30;event_frame(&evscore_method,scores);assert(scores[1]==30);
 event_multiplier=1;event_frame(&evscore_method,scores);assert(scores[1]==30);
 event_multiplier=100;feature_flags[MULTI_HIT]=1;multi_count=3;
 uint64_t independent_damage[4]={(uintptr_t)monster,100,0,0};enter_frame_hook(NULL,&mi_real,independent_damage);
 assert(independent_damage[1]==300&&multi_count==3&&event_multiplier==100);
 puts("PASS configurable event factor x100, invalid range rejection and independent Player multiplier");
 return 0;
}
