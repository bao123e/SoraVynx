# SoraVynx release notes

## 1.1.0-preview.10

- Pause Shop work as requested: clear and persist its USE bit, lock the Shop row to SKIP and remove the Shop command from the runtime resource.
- Switch the action button to Tower reward / status using report v5's confirmed PlayerData.InfinityTower instance and ITDailyRewardReq(self) signature.
- Validate class identity, handler arity and ready reward-table data before one explicit request. Repeated clicks only observe the reward table; no retry after handler errors.
- Copy only observation counts/status, never reward-field scalar values or IDs, and do not infer server success or daily completion from a returned handler or changed fields.
- Keep the main-thread bridge, queued cancellation, F1 restore, Player/Event functions and no-log behavior. No Shop UI lookup, battle entry, upgrade or gift-send call is used by the Tower command.
- Replace the active Lua regression with Tower coverage and update Windows/UI tests for the Tower USE bit and disabled Shop row.
- Full Auto Daily remains unimplemented; real-game verification is required. Close the game and replace both EXE/DLL.

## 1.1.0-preview.9

- Address the real .8 failure reporting zero LuaBehaviourBase objects while Shop was visible. A normal Unity object search is no longer the sole discovery route.
- Add Resources.FindObjectsOfTypeAll for LuaBehaviourBase and LuaUIController, following the latter's public luaBehaviour link. Require active hierarchy and valid/loaded scenes to exclude prefabs, assets and inactive objects.
- Add a read-only pure-Lua registry route when the verified zero-argument PanelManager.GetCurPanelId getter matches a known Shop enum ID. Inspect bounded UI-closure upvalue tables and same-source helper closures without calling panel-opening helpers or exposing IDs/values.
- Resolve only the current Shop registry entry, then retain existing namespace, handler-arity, ambiguity and single-attempt guards. No direct reward-request fallback or coordinate click is added.
- Report per-route counts and current-Shop proof status, so empty Unity searches can be distinguished from missing Lua registry/context information.
- Extend real Lua VM tests with the exact zero-normal-objects case, live runtime objects, controller links with disabled Lua update components, scene/prefab rejection, nested Lua registry helpers and current-page mismatches.
- Auto Daily remains partial. Mock/Wine tests do not replace real-game verification; close the game and replace both EXE/DLL.

## 1.1.0-preview.8

- Fix the .7 lookup design that could stop before live UI discovery when the canonical package.loaded class entry or raw declared method was absent. The screenshot's generic message did not distinguish these cases from missing signature metadata.
- Resolve the fixed Daily Gift method from active Shop instances and their bounded class/metatable chains, including normal function-valued __index dispatch. Treat the module cache as an optional identity hint.
- Support a pure Lua ShopCtrl stored under the active ShopPanel. For short component names, require a module-class/function identity link or matching function source-path metadata; no function bodies or source paths are copied into reports.
- Keep a known one-argument/non-vararg arity mandatory. Missing debug local-variable names no longer fail a valid binding, while available incompatible parameter names still stop it.
- Repaint the tool and all child controls when restoring it for an error/status dialog after hiding.
- Replace generic Open Shop messages with explicit codes for UI discovery, instance availability, handler resolution, namespace proof, arity and parameter metadata failures.
- Extend real Lua VM regressions for absent/aliased caches, inherited and function-metatable methods, nested controllers, unavailable local names and exact diagnostic codes. Preserve no direct-RPC fallback and one handler attempt per game session.
- Auto Daily remains partial, and real-game verification is still required. Close the game and replace both EXE/DLL.

## 1.1.0-preview.7

- Fix Shop action bypassing the Daily Gift UI button by sending a direct reward request.
- Discover active LuaBehaviourBase components through the game's XLua/Unity bridge, require one ShopCtrl instance with the exact loaded class, and invoke OnBtnClick_DailyGift(self) once.
- Validate the live handler signature and stop on absent, disabled, ambiguous or incompatible controllers. Do not fall back to the direct Shop request or screen coordinates.
- Rename the action to Daily Gift / status. Hide the tool and focus this game's Unity window while the queued action runs; suppress the initial status dialog after handler invocation so it cannot cover the game UI. F1 restores the tool.
- Preserve one invocation per game session and status-only repeat clicks. Handler return does not prove the UI opened or rewards were received; real-game validation remains required.
- Extend Lua and Windows/Wine tests for active UI discovery, identity/signature checks, ambiguity, errors, no direct-request fallback and hiding without a modal dialog.
- Auto Daily is still partial; other 11 tasks and the full scheduler remain unimplemented. Close the game and update both EXE/DLL.

## 1.1.0-preview.6

