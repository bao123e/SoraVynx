-- SPDX-License-Identifier: AGPL-3.0-or-later
-- One explicit free-Shop claim attempt per game session. Never purchase goods.
local kind, raw, write, protected = type, rawget, rawset, pcall
local concat = table.concat
local function result(message)
 return "SoraVynx - Shop daily gift preview\r\n" .. message ..
  "\r\nOnly the free daily Shop request is supported here. Other daily tasks were not run.\r\n" ..
  "No purchase, upgrade, gift-send or battle request was made by this action.\r\n"
end
local pkg=raw(_G,"package")
local loaded=kind(pkg)=="table" and raw(pkg,"loaded") or nil
local class=kind(loaded)=="table" and raw(loaded,"GameCore.Data.DataClass.PlayerShopData") or nil
local player=raw(_G,"PlayerData")
local shop=kind(player)=="table" and raw(player,"Shop") or nil
if kind(class)~="table" or kind(shop)~="table" or raw(shop,"class")~=class then
 return result("Stopped: the confirmed PlayerData.Shop instance is not available. No request sent.")
end
local sender, getter=raw(class,"SendDailyShopRewardReceiveReq"),raw(class,"GetDailyShopReward")
local dbg=raw(_G,"debug")
local info=kind(dbg)=="table" and raw(dbg,"getinfo") or nil
local param=kind(dbg)=="table" and raw(dbg,"getlocal") or nil
local function matches(fn,n,names)
 if kind(fn)~="function" or kind(info)~="function" or kind(param)~="function" then return false end
 local ok,meta=protected(info,fn,"u")
 if not ok or kind(meta)~="table" or raw(meta,"nparams")~=n or raw(meta,"isvararg")~=false then return false end
 for i=1,n do
  local valid,name=protected(param,fn,i)
  if not valid or name~=names[i] then return false end
 end
 return true
end
if not matches(sender,2,{"self","callback"}) or not matches(getter,1,{"self"}) then
 return result("Stopped: Shop function signatures differ from report v5 or debug metadata is unavailable. No request sent.")
end
local key="_SORAVYNX_SHOP_CLAIM_V1"
local state=raw(_G,key)
local function status(s)
 local text="Request attempts: 1 (no automatic retry).\r\n"
 if s.phase=="handler_error" then
  text=text .. "The request handler raised an error; a request may already have been sent. Check the game."
 elseif s.callbacks>0 then
  text=text .. "Callback received: " .. s.callbacks .. ". Server success has not been decoded.\r\n"
  text=text .. "Callback argument kinds: " .. (s.kinds or "none") .. ".\r\n"
  if s.changed then text=text .. "The Shop reward flag changed; confirm the reward in game."
  else text=text .. "No verified reward-flag change was observed; confirm the result in game." end
 else
  text=text .. "Request handler returned. Waiting for its callback; reward receipt is not confirmed."
 end
 return result(text .. "\r\nClick Shop gift / status again to refresh this status without resending.")
end
if state~=nil then
 if kind(state)~="table" or raw(state,"marker")~="SoraVynx.Shop.v1" or raw(state,"shop")~=shop or raw(state,"sender")~=sender then
  return result("Stopped: Shop session context changed or the preview state is incompatible. No new request sent.")
 end
 -- Read-only refresh of the specific reward flag, never exported as a raw value.
 local ok,value=protected(getter,shop)
 if ok and kind(value)=="boolean" and value~=state.before then state.changed=true end
 return status(state)
end
local ok,before=protected(getter,shop)
if not ok or kind(before)~="boolean" then
 return result("Stopped: the Shop reward getter did not return the expected boolean. No request sent.")
end
-- Persist the attempt before calling the sender so exceptions cannot cause retries.
state={marker="SoraVynx.Shop.v1",shop=shop,sender=sender,phase="attempted",callbacks=0,before=before,changed=false}
write(_G,key,state)
local function callback(...)
 -- Do not interpret unknown callback payload values as success or print them.
 state.callbacks=state.callbacks+1
 local kinds={}
 for i=1,math.min(select("#",...),8) do kinds[#kinds+1]=kind(select(i,...)) end
 state.kinds=#kinds>0 and concat(kinds,",") or "none"
 local valid,value=protected(getter,shop)
 if valid and kind(value)=="boolean" and value~=state.before then state.changed=true end
end
local sent=protected(sender,shop,callback)
state.phase=sent and "returned" or "handler_error"
return status(state)
