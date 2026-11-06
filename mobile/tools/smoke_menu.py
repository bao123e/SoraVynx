#!/usr/bin/env python3
"""Exercise the real release menu APK on an emulator; never installs/claims to test the game."""
import re
import struct
import subprocess
import time
import xml.etree.ElementTree as ET
from pathlib import Path

OUT = Path("mobile/dist/ui")
OUT.mkdir(parents=True, exist_ok=True)


def adb(*args):
    return subprocess.check_output(["adb", *args])


def tree():
    adb("shell", "uiautomator", "dump", "/sdcard/soravynx-ui.xml")
    return ET.fromstring(adb("exec-out", "cat", "/sdcard/soravynx-ui.xml"))


def find(text, timeout=20):
    until = time.monotonic() + timeout
    while time.monotonic() < until:
        root = tree()
        for node in root.iter("node"):
            if node.attrib.get("text") == text or node.attrib.get("content-desc") == text:
                return node
        time.sleep(0.5)
    raise AssertionError("UI element not visible: " + text)


def click(text):
    node = find(text)
    bounds = [int(n) for n in re.findall(r"\d+", node.attrib["bounds"])]
    adb("shell", "input", "tap", str((bounds[0]+bounds[2])//2), str((bounds[1]+bounds[3])//2))


def reveal(text):
    for attempt in range(6):
        root = tree()
        for node in root.iter("node"):
            if node.attrib.get("text") == text:
                return node
        png = adb("exec-out", "screencap", "-p")
        width, height = struct.unpack(">II", png[16:24])
        adb("shell", "input", "swipe", str(int(width*.75)), str(int(height*.85)), str(int(width*.75)), str(int(height*.4)), "300")
        time.sleep(.4)
    raise AssertionError("Cannot scroll to control: " + text)


def snapshot(name):
    (OUT / (name + ".png")).write_bytes(adb("exec-out", "screencap", "-p"))


try:
    adb("install", "-r", "mobile/dist/SoraVynx-Menu-0.1.0-arm64-preview.apk")
    adb("shell", "settings", "put", "system", "accelerometer_rotation", "0")
    adb("shell", "settings", "put", "system", "user_rotation", "0")
    adb("shell", "am", "start", "-W", "-n", "com.soravynx.android/.MainActivity")
    find("SoraVynx")
    find("God Mode")
    root = tree()
    for node in root.iter("node"):
        if node.attrib.get("class") == "android.widget.Switch":
            assert node.attrib.get("enabled") == "false", "Stock/missing game cannot enable a feature"
    snapshot("phone-player")
    click("Event")
    find("Bouncing Bumper Bash")
    find("Score Multiplier")
    snapshot("phone-event")
    click("Custom BG")
    find("Custom Background")
    find("Chọn ảnh…")
    snapshot("phone-background")
    # This must be the real Android PopupMenu supplied by @react-native-menu/menu.
    click("Mở menu")
    find("Thông tin bản thử nghiệm")
    snapshot("native-menu")
    click("Thông tin bản thử nghiệm")
    find("SoraVynx · Preview")
    click("OK")
    click("Player")
    adb("shell", "settings", "put", "system", "user_rotation", "1")
    time.sleep(2)
    reveal("God Mode")
    snapshot("phone-landscape")
    adb("shell", "settings", "put", "system", "user_rotation", "0")
    adb("shell", "wm", "size", "1200x1920")
    adb("shell", "wm", "density", "240")
    time.sleep(2)
    reveal("God Mode")
    snapshot("tablet-player")
    adb("shell", "settings", "put", "system", "font_scale", "1.3")
    adb("shell", "wm", "size", "720x1280")
    adb("shell", "wm", "density", "320")
    time.sleep(2)
    find("SoraVynx")
    snapshot("large-text")
    print("Release companion UI, disabled controls and Android PopupMenu smoke checks passed. Game not tested.")
except Exception:
    snapshot("failure")
    (OUT / "failure.xml").write_bytes(ET.tostring(tree()))
    raise
