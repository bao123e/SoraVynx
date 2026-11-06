#!/usr/bin/env python3
"""Artifact checks; these do not establish in-game compatibility."""
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path
from zipfile import ZipFile

root = Path(sys.argv[1])
game = root / "StellaSora-SoraVynx-2.0.0-arm64-preview.apk"
menu = root / "SoraVynx-Menu-0.1.0-arm64-preview.apk"
certificates = []
for apk in (game, menu):
    verified = subprocess.check_output(["apksigner", "verify", "--verbose", "--print-certs", str(apk)], text=True)
    match = re.search(r"Signer #1 certificate SHA-256 digest: (\w+)", verified)
    if not match:
        raise SystemExit("No verified signing certificate")
    certificates.append(match.group(1))
if certificates[0] != certificates[1]:
    raise SystemExit("Game and menu must have the same signing identity")
profile = json.loads((Path(__file__).resolve().parents[1] / "integration/profile.json").read_text())
with ZipFile(game) as apk:
    assert apk.testzip() is None
    for entry, info in profile["entries"].items():
        assert hashlib.sha256(apk.read(entry)).hexdigest() == info["sha256"], entry
    native = apk.read("lib/arm64-v8a/libsoravynx_game.so")
    assert native[:4] == b"\x7fELF" and native[18:20] == b"\xb7\x00", "Expected ARM64 ELF"
    assert all(name.encode() in native for name in (
        "Java_com_soravynx_bridge_GameBridge_startNative",
        "Java_com_soravynx_bridge_GameBridge_statusNative",
        "Java_com_soravynx_bridge_GameBridge_commandNative",
    ))
    assert b"__android_log_print" not in native, "Runtime logging must be disabled"
with ZipFile(menu) as apk:
    assert apk.testzip() is None
    js = apk.read("assets/index.android.bundle")
    # Hermes bytecode retains native component/module names in the string table.
    assert b"SoraVynxGame" in js and b"MenuView" in js
    assert "assets/fonts/BeVietnamPro-Regular.ttf" in apk.namelist()
game_manifest = subprocess.check_output(["aapt2", "dump", "xmltree", str(game), "--file", "AndroidManifest.xml"], text=True)
menu_manifest = subprocess.check_output(["aapt2", "dump", "xmltree", str(menu), "--file", "AndroidManifest.xml"], text=True)
assert "com.soravynx.bridge.ControlReceiver" in game_manifest
assert "com.soravynx.android.CONTROL" in game_manifest
assert "com.ano.gshell.AnoApplication" in game_manifest
assert "android.permission.INTERNET" not in menu_manifest, "Release companion must not use network access"
(root / "BUILD-STATUS.txt").write_text(
    "SoraVynx Android 0.1.0-preview.1\n"
    "Build and structural checks passed. In-game behavior NOT VERIFIED.\n"
    "Per-build signing certificate SHA-256: " + certificates[0] + "\n"
    "This preview does not establish compatibility with game integrity or login checks.\n"
)
print("APK signatures, original game data, ARM64 JNI library and menu assets verified.")
