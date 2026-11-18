/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <windows.h>
#include <stdbool.h>
#include <wchar.h>
/* UI-thread only. Calibration never sends input; coordinates are not persisted. */
struct ClaimClick {
 HWND game;POINT point,restore,target;SIZE size;
 bool marked,attempted,held,moved,double_click;int phase,completed_clicks;
 const wchar_t *message;
};
enum { CLAIM_TOWER, CLAIM_COMMISSION_ALL, CLAIM_COMMISSION_TEAM, CLAIM_COMMISSION_START, CLAIM_TARGET_COUNT };
static struct ClaimClick claim_slots[CLAIM_TARGET_COUNT]={[CLAIM_COMMISSION_ALL]={.double_click=true}};
static int claim_selected;
#define claim_click claim_slots[claim_selected]
static bool claim_select(int selected){
 if(selected<0||selected>=CLAIM_TARGET_COUNT)return false;
 for(int i=0;i<CLAIM_TARGET_COUNT;i++)if(claim_slots[i].phase||claim_slots[i].held)return false;
 claim_selected=selected;return true;
}
static bool claim_set_double(bool enabled){
 if(claim_click.attempted)return false;
 for(int i=0;i<CLAIM_TARGET_COUNT;i++)if(claim_slots[i].phase||claim_slots[i].held)return false;
 claim_click.double_click=enabled;return true;
}
static const wchar_t *claim_click_mode_label(void){return claim_click.double_click?L"Clicks: Double / change":L"Clicks: Single / change";}
static const wchar_t *claim_target_label(void){
 static const wchar_t *const labels[]={L"Target: Tower / change",L"Target: Claim All / change",L"Target: Auto team / change",L"Target: Start Dispatch / change"};return labels[claim_selected];
}
static const wchar_t *claim_action_label(void){
 static const wchar_t *const labels[]={L"Claim Gifts / status",L"Claim All / status",L"Auto team / status",L"Start Dispatch / status"};return labels[claim_selected];
}
static const wchar_t *claim_action_title(void){return claim_selected?L"SoraVynx - Commission step / status":L"SoraVynx - Claim Gifts / status";}
static const wchar_t *claim_click_message(void){
 if(claim_click.message)return claim_click.message;
 static const wchar_t *const help[]={
  L"Menu key: hide tool. Hover Claim Gifts, press Mark key to mark (no click). Menu key: return, then Claim Gifts / status.",
  L"Open Commission with completed rewards. Menu key: hide tool, hover CLAIM ALL and press Mark key (mark only). Menu key: return, then Claim All / status. Check the resulting screen and commission limit before continuing.",
  L"Use only when the game's auto-team button is visible. Menu key: hide tool, hover that button and press Mark key (mark only). Menu key: return, then Auto team / status. This step does not automatically press Start Dispatch.",
  L"Use only when the game permits dispatch and the team is ready. Menu key: hide tool, hover START DISPATCH and press Mark key (mark only). Menu key: return, then Start Dispatch / status."};return help[claim_selected];
}
static const wchar_t *claim_click_hint(void){
 if(claim_click.phase)return claim_click.double_click?L"Double click is queued.\r\nKeep the game focused.":L"One click is queued.\r\nKeep the game focused.";
 if(claim_click.attempted)return L"ONE ATTEMPT USED\r\nCheck the game and daily.\r\nButton shows status only.";
 if(claim_click.marked)return L"POINT MARKED (no click)\r\nKeep the game page open.\r\nUse this step's action.\r\nCheck the result in game.";
 static const wchar_t *const hints[]={
  L"Menu key hide; hover Claim Gifts\r\nMark key mark (no click)\r\nMenu key return to tool\r\nClaim Gifts / status",
  L"Menu key hide; hover CLAIM ALL\r\nMark key mark (no click)\r\nMenu key return to tool\r\nClaim All / status",
  L"Menu key hide; hover auto-team\r\nMark key mark (no click)\r\nMenu key return to tool\r\nAuto team / status",
  L"Menu key hide; hover START\r\nMark key mark (no click)\r\nMenu key return to tool\r\nStart Dispatch / status"};return hints[claim_selected];
}
static bool claim_game(HWND game){
 DWORD pid=0;wchar_t name[64]={0};
 return game&&IsWindow(game)&&!IsIconic(game)&&GetWindowThreadProcessId(game,&pid)&&pid==GetCurrentProcessId()&&GetClassNameW(game,name,64)&&!wcscmp(name,L"UnityWndClass");
}
static bool claim_inside(POINT p,RECT r){return p.x>=r.left&&p.x<r.right&&p.y>=r.top&&p.y<r.bottom;}
static bool claim_mouse(DWORD flags){INPUT input={0};input.type=INPUT_MOUSE;input.mi.dwFlags=flags;return SendInput(1,&input,sizeof(input))==1;}
static bool claim_release_press(void){
 if(claim_click.held){for(int i=0;i<3;i++)if(claim_mouse(MOUSEEVENTF_LEFTUP)){claim_click.held=false;break;}}
 return !claim_click.held;
}
static bool claim_release(void){
 claim_release_press();
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
  claim_click.message=L"Not marked. Focus the game, hover the selected task button without holding the mouse button, then press Mark key.";return false;
 }
 point=screen;
 if(!ScreenToClient(game,&point)||!GetClientRect(game,&rect)||!claim_inside(point,rect))return false;
 claim_click.game=game;claim_click.point=point;claim_click.size.cx=rect.right-rect.left;claim_click.size.cy=rect.bottom-rect.top;
 claim_click.marked=true;claim_click.message=L"Selected task point marked (no click sent). Menu key to return, then press the task action once.";return true;
}
static bool claim_context(void){
 RECT rect;
 if(!claim_click.marked||!claim_game(claim_click.game)||!GetClientRect(claim_click.game,&rect)||
  rect.right-rect.left!=claim_click.size.cx||rect.bottom-rect.top!=claim_click.size.cy||!claim_inside(claim_click.point,rect)){
  claim_click.message=L"Game window changed. No click sent. Hover the selected task button and press Mark key to mark again.";return false;
 }
 return true;
}
static bool claim_start(HWND owner){
 if(claim_click.attempted||claim_click.phase){return false;}
 if(!claim_click.marked){claim_click.message=L"No point marked for this task. Menu key to hide the tool, hover its game button and press Mark key (no click), then Menu key to return.";return false;}
 if(!claim_context()||!GetCursorPos(&claim_click.restore))return false;
 ShowWindow(owner,SW_HIDE);SetForegroundWindow(claim_click.game);claim_click.phase=1;claim_click.completed_clicks=0;
 if(!SetTimer(owner,3,350,NULL)){claim_click.phase=0;claim_click.message=L"Could not queue the click. No input sent.";return false;}
 claim_click.message=claim_click.double_click?L"Waiting for game focus. One double-click batch is queued.":L"Waiting for game focus. One marked click is queued.";return true;
}
/* -1 stopped, 0 idle/waiting, 1 click input completed. Release a sent press even after focus loss. */
static int claim_tick(HWND owner){
 if(!claim_click.phase)return 0;
 if(claim_click.phase==5){
  KillTimer(owner,3);claim_click.phase=0;claim_release();
  claim_click.message=claim_click.completed_clicks==2?L"Two mouse click pairs were sent at the marked point. This is not server confirmation. Check the game; further presses show status only.":L"One mouse click was sent to the marked task point. Check the result and daily in game. This is not server confirmation. Further presses show status only.";
  return 1;
 }
 if(claim_click.phase==2||claim_click.phase==4){
  bool first=claim_click.phase==2;
  if(!claim_release_press()){claim_cancel(owner);claim_click.message=L"Mouse release was blocked. Release the mouse manually. No further click was sent.";return -1;}
  claim_click.completed_clicks++;
  if(first&&claim_click.double_click){
   claim_click.phase=3;
   if(!SetTimer(owner,3,120,NULL)){claim_cancel(owner);claim_click.message=L"Second-click timing failed. First click may have acted; no retry.";return -1;}
   return 0;
  }
  /* Let the game process UP at the target before restoring the cursor. */
  claim_click.phase=5;
  if(!SetTimer(owner,3,100,NULL)){
   KillTimer(owner,3);claim_click.phase=0;claim_click.moved=false;
   claim_click.message=L"Click input was sent but final settle timing failed. Cursor left at the target; check the game. No retry.";return -1;
  }
  return 0;
 }
 bool second=claim_click.phase==3;
 if(!claim_context()){const wchar_t *reason=claim_click.message;claim_cancel(owner);if(!claim_click.held)claim_click.message=second?L"Game window changed after the first click. Second click skipped; check the game.":reason;return -1;}
 if(GetForegroundWindow()!=claim_click.game||(GetAsyncKeyState(VK_LBUTTON)&0x8000)){
  claim_cancel(owner);claim_click.message=second?L"Focus or mouse state changed. Second click skipped; first click may have acted.":L"Game lost focus or the mouse button is held. No click sent. Return to the selected game page and retry.";return -1;
 }
 POINT point=claim_click.point,now;
 if(!ClientToScreen(claim_click.game,&point)){claim_cancel(owner);return -1;}
 if(second){
  if(!GetCursorPos(&now)||now.x!=claim_click.target.x||now.y!=claim_click.target.y||point.x!=claim_click.target.x||point.y!=claim_click.target.y){
   claim_cancel(owner);claim_click.message=L"Cursor or game position changed after the first click. Second click skipped.";return -1;
  }
 }else{
  if(!SetCursorPos(point.x,point.y)){claim_cancel(owner);claim_click.message=L"Could not position the cursor. No click sent.";return -1;}
  claim_click.target=point;claim_click.moved=true;
 }
 if(GetAncestor(WindowFromPoint(point),GA_ROOT)!=claim_click.game||GetForegroundWindow()!=claim_click.game){
  claim_cancel(owner);claim_click.message=second?L"The point became covered. Second click skipped; first click may have acted.":L"The marked point is covered by another window. No click sent.";return -1;
 }
 claim_click.attempted=true;
 if(!claim_mouse(MOUSEEVENTF_LEFTDOWN)){
  claim_cancel(owner);claim_click.message=second?L"Windows blocked the second press. First click may have acted; no automatic retry.":L"Windows blocked the mouse press. No automatic retry was made.";return -1;
 }
 claim_click.held=true;claim_click.phase=second?4:2;
 if(!SetTimer(owner,3,80,NULL)){claim_cancel(owner);claim_click.message=L"Click timing failed; the mouse was released. Check the game; no automatic retry.";return -1;}
 return 0;
}

static void claim_cancel_all(HWND owner){
 int selected=claim_selected;
 for(int i=0;i<CLAIM_TARGET_COUNT;i++){claim_selected=i;claim_cancel(owner);}
 claim_selected=selected;
}
