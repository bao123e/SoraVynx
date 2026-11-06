#!/usr/bin/env python3
"""Download the public XAPK, then verify it before decoding or executing tools."""
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path
from urllib.parse import urlsplit

profile = json.loads((Path(__file__).resolve().parents[1] / "integration/profile.json").read_text())
url = os.environ.get("XAPK_URL") or profile["sourceUrl"]
parsed = urlsplit(url)
if parsed.scheme != "https" or parsed.username or parsed.password:
    raise SystemExit("Use a public HTTPS download URL")
destination = Path(sys.argv[1])
subprocess.run(["curl", "--fail", "--location", "--retry", "2", "--max-time", "180", "--output", str(destination), url], check=True)
digest = hashlib.sha256()
with destination.open("rb") as stream:
    for chunk in iter(lambda: stream.read(1024 * 1024), b""):
        digest.update(chunk)
if digest.hexdigest() != profile["xapkSha256"]:
    destination.unlink()
    raise SystemExit("XAPK hash mismatch. Re-analyze changed game files before building.")
print("Original game download matches the analyzed SHA-256.")
