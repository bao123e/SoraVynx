/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Exact-path process discovery using Windows metadata queries only.
 */
#pragma once
#include <tlhelp32.h>
#include <winver.h>

#define MONITOR_PATH_CAP 32768
enum MonitorStatus { MON_SELECT, MON_CHECKING, MON_STOPPED, MON_RUNNING, MON_RESTRICTED, MON_ERROR };
struct ProcessProbe {enum MonitorStatus status;DWORD pid,count,error;};
struct ProcessMonitor {
 SRWLOCK lock;HANDLE wake,stop,thread;bool busy;
 ULONGLONG generation;wchar_t target[MONITOR_PATH_CAP];struct ProcessProbe result;
};
static struct ProcessMonitor monitor;

static const wchar_t *path_filename(const wchar_t *path){
 const wchar_t *name=wcsrchr(path,L'\\');return name?name+1:path;
}
static bool executable_path(const wchar_t *input,wchar_t *output){
 wchar_t full[MONITOR_PATH_CAP];DWORD n=GetFullPathNameW(input,MONITOR_PATH_CAP,full,NULL);
 if(!n||n>=MONITOR_PATH_CAP)return false;
 size_t len=wcslen(full);if(len<4||_wcsicmp(full+len-4,L".exe"))return false;
 DWORD attributes=GetFileAttributesW(full);
 if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_DIRECTORY))return false;
 n=GetLongPathNameW(full,output,MONITOR_PATH_CAP);
 if(!n||n>=MONITOR_PATH_CAP)wcscpy(output,full);return true;
}
static struct ProcessProbe probe_process_path(const wchar_t *target){
 struct ProcessProbe r={MON_SELECT,0,0,0};if(!target[0])return r;
 r.status=MON_STOPPED;
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(snapshot==INVALID_HANDLE_VALUE){r.status=MON_ERROR;r.error=GetLastError();return r;}
 PROCESSENTRY32W entry={0};entry.dwSize=sizeof(entry);DWORD blocked=0;
 BOOL more=Process32FirstW(snapshot,&entry);
 if(!more&&GetLastError()!=ERROR_NO_MORE_FILES){r.status=MON_ERROR;r.error=GetLastError();}
 while(more){
  if(!_wcsicmp(entry.szExeFile,path_filename(target))){
   HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);
   if(process){
    wchar_t image[MONITOR_PATH_CAP];DWORD length=MONITOR_PATH_CAP,exit_code=0;
    if(QueryFullProcessImageNameW(process,0,image,&length)){
     const wchar_t *actual=(length>6&&!wcsncmp(image,L"\\\\?\\",4)&&image[5]==L':')?image+4:image;
     wchar_t expanded[MONITOR_PATH_CAP];DWORD expanded_length=GetLongPathNameW(actual,expanded,MONITOR_PATH_CAP);
     if(expanded_length&&expanded_length<MONITOR_PATH_CAP)actual=expanded;
     if(!_wcsicmp(actual,target)&&GetExitCodeProcess(process,&exit_code)&&exit_code==STILL_ACTIVE){
      r.count++;if(!r.pid||entry.th32ProcessID<r.pid)r.pid=entry.th32ProcessID;
     }
    }else if(GetLastError()==ERROR_ACCESS_DENIED)blocked=ERROR_ACCESS_DENIED;
    CloseHandle(process);
   }else if(GetLastError()==ERROR_ACCESS_DENIED)blocked=ERROR_ACCESS_DENIED;
  }
  more=Process32NextW(snapshot,&entry);
 }
 CloseHandle(snapshot);
 if(r.count)r.status=MON_RUNNING;
 else if(blocked){r.status=MON_RESTRICTED;r.error=blocked;}
 return r;
}
static bool executable_version(const wchar_t *path,wchar_t *output,size_t capacity){
 DWORD ignored=0,bytes=GetFileVersionInfoSizeW(path,&ignored);if(!bytes||bytes>16*1024*1024)return false;
 void *data=HeapAlloc(GetProcessHeap(),0,bytes);if(!data)return false;
 VS_FIXEDFILEINFO *info=NULL;UINT size=0;bool found=false;
 if(GetFileVersionInfoW(path,0,bytes,data)&&VerQueryValueW(data,L"\\",(void**)&info,&size)&&
  size>=sizeof(*info)&&info->dwSignature==0xfeef04bd){
  swprintf(output,capacity,L"%u.%u.%u.%u",HIWORD(info->dwFileVersionMS),LOWORD(info->dwFileVersionMS),HIWORD(info->dwFileVersionLS),LOWORD(info->dwFileVersionLS));found=true;
 }
 HeapFree(GetProcessHeap(),0,data);return found;
}
static DWORD WINAPI monitor_worker(void *unused){
 (void)unused;HANDLE waits[]={monitor.stop,monitor.wake};
 while(WaitForMultipleObjects(2,waits,FALSE,INFINITE)==WAIT_OBJECT_0+1){
  wchar_t target[MONITOR_PATH_CAP];ULONGLONG generation;
  AcquireSRWLockExclusive(&monitor.lock);wcscpy(target,monitor.target);generation=monitor.generation;ReleaseSRWLockExclusive(&monitor.lock);
  struct ProcessProbe result=probe_process_path(target);
  AcquireSRWLockExclusive(&monitor.lock);
  if(generation==monitor.generation){monitor.result=result;monitor.busy=false;}
  ReleaseSRWLockExclusive(&monitor.lock);
 }
 return 0;
}
static void monitor_refresh(void){
 if(!monitor.thread)return;
 AcquireSRWLockExclusive(&monitor.lock);bool request=!monitor.busy;
 if(request)monitor.busy=true;ReleaseSRWLockExclusive(&monitor.lock);
 if(request)SetEvent(monitor.wake);
}
static void monitor_target(const wchar_t *path){
 AcquireSRWLockExclusive(&monitor.lock);wcscpy(monitor.target,path);monitor.generation++;
 monitor.result=(struct ProcessProbe){path[0]?MON_CHECKING:MON_SELECT,0,0,0};monitor.busy=true;ReleaseSRWLockExclusive(&monitor.lock);
 if(monitor.thread)SetEvent(monitor.wake);
 else{AcquireSRWLockExclusive(&monitor.lock);monitor.result.status=MON_ERROR;monitor.busy=false;ReleaseSRWLockExclusive(&monitor.lock);}
}
static struct ProcessProbe monitor_result(void){
 AcquireSRWLockShared(&monitor.lock);struct ProcessProbe result=monitor.result;ReleaseSRWLockShared(&monitor.lock);return result;
}
static bool monitor_busy(void){
 AcquireSRWLockShared(&monitor.lock);bool busy=monitor.busy;ReleaseSRWLockShared(&monitor.lock);return busy;
}
static void monitor_init(const wchar_t *path){
 InitializeSRWLock(&monitor.lock);monitor.stop=CreateEventW(NULL,TRUE,FALSE,NULL);monitor.wake=CreateEventW(NULL,FALSE,FALSE,NULL);
 if(monitor.stop&&monitor.wake)monitor.thread=CreateThread(NULL,0,monitor_worker,NULL,0,NULL);
 monitor_target(path);
}
static void monitor_shutdown(void){
 if(monitor.stop)SetEvent(monitor.stop);
 if(monitor.thread){WaitForSingleObject(monitor.thread,INFINITE);CloseHandle(monitor.thread);monitor.thread=NULL;}
 if(monitor.wake){CloseHandle(monitor.wake);monitor.wake=NULL;}
 if(monitor.stop){CloseHandle(monitor.stop);monitor.stop=NULL;}
}
