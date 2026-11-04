-- SPDX-License-Identifier: AGPL-3.0-or-later
-- Explicit daily Tower reward request only. Shop and battle/upgrade actions are not run.
local kind,raw,write,protected=type,rawget,rawset,pcall
local function result(status,message,code)
 return "SoraVynx - Tower daily reward preview\r\nStatus: " .. status .. "\r\n" ..
  (code and "Code: " .. code .. "\r\n" or "") .. message ..
  "\r\nShop is skipped. No battle, upgrade, gift-send or other daily task was run.\r\n"
end
local function stop(code,message)return result("STOPPED",message,code)end
local player=raw(_G,"PlayerData")
local tower=kind(player)=="table" and raw(player,"InfinityTower") or nil
local class=kind(tower)=="table" and raw(tower,"class") or nil
if kind(tower)~="table" or kind(class)~="table" then return stop("TOWER_INSTANCE_UNAVAILABLE","The existing InfinityTower data instance is not ready.") end
local handler=raw(class,"ITDailyRewardReq")
local dbg=raw(_G,"debug")
local info=kind(dbg)=="table" and raw(dbg,"getinfo") or nil
local param=kind(dbg)=="table" and raw(dbg,"getlocal") or nil
if kind(handler)~="function" then return stop("TOWER_HANDLER_MISSING","ITDailyRewardReq is unavailable on the existing Tower class.") end
if kind(info)~="function" then return stop("TOWER_METADATA_UNAVAILABLE","Function arity metadata is unavailable; no request sent.") end
local ok,meta=protected(info,handler,"u")
if not ok or kind(meta)~="table" or raw(meta,"nparams")~=1 or raw(meta,"isvararg")~=false then
 return stop("TOWER_SIGNATURE_MISMATCH","The expected one-self-argument Tower handler could not be verified; no request sent.")
end
if kind(param)=="function" then
 local valid,name=protected(param,handler,1)
 if valid and kind(name)=="string" and name:match("^[%a_][%w_]*$") and name~="self" then
  return stop("TOWER_PARAMETER_MISMATCH","The Tower handler parameter differs from the confirmed signature.")
 end
end
local pkg=raw(_G,"package")
local loaded=kind(pkg)=="table" and raw(pkg,"loaded") or nil
local proven=false
for _,name in ipairs({"GameCore.Data.DataClass.PlayerInfinityTowerData","GameCore/Data/DataClass/PlayerInfinityTowerData","GameCore/Data/DataClass/PlayerInfinityTowerData.lua"}) do
 if kind(loaded)=="table" and raw(loaded,name)==class then proven=true end
end
if not proven then
 local valid,source_meta=protected(info,handler,"S")
 local source=valid and kind(source_meta)=="table" and raw(source_meta,"source") or nil
 if kind(source)=="string" and #source<=1024 then
  source=source:gsub("^@",""):gsub("\\","/"):gsub("/","."):gsub("%.lua$","")
  local expected="GameCore.Data.DataClass.PlayerInfinityTowerData"
  proven=source==expected or source:sub(-#expected-1)=="." .. expected
 end
end
if not proven then return stop("TOWER_CLASS_UNVERIFIED","The existing Tower object's class could not be linked to the confirmed data module.") end
local rewards=raw(tower,"InfinityTowerRewardsTab")
if kind(rewards)~="table" then return stop("TOWER_STATE_UNAVAILABLE","Tower reward data is not ready; no request sent.") end
-- Observe only this reward table; do not copy scalar values or interpret changes as success.
local function snapshot(root)
 local values,queue,seen,count={},{{root,"",0}},{},0
 for q=1,32 do
  local item=queue[q];if not item then break end
  local t,path,depth=item[1],item[2],item[3]
  if not seen[t] then
   seen[t]=true;local key,value=nil,nil
   for i=1,128 do
    key,value=next(t,key);if key==nil then break end
    if kind(key)=="string" or kind(key)=="number" then
     local p=path .. "/" .. key
     if kind(value)=="boolean" or kind(value)=="number" then values[p]=value;count=count+1
     elseif kind(value)=="table" and depth<2 and key~="class" then queue[#queue+1]={value,p,depth+1} end
    end
   end
  end
 end
 return values,count
end
local key="_SORAVYNX_TOWER_DAILY_V1"
local state=raw(_G,key)
if state~=nil then
 if kind(state)~="table" or raw(state,"marker")~="SoraVynx.Tower.v1" or raw(state,"tower")~=tower or raw(state,"handler")~=handler then
  return stop("TOWER_CONTEXT_CHANGED","The Tower session changed; no new request sent.")
 end
 local now=snapshot(rewards);local changes=0
 for k,v in next,state.before do if now[k]~=v then changes=changes+1 end end
 for k in next,now do if state.before[k]==nil then changes=changes+1 end end
 local message=state.phase=="returned" and "The Tower request handler returned. Server success/reward receipt is not confirmed." or
  "The Tower handler raised an error after the attempt; a request may already have been sent."
 return result("TOWER_STATUS",message .. "\r\nObserved reward-table field changes: " .. changes ..
  ". Changes alone do not prove task completion.\r\nAttempts: 1. Further clicks refresh status only; no automatic retry.")
end
local before,fields=snapshot(rewards)
state={marker="SoraVynx.Tower.v1",tower=tower,handler=handler,before=before,phase="attempted"}
write(_G,key,state)
local sent=protected(handler,tower)
state.phase=sent and "returned" or "handler_error"
if not sent then return result("TOWER_STATUS","The Tower handler raised an error after the attempt. Check the game; no automatic retry was made.","TOWER_HANDLER_ERROR") end
return result("TOWER_REQUEST_ATTEMPTED","Called ITDailyRewardReq on the existing Tower data instance once.\r\nThe tool is hidden so game reward dialogs remain visible. Press F1 to restore it, then click Tower reward / status to check observations.\r\nReward receipt is not yet confirmed.")
