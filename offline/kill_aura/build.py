#!/usr/bin/env python3
"""Build and package portable C selection rules and an offline demo."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent
SCRIPT_SHA256 = "fdce0394d6991fd78774245ab3930defdf4df15793ae587df52acc2efe3944b1"


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--cc", default="cc")
    p.add_argument("--zig")
    p.add_argument("--target", choices=["native", "windows"], default="native")
    p.add_argument("--run-tests", action="store_true")
    p.add_argument("--out", type=Path, default=ROOT / "dist")
    args = p.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    suffix = ".exe" if args.target == "windows" else ""
    if args.target == "windows":
        command = [args.zig or "zig", "cc", "-target", "x86_64-windows-gnu"]
    else:
        command = shlex.split(args.cc)
    flags = ["-std=c11", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    for source, name in [("demo.c", "KaRulesDemo"), ("test_kill_aura.c", "KaRulesTests")]:
        subprocess.run(command + flags + [str(ROOT / "kill_aura.c"), str(ROOT / source),
                       "-lm", "-o", str(out / (name + suffix))], check=True)
    if args.target == "native" or args.run_tests:
        result = subprocess.run([str(out / ("KaRulesTests" + suffix))],
                                text=True, capture_output=True)
        (out / "test-report.txt").write_text(result.stdout + result.stderr, encoding="utf-8")
        print(result.stdout, end="")
        if result.returncode:
            raise SystemExit(result.returncode)
        subprocess.run([str(out / ("KaRulesDemo" + suffix))], check=True)
    sources = sorted(ROOT.glob("*.c")) + sorted(ROOT.glob("*.h")) + [ROOT / "build.py", ROOT / "README.txt"]
    license_file = ROOT.parent.parent / "LICENSE.txt"
    binaries = [out / (name + suffix) for name in ["KaRulesDemo", "KaRulesTests"]]
    metadata = {
        "product": "Offline C Kill Aura selection rules", "version": "0.1.0",
        "target": args.target, "original_script_sha256": SCRIPT_SHA256,
        "game_integration": False,
        "sha256": {f.name: hashlib.sha256(f.read_bytes()).hexdigest() for f in binaries},
        "sources": {f.name: hashlib.sha256(f.read_bytes()).hexdigest() for f in sources},
    }
    (out / "build.json").write_text(json.dumps(metadata, indent=2) + "\n")
    archive = out / ("KaRules-offline-C-" + ("win64" if suffix else "native") + ".zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for f in binaries + [out / "build.json"]:
            z.write(f, f.name)
        for f in sources:
            z.write(f, "source/" + f.name)
        z.write(ROOT / "README.txt", "README.txt")
        z.write(license_file, "LICENSE.txt")
        if (out / "test-report.txt").exists():
            z.write(out / "test-report.txt", "test-report.txt")
    print(archive)


if __name__ == "__main__":
    main()
