SSTool v1.0 release source.

No launcher/runtime file logs. Initialization is independent of logging. Errors display in UI. Reopening SSTool.exe restores an existing SSToolRuntime2 panel for the running game PID, without loading another DLL. Otherwise the native picker opens and selection launches a fresh game.

Build: python build.py --zig /path/to/zig-0.14.1
Tests: python test.py
Windows panel restore and release GUI still require real Windows verification.
