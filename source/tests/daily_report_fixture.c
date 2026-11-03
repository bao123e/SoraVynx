/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
__declspec(dllexport) size_t il2cpp_image_get_class_count(const void *image){return (uintptr_t)image==2?3:(uintptr_t)image==3?1000:110;}
__declspec(dllexport) void *il2cpp_image_get_class(const void *image,size_t i){return (void*)(((uintptr_t)image==2?2001:1)+i);}
__declspec(dllexport) const char *il2cpp_class_get_name(void *klass){
 uintptr_t c=(uintptr_t)klass;
 const char *names[]={"DailyMissionController","HeartlinkManager","DispatchController","DiscUpgradeController","TrialStageController","ShopRewardManager","DailyMissionModel","UIDailyMissionPanel","DailyInstanceGameplayRoom","NoRelatedType"};
 if(c==2001)return "LuaEnv";if(c==2002)return "LuaTable";if(c==2003)return "LuaManager";
 if(c==1000)return "Singleton`1";
 return c<=100?"DailyMissionProtocol":names[c-101];
}
__declspec(dllexport) const char *il2cpp_class_get_namespace(void *klass){return (uintptr_t)klass==2001||(uintptr_t)klass==2002?"XLua":(uintptr_t)klass<=101?"Nova.Hotfix":"";}
__declspec(dllexport) void *il2cpp_class_get_fields(void *klass,void **iterator){if(*iterator)return NULL;*iterator=(void*)1;return (void*)(((uintptr_t)klass<<4)|1);}
__declspec(dllexport) const char *il2cpp_field_get_name(void *field){uintptr_t c=(uintptr_t)field>>4;return c==2001?"rawL":c==2002?"luaReference":c==2003?"luaEnv":c==1000?"_instance":"SelectedId";}
__declspec(dllexport) const char *il2cpp_method_get_param_name(const void *method,unsigned index){uintptr_t c=(uintptr_t)method>>10;return c==2001?(index?"chunkName":"script"):c==2002?"key":index?"useCurrentSelection":"commissionId";}
