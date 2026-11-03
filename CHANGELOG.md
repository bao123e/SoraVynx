# SoraVynx release notes

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
