OFFLINE C PORT — KUNMOD KILL AURA SELECTION RULES

This is a C11 translation of the decision rules in the uploaded KillAura.js.
It evaluates caller-owned entity snapshots and returns an action classification.
The demo prints classifications. It does not modify health, invoke the original
game-specific methods, send requests, inject a DLL, or connect to WuWa.

SOURCE BEHAVIOR PRESERVED
- killAuranew -> monsters_enabled.
- killAuraRadius -> radius_m; positions use centimetres, so multiply by 100.
- Monster blueprint prefix matching is case sensitive.
- 3D distance must be strictly less than the configured radius.
- KillAnimal -> animals_enabled.
- Five small-animal IDs select the animal classification, three select drop.
- Animal classification has no distance gate in the original script.
- The source's large-animal list is unused and remains inactive.
- The original has no cooldown or line-of-sight check; none were invented here.

The original invokes game-specific actions after these decisions. This port
returns enums at that boundary. It is not a WuWa combat adapter or an executable
version of the original requests. No compatibility with WuWa 3.8 is claimed.
Null C pointers are handled by returning no match where relevant.

FILES
kill_aura.h / kill_aura.c: portable C API and rule implementation.
demo.c: a small local fixture demo and batch evaluator.
test_kill_aura.c: boundary, units, entity types, disabled flags and input tests.
build.py: native or Windows x64 build and packaging.

BUILD ON LINUX
  python3 build.py
Or directly:
  cc -std=c11 -O2 -Wall -Wextra -Werror kill_aura.c demo.c -lm -o demo
  cc -std=c11 -O2 -Wall -Wextra -Werror kill_aura.c test_kill_aura.c -lm -o tests
  ./tests
  ./demo

BUILD WINDOWS X64 WITH ZIG 0.14.1
  python build.py --target windows --zig path-to-zig.exe --run-tests
For cross-compilation on Linux, omit --run-tests.
The native build runs its tests by default. Output is written under dist/.

RUN ON WINDOWS
Open a terminal in the extracted directory:
  .\KaRulesDemo.exe
  .\KaRulesTests.exe
These are console programs; a double-clicked terminal may close immediately.

BATCH EVALUATION
  KaRulesDemo --evaluate
Input rows (space separated; EOF ends input):
  blueprint x y z player_x player_y player_z radius_m monsters animals
Example:
  Monster001 300 400 0 0 0 0 10 1 1
Output:
  monster_match
Use '-' for an empty blueprint. Positions are centimetres. Flags must be 0/1.

ORIGINAL SCRIPT SHA-256
fdce0394d6991fd78774245ab3930defdf4df15793ae587df52acc2efe3944b1

This module is separate from Workbench; it adds no pretend gameplay toggles to
the process monitor. Tests establish local decision behavior, not game support.
See LICENSE.txt for the repository license (AGPL-3.0-or-later).
