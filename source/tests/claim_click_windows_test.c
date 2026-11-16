/* Exercise the actual marked-click implementation with fake desktop/input APIs. */
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#undef NDEBUG
#include <assert.h>
static const HWND game=(HWND)(uintptr_t)1,tool=(HWND)(uintptr_t)2,other=(HWND)(uintptr_t)3;
static struct {HWND foreground;POINT cursor;LONG width,height;bool exists,covered,user_held,foreign,fail_down,fail_timer;DWORD flags[16];int inputs,ups;} f;
static HWND WINAPI fake_foreground(void){return f.foreground;}
static BOOL WINAPI fake_window(HWND h){return f.exists&&h==game;}
static BOOL WINAPI fake_iconic(HWND h){(void)h;return FALSE;}
static DWORD WINAPI fake_pid(HWND h,DWORD *pid){(void)h;*pid=GetCurrentProcessId()+(f.foreign?1:0);return 1;}
static int WINAPI fake_class(HWND h,LPWSTR name,int cap){(void)h;assert(cap>=14);wcscpy(name,L"UnityWndClass");return 13;}
static BOOL WINAPI fake_cursor(LPPOINT p){*p=f.cursor;return TRUE;}
static HWND WINAPI fake_at(POINT p){(void)p;return f.covered?other:game;}
static HWND WINAPI fake_ancestor(HWND h,UINT flags){(void)flags;return h;}
static SHORT WINAPI fake_key(int key){assert(key==VK_LBUTTON);return f.user_held?(SHORT)0x8000:0;}
static BOOL WINAPI fake_to_client(HWND h,LPPOINT p){assert(h==game);p->x-=100;p->y-=80;return TRUE;}
static BOOL WINAPI fake_rect(HWND h,LPRECT r){assert(h==game);*r=(RECT){0,0,f.width,f.height};return TRUE;}
static BOOL WINAPI fake_to_screen(HWND h,LPPOINT p){assert(h==game);p->x+=100;p->y+=80;return TRUE;}
static BOOL WINAPI fake_set_cursor(int x,int y){f.cursor=(POINT){x,y};return TRUE;}
static BOOL WINAPI fake_show(HWND h,int show){assert(h==tool&&show==SW_HIDE);return TRUE;}
static BOOL WINAPI fake_focus(HWND h){f.foreground=h;return TRUE;}
static UINT_PTR WINAPI fake_timer(HWND h,UINT_PTR id,UINT interval,TIMERPROC fn){assert(h==tool&&id==3&&(interval==200||interval==60)&&!fn);return f.fail_timer?0:1;}
static BOOL WINAPI fake_kill(HWND h,UINT_PTR id){assert(h==tool&&id==3);return TRUE;}
static UINT WINAPI fake_input(UINT count,LPINPUT input,int size){
 assert(count==1&&size==sizeof(INPUT)&&input->type==INPUT_MOUSE&&f.inputs<16);
 DWORD flag=input->mi.dwFlags;f.flags[f.inputs++]=flag;
 if(flag==MOUSEEVENTF_LEFTUP)f.ups++;
 return flag==MOUSEEVENTF_LEFTDOWN&&f.fail_down?0:1;
}
#define GetForegroundWindow fake_foreground
#define IsWindow fake_window
#define IsIconic fake_iconic
#define GetWindowThreadProcessId fake_pid
#define GetClassNameW fake_class
#define GetCursorPos fake_cursor
#define WindowFromPoint fake_at
#define GetAncestor fake_ancestor
#define GetAsyncKeyState fake_key
#define ScreenToClient fake_to_client
#define GetClientRect fake_rect
#define ClientToScreen fake_to_screen
#define SetCursorPos fake_set_cursor
#define ShowWindow fake_show
#define SetForegroundWindow fake_focus
#define SetTimer fake_timer
#define KillTimer fake_kill
#define SendInput fake_input
#include "../claim_click.h"
static void reset(void){memset(&f,0,sizeof(f));memset(claim_slots,0,sizeof(claim_slots));claim_selected=0;f.foreground=game;f.cursor=(POINT){600,450};f.width=1000;f.height=600;f.exists=true;}
static void marked(void){reset();assert(claim_mark());assert(claim_click.marked&&!f.inputs);}
int main(void){
 marked();assert(claim_click.point.x==500&&claim_click.point.y==370);
 f.cursor=(POINT){900,500};f.foreground=tool;assert(claim_start(tool)&&!f.inputs);
 assert(claim_tick(tool)==0&&f.inputs==1&&claim_click.held);
 /* Foreground loss after DOWN must still release UP, without a second click. */
 f.foreground=other;assert(claim_tick(tool)==1&&f.inputs==2&&f.ups==1&&!claim_click.held);
 assert(f.flags[0]==MOUSEEVENTF_LEFTDOWN&&f.flags[1]==MOUSEEVENTF_LEFTUP);
 assert(f.cursor.x==900&&f.cursor.y==500);assert(!claim_start(tool)&&!claim_mark()&&f.inputs==2);
 marked();assert(claim_start(tool));f.foreground=other;assert(claim_tick(tool)==-1&&!f.inputs&&!claim_click.attempted);
 marked();assert(claim_start(tool));f.width++;assert(claim_tick(tool)==-1&&!f.inputs);
 marked();assert(claim_start(tool));f.covered=true;assert(claim_tick(tool)==-1&&!f.inputs);
 marked();assert(claim_start(tool));f.user_held=true;assert(claim_tick(tool)==-1&&!f.inputs&&!f.ups);
 marked();assert(claim_start(tool));claim_cancel(tool);assert(!f.inputs&&!claim_click.phase);
 marked();assert(claim_start(tool));claim_tick(tool);claim_cancel(tool);assert(f.inputs==2&&f.ups==1&&!claim_click.held);
 marked();f.fail_down=true;assert(claim_start(tool));assert(claim_tick(tool)==-1&&f.inputs==1&&!f.ups&&claim_click.attempted);assert(!claim_start(tool));
 marked();f.fail_timer=true;assert(!claim_start(tool)&&!f.inputs);
 marked();assert(claim_start(tool));claim_tick(tool);f.cursor=(POINT){300,200};claim_tick(tool);assert(f.cursor.x==300&&f.cursor.y==200);
 reset();f.foreground=other;assert(!claim_mark()&&!f.inputs);
 reset();f.foreign=true;assert(!claim_mark()&&!f.inputs);
 reset();f.covered=true;assert(!claim_mark()&&!f.inputs);
 reset();f.user_held=true;assert(!claim_mark()&&!f.inputs);
 reset();f.cursor=(POINT){-1,-1};assert(!claim_mark()&&!f.inputs);
 /* Separate calibration/attempt state; switching while a press is pending is forbidden. */
 marked();assert(claim_select(1)&&!claim_click.marked);assert(!claim_start(tool)&&!f.inputs);
 f.cursor=(POINT){800,500};assert(claim_mark());assert(claim_click.point.x==700);
 assert(claim_start(tool));assert(!claim_select(0)&&claim_selected==1);claim_tick(tool);
 assert(!claim_select(0));claim_cancel_all(tool);assert(!claim_click.held&&f.ups==1);
 assert(claim_select(0)&&claim_click.marked&&!claim_click.attempted&&claim_click.point.x==500);
 assert(claim_start(tool));claim_tick(tool);claim_tick(tool);assert(f.inputs==4);
 assert(claim_select(1)&&claim_click.attempted&&!claim_start(tool));assert(!claim_select(-1)&&!claim_select(2));
 puts("PASS independent Tower/Dispatch marks and attempt caps; target switching blocked until mouse release");
 puts("PASS marked Claim Gifts: hover never clicks, correct window/client point, delayed down/up, focus/resize/cover/mouse guards, cancel releases press, cursor preservation and no duplicate attempts");return 0;
}
