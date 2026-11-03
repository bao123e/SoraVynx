/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
__declspec(dllexport) size_t il2cpp_image_get_class_count(const void *image){(void)image;return 110;}
__declspec(dllexport) void *il2cpp_image_get_class(const void *image,size_t i){(void)image;return (void*)(i+1);}
__declspec(dllexport) const char *il2cpp_class_get_name(void *klass){
 uintptr_t c=(uintptr_t)klass;
 const char *names[]={"DailyMissionController","HeartlinkManager","DispatchController","DiscUpgradeController","TrialStageController","ShopRewardManager","DailyMissionModel","UIDailyMissionPanel","DailyInstanceGameplayRoom","NoRelatedType"};
 if(c==1000)return "Singleton`1";
 return c<=100?"DailyMissionProtocol":names[c-101];
}
__declspec(dllexport) const char *il2cpp_class_get_namespace(void *klass){return (uintptr_t)klass<=101?"Nova.Hotfix":"";}
__declspec(dllexport) void *il2cpp_class_get_fields(void *klass,void **iterator){if(*iterator)return NULL;*iterator=(void*)1;return (void*)(((uintptr_t)klass<<4)|1);}
__declspec(dllexport) const char *il2cpp_field_get_name(void *field){return ((uintptr_t)field>>4)==1000?"_instance":"SelectedId";}
__declspec(dllexport) const char *il2cpp_method_get_param_name(const void *method,unsigned index){(void)method;return index?"useCurrentSelection":"commissionId";}
