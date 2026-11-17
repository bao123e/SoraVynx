/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
static struct {
 HANDLE process,job,pipe,script;
 bool running,mouse_held,escape_held;
 int terminal;ULONGLONG started;
 char pending[128];size_t used;
 wchar_t message[256];
} commission_auto;
static const wchar_t *commission_auto_message(void){return commission_auto.message[0]?commission_auto.message:L"Start on the main screen. Selected home, Disc, Trekker and Commission tasks run once; F1 stops.";}
static void commission_status(const wchar_t *text){wcsncpy(commission_auto.message,text,255);commission_auto.message[255]=0;}
/* Only enumerated progress codes cross the pipe; no OCR text or screenshot is returned. */
static void commission_code(const char *code){
 if(!strcmp(code,"HOLD_MOUSE")){commission_auto.mouse_held=true;return;}
 if(!strcmp(code,"UP_MOUSE")){commission_auto.mouse_held=false;return;}
 if(!strcmp(code,"HOLD_ESCAPE")){commission_auto.escape_held=true;return;}
 if(!strcmp(code,"UP_ESCAPE")){commission_auto.escape_held=false;return;}
 if(!strcmp(code,"DONE")){commission_auto.terminal=1;commission_status(L"Selected daily inputs finished. Check daily progress in game; server completion is not auto-verified.");return;}
 if(!strcmp(code,"NO_REWARDS")){commission_auto.terminal=2;commission_status(L"No enabled Claim All button found. No reward click was sent; check whether rewards are ready.");return;}
 if(!strncmp(code,"ERR_",4)){
  for(size_t i=0;code[i];i++)if(i>60||!((code[i]>='A'&&code[i]<='Z')||code[i]=='_')){commission_auto.terminal=-1;commission_status(L"Stopped: invalid helper output.");return;}
  commission_auto.terminal=-1;
  if(!strcmp(code,"ERR_OCR_LANGUAGE"))commission_status(L"English OCR is unavailable. Install English OCR in Windows language settings, then retry. No game input was sent.");
  else if(!strcmp(code,"ERR_HOME_REQUIRED"))commission_status(L"Home interaction or upgrades are USE. Return to the main screen, then start this run again.");
  else if(!strcmp(code,"ERR_UPGRADE_TAB_NOT_OPENED"))commission_status(L"ERR_UPGRADE_TAB_NOT_OPENED: the Upgrade page did not open after two navigation attempts. No confirmation spend was sent.");
  else swprintf(commission_auto.message,256,L"Commission stopped: %hs. Check the game; no blind retry was made.",code);
  return;
 }
 static const struct {const char *code;const wchar_t *text;} messages[]={
  {"READY",L"Screen reader ready. Keep the game focused; F1 or Disable all stops this run."},
  {"STATE_HOME",L"Main screen recognized. Running selected daily tasks."},{"STATE_CLAIM",L"Commission rewards recognized."},
  {"STATE_STORY",L"Story overlay recognized (Esc Skip and Space Next)."},{"STATE_ITEMS",L"Items Obtained overlay recognized."},
  {"STATE_RESULT",L"Commission Complete and Dispatch Again recognized."},{"STATE_UNKNOWN",L"Waiting for a recognized Commission screen; no input on unknown screens."},
  {"STATE_NO_REWARDS",L"Claim All is not enabled."},
  {"STATE_DISC_LIST",L"Disc list recognized."},{"STATE_TREKKER_LIST",L"Trekker list recognized."},
  {"STATE_DISC_DETAILS",L"Disc details recognized."},{"STATE_TREKKER_DETAILS",L"Trekker details recognized."},
  {"STATE_DISC_EDIT",L"Disc upgrade screen recognized."},{"STATE_TREKKER_EDIT",L"Trekker training screen recognized."},
  {"ACTION_ENTER_DISC",L"Opening Disc list."},{"ACTION_ENTER_TREKKER",L"Opening Trekker list."},
  {"ACTION_BACK_LIST",L"Skipping an unreadable or capped item. Checking another visible card."},
  {"ACTION_SELECT_LOW",L"Inspecting a candidate; only a readable detail level 1-9 can be upgraded."},
  {"ACTION_REOPEN_UPGRADE",L"Upgrade page did not open; retrying this navigation once. No upgrade spend was repeated."},
  {"ACTION_OPEN_UPGRADE",L"Opening the selected item's upgrade screen."},
  {"ACTION_PREPARE_LEVEL",L"Selecting materials with Upgrade 1 level."},
  {"ACTION_COMMIT_UPGRADE",L"Confirming one upgrade; this spend is never retried in the run."},
  {"ACTION_RETURN_HOME",L"Upgrade level change and cleared selection observed. Returning home."},
  {"ACTION_INTERACT",L"Touching the home character once at the center of the recognized home screen."},
  {"ACTION_OPEN",L"Opening Commission."},{"ACTION_CLAIM",L"Clicking the recognized Claim All button."},
  {"ACTION_ESC",L"Sending Esc to the recognized story overlay."},{"ACTION_CONTINUE",L"Closing the recognized Items Obtained overlay."},
  {"ACTION_AGAIN",L"Clicking Dispatch Again once; continuing to watch for story and reward overlays."}
 };
 for(size_t i=0;i<sizeof(messages)/sizeof(messages[0]);i++)if(!strcmp(code,messages[i].code)){if(!commission_auto.terminal)commission_status(messages[i].text);return;}
 commission_auto.terminal=-1;commission_status(L"Stopped: unknown helper output.");
}
static void commission_read(void){
 if(!commission_auto.pipe)return;
 for(int chunk=0;chunk<32;chunk++){
  DWORD available=0,read=0;char buffer[256];
  if(!PeekNamedPipe(commission_auto.pipe,NULL,0,NULL,&available,NULL)||!available)break;
  if(!ReadFile(commission_auto.pipe,buffer,available<sizeof(buffer)?available:sizeof(buffer),&read,NULL)||!read)break;
  for(DWORD i=0;i<read;i++){
   if(buffer[i]=='\r')continue;
   if(buffer[i]=='\n'){
    commission_auto.pending[commission_auto.used]=0;
    if(commission_auto.used){if(!strncmp(commission_auto.pending,"SV1|",4))commission_code(commission_auto.pending+4);else{commission_auto.terminal=-1;commission_status(L"Stopped: invalid screen-reader protocol.");}}
    commission_auto.used=0;
   }else if(commission_auto.used+1<sizeof(commission_auto.pending))commission_auto.pending[commission_auto.used++]=buffer[i];
   else{commission_auto.terminal=-1;commission_status(L"Stopped: oversized helper output.");commission_auto.used=0;}
  }
 }
}
static void commission_release_owned(void){
 INPUT input={0};
 if(commission_auto.mouse_held){input.type=INPUT_MOUSE;input.mi.dwFlags=MOUSEEVENTF_LEFTUP;for(int i=0;i<3;i++)if(SendInput(1,&input,sizeof(input))==1)break;commission_auto.mouse_held=false;}
 if(commission_auto.escape_held){memset(&input,0,sizeof(input));input.type=INPUT_KEYBOARD;input.ki.wVk=VK_ESCAPE;input.ki.dwFlags=KEYEVENTF_KEYUP;for(int i=0;i<3;i++)if(SendInput(1,&input,sizeof(input))==1)break;commission_auto.escape_held=false;}
}
static void commission_end(HWND owner,bool terminate){
 KillTimer(owner,4);
 if(terminate&&commission_auto.job)TerminateJobObject(commission_auto.job,1);
 if(commission_auto.process){WaitForSingleObject(commission_auto.process,2000);commission_read();}
 commission_release_owned();
 if(commission_auto.process)CloseHandle(commission_auto.process);
 if(commission_auto.job)CloseHandle(commission_auto.job);
 if(commission_auto.pipe)CloseHandle(commission_auto.pipe);
 if(commission_auto.script)CloseHandle(commission_auto.script);
 commission_auto.process=commission_auto.job=commission_auto.pipe=commission_auto.script=NULL;commission_auto.running=false;
}
static void commission_cancel(HWND owner){if(!commission_auto.running)return;commission_end(owner,true);commission_status(L"Auto Commission stopped. Any owned press was released. Check the game before starting another run.");}
static bool commission_script_matches(HANDLE file,const void *expected,DWORD bytes){
 LARGE_INTEGER length;DWORD read=0;
 if(!expected||!bytes||bytes>262144||!GetFileSizeEx(file,&length)||length.QuadPart!=bytes)return false;
 void *buffer=malloc(bytes);if(!buffer)return false;
 bool ok=ReadFile(file,buffer,bytes,&read,NULL)&&read==bytes&&!memcmp(buffer,expected,bytes);free(buffer);return ok;
}
static bool commission_start(HWND owner,HMODULE module,HWND game,unsigned tasks){
 tasks&=(1u<<2)|(1u<<3)|(1u<<6)|(1u<<9);
 if(!tasks){commission_status(L"Set a supported daily task to USE first.");return false;}
 if(commission_auto.running)return false;
 wchar_t name[64],path[2048],exe[1024],command[4096];DWORD pid=0;
 if(!game||!IsWindow(game)||IsIconic(game)||!GetWindowThreadProcessId(game,&pid)||pid!=GetCurrentProcessId()||!GetClassNameW(game,name,64)||wcscmp(name,L"UnityWndClass")){commission_status(L"Open the game main screen or Commission before starting.");return false;}
 DWORD n=GetModuleFileNameW(module,path,2048);wchar_t *slash=n&&n<2048?wcsrchr(path,L'\\'):NULL;
 if(!slash||wcschr(path,L'"')){commission_status(L"Could not locate the pinned screen reader.");return false;}
 size_t prefix=(size_t)(slash-path)+1;const wchar_t *suffix=L"automation\\commission.ps1";
 if(prefix+wcslen(suffix)>=2048)return false;wcscpy(path+prefix,suffix);
 HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(406),MAKEINTRESOURCEW(10));
 HGLOBAL loaded=resource?LoadResource(module,resource):NULL;DWORD bytes=resource?SizeofResource(module,resource):0;
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(file==INVALID_HANDLE_VALUE||!commission_script_matches(file,loaded?LockResource(loaded):NULL,bytes)){
  if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);commission_status(L"Screen-reader script is missing or mismatched. Extract the whole ZIP, including automation/commission.ps1.");return false;
 }
 DWORD directory=GetSystemDirectoryW(exe,1024);const wchar_t *ps=L"\\WindowsPowerShell\\v1.0\\powershell.exe";
 if(!directory||directory+wcslen(ps)>=1024){CloseHandle(file);return false;}wcscat(exe,ps);
 int chars=swprintf(command,4096,L"\"%ls\" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"%ls\" -GameWindow %llu -GamePid %lu -Tasks %u",exe,path,(unsigned long long)(uintptr_t)game,(unsigned long)pid,tasks);
 if(chars<0||chars>=4096){CloseHandle(file);return false;}
 SECURITY_ATTRIBUTES security={sizeof(security),NULL,TRUE};HANDLE read_pipe=NULL,write_pipe=NULL,input=NULL,error=NULL,job=NULL;
 STARTUPINFOEXW startup={0};PROCESS_INFORMATION process={0};SIZE_T attribute_bytes=0;bool attributes_ready=false,created=false,ok=false;
 if(!CreatePipe(&read_pipe,&write_pipe,&security,4096)||!SetHandleInformation(read_pipe,HANDLE_FLAG_INHERIT,0))goto cleanup;
 input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,NULL);
 error=CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,NULL);
 if(input==INVALID_HANDLE_VALUE||error==INVALID_HANDLE_VALUE)goto cleanup;
 InitializeProcThreadAttributeList(NULL,1,0,&attribute_bytes);startup.lpAttributeList=malloc(attribute_bytes);if(!startup.lpAttributeList)goto cleanup;
 if(!InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&attribute_bytes))goto cleanup;attributes_ready=true;
 HANDLE inherited[]={write_pipe,input,error};
 if(!UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),NULL,NULL))goto cleanup;
 startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;startup.StartupInfo.wShowWindow=SW_HIDE;
 startup.StartupInfo.hStdInput=input;startup.StartupInfo.hStdOutput=write_pipe;startup.StartupInfo.hStdError=error;
 job=CreateJobObjectW(NULL,NULL);if(!job)goto cleanup;
 JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits={0};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
 if(!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))goto cleanup;
 created=CreateProcessW(exe,command,NULL,NULL,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED|EXTENDED_STARTUPINFO_PRESENT,NULL,NULL,&startup.StartupInfo,&process)!=FALSE;
 if(!created||!AssignProcessToJobObject(job,process.hProcess))goto cleanup;
 memset(&commission_auto,0,sizeof(commission_auto));commission_auto.process=process.hProcess;commission_auto.job=job;commission_auto.pipe=read_pipe;commission_auto.script=file;
 commission_auto.running=true;commission_auto.started=GetTickCount64();commission_status(L"Starting local English OCR. Keep the game focused; F1 stops this run.");
 ShowWindow(owner,SW_HIDE);SetForegroundWindow(game);
 if(ResumeThread(process.hThread)==(DWORD)-1||!SetTimer(owner,4,100,NULL)){commission_end(owner,true);file=NULL;job=NULL;read_pipe=NULL;process.hProcess=NULL;goto cleanup;}
 file=NULL;job=NULL;read_pipe=NULL;process.hProcess=NULL;ok=true;
 cleanup:
 if(created&&!ok&&process.hProcess)TerminateProcess(process.hProcess,1);
 if(process.hThread)CloseHandle(process.hThread);if(process.hProcess)CloseHandle(process.hProcess);
 if(attributes_ready)DeleteProcThreadAttributeList(startup.lpAttributeList);free(startup.lpAttributeList);
 if(input&&input!=INVALID_HANDLE_VALUE)CloseHandle(input);if(error&&error!=INVALID_HANDLE_VALUE)CloseHandle(error);
 if(write_pipe)CloseHandle(write_pipe);if(read_pipe)CloseHandle(read_pipe);if(job)CloseHandle(job);if(file)CloseHandle(file);
 if(!ok)commission_status(L"Could not start the local OCR helper. No automatic game input was queued.");return ok;
}
static bool commission_poll(HWND owner){
 if(!commission_auto.running)return false;
 commission_read();DWORD exit_code=0;
 if(commission_auto.terminal<0||GetTickCount64()-commission_auto.started>190000){
  wchar_t reason[256];wcscpy(reason,commission_auto_message());if(commission_auto.terminal>=0)wcscpy(reason,L"Commission stopped after its time limit.");commission_end(owner,true);commission_status(reason);return true;
 }
 if(!GetExitCodeProcess(commission_auto.process,&exit_code)||exit_code!=STILL_ACTIVE){
  commission_end(owner,false);if(!commission_auto.terminal)commission_status(L"Screen reader exited without a result. Check Windows OCR availability; no automatic retry.");return true;
 }
 return false;
}
