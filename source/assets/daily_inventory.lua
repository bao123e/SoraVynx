-- SPDX-License-Identifier: AGPL-3.0-or-later
-- Fixed inspection only: never require modules or call functions found in game tables.
local kind, raw, advance = type, rawget, next
local lower, find, match = string.lower, string.find, string.match
local sort, concat = table.sort, table.concat
local words = {"daily", "mission", "quest", "task", "affair", "heartlink",
 "affinity", "gift", "home", "lobby", "dispatch", "commission", "trekker",
 "disc", "character", "upgrade", "enhance", "trial", "ascension", "battle",
 "stage", "tower", "vigor", "shop", "reward", "login", "signin", "checkin"}
local infrastructure = {"manager", "module", "controller", "panel", "view", "system"}
local MAX_ENTRIES, MAX_TABLES, MAX_LINES, MAX_BYTES = 30000, 512, 2400, 196608
local entries, tables, lines, bytes, cut = 0, 0, {}, 0, false
local visited = {}
local function identifier(s)
 return kind(s) == "string" and #s <= 160 and match(s, "^[%a_][%w_./%-]*$") ~= nil
end
local function relevant(s, generic)
 s = lower(s)
 for i = 1, #words do if find(s, words[i], 1, true) then return 0 end end
 if generic then
  for i = 1, #infrastructure do if find(s, infrastructure[i], 1, true) then return 1 end end
 end
 return 2
end
local function emit(s)
 if #lines >= MAX_LINES or bytes + #s + 2 > MAX_BYTES then cut = true; return end
 lines[#lines + 1], bytes = s, bytes + #s + 2
end
local function members(t)
 local list, key = {}, nil
 while entries < MAX_ENTRIES do
  local value
  key, value = advance(t, key)
  if key == nil then break end
  entries = entries + 1
  local k = kind(value)
  if identifier(key) and (k == "table" or k == "function") then
   list[#list + 1] = {key, value, k}
  end
 end
 if entries >= MAX_ENTRIES then cut = true end
 sort(list, function(a, b)
  local ra, rb = relevant(a[1], true), relevant(b[1], true)
  if ra ~= rb then return ra < rb end
  return a[1] < b[1]
 end)
 return list
end
local function walk(t, path, depth)
 if visited[t] then emit("ALIAS " .. path); return end
 if tables >= MAX_TABLES or depth > 4 then cut = true; return end
 visited[t], tables = true, tables + 1
 for _, item in ipairs(members(t)) do
  local name, value, k = item[1], item[2], item[3]
  local p = path .. "." .. name
  if k == "function" then emit("FUNCTION " .. p)
  elseif depth < 4 then emit("TABLE " .. p); walk(value, p, depth + 1)
  else cut = true end
 end
end
local roots = members(_G)
local pkg = raw(_G, "package")
local loaded = kind(pkg) == "table" and raw(pkg, "loaded") or nil
local modules = kind(loaded) == "table" and members(loaded) or {}
-- Inventory names first so a large module cannot hide later candidates.
for _, item in ipairs(roots) do emit("GLOBAL " .. item[1] .. " " .. item[3]) end
for _, item in ipairs(modules) do emit("MODULE " .. item[1] .. " " .. item[3]) end
-- Domain modules before generic UI/framework modules, independent of click order.
for priority = 0, 1 do
 for _, item in ipairs(modules) do
  if item[3] == "table" and relevant(item[1], true) == priority then
   walk(item[2], "loaded/" .. item[1], 0)
  end
 end
 for _, item in ipairs(roots) do
  if item[3] == "table" and relevant(item[1], true) == priority then
   walk(item[2], "global/" .. item[1], 0)
  end
 end
end
sort(lines)
return "SoraVynx Auto Daily setup - Lua report v4\r\n" ..
 "Mode: keep current game selections\r\nStatus: inspection only; no daily actions executed\r\n" ..
 "Names and function/table kinds only. Scalar values and function bodies excluded.\r\n" ..
 concat(lines, "\r\n") .. "\r\nCoverage: " .. entries .. " entries inspected; " ..
 tables .. " tables visited; " .. #lines .. " lines. Limited=" .. (cut and "yes" or "no") ..
 "\r\nLimits: 30000 entries, 512 tables, depth 4, 2400 lines, 192 KiB body.\r\n" ..
 "Unloaded modules, metatable methods and functions beyond these limits are not inspected.\r\n"
