-- SPDX-License-Identifier: AGPL-3.0-or-later
-- Invoke the real, active Shop UI button handler once. No direct-request fallback.
local kind, raw, write, protected = type, rawget, rawset, pcall
local function result(status,message)
 return "SoraVynx - Daily Gift UI preview\r\nStatus: " .. status .. "\r\n" .. message ..
  "\r\nOther daily tasks were not run. UI rendering and reward receipt must be checked in game.\r\n"
end
local pkg=raw(_G,"package")
local loaded=kind(pkg)=="table" and raw(pkg,"loaded") or nil
local class=kind(loaded)=="table" and raw(loaded,"Game.UI.ShopEx.ShopCtrl") or nil
local handler=kind(class)=="table" and raw(class,"OnBtnClick_DailyGift") or nil
local dbg=raw(_G,"debug")
local info=kind(dbg)=="table" and raw(dbg,"getinfo") or nil
local param=kind(dbg)=="table" and raw(dbg,"getlocal") or nil
local function matches(fn,n,names)
 if kind(fn)~="function" or kind(info)~="function" or kind(param)~="function" then return false end
 local ok,meta=protected(info,fn,"u")
 if not ok or kind(meta)~="table" or raw(meta,"nparams")~=n or raw(meta,"isvararg")~=false then return false end
 for i=1,n do local valid,name=protected(param,fn,i);if not valid or name~=names[i] then return false end end
 return true
end
if not matches(handler,1,{"self"}) then
 return result("STOPPED","Open Shop first. The expected Daily Gift UI handler or its signature metadata is unavailable. No button handler invoked.")
end
local player=raw(_G,"PlayerData")
local shop=kind(player)=="table" and raw(player,"Shop") or nil
local data=kind(loaded)=="table" and raw(loaded,"GameCore.Data.DataClass.PlayerShopData") or nil
local getter=kind(data)=="table" and raw(data,"GetDailyShopReward") or nil
local function reward_flag()
 if kind(shop)~="table" or raw(shop,"class")~=data or not matches(getter,1,{"self"}) then return nil end
 local ok,value=protected(getter,shop)
 if ok and kind(value)=="boolean" then return value end
 return nil
end
local key="_SORAVYNX_DAILY_GIFT_UI_V1"
local state=raw(_G,key)
if state~=nil then
 if kind(state)~="table" or raw(state,"marker")~="SoraVynx.ShopUI.v1" or raw(state,"handler")~=handler then
  return result("STOPPED","The UI preview context changed. No new handler invocation made.")
 end
 local flag=reward_flag()
 local changed=kind(flag)=="boolean" and kind(state.before)=="boolean" and flag~=state.before
 local message=state.phase=="returned" and "Daily Gift button handler returned. It has not been invoked again." or
  "Daily Gift handler raised an error after the attempt. No automatic retry was made."
 if changed then message=message .. "\r\nThe Shop reward flag changed; this is not independent proof of task completion." end
 return result("UI_STATUS",message .. "\r\nUse F1 to show the tool. Further clicks only refresh this status in this game session.")
end
-- CS/typeof and LuaBehaviourBase public properties are exposed by the game's XLua bridge.
local ok,objects=protected(function()
 local cs,typeof=raw(_G,"CS"),raw(_G,"typeof")
 if kind(cs)~="table" or kind(typeof)~="function" then return nil end
 return cs.UnityEngine.Object.FindObjectsOfType(typeof(cs.LuaBehaviourBase))
end)
if not ok or objects==nil then return result("STOPPED","Could not enumerate live Lua UI behaviours. Open Shop and retry. No direct request was sent.") end
local count_ok,count=protected(function() return objects.Length end)
if not count_ok or kind(count)~="number" or count<0 or count>4096 or count%1~=0 then
 return result("STOPPED","Unexpected UI behaviour list. No handler invoked.")
end
local function owns(t)
 if kind(t)~="table" then return false end
 if raw(t,"class")==class then return true end
 local mt=getmetatable(t)
 return mt==class or (kind(mt)=="table" and raw(mt,"__index")==class)
end
local candidate,found=nil,0
for index=0,count-1 do
 local valid,instance=protected(function()
  local behaviour=objects[index]
  if not behaviour.isActiveAndEnabled then return nil end
  local name=behaviour.luaClassName
  if kind(name)~="string" then return nil end
  name=name:gsub("\\","/"):gsub("/","."):gsub("%.lua$","")
  if name~="Game.UI.ShopEx.ShopCtrl" and name~="ShopCtrl" then return nil end
  local value=behaviour.luaInstance
  if owns(value) then return value end
 end)
 if valid and instance~=nil then
  -- Deduplicate wrappers that refer to the same actual Lua controller.
  if candidate~=instance then found=found+1;candidate=instance end
 end
end
if found~=1 then
 return result("STOPPED",found==0 and "No active Shop controller matched. Open Shop, keep it visible, then retry." or
  "More than one active Shop controller matched. Wait for the page transition to finish and retry.")
end
-- Record before invoking: even an exception may happen after an actual UI/network action.
state={marker="SoraVynx.ShopUI.v1",handler=handler,phase="attempted",before=reward_flag()}
write(_G,key,state)
local clicked=protected(handler,candidate)
state.phase=clicked and "returned" or "handler_error"
if not clicked then return result("UI_STATUS","The Daily Gift handler raised an error. It may have partially executed; no direct-request fallback or automatic retry was used.") end
return result("UI_HANDLER_INVOKED","Called the active Shop controller's Daily Gift button handler once.\r\nThe tool is hidden so the game UI remains visible. Press F1 to restore it.")
