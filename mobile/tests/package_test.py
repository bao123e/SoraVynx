import importlib.util
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from zipfile import ZipFile

module_path = Path(__file__).resolve().parents[1] / "tools/package_game.py"
spec = importlib.util.spec_from_file_location("package_game", module_path)
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class PackagingTests(unittest.TestCase):
    def test_receiver_is_signature_protected_and_existing_game_components_retained(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = Path(tmp) / "AndroidManifest.xml"
            manifest.write_text('''<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.YoStarEN.StellaSora" android:requiredSplitTypes="base__abi"><uses-permission android:name="android.permission.INTERNET"/><application android:name="com.ano.gshell.AnoApplication"><activity android:name="Game"/><meta-data android:name="com.android.vending.splits.required" android:value="true"/></application></manifest>''')
            builder.patch_manifest(manifest)
            root = ET.parse(manifest).getroot()
            self.assertNotIn(builder.ANDROID + "requiredSplitTypes", root.attrib)
            app = root.find("application")
            self.assertEqual(app.attrib[builder.ANDROID + "name"], "com.ano.gshell.AnoApplication")
            self.assertIsNotNone(app.find("activity"))
            self.assertEqual(app.find("receiver").attrib[builder.ANDROID + "permission"], builder.PERMISSION)
            self.assertEqual(len(root.findall("uses-permission")), 2)
            with self.assertRaises(ValueError): builder.patch_manifest(manifest)

    def test_merged_apk_preserves_assets_and_uses_new_dex_slot(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            base, split = root / "base.apk", root / "split.apk"
            with ZipFile(base, "w") as z:
                z.writestr("classes.dex", b"dex1"); z.writestr("classes5.dex", b"dex5")
                z.writestr("assets/test", b"original-data")
                z.writestr("META-INF/CERT.RSA", b"stale-signature")
            with ZipFile(split, "w") as z:
                z.writestr("lib/arm64-v8a/libil2cpp.so", b"original-library")
                z.writestr("AndroidManifest.xml", b"must-not-merge")
            native, helper = root / "lib.so", root / "classes.dex"
            native.write_bytes(b"bridge-library"); helper.write_bytes(b"new-dex")
            output = root / "output.apk"
            builder.merge_apk(base, split, native, helper, output)
            with ZipFile(output) as z:
                self.assertEqual(z.read("classes6.dex"), b"new-dex")
                self.assertEqual(z.read("classes.dex"), b"dex1")
                self.assertEqual(z.read("assets/test"), b"original-data")
                self.assertEqual(z.read("lib/arm64-v8a/libil2cpp.so"), b"original-library")
                self.assertNotIn("META-INF/CERT.RSA", z.namelist())
                self.assertNotIn("AndroidManifest.xml", z.namelist())

    def test_activity_patch_keeps_original_sdk_call_and_refuses_unknown_or_duplicate(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "smali_classes4/com/yostar/sdk/bridge/YostarSDKPlayerAct.smali"
            path.parent.mkdir(parents=True)
            path.write_text("invoke-super {p0, p1}, Lcom/unity3d/player/UnityPlayerActivity;->onCreate(Landroid/os/Bundle;)V\ninvoke-static {p0, p1}, Lcom/yostar/airisdk/core/YoStarSDK;->onCreate(Landroid/app/Activity;Landroid/os/Bundle;)V")
            builder.patch_activity(root)
            self.assertIn("YoStarSDK;->onCreate", path.read_text())
            self.assertEqual(path.read_text().count("GameBridge;->attach"), 1)
            with self.assertRaises(ValueError): builder.patch_activity(root)


if __name__ == "__main__": unittest.main()
