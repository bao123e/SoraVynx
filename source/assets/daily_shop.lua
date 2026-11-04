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
-- Unity's normal finder excludes inactive/DontSave objects. Record every route separately.
local records,record_seen={},{}
local function class_definition(t)
 return module_classes[t] or raw(t,"__index")==t or raw(t,"class")==t or
  (kind(raw(t,"new"))=="function" and (kind(raw(t,"__cname"))=="string" or kind(raw(t,"ctor"))=="function"))
end
local discovery={normal="unavailable",all="unavailable",controllers="unavailable",registry=0,shop="unavailable"}
local enumeration_ok=false
local function add_behaviour(b,active_proof)
 if b~=nil and not record_seen[b] then record_seen[b]=true;records[#records+1]={behaviour=b,active=active_proof} end
end
local function active_scene(component)
 local ok,active=protected(function()
  local go=component.gameObject
  if component.isActiveAndEnabled~=true or go==nil or go.activeInHierarchy~=true then return false end
  local scene=go.scene;if scene==nil then return false end
  local valid_ok,valid=protected(function() return scene:IsValid() end)
  local loaded_ok,loaded_scene=protected(function() return scene.isLoaded end)
  if (valid_ok and valid==false) or (loaded_ok and loaded_scene==false) then return false end
  return (valid_ok and valid==true) or (loaded_ok and loaded_scene==true)
 end)
 return ok and active==true
end
local function enumerate(route,controller)
 local ok,objects=protected(function()
  local cs,typeof=raw(_G,"CS"),raw(_G,"typeof")
  if kind(cs)~="table" or kind(typeof)~="function" then return nil end
  local type_class;if controller then type_class=cs.LuaUIController else type_class=cs.LuaBehaviourBase end
  if type_class==nil then return nil end
  local target_type=typeof(type_class)
  if route=="normal" then return cs.UnityEngine.Object.FindObjectsOfType(target_type) end
  return cs.UnityEngine.Resources.FindObjectsOfTypeAll(target_type)
 end)
 if not ok or objects==nil then return end
 local valid,length=protected(function() return objects.Length end)
 if not valid or kind(length)~="number" or length<0 or length%1~=0 or length>(route=="normal" and 4096 or 8192) then
  discovery[route]="invalid";return
 end
 enumeration_ok=true;discovery[route]=length
 for i=0,length-1 do
  local valid,b,proof=protected(function()
   local object=objects[i]
   if controller then
    if active_scene(object) then return object.luaBehaviour,true end
   elseif route=="normal" then return object,false
   elseif active_scene(object) then return object,true end
  end)
  if valid then add_behaviour(b,proof) end
 end
end
enumerate("normal",false)
enumerate("all",false)
enumerate("controllers",true)
-- A pure Lua UI can be kept in PanelManager closures without any LuaBehaviour component.
-- The registry route is only enabled when the known zero-argument getter proves Shop is current.
local panel_manager=raw(_G,"PanelManager")
local panel_ids=raw(_G,"PanelId")
if kind(panel_ids)~="table" and kind(loaded)=="table" then panel_ids=raw(loaded,"GameCore.UI.PanelId") end
local function zero_arg(fn)
 if kind(fn)~="function" or kind(info)~="function" then return false end
 local ok,meta=protected(info,fn,"u")
 return ok and kind(meta)=="table" and raw(meta,"nparams")==0 and raw(meta,"isvararg")==false
end
local current_id=nil
local enum_keys={}
local shop_ids={}
if kind(panel_ids)=="table" then
 local key,value=nil,nil
 for i=1,2048 do
  key,value=next(panel_ids,key);if key==nil then break end
  local enum_name,enum_id=key,value
  if kind(key)=="number" and kind(value)=="string" then enum_name,enum_id=value,key end
  if kind(enum_name)=="string" and #enum_name<=80 then
   local name=enum_name:lower():gsub("[^%w]","")
   if enum_name:match("^[%a_][%w_]*$") and name:find("shop",1,true) and #enum_keys<12 then enum_keys[#enum_keys+1]=enum_name end
   if name=="shop" or name=="shopex" or name=="shoppanel" or name=="panelshop" or name=="panelshopex" then
    if kind(enum_id)=="number" or kind(enum_id)=="string" then shop_ids[enum_id]=true end
   end
  end
 end
end
local current_getter=kind(panel_manager)=="table" and raw(panel_manager,"GetCurPanelId") or nil
if not zero_arg(current_getter) then discovery.shop="getter_unavailable"
elseif next(shop_ids)==nil then discovery.shop="enum_unavailable"
else
 local ok,value=protected(current_getter)
 if ok and (kind(value)=="number" or kind(value)=="string") then
  current_id=value;discovery.shop=shop_ids[value] and "verified" or "not_current"
 else discovery.shop=ok and "getter_type_unsupported" or "getter_failed" end
end
local upvalue=kind(dbg)=="table" and raw(dbg,"getupvalue") or nil
discovery.upvalues=kind(upvalue)=="function" and "ready" or "unavailable"
if discovery.shop=="verified" and kind(upvalue)=="function" then
 local roots,queued,registry_seen={},{},{}
 local function enqueue(t,depth)
  if kind(t)=="table" and t~=_G and t~=player and t~=loaded and t~=panel_ids and not queued[t] then
   queued[t]=true;roots[#roots+1]={t,depth}
  end
 end
 local functions,seen_functions,sources={},{},{}
 local function source_of(fn)
  if kind(info)~="function" then return nil end
  local ok,meta=protected(info,fn,"S")
  return ok and kind(meta)=="table" and raw(meta,"source") or nil
 end
 for _,name in ipairs({"GetCurPanelId","OpenPanelWithContext","CheckPanelOpen","SwitchAllUI","Home"}) do
  local fn=raw(panel_manager,name)
  if kind(fn)=="function" then
   functions[#functions+1]={fn,0};local source=source_of(fn);if kind(source)=="string" then sources[source]=true end
  end
 end
 for q=1,32 do
  local item=functions[q];if not item then break end
  local fn,depth=item[1],item[2]
  if not seen_functions[fn] then
   seen_functions[fn]=true
   for index=1,32 do
    local ok,label,value=protected(upvalue,fn,index);if not ok or label==nil then break end
    if label~="_ENV" and label~="_G" then
     if kind(value)=="table" then enqueue(value,0)
     elseif kind(value)=="function" and depth<2 and sources[source_of(value)] then functions[#functions+1]={value,depth+1} end
    end
   end
  end
 end
 for q=1,128 do
  local item=roots[q];if not item then break end
  local t,depth=item[1],item[2]
  local entry=raw(t,current_id)
  if kind(entry)=="table" and not class_definition(entry) and not registry_seen[entry] then
   registry_seen[entry]=true;discovery.registry=discovery.registry+1
   records[#records+1]={root=entry,registry=true}
  elseif kind(entry)=="userdata" then
   local ok,b=protected(function() if active_scene(entry) then return entry.luaBehaviour end end)
   if ok then add_behaviour(b,true) end
  end
  if depth<2 then
   local key,value=nil,nil
   for step=1,256 do
    key,value=next(t,key);if key==nil then break end
    if key~="class" and key~="__index" and key~="super" and kind(value)=="table" and raw(value,"__index")~=value then enqueue(value,depth+1) end
   end
  end
 end
end
local function discovery_text()
 return "Discovery: normal=" .. discovery.normal .. "; all=" .. discovery.all .. "; UIControllers=" ..
  discovery.controllers .. "; registry=" .. discovery.registry .. "; currentShop=" .. discovery.shop .. "; upvalues=" .. discovery.upvalues .. "."
end
if discovery.shop=="not_current" then return stop("SHOP_NOT_CURRENT","PanelManager reports a different current page. No handler invoked.\r\n" .. discovery_text()) end
if not enumeration_ok and #records==0 then return stop("UNITY_UI_ENUMERATION_FAILED","The available UI discovery routes failed.\r\n" .. discovery_text()) end
local count=#records
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
  local record=records[index+1]
  if record.registry then named=named+1;return record.root,false,true end
  local b=record.behaviour;if not record.active and not b.isActiveAndEnabled then return nil end
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
       if key~="class" and key~="super" and key~="__index" and kind(value)=="table" and not class_definition(value) then
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
  if kind(fn)~="function" then missing=missing+1
  elseif not qualified and not hint and not source_proves_owner(fn) then unresolved=unresolved+1
  elseif not found_instances[instance] then found_instances[instance]=true;found=found+1;candidate=instance;handler=fn end
  end
 end
end
if found>1 then return stop("SHOP_CONTROLLER_AMBIGUOUS","More than one active Shop controller exposes Daily Gift. Wait for the page transition to finish.") end
if found==0 then
 local code=named==0 and "SHOP_CONTROLLER_NOT_FOUND" or instances==0 and "SHOP_INSTANCE_UNAVAILABLE" or
  unresolved>0 and "SHORT_SHOP_NAME_UNRESOLVED" or lookup_errors>0 and "SHOP_HANDLER_LOOKUP_FAILED" or "SHOP_HANDLER_MISSING"
 return stop(code,"No compatible Daily Gift handler resolved from the active UI.\r\nObserved: " .. count ..
  " eligible behaviours/roots; " .. named .. " Shop roots; " .. instances .. " UI tables; " .. missing .. " missing handlers.\r\n" .. discovery_text() ..
  (#enum_keys>0 and "\r\nShop enum keys: " .. table.concat(enum_keys,",") or ""))
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
