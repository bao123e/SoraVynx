/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <stddef.h>
#include <stdio.h>

#define SV_BUILD_VERSION "1.3.1-home-commission-preview.1"
enum StartupCode {
 SV_STARTING, SV_WAIT_MODULE, SV_WAIT_DOMAIN, SV_WAIT_HOTFIX, SV_BIND_BACKEND,
 SV_READY, SV_DUPLICATE_RUNTIME, SV_MODULE_TIMEOUT, SV_BINARY_CHECK_FAILED,
 SV_API_EXPORT_MISSING, SV_DOMAIN_TIMEOUT, SV_THREAD_ATTACH_FAILED,
 SV_HOTFIX_TIMEOUT, SV_LICENSE_EXPIRED, SV_PLAYER_CLASS_MISSING,
 SV_MONSTER_CLASS_MISSING, SV_MINHOOK_INIT_FAILED, SV_FRAME_PROFILE_MISMATCH,
 SV_FRAME_CREATE_FAILED, SV_FRAME_ENABLE_FAILED, SV_INIT_THREAD_FAILED,
 SV_STARTUP_CODE_COUNT
};
static inline const char *startup_code_name(int code) {
 static const char *const names[]={
  "STARTING","WAIT_MODULE","WAIT_DOMAIN","WAIT_HOTFIX","BIND_BACKEND",
  "READY","DUPLICATE_RUNTIME","MODULE_TIMEOUT","BINARY_CHECK_FAILED",
  "API_EXPORT_MISSING","DOMAIN_TIMEOUT","THREAD_ATTACH_FAILED",
  "HOTFIX_TIMEOUT","LICENSE_EXPIRED","PLAYER_CLASS_MISSING",
  "MONSTER_CLASS_MISSING","MINHOOK_INIT_FAILED","FRAME_PROFILE_MISMATCH",
  "FRAME_CREATE_FAILED","FRAME_ENABLE_FAILED","INIT_THREAD_FAILED"
 };
 return code>=0&&code<SV_STARTUP_CODE_COUNT?names[code]:"UNKNOWN";
}
/* Scalar snapshots only: no account, license, addresses or filesystem paths. */
static inline int startup_report_format(char *out,size_t size,int code,int binary_check,
 int hotfix,int hook_status,int player,int events) {
 return snprintf(out,size,
  "SoraVynx compatibility report v1\r\nBuild: " SV_BUILD_VERSION "\r\n"
  "Startup: %s\r\nGameAssembly SHA256 allowlist: %s\r\n"
  "Hotfix image: %s\r\nMinHook status: %d\r\n"
  "Player signatures ready: %d/5\r\nEvent signatures ready: %d/3\r\n"
  "Gameplay effects: unverified\r\nNo key, account or path included. No log file created.\r\n",
  startup_code_name(code),binary_check==1?"matched":binary_check<0?"failed":"not checked",
  hotfix?"found":"not found",hook_status,player,events);
}
