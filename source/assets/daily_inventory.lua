-- SPDX-License-Identifier: AGPL-3.0-or-later
-- Fixed inspection: no require(), discovered function calls, scalar values or bodies.
local kind, raw, advance, protected = type, rawget, next, pcall
local lower, find, match = string.lower, string.find, string.match
local sort, concat = table.sort, table.concat
local getmeta = getmetatable
local dbg = raw(_G, "debug")
local infofn = kind(dbg) == "table" and raw(dbg, "getinfo") or nil
local localfn = kind(dbg) == "table" and raw(dbg, "getlocal") or nil
local MAX_ENTRIES, MAX_LINES, MAX_BYTES = 30000, 2400, 196608
local entries, bytes, lines, limited = 0, 0, {}, false
local function identifier(s)
 return kind(s) == "string" and #s <= 160 and match(s, "^[%a_][%w_./%-]*$") ~= nil
end
local function emit(s)
 if #lines >= MAX_LINES or bytes + #s + 2 > MAX_BYTES then limited = true; return false end
 lines[#lines + 1], bytes = s, bytes + #s + 2
 return true
end
local function members(t)
 local list, key, inspected = {}, nil, 0
 if kind(t) ~= "table" then return list end
 while entries < MAX_ENTRIES and inspected < 2048 do
  local value
  key, value = advance(t, key)
  if key == nil then break end
  entries, inspected = entries + 1, inspected + 1
  if identifier(key) and key ~= "super" and key ~= "__index" and key ~= "__newindex" then
   local k = kind(value)
   if k == "table" or k == "function" then list[#list + 1] = {key, value, k} end
  end
 end
 if entries >= MAX_ENTRIES or inspected >= 2048 then limited = true end
 sort(list, function(a,b) return a[1] < b[1] end)
 return list
end
local tasks = {"daily", "quest", "mission", "affair", "dispatch", "commission",
 "heartlink", "affinity", "dating", "dorm", "trekker", "disc", "character",
 "trial", "ascension", "tower", "vigor", "shop", "gift", "checkin"}
local core = {
 "PlayerQuestData", "PlayerDailyData", "PlayerShopData", "DispatchData",
 "PlayerDiscData", "PlayerCharData", "PlayerDatingData", "PlayerDormData",
 "PlayerInfinityTowerData", "PlayerDailyInstanceData", "PlayerTrialData",
 "PlayerMainlineDataEx", "PlayerEquipmentInstanceData", "PlayerSkillInstanceData",
 "PlayerStarTowerData", "PlayerRoguelikeData", "PlayerItemData", "PlayerCoinData"
}
local exact = {}
for i=1,#core do exact["GameCore.Data.DataClass." .. core[i]] = true end
local function priority(name)
 if exact[name] then return 0 end
 if find(name, "Game.UI.", 1, true) == 1 then
  local s=lower(name)
  if find(s,"activity",1,true) then return nil end
  for i=1,#tasks do if find(s,tasks[i],1,true) then return 1 end end
  if find(s,"mainview",1,true) or find(s,"character",1,true) then return 1 end
 end
 if name == "GameCore.Data.PlayerData" or name == "GameCore.UI.PanelManager" or
    name == "GameCore.Module.ModuleManager" or name == "Game.Common.Utils.JumpUtil" then return 2 end
 return nil
end
local function actionrank(name)
 local s=lower(name)
 local words={"send", "receive", "claim", "req", "apply", "gift", "upgrade", "strengthen", "enter", "onbtnclick", "getcur", "getsel", "getlast", "getdaily", "getquest", "checkdaily"}
 for i=1,#words do if find(s,words[i],1,true) then return 0 end end
 return 1
end
local function signature(fn)
 local params, count, vararg, names = {}, "unknown", "unknown", false
 if kind(infofn) == "function" then
  local ok, info = protected(infofn, fn, "u")
  if ok and kind(info) == "table" then
   local n=raw(info,"nparams")
   if kind(n)=="number" and n>=0 and n<=128 and n%1==0 then count=n end
   local v=raw(info,"isvararg")
   if kind(v)=="boolean" then vararg=v and "yes" or "no" end
  end
 end
 if kind(localfn) == "function" then
  for i=1,16 do
   local ok, name = protected(localfn, fn, i)
   if not ok or name==nil then break end
   if identifier(name) then params[#params+1]=name;names=true else params[#params+1]="?" end
  end
 end
 return " [nparams=" .. count .. "; vararg=" .. vararg .. "; params=" ..
  (names and concat(params,",") or "unknown") .. "]"
end
local pkg=raw(_G,"package")
local loaded=kind(pkg)=="table" and raw(pkg,"loaded") or nil
local modules=members(loaded)
local classnames={}
for i=1,#modules do if modules[i][3]=="table" then classnames[modules[i][2]]=modules[i][1] end end
local targets={}
for i=1,#modules do
 local item=modules[i]
 emit("MODULE " .. item[1] .. " " .. item[3])
 local rank=priority(item[1])
 if rank and item[3]=="table" then targets[#targets+1]={name=item[1],value=item[2],rank=rank} end
end
for i=1,#core do
 local name="GameCore.Data.DataClass." .. core[i]
 if kind(loaded)~="table" or kind(raw(loaded,name))~="table" then emit("NOT_LOADED " .. name) end
end
sort(targets,function(a,b) if a.rank~=b.rank then return a.rank<b.rank end return a.name<b.name end)
-- Detail only declared functions. Never recurse into superclass aliases or runtime data.
for i=1,#targets do
 local target=targets[i]
 target.functions={}
 for _,item in ipairs(members(target.value)) do
  if item[3]=="function" then target.functions[#target.functions+1]=item end
 end
 sort(target.functions,function(a,b)
  local ra,rb=actionrank(a[1]),actionrank(b[1]);if ra~=rb then return ra<rb end return a[1]<b[1]
 end)
 target.done,target.used=0,0
 emit("TARGET " .. target.name .. " declared-functions=" .. #target.functions)
end
local function classof(t)
 if kind(t)~="table" then return nil end
 local mt=getmeta(t) -- Does not execute __index, __pairs or __tostring.
 if kind(mt)=="table" then
  if classnames[mt] then return classnames[mt] end
  local index=raw(mt,"__index")
  if kind(index)=="table" and classnames[index] then return classnames[index] end
 end
 for _,key in ipairs({"class","__class","_class"}) do
  local class=raw(t,key)
  if kind(class)=="table" and classnames[class] then return classnames[class] end
 end
 return classnames[t]
end
local function shape(value,path,depth)
 local class=classof(value)
 if class then emit("OBJECT " .. path .. " class=" .. class) else emit("OBJECT " .. path .. " class=unknown") end
 local items=members(value)
 for i=1,#items do
  local item=items[i]
  emit("FIELD " .. path .. "." .. item[1] .. " " .. item[3])
  if depth==0 and item[3]=="table" then
   local name=classof(item[2])
   if name and priority(name) then shape(item[2],path .. "." .. item[1],1) end
  end
 end
end
-- Only structural names of the existing PlayerData/PanelManager objects are copied.
for _,name in ipairs({"PlayerData","PanelManager"}) do
 local object=raw(_G,name)
 if kind(object)=="table" then shape(object,"global/" .. name,0) else emit("NOT_PRESENT global/" .. name) end
end
-- Round-robin quotas prevent a large early class from starving later classes.
for index=1,96 do
 for i=1,#targets do
  local target=targets[i]
  local item=target.functions[index]
  if item then
   local line="FUNCTION loaded/" .. target.name .. "." .. item[1] .. signature(item[2])
   if target.used+#line+2<=12288 then
    if emit(line) then target.done=target.done+1 end
    target.used=target.used+#line+2
   else limited=true end
  end
 end
end
for i=1,#targets do
 local target=targets[i]
 if target.done<#target.functions then limited=true end
 emit("DETAIL_COVERAGE " .. target.name .. " " .. target.done .. "/" .. #target.functions)
end

sort(lines)
return "SoraVynx Auto Daily setup - Lua report v5\r\n" ..
 "Mode: keep current game selections\r\nStatus: inspection only; no daily actions executed\r\n" ..
 "Declared functions, optional debug parameter metadata and object structure only. No scalar values or bodies.\r\n" ..
 concat(lines,"\r\n") .. "\r\nCoverage: " .. entries .. " entries inspected; " .. #targets ..
 " target modules; " .. #lines .. " lines. Limited=" .. (limited and "yes" or "no") ..
 "\r\nLimits: 30000 entries, 2400 lines, 192 KiB body; 2048 entries per enumeration; 96 functions and 12 KiB per module.\r\n" ..
 "Superclass recursion excluded. Unknown parameter metadata is not evidence of zero arguments.\r\n" ..
 "NOT_LOADED means absent from package.loaded; opening a game page may load it. No require() was executed.\r\n"
