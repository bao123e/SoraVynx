#!/usr/bin/env python3
"""Build a rootless, profile-locked integration from a user-supplied XAPK.

No game files, offsets, signing keys or APKs are stored in the source repository.
The supplied original game protection, SDK, assets and network behavior are retained.
"""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
ANDROID = "{http://schemas.android.com/apk/res/android}"
ET.register_namespace("android", ANDROID[1:-1])
PERMISSION = "com.soravynx.android.CONTROL"


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def patch_manifest(path):
    tree = ET.parse(path)
    root = tree.getroot()
    if root.attrib.get("package") != "com.YoStarEN.StellaSora":
        raise ValueError("Unexpected game package")
    # This build includes every ARM64 split library in the base APK.
    for name in ("requiredSplitTypes", "splitTypes", "isSplitRequired"):
        root.attrib.pop(ANDROID + name, None)
    if any(n.attrib.get(ANDROID + "name") == PERMISSION for n in root.findall("uses-permission")):
        raise ValueError("Already integrated")
    root.insert(0, ET.Element("uses-permission", {ANDROID + "name": PERMISSION}))
    app = root.find("application")
    if app is None:
        raise ValueError("Missing game application")
    for node in list(app):
        if node.tag == "meta-data" and node.attrib.get(ANDROID + "name") in (
            "com.android.vending.splits", "com.android.vending.splits.required",
        ):
            app.remove(node)
    ET.SubElement(app, "receiver", {
        ANDROID + "name": "com.soravynx.bridge.ControlReceiver",
        ANDROID + "exported": "true",
        ANDROID + "permission": PERMISSION,
    })
    tree.write(path, encoding="utf-8", xml_declaration=True)


def patch_activity(decoded):
    candidates = list(decoded.glob("smali*/com/yostar/sdk/bridge/YostarSDKPlayerAct.smali"))
    if len(candidates) != 1:
        raise ValueError("Game activity is missing or ambiguous")
    path = candidates[0]
    text = path.read_text()
    pattern = r"(invoke-super \{p0, p1\}, Lcom/unity3d/player/UnityPlayerActivity;->onCreate\(Landroid/os/Bundle;\)V)"
    if len(re.findall(pattern, text)) != 1 or "Lcom/soravynx/bridge/" in text:
        raise ValueError("Game activity does not match the expected profile")
    path.write_text(re.sub(pattern, r"\1\n\n    invoke-static {p0}, Lcom/soravynx/bridge/GameBridge;->attach(Landroid/app/Activity;)V", text))


def merge_apk(base, split, native, helper, output):
    with ZipFile(base) as source, ZipFile(split) as config, ZipFile(output, "w", ZIP_DEFLATED) as destination:
        names = set(source.namelist())
        dex_ids = [int(m.group(1) or "1") for n in names if (m := re.fullmatch(r"classes(\d*)\.dex", n))]
        dex_name = f"classes{max(dex_ids, default=1) + 1}.dex"
        for info in source.infolist():
            if info.filename.startswith("META-INF/") and info.filename.upper().endswith((".SF", ".RSA", ".DSA", ".EC", "/MANIFEST.MF")):
                continue
            destination.writestr(info, source.read(info.filename))
        for info in config.infolist():
            if info.filename.startswith("lib/arm64-v8a/") and info.filename.endswith(".so"):
                if info.filename in names:
                    raise ValueError("Duplicate native library")
                destination.writestr(info, config.read(info.filename))
        destination.write(native, "lib/arm64-v8a/libsoravynx_game.so")
        destination.write(helper, dex_name)
        for license_file in (ROOT / "integration/licenses").glob("*.txt"):
            destination.write(license_file, "assets/soravynx/licenses/" + license_file.name)
    with ZipFile(output) as result:
        if result.testzip() is not None:
            raise ValueError("Bad merged ZIP")


def main():
    parser = argparse.ArgumentParser()
    for name in ("xapk", "apktool", "native", "helper", "output", "work"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    profile = json.loads((ROOT / "integration/profile.json").read_text())
    if sha256(args.xapk) != profile["xapkSha256"]:
        raise SystemExit("XAPK differs from the analyzed 2.0.0 profile. Re-analyze before building.")
    args.work.mkdir(parents=True, exist_ok=True)
    with ZipFile(args.xapk) as package:
        for name, expected in profile["apkSha256"].items():
            data = package.read(name)
            if hashlib.sha256(data).hexdigest() != expected:
                raise SystemExit("Original APK hash mismatch")
            (args.work / name).write_bytes(data)
    decoded = args.work / "decoded"
    subprocess.run(["java", "-jar", str(args.apktool), "d", "-f", str(args.work / "com.YoStarEN.StellaSora.apk"), "-o", str(decoded)], check=True)
    patch_manifest(decoded / "AndroidManifest.xml")
    patch_activity(decoded)
    rebuilt = args.work / "rebuilt.apk"
    subprocess.run(["java", "-jar", str(args.apktool), "b", str(decoded), "-o", str(rebuilt)], check=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    merge_apk(rebuilt, args.work / "config.arm64_v8a.apk", args.native, args.helper, args.output)
    with ZipFile(args.output) as apk:
        for entry, expected in profile["entries"].items():
            if hashlib.sha256(apk.read(entry)).hexdigest() != expected["sha256"]:
                raise SystemExit("Game data changed unexpectedly")
    print("Built profile-locked integration; on-device validation is still required.")


if __name__ == "__main__":
    main()
