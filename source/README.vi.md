# SSTool Player / Event source

Player and Bouncing Bumper Bash controls are separate pages. Event score factor is configurable 2–100 (default 2); positive controller score deltas only, clamped against Int32 total overflow. The tested x2 event backend is preserved. Higher factors and the new Windows UI need real game verification.

The native UI embeds assets/background.bmp through resource 301. Artwork is a softened/darkened derivative of the user-provided Stella Sora illustration; original characters/art belong to their respective rights holders. The BMP is the exact embedded asset; the generated original PNG remains outside the distribution.

Build: python source/build.py --zig /path/to/zig-0.14.1
Tests: python source/test.py
Build creates the bitmap resource, Mastercheat.dll, then matching SSTool.exe. Windows x64 cross-build and 18 native mock/helper test groups pass. No file logging. F1 restores/hides the panel; reopening the EXE restores an existing panel. Initial launch opens the game file picker directly.