- Use report v5's confirmed PlayerData.Shop instance and SendDailyShopRewardReceiveReq(self, callback) signature to add Shop gift / status.
- Validate live class identity, parameter metadata and the reward getter before one explicit free-gift request. Later clicks only refresh status; no automatic retry after exceptions or callback errors.
- Track callback delivery and reward-flag changes without copying payload values or declaring server success/task completion. Purchases, upgrades, gifts and battles are not invoked by this command.
- Run the fixed Shop command through the existing main-thread Lua bridge, respect the Shop USE/SKIP choice and cancel queued commands on Disable all/close.
- Add real Lua VM coverage for synchronous/asynchronous callbacks, duplicate clicks, handler exceptions, context changes, signature guards and payload exclusion; extend Windows/Wine queue and UI checks.
- Auto Daily remains partial: the other 11 tasks and the full scheduler are NOT implemented. In-game verification of this new action remains required. Replace both EXE/DLL while the game is closed.

## 1.1.0-preview.5

- Fix Lua report v4 exhausting 2,400 lines on inherited superclass helpers before reaching all daily APIs.
- Target the confirmed PlayerQuestData, PlayerShopData, DispatchData, Disc/Char and battle data modules; exclude superclass/__index recursion and unrelated activity modules.
- Add optional debug parameter count/names without calling discovered functions, and structural links from existing PlayerData/PanelManager objects to loaded classes. Unknown metadata is never treated as zero arguments.
- Apply round-robin per-module quotas and explicit missing-module/coverage markers. Keep the existing bounded main-thread bridge, clipboard workflow and Player/Event backend.
- Extend real Lua VM regressions for superclass flooding, later-module coverage, parameter metadata/fallback and object-class associations.
- Auto Daily execution is still NOT implemented. Close the game and update both EXE/DLL. Copy Lua name report produces report v5; no need to repeat tasks or send C# report v3 again.

## 1.1.0-preview.4

- Use report v3's confirmed Game/XLua signatures to add a separate, optional Lua name inspection bridge.
- Queue one inspection for LuaManager.Update, run the original Update first and verify Unity's main thread before touching the existing VM. Idle callbacks do not invoke the VM. Keep Player/Event backend unchanged.
- Execute only the bundled fixed inspection script: list loaded module/function/table names through raw traversal, with no require, discovered function calls, scalar values, function bodies or account values in the report.
- Bound traversal and clipboard output; guard duplicate requests, reentrancy, exceptions, managed lifetimes and missing Update timeout. Preserve Copy API metadata as a diagnostic fallback.
- Add real Lua VM tests and a Windows/Wine queue/clipboard regression. In-game validation remains required.
- Auto Daily execution is still NOT implemented. Close the game and replace both EXE/DLL before using Copy Lua name report; no need to repeat tasks.

## 1.1.0-preview.3

- Expand setup report discovery beyond Hotfix: inventory loaded assemblies and scan application assemblies including Game, while excluding framework libraries.
- Prioritize Lua/UI infrastructure metadata, including XLua LuaEnv/LuaTable and bridge signatures. No scripts or game actions are executed.
- Include assembly-qualified class names, bounded coverage counts and a 512 KiB report budget with separate Lua/UI space.
- Add a Windows/Wine regression covering Game/XLua discovery, framework exclusion, inherited metadata, clipboard copying and zero action invocations.
- Auto Daily execution is still NOT implemented. Close the game and use both updated EXE/DLL, then copy setup report v3. Daily tasks do not need to be repeated.

## 1.1.0-preview.2

- Fix the setup report that was filled by generated Protobuf configuration classes.
- Detect Protobuf by method return type, retain real runtime controllers in the same namespace, and exclude generated table wrappers.
- Scan all candidate classes before formatting; include a compact index and separate signature budgets for all six daily task groups.
- Prioritize controllers/managers and action handlers, including handlers declared after the first 32 methods. Include parameter names, inherited singleton accessors and field names/types, never field values.
- Rename the button to Copy setup report v2; add a portable Windows/Wine regression with 100 leading Protobuf types.
- Auto Daily execution is still NOT implemented. Close the game and use both updated EXE/DLL before copying the new report; tasks do not need to be repeated.

## 1.1.0-preview.1

- Rename SSTool to SoraVynx; matching SoraVynx.exe / SoraVynx.Runtime.dll.
- Original generated celestial S/lynx logo, executable icon, Be Vietnam Pro typography.
- Preserve full-width Player/Event layout and independent image/panel/button opacity.
- Add the fourth, unnumbered Auto Daily page with the twelve requested daily tasks.
- Auto Daily is setup-only: task preferences and bounded clipboard metadata report. Task execution is NOT implemented yet; no fake completion or blind clicks.
- Track expanded runtime files and source in Git; distribute ZIP through GitHub Releases.
- 20 native test groups and 13 Wine UI groups passed; gameplay validation of this new build remains necessary.
