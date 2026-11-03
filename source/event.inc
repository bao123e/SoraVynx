/* SPDX-License-Identifier: AGPL-3.0-or-later */
enum {EV_CD,EV_ENERGY,EV_SCORE,EV_COUNT};
volatile LONG event_flags[3],event_ready[3],event_hits[3];
volatile LONG event_multiplier=2;
static struct {
 void *skill_class,*current_energy,*maximum_energy,*cooldown,*dictionary,*score;
 const void *full,*sync,*update,*add_score,*zero;
} event;
static bool event_on(int i){return read_atomic(&event_flags[i])&&read_atomic(&event_ready[i]);}
static void setup_event(void) {
 event.skill_class=find_class("","BrickSkillInfo");
 void *component=find_class("","BrickActorSkillCom"),*controller=find_class("","BrickBreakerController");
 event.current_energy=find_field(event.skill_class,"curSkillEnergy","TrueSync.FP",false);
 event.maximum_energy=find_field(event.skill_class,"maxSkillEnergy","TrueSync.FP",false);
 event.cooldown=find_field(event.skill_class,"curTime","TrueSync.FP",false);
 event.dictionary=find_field(component,"DicSkill","System.Collections.Generic.Dictionary<System.Int32,BrickSkillInfo>",false);
 event.score=find_field(controller,"currentScrole","System.Int32",false);
 event.full=method0(event.skill_class,"get_IsFull","System.Boolean");
 event.sync=method1(component,"SyncLua","System.Void","BrickSkillInfo");
 event.update=method1(component,"OnLogicUpdate","System.Void","TrueSync.FP");
 event.add_score=method1(controller,"UpdateScrole","System.Void","System.Int32");
 const char *integer[]={"System.Int32"};
 event.zero=find_method(find_class("TrueSync","FP"),"op_Implicit","TrueSync.FP",1,integer,true);
 bool fp8=false;
 if(event.cooldown){uint32_t alignment;void *klass=api.class_from_type(api.field_get_type(event.cooldown));fp8=klass&&api.class_value_size(klass,&alignment)==8;}
 InterlockedExchange(&event_ready[EV_CD],fp8&&event.cooldown&&event.zero&&event.full&&event.sync&&event.update&&event.dictionary);
 InterlockedExchange(&event_ready[EV_ENERGY],fp8&&event.current_energy&&event.maximum_energy&&event.full&&event.sync&&event.update&&event.dictionary);
 InterlockedExchange(&event_ready[EV_SCORE],event.add_score&&event.score);
}
static bool event_fill(void *skill) {
 if(!skill||!event.skill_class||!api.class_is_assignable_from(event.skill_class,api.object_get_class(skill)))return false;
 bool changed=false;
 if(event_on(EV_CD)) {
  int32_t n=0;void *args[]={&n};bool ok;uint64_t zero,current;
  void *box=invoke(event.zero,NULL,args,&ok);
  if(ok&&read_box(box,&zero,sizeof(zero))) {
   api.field_get_value(skill,event.cooldown,&current);
   if(current!=zero){api.field_set_value(skill,event.cooldown,&zero);InterlockedIncrement(&event_hits[EV_CD]);changed=true;}
  }
 }
 if(event_on(EV_ENERGY)) {
  uint64_t maximum,current;api.field_get_value(skill,event.maximum_energy,&maximum);api.field_get_value(skill,event.current_energy,&current);
  if(current!=maximum){api.field_set_value(skill,event.current_energy,&maximum);InterlockedIncrement(&event_hits[EV_ENERGY]);changed=true;}
 }
 return changed;
}
static void event_tick(void *component) {
 static ULONGLONG last;ULONGLONG now=GetTickCount64();if(now-last<100)return;last=now;
 void *dictionary=NULL;api.field_get_value(component,event.dictionary,&dictionary);if(!dictionary)return;
 const void *values_method=method0(api.object_get_class(dictionary),"get_Values",NULL);bool ok;
 void *values=invoke(values_method,dictionary,NULL,&ok);if(!ok||!values)return;
 void *klass=api.object_get_class(values);const void *count_method=method0(klass,"get_Count","System.Int32");
 int32_t count;void *box=invoke(count_method,values,NULL,&ok);
 if(!ok||!read_box(box,&count,sizeof(count))||count<=0||count>64)return;
 const char *params[]={"BrickSkillInfo[]","System.Int32"};
 const void *copy=find_method(klass,"CopyTo","System.Void",2,params,false);
 if(!copy)return;
 void *array=api.array_new(event.skill_class,(uintptr_t)count);if(!array)return;
 int32_t start=0;void *copy_args[]={array,&start};invoke(copy,values,copy_args,&ok);if(!ok)return;
 const void *get=method1(api.object_get_class(array),"GetValue","System.Object","System.Int32");if(!get)return;
 for(int32_t i=0;i<count;i++) {
  void *args[]={&i};void *skill=invoke(get,array,args,&ok);if(!ok)break;
  if(event_fill(skill)){void *sync_args[]={skill};invoke(event.sync,component,sync_args,&ok);}
 }
}
static void event_frame(const void *method,void *arguments) {
 if(!method||!arguments)return;
 unsigned char *slots=arguments;void *self;memcpy(&self,slots,sizeof(self));
 if(event_on(EV_CD)||event_on(EV_ENERGY)) {
  if(method==event.full)event_fill(self);
  else if(method==event.sync){void *skill;memcpy(&skill,slots+8,sizeof(skill));event_fill(skill);}
  else if(method==event.update)event_tick(self);
 }
 /* Scale only the controller delta, not the forwarding manager. Configurable factor, independent of Player damage. */
 if(method==event.add_score&&event_on(EV_SCORE)) {
  int32_t delta,current;memcpy(&delta,slots+8,sizeof(delta));api.field_get_value(self,event.score,&current);
  LONG factor=read_atomic(&event_multiplier);
  if(delta>0&&current>=0&&factor>=2&&factor<=100) {
   int64_t scaled=(int64_t)delta*factor,room=(int64_t)INT32_MAX-current;
   if(scaled>room)scaled=room;
   if(scaled!=delta){delta=(int32_t)scaled;memcpy(slots+8,&delta,sizeof(delta));InterlockedIncrement(&event_hits[EV_SCORE]);}
  }
 }
}
