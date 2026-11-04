-- SPDX-License-Identifier: AGPL-3.0-or-later
-- Resolve from the active UI instance; package.loaded is only an optional identity hint.
local kind, raw, write, protected = type, rawget, rawset, pcall
local function result(status,message,code)
 return "SoraVynx - Daily Gift UI preview\r\nStatus: " .. status .. "\r\n" ..
  (code and "Code: " .. code .. "\r\n" or "") .. message ..
  "\r\nOther daily tasks were not run. Check UI rendering and rewards in game.\r\n"
end
local function stop(code,message) return result("STOPPED",message,code) end
local dbg=raw(_G,"debug")
local info=kind(dbg)=="table" and raw(dbg,"getinfo") or nil
local param=kind(dbg)=="table" and raw(dbg,"getlocal") or nil
local function signature(fn)
 if kind(fn)~="function" then return false,"HANDLER_MISSING" end
 if kind(info)~="function" then return false,"DEBUG_ARITY_UNAVAILABLE" end
 local ok,meta=protected(info,fn,"u")
 if not ok or kind(meta)~="table" then return false,"DEBUG_ARITY_FAILED" end
 local n,vararg=raw(meta,"nparams"),raw(meta,"isvararg")
 if kind(n)~="number" or kind(vararg)~="boolean" then return false,"DEBUG_ARITY_UNAVAILABLE" end
 if n~=1 or vararg then return false,"HANDLER_ARITY_MISMATCH" end
 -- Count/owner/name establish the binding. Missing parameter names do not imply a mismatch.
 if kind(param)=="function" then
  local valid,name=protected(param,fn,1)
  if valid and kind(name)=="string" and name:match("^[%a_][%w_]*$") and name~="self" then
   return false,"HANDLER_PARAMETER_MISMATCH"
  end
 end
 return true
end
local pkg=raw(_G,"package")
local loaded=kind(pkg)=="table" and raw(pkg,"loaded") or nil
local module_names={"Game.UI.ShopEx.ShopCtrl","Game/UI/ShopEx/ShopCtrl","Game/UI/ShopEx/ShopCtrl.lua"}
local module_classes={}
if kind(loaded)=="table" then for i=1,#module_names do
 local t=raw(loaded,module_names[i]);if kind(t)=="table" then module_classes[t]=true end
end end
local player=raw(_G,"PlayerData")
local shop=kind(player)=="table" and raw(player,"Shop") or nil
local function reward_flag()
 if kind(shop)~="table" then return nil end
 local class=raw(shop,"class")
 local getter=kind(class)=="table" and raw(class,"GetDailyShopReward") or nil
 if not signature(getter) then return nil end
 local ok,value=protected(getter,shop)
 if ok and kind(value)=="boolean" then return value end
 return nil
end
local key="_SORAVYNX_DAILY_GIFT_UI_V2"
local state=raw(_G,key)
if state~=nil then
 if kind(state)~="table" or raw(state,"marker")~="SoraVynx.ShopUI.v2" or raw(state,"shop")~=shop then
  return stop("SESSION_CONTEXT_CHANGED","The UI preview session changed. No further handler invocation made.")
 end
 local flag=reward_flag()
 local changed=kind(flag)=="boolean" and kind(state.before)=="boolean" and flag~=state.before
 local message=state.phase=="returned" and "Daily Gift handler returned earlier; it was not called again." or
  "Daily Gift handler raised an error after the attempt; it was not called again."
 if changed then message=message .. "\r\nThe Shop reward flag changed. Confirm the reward in game." end
 return result("UI_STATUS",message .. "\r\nFurther clicks refresh this status only in this game session.")
end
-- Do not repeat an action from a previous UI-command revision in the same VM.
if raw(_G,"_SORAVYNX_DAILY_GIFT_UI_V1")~=nil then
 return stop("OLDER_ATTEMPT_PRESENT","An earlier UI command already attempted this session. Restart the game before testing this revision.")
end
local ok,objects=protected(function()
 local cs,typeof=raw(_G,"CS"),raw(_G,"typeof")
 if kind(cs)~="table" or kind(typeof)~="function" then return nil end
 return cs.UnityEngine.Object.FindObjectsOfType(typeof(cs.LuaBehaviourBase))
end)
if not ok or objects==nil then return stop("UNITY_UI_ENUMERATION_FAILED","The game's Lua UI bridge could not list active behaviours. No handler invoked.") end
local count_ok,count=protected(function() return objects.Length end)
if not count_ok or kind(count)~="number" or count<0 or count>4096 or count%1~=0 then
 return stop("UNITY_UI_LIST_INVALID","The UI bridge returned an unexpected behaviour list. No handler invoked.")
end
local function normalise(name)
 return name:gsub("^@",""):gsub("\\","/"):gsub("/","."):gsub("%.lua$",""):gsub("^%.*","")
