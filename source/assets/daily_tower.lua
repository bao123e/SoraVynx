-- SPDX-License-Identifier: AGPL-3.0-or-later
-- Claim Gifts uses the active Dorm UI, not the unrelated InfinityTower request.
local kind,raw,write,protected,advance=type,rawget,rawset,pcall,next
local function result(status,message,code)
 return "SoraVynx - Dorm Claim Gifts preview\r\nStatus: " .. status .. "\r\n" ..
  (code and "Code: " .. code .. "\r\n" or "") .. message ..
  "\r\nInfinityTower and Shop were not called. No battle, upgrade or gift-send task was run.\r\n"
end
local function stop(code,message)return result("STOPPED",message,code)end
local dbg=raw(_G,"debug")
local info=kind(dbg)=="table" and raw(dbg,"getinfo") or nil
local registry=kind(dbg)=="table" and raw(dbg,"getregistry") or nil
local upvalue=kind(dbg)=="table" and raw(dbg,"getupvalue") or nil
local pkg=raw(_G,"package")
local loaded=kind(pkg)=="table" and raw(pkg,"loaded") or nil
local target="Game.UI.Dorm.DormInteractive.DormInteractiveCtrl"
local classes,module_defs={},{ }
if kind(loaded)=="table" then
 local k,v=nil,nil
 for i=1,4096 do
  k,v=advance(loaded,k);if k==nil then break end
  if kind(v)=="table" then
   module_defs[v]=true
   if kind(k)=="string" and k:gsub("/","."):gsub("%.lua$","")==target then classes[v]=true end
  end
 end
end
if advance(classes)==nil then return stop("DORM_MODULE_NOT_LOADED","Open Tyrant's Tower / Hall first, then retry. No action was sent.") end
local function signature(fn)
 if kind(fn)~="function" or kind(info)~="function" then return false end
 local ok,m=protected(info,fn,"u")
 if not ok or kind(m)~="table" or raw(m,"nparams")~=1 or raw(m,"isvararg")~=false then return false end
 local param=raw(dbg,"getlocal")
 if kind(param)=="function" then
  local valid,name=protected(param,fn,1)
  if valid and kind(name)=="string" and name~="self" then return false end
 end
 return true
end
local key="_SORAVYNX_DORM_CLAIM_V1"
local state=raw(_G,key)
if state~=nil then
 if kind(state)~="table" or raw(state,"marker")~="SoraVynx.Dorm.v1" then return stop("DORM_STATE_CHANGED","No further action sent.") end
 return result("TOWER_STATUS",raw(state,"phase")=="returned" and
  "Claim Gifts UI handler returned. Receipt of items and daily completion must be checked in game.\r\nAttempts: 1. Further clicks show status only." or
  "Claim Gifts handler raised an error after the attempt; an action may already have been sent. No retry was made.")
end
local function owner(instance)
 local queue,seen={instance},{}
 local matched,handler=false,nil
 for q=1,24 do
  local t=queue[q];if not t then break end
  if kind(t)=="table" and not seen[t] then
   seen[t]=true;if classes[t] then matched=true end
   if not handler then local fn=raw(t,"OnBtnClick_ReceiveDormReward");if kind(fn)=="function" then handler=fn end end
   for _,name in ipairs({"class","__class","__index","super"}) do local v=raw(t,name);if kind(v)=="table" then queue[#queue+1]=v end end
   local mt=getmetatable(t);if kind(mt)=="table" then queue[#queue+1]=mt end
  end
 end
 return matched and handler or nil
end
-- Active Unity scene objects are required; a Lua class or stale instance alone is insufficient.
local function active_object(value)
 if kind(value)~="userdata" then return false end
 local ok,active=protected(function()
  local direct,active=protected(function()return value.activeInHierarchy end)
  local go=direct and kind(active)=="boolean" and value or value.gameObject
  if go==nil or go.activeInHierarchy~=true then return false end
  local checked,enabled=protected(function()return value.isActiveAndEnabled end)
  if checked and enabled==false then return false end
  local scene=go.scene
  return scene~=nil and scene.isLoaded==true
 end)
 return ok and active==true
end
local function visible(instance)
 local k,v=nil,nil
 for i=1,128 do
  k,v=advance(instance,k);if k==nil then break end
  if active_object(v) then return true end
 end
 return false
end
local roots,queued={},{}
local limited=false
local function enqueue(t,depth)
 if kind(t)~="table" or queued[t] or t==_G or t==loaded or t==pkg or t==raw(_G,"PlayerData") or module_defs[t] then return end
 if #roots>=2048 then limited=true;return end
 queued[t]=true;roots[#roots+1]={t,depth}
end
if kind(registry)=="function" then local ok,r=protected(registry);if ok then enqueue(r,0) end end
-- Known PanelManager closure captures may also retain the live controllers.
local manager=raw(_G,"PanelManager")
if kind(manager)=="table" and kind(upvalue)=="function" then
 for _,name in ipairs({"GetCurPanelId","OpenPanelWithContext","CheckPanelOpen","SwitchAllUI","Home"}) do
  local fn=raw(manager,name)
  if kind(fn)=="function" then
   for index=1,32 do
    local ok,label,value=protected(upvalue,fn,index);if not ok or label==nil then break end
    if label~="_ENV" and label~="_G" then enqueue(value,0) end
   end
  end
 end
end
local candidates,seen_candidates={},{}
local inspected,visited,class_hits=0,0,0
for q=1,2048 do
 local item=roots[q];if not item then break end
 local t,depth=item[1],item[2];visited=visited+1
 if raw(t,"__index")~=t then
  local handler=owner(t)
  if handler then
   class_hits=class_hits+1
   if signature(handler) and visible(t) and not seen_candidates[t] then
    seen_candidates[t]=true;candidates[#candidates+1]={t,handler}
   end
  end
 end
 local k,v=nil,nil
 local complete=false
 for i=1,(q==1 and 4096 or 256) do
  if inspected>=32768 then limited=true;break end
  k,v=advance(t,k);if k==nil then complete=true;break end
  inspected=inspected+1
  if depth<6 and k~="class" and k~="super" and k~="__index" and k~="_ENV" and k~="_G" then enqueue(v,depth+1) end
 end
 if not complete and advance(t,k)~=nil then limited=true end
 if inspected>=32768 then limited=true;break end
end
if limited then return stop("DORM_DISCOVERY_LIMIT","UI discovery reached its bound. No action was sent; no incomplete uniqueness check is accepted.") end
if #candidates~=1 then
 return stop(#candidates==0 and "DORM_ACTIVE_UI_NOT_FOUND" or "DORM_ACTIVE_UI_AMBIGUOUS",
  "No action sent. Open the Hall with Claim Gifts visible.\r\nDiscovery: tables=" .. visited .. "; entries=" .. inspected ..
  "; matching instances=" .. class_hits .. "; active candidates=" .. #candidates .. ".\r\nClass definitions and inactive or unverified objects cannot receive the action.")
end
state={marker="SoraVynx.Dorm.v1",phase="attempted"};write(_G,key,state)
local sent=protected(candidates[1][2],candidates[1][1]);state.phase=sent and "returned" or "handler_error"
if not sent then return result("TOWER_STATUS","Claim Gifts UI handler raised an error after the attempt. Check the game; no retry was made.","DORM_HANDLER_ERROR") end
return result("TOWER_REQUEST_ATTEMPTED","Called the active Dorm Claim Gifts UI handler once.\r\nThe tool is hidden so reward dialogs remain visible. F1 restores the tool; further clicks show status only.\r\nItem receipt and daily completion are not yet confirmed.")
