/* Host integration tests for typed dispatch and command gating; no game execution. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../integration/native/backend.c"
#include "../integration/native/controls.h"
struct RuntimeAPI api;
const void *hotfix_image;
void *player_class=(void*)1,*monster_class=(void*)2;
static char player,monster,real_method,other_method,energy_method,score_method;
static int calls,resets;
static int64_t damage_value;
static uint64_t energy_value,full_energy=12345;
static int32_t current_score=10,delta_value;
void log_line(const char *format,...){(void)format;}
void *find_class(const char *ns,const char *name){(void)ns;(void)name;return NULL;}
const void *find_method(void *k,const char *n,const char *r,int c,const char *const*p,bool s){(void)k;(void)n;(void)r;(void)c;(void)p;(void)s;return NULL;}
void *find_field(void*k,const char*n,const char*t,bool s){(void)k;(void)n;(void)t;(void)s;return NULL;}
bool is_player(void *p){return p==&player;}
bool is_monster(void *p){return p==&monster;}
static int32_t size8(void *p,uint32_t *align){(void)p;*align=8;return 8;}
static int32_t size16(void *p,uint32_t *align){(void)p;*align=8;return 16;}
static void *unbox(void *p){return p;}
static void *object_class(void *p){return p;}
static void field_get(void *obj,void *field,void *out){(void)obj;(void)field;memcpy(out,&current_score,sizeof(current_score));}
void *invoke(const void *method,void *obj,void **args,bool *ok){(void)obj;(void)args;*ok=true;if(method==total_energy)return &full_energy;if(method==reset_all_cd){resets++;return NULL;}*ok=false;return NULL;}
static void original_damage(void *obj,int64_t damage,bool flag,void *data,const void *method){(void)obj;(void)flag;(void)data;(void)method;calls++;damage_value=damage;}
static void original_energy(void *obj,uint64_t value,const void *method){(void)obj;(void)method;calls++;energy_value=value;}
static void original_score(void *obj,int32_t value,const void *method){(void)obj;(void)method;calls++;delta_value=value;}
static void clear(void){
 memset((void*)feature_flags,0,sizeof(feature_flags));memset((void*)capabilities,0,sizeof(capabilities));
 memset((void*)event_flags,0,sizeof(event_flags));memset((void*)event_ready,0,sizeof(event_ready));
 memset(groups,0,sizeof(groups));group_count=1;groups[0].enabled=true;calls=0;
 for(int i=0;i<TARGET_COUNT;i++){targets[i].method=NULL;targets[i].group=-1;}
 for(int i=0;i<4;i++){event_methods[i]=NULL;event_groups[i]=-1;}
 api.class_value_size=size8;api.object_unbox=unbox;api.object_get_class=object_class;api.field_get_value=field_get;
}
struct FakeMethod {void *entry;const char *result,*parameter;bool is_static;};
static struct FakeMethod fake_known={(void*)0x1234,"System.Void","TrueSync.FP",false};
static struct FakeMethod fake_foreign={(void*)0x1234,"System.Void","System.String",false};
static const void *fake_assemblies[]={&fake_known};
static const void **fake_domain_assemblies(void *domain,size_t *count){(void)domain;*count=1;return fake_assemblies;}
static void *fake_domain(void){return &fake_known;}
static const void *fake_image(const void *assembly){return assembly;}
static size_t fake_class_count(const void *image){(void)image;return 1;}
static void *fake_get_class(const void *image,size_t index){(void)index;return (void*)image;}
static const void *fake_methods(void *klass,void **iterator){(void)klass;uintptr_t i=(uintptr_t)*iterator;*iterator=(void*)(i+1);return i==0?&fake_known:i==1?&fake_foreign:NULL;}
static uint32_t fake_count(const void *method){(void)method;return 1;}
static const void *fake_parameter(const void *method,uint32_t index){(void)index;return ((const struct FakeMethod*)method)->parameter;}
static const void *fake_return(const void *method){return ((const struct FakeMethod*)method)->result;}
static uint32_t fake_flags(const void *method,uint32_t *impl){(void)impl;return ((const struct FakeMethod*)method)->is_static?0x10:0;}
static char *fake_type_name(const void *type){return strdup(type);}
static void *fake_class_from_type(const void *type){return (void*)type;}
static bool fake_value_type(void *klass){return !strcmp(klass,"TrueSync.FP")||!strcmp(klass,"OtherStruct");}
int main(void){
 clear();
 assert(!apply_control(GOD_MODE,1,true));assert(feature_flags[GOD_MODE]==0);
 capabilities[GOD_MODE]=1;assert(!apply_control(GOD_MODE,1,false));assert(!feature_flags[GOD_MODE]);
 assert(!apply_control(GOD_MODE,2,true));assert(apply_control(GOD_MODE,1,true));assert(feature_flags[GOD_MODE]);
 assert(!apply_control(10,51,true));assert(!apply_control(12,101,true));assert(!apply_control(11,0,true));
 assert(apply_control(10,50,true));assert(multi_count==50);
 event_ready[2]=1;assert(apply_control(8,1,true));assert(event_flags[2]&&feature_flags[GOD_MODE]);
 assert(apply_control(-1,0,false));assert(!event_flags[2]&&!feature_flags[GOD_MODE]);
 puts("PASS unavailable/disconnected controls fail closed; tuning bounds and independent all-off");
 clear();targets[REAL_DAMAGE].method=&real_method;targets[REAL_DAMAGE].group=0;groups[0].original=(dobby_dummy_func_t)original_damage;
 capabilities[GOD_MODE]=feature_flags[GOD_MODE]=1;
 damage_0(&player,30,false,NULL,&real_method);assert(calls==0);
 damage_0(&monster,40,false,NULL,&real_method);assert(calls==1&&damage_value==40);
 damage_0(&player,50,false,NULL,&other_method);assert(calls==2&&damage_value==50);
 damage_0(&player,60,false,NULL,NULL);assert(calls==3&&damage_value==60);
 feature_flags[GOD_MODE]=0;damage_0(&player,70,false,NULL,&real_method);assert(calls==4&&damage_value==70);
 puts("PASS typed damage wrapper preserves monster, unrelated/null method and disabled paths");
 clear();targets[SET_ENERGY].method=&energy_method;targets[SET_ENERGY].group=0;
 groups[0].original=(dobby_dummy_func_t)original_energy;total_energy=(void*)111;
 capabilities[UNLIMITED_ENERGY]=feature_flags[UNLIMITED_ENERGY]=1;
 value_0(&player,5,&energy_method);assert(calls==1&&energy_value==full_energy);
 value_0(&player,6,&other_method);assert(energy_value==6);
 api.class_value_size=size16;value_0(&player,7,&energy_method);assert(energy_value==7);
 puts("PASS boxed FDP size validation and shared-entry passthrough");
 clear();event.add_score=&score_method;event_methods[3]=&score_method;event_groups[3]=0;
 groups[0].original=(dobby_dummy_func_t)original_score;event_ready[2]=event_flags[2]=1;event_multiplier=100;
 int_0(&monster,20,&score_method);assert(delta_value==2000);
 int_0(&monster,-20,&score_method);assert(delta_value==-20);
 current_score=INT32_MAX-1;int_0(&monster,20,&score_method);assert(delta_value==1);
 int_0(&monster,20,&other_method);assert(delta_value==20);
 event_flags[2]=0;int_0(&monster,20,&score_method);assert(delta_value==20);
 puts("PASS native Event score dispatch, positive delta, saturation and identity guards");
 clear();
 api.domain_get=fake_domain;api.domain_get_assemblies=fake_domain_assemblies;
 api.assembly_get_image=fake_image;api.image_get_class_count=fake_class_count;api.image_get_class=fake_get_class;
 api.class_get_methods=fake_methods;api.method_get_param_count=fake_count;api.method_get_param=fake_parameter;
 api.method_get_return_type=fake_return;api.method_get_flags=fake_flags;api.type_get_name=fake_type_name;
 api.free=free;api.class_from_type=fake_class_from_type;api.class_is_valuetype=fake_value_type;
 assert(native_parameters_safe(&fake_known));
 fake_known.parameter="OtherStruct";assert(!native_parameters_safe(&fake_known));fake_known.parameter="TrueSync.FP";
 groups[0].entry=fake_known.entry;groups[0].shape=&fake_known;
 assert(audit_shared_entries());assert(groups[0].conflict);
 groups[0].conflict=false;fake_foreign.parameter="TrueSync.FP";
 assert(audit_shared_entries());assert(!groups[0].conflict);
 fake_foreign.is_static=true;assert(audit_shared_entries());assert(groups[0].conflict);
 puts("PASS unknown value structs and foreign/static shared-entry aliases cannot reach typed wrappers");
 return 0;
}