end
local target="Game.UI.ShopEx.ShopCtrl"
local function owner_chain(instance)
 local queue,seen,hint={instance},{},false
 local function add(t) if kind(t)=="table" then queue[#queue+1]=t end end
 add(raw(instance,"class"));add(raw(instance,"__class"))
 local mt=getmetatable(instance);add(mt);if kind(mt)=="table" then add(raw(mt,"__index")) end
 local handler=nil
 for i=1,24 do
  local t=queue[i];if not t then break end
  if not seen[t] then
   seen[t]=true;if module_classes[t] then hint=true end
   local fn=raw(t,"OnBtnClick_DailyGift");if not handler and kind(fn)=="function" then handler=fn end
   add(raw(t,"super"));add(raw(t,"__index"))
  end
 end
 return handler,hint
end
local function source_proves_owner(fn)
 if kind(fn)~="function" or kind(info)~="function" then return false end
 local ok,meta=protected(info,fn,"S")
 local source=ok and kind(meta)=="table" and raw(meta,"source") or nil
 if kind(source)~="string" or #source>1024 then return false end
 source=normalise(source)
 return source==target or source:sub(-#target-1)=="." .. target
end
local candidate,handler,found=nil,nil,0
local seen_instances,found_instances={},{}
local named,instances,missing,unresolved,lookup_errors=0,0,0,0,0
for index=0,count-1 do
 local valid,root,full,panel=protected(function()
  local b=objects[index];if not b.isActiveAndEnabled then return nil end
  local name=b.luaClassName;if kind(name)~="string" then return nil end
  name=normalise(name)
  local qualified=name==target or name:sub(-#target-1)=="." .. target
  local panel_name="Game.UI.ShopEx.ShopPanel"
  local is_panel=name==panel_name or name:sub(-#panel_name-1)=="." .. panel_name or name=="ShopPanel" or name=="UI.ShopEx.ShopPanel" or name=="ShopEx.ShopPanel"
  if not qualified and not is_panel and name~="ShopCtrl" and name~="ShopEx.ShopCtrl" and name~="UI.ShopEx.ShopCtrl" then return nil end
  named=named+1
  local value=b.luaInstance;if kind(value)~="table" then return nil end
  return value,qualified,is_panel
 end)
 local candidates={}
 if valid and root~=nil then
  if not panel then candidates[1]={root,full}
  else
   -- Some panels keep ShopCtrl as a pure Lua child rather than a separate MonoBehaviour.
   local queue,walk_seen={{root,0}},{}
   for q=1,64 do
    local item=queue[q];if not item then break end
    local t,depth=item[1],item[2]
    if not walk_seen[t] then
     walk_seen[t]=true;candidates[#candidates+1]={t,false}
     if depth<2 then
      local key,value=nil,nil
      for step=1,256 do
       key,value=next(t,key);if key==nil then break end
       if key~="class" and key~="super" and key~="__index" and kind(value)=="table" and not module_classes[value] and raw(value,"__index")~=value then
        queue[#queue+1]={value,depth+1}
       end
      end
     end
    end
   end
  end
 end
 for c=1,#candidates do
  local instance,qualified=candidates[c][1],candidates[c][2]
  if not seen_instances[instance] or (qualified and seen_instances[instance]~="full") then
  seen_instances[instance]=qualified and "full" or "hint";instances=instances+1
  local fn,hint=owner_chain(instance)
  if not fn then
   -- Normal Lua dispatch can use a function-valued __index. Resolve only the fixed method name.
   local resolved,value=protected(function() return instance.OnBtnClick_DailyGift end)
   if resolved and kind(value)=="function" then fn=value elseif not resolved then lookup_errors=lookup_errors+1 end
  end
  if kind(fn)=="function" and not hint then
   for class in next,module_classes do
    if raw(class,"OnBtnClick_DailyGift")==fn then hint=true;break end
   end
  end
  -- With a short component name, prove its namespace through a loaded class or function source metadata.
  if not qualified and not hint and not source_proves_owner(fn) then unresolved=unresolved+1
  elseif kind(fn)=="function" then
   if not found_instances[instance] then found_instances[instance]=true;found=found+1;candidate=instance;handler=fn end
  else missing=missing+1 end
  end
 end
end
if found>1 then return stop("SHOP_CONTROLLER_AMBIGUOUS","More than one active Shop controller exposes Daily Gift. Wait for the page transition to finish.") end
if found==0 then
 local code=named==0 and "SHOP_CONTROLLER_NOT_FOUND" or instances==0 and "SHOP_INSTANCE_UNAVAILABLE" or
  unresolved>0 and "SHORT_SHOP_NAME_UNRESOLVED" or lookup_errors>0 and "SHOP_HANDLER_LOOKUP_FAILED" or "SHOP_HANDLER_MISSING"
 return stop(code,"No compatible Daily Gift handler resolved from the active UI.\r\nObserved: " .. count ..
  " behaviours; " .. named .. " Shop names; " .. instances .. " UI tables; " .. missing .. " missing handlers.")
end
local compatible,reason=signature(handler)
if not compatible then return stop(reason,"The active Shop's Daily Gift handler failed signature validation. No handler invoked.") end
-- Record before calling: handler exceptions can occur after UI/network work.
state={marker="SoraVynx.ShopUI.v2",shop=shop,handler=handler,phase="attempted",before=reward_flag()}
write(_G,key,state)
local clicked=protected(handler,candidate)
state.phase=clicked and "returned" or "handler_error"
if not clicked then return result("UI_STATUS","The UI handler raised an error after the attempt. No direct request or automatic retry was used.","SHOP_HANDLER_ERROR") end
return result("UI_HANDLER_INVOKED","Called Daily Gift on the live Shop controller once.\r\nThe tool is hidden; press F1 to restore it.")
