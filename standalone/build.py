#!/usr/bin/env python3
"""Build the standalone Win64 UI with Zig 0.14.1; no game files required."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent
BASE = "80fad7a42f1a8f0ec2a30d26270f9e163b849c7b"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zig", default="zig")
    parser.add_argument("--out", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    exe = out / "WuwaWorkbench.exe"
    subprocess.run([
        args.zig, "cc", "-target", "x86_64-windows-gnu", "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
        "-Wl,--dynamicbase", "-Wl,--nxcompat", "-municode", "-Wl,--subsystem,windows",
        str(ROOT / "main.c"), "-luser32", "-lgdi32", "-lcomctl32",
        "-lcomdlg32", "-lshell32", "-lgdiplus", "-lmsimg32", "-lole32",
        "-o", str(exe),
    ], check=True)
    sources = sorted(p for p in ROOT.iterdir() if p.is_file() and p.name != ".gitignore")
    manifest = {
        "product": "Wuwa Workbench", "version": "0.1.0-standalone-preview",
        "source_base": BASE, "mode": "standalone-ui", "target": "windows-x64",
        "compiler": "Zig " + subprocess.check_output([args.zig, "version"], text=True).strip(),
        "sha256": hashlib.sha256(exe.read_bytes()).hexdigest(),
        "sources": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
    }
    (out / "build.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (out / "SHA256SUMS.txt").write_text(manifest["sha256"] + "  WuwaWorkbench.exe\n")
    archive = out / "WuwaWorkbench-0.1.0-standalone-win64.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for name in ["WuwaWorkbench.exe", "build.json", "SHA256SUMS.txt"]:
            z.write(out / name, name)
        for p in sources:
            z.write(p, "source/" + p.name)
        z.write(ROOT / "README.vi.txt", "README.vi.txt")
        z.write(ROOT / "LICENSE.txt", "LICENSE.txt")
    print(archive)


if __name__ == "__main__":
    main()
