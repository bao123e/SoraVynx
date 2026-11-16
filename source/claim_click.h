/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <windows.h>
#include <stdbool.h>
#include <wchar.h>
/* UI-thread only. Calibration never sends input; coordinates are not persisted. */
struct ClaimClick {
 HWND game;POINT point,restore,target;SIZE size;
 bool marked,attempted,held,moved;int phase;
 const wchar_t *message;
};
static struct ClaimClick claim_slots[2];
static int claim_selected; /* Tower, Dispatch */
#define claim_click claim_slots[claim_selected]
static bool claim_select(int selected){
 if(selected<0||selected>=2)return false;
 for(int i=0;i<2;i++)if(claim_slots[i].phase||claim_slots[i].held)return false;
 claim_selected=selected;return true;
}
static const wchar_t *claim_action_title(void){return claim_selected?L"SoraVynx - Dispatch / status":L"SoraVynx - Claim Gifts / status";}
static const wchar_t *claim_click_message(void){
 if(claim_click.message)return claim_click.message;
 return claim_selected?L"Open Dispatch and select a commission/team in game. F1: hide tool, hover the START DISPATCH button and press F9 (mark only). F1: return, then Dispatch / status.":L"F1: hide tool. Hover Claim Gifts, press F9 to mark (no click). F1: return, then Claim Gifts / status.";
}
static const wchar_t *claim_click_hint(void){
 if(claim_click.phase)return L"One click is queued.\r\nKeep the game focused.";
 if(claim_click.attempted)return L"ONE ATTEMPT USED\r\nCheck the game and daily.\r\nButton shows status only.";
 if(claim_click.marked)return claim_selected?L"DISPATCH POINT MARKED\r\nKeep commission/team set.\r\nDispatch / status\r\nsends one mouse click.":L"TOWER POINT MARKED\r\nKeep the Hall open.\r\nClaim Gifts / status\r\nsends one mouse click.";
 return claim_selected?L"Choose commission/team.\r\nF1: hide tool\r\nHover START DISPATCH + F9\r\n(mark only, no click)\r\nF1, then Dispatch / status":L"F1: hide tool\r\nHover Claim Gifts + F9\r\n(mark only, no click)\r\nF1: return to tool\r\nClaim Gifts / status";
}
static bool claim_game(HWND game){
 DWORD pid=0;wchar_t name[64]={0};
 return game&&IsWindow(game)&&!IsIconic(game)&&GetWindowThreadProcessId(game,&pid)&&pid==GetCurrentProcessId()&&GetClassNameW(game,name,64)&&!wcscmp(name,L"UnityWndClass");
}
static bool claim_inside(POINT p,RECT r){return p.x>=r.left&&p.x<r.right&&p.y>=r.top&&p.y<r.bottom;}
static bool claim_mouse(DWORD flags){INPUT input={0};input.type=INPUT_MOUSE;input.mi.dwFlags=flags;return SendInput(1,&input,sizeof(input))==1;}
static bool claim_release(void){
 if(claim_click.held){for(int i=0;i<3;i++)if(claim_mouse(MOUSEEVENTF_LEFTUP)){claim_click.held=false;break;}}
 POINT now;
 if(claim_click.moved&&GetCursorPos(&now)&&now.x==claim_click.target.x&&now.y==claim_click.target.y)SetCursorPos(claim_click.restore.x,claim_click.restore.y);
 claim_click.moved=false;return !claim_click.held;
}
static void claim_cancel(HWND owner){
 KillTimer(owner,3);bool pending=claim_click.phase!=0;claim_click.phase=0;
 if(!claim_release())claim_click.message=L"Mouse release was blocked. Release the left mouse button manually; no second click will be sent.";
 else if(pending)claim_click.message=L"Selected action cancelled. Check the game if a press had already been sent.";
}
static bool claim_mark(void){
 if(claim_click.attempted||claim_click.phase){claim_click.message=L"One attempt is already pending or used. No new point was marked.";return false;}
 HWND game=GetForegroundWindow();POINT screen,point;RECT rect;
 if(!claim_game(game)||!GetCursorPos(&screen)||GetAncestor(WindowFromPoint(screen),GA_ROOT)!=game||(GetAsyncKeyState(VK_LBUTTON)&0x8000)){
  claim_click.message=L"Not marked. Focus the game, hover the selected task button without holding the mouse button, then press F9.";return false;
 }
 point=screen;
 if(!ScreenToClient(game,&point)||!GetClientRect(game,&rect)||!claim_inside(point,rect))return false;
 claim_click.game=game;claim_click.point=point;claim_click.size.cx=rect.right-rect.left;claim_click.size.cy=rect.bottom-rect.top;
 claim_click.marked=true;claim_click.message=L"Selected task point marked (no click sent). F1 to return, then press the task action once.";return true;
}
static bool claim_context(void){
 RECT rect;
 if(!claim_click.marked||!claim_game(claim_click.game)||!GetClientRect(claim_click.game,&rect)||
  rect.right-rect.left!=claim_click.size.cx||rect.bottom-rect.top!=claim_click.size.cy||!claim_inside(claim_click.point,rect)){
  claim_click.message=L"Game window changed. No click sent. Hover the selected task button and press F9 to mark again.";return false;
 }
 return true;
}
static bool claim_start(HWND owner){
 if(claim_click.attempted||claim_click.phase){return false;}
 if(!claim_click.marked){claim_click.message=L"No point marked for this task. F1 to hide the tool, hover its game button and press F9 (no click), then F1 to return.";return false;}
 if(!claim_context()||!GetCursorPos(&claim_click.restore))return false;
 ShowWindow(owner,SW_HIDE);SetForegroundWindow(claim_click.game);claim_click.phase=1;
 if(!SetTimer(owner,3,200,NULL)){claim_click.phase=0;claim_click.message=L"Could not queue the click. No input sent.";return false;}
 claim_click.message=L"Waiting for game focus. One marked click is queued.";return true;
}
/* -1 stopped, 0 idle/waiting, 1 click input completed. Release a sent press even after focus loss. */
static int claim_tick(HWND owner){
 if(!claim_click.phase)return 0;
 if(claim_click.phase==2){
  KillTimer(owner,3);claim_click.phase=0;
  bool released=claim_release();
  claim_click.message=released?L"One mouse click was sent to the marked task point. Check the result and daily 0/1 -> 1/1 in game. This is not server confirmation. Further presses show status only.":L"Mouse release was blocked. Release the left mouse button manually. No automatic second click.";
  return released?1:-1;
 }
 if(!claim_context()){const wchar_t *reason=claim_click.message;claim_cancel(owner);if(!claim_click.held)claim_click.message=reason;return -1;}
 if(GetForegroundWindow()!=claim_click.game||(GetAsyncKeyState(VK_LBUTTON)&0x8000)){
  claim_cancel(owner);claim_click.message=L"Game lost focus or the mouse button is held. No click sent. Return to the selected game page and retry.";return -1;
 }
 POINT point=claim_click.point;
 if(!ClientToScreen(claim_click.game,&point)||!SetCursorPos(point.x,point.y)){
  claim_cancel(owner);claim_click.message=L"Could not position the cursor. No click sent.";return -1;
 }
 claim_click.target=point;claim_click.moved=true;
 if(GetAncestor(WindowFromPoint(point),GA_ROOT)!=claim_click.game||GetForegroundWindow()!=claim_click.game){
  claim_cancel(owner);claim_click.message=L"The marked point is covered by another window. No click sent.";return -1;
 }
 claim_click.attempted=true;
 if(!claim_mouse(MOUSEEVENTF_LEFTDOWN)){
  claim_cancel(owner);claim_click.message=L"Windows blocked the mouse press. No automatic retry was made.";return -1;
 }
 claim_click.held=true;claim_click.phase=2;
 if(!SetTimer(owner,3,60,NULL)){claim_cancel(owner);claim_click.message=L"Click timing failed; the mouse was released. Check the game; no automatic retry.";return -1;}
 return 0;
}

static void claim_cancel_all(HWND owner){
 int selected=claim_selected;
 for(int i=0;i<2;i++){claim_selected=i;claim_cancel(owner);}
 claim_selected=selected;
}
