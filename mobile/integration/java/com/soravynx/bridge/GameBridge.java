/* SPDX-License-Identifier: AGPL-3.0-or-later */
package com.soravynx.bridge;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/** Loaded inside the game process; never injects into another application. */
public final class GameBridge {
    private static volatile boolean started, loaded;
    private static volatile String status = "{\"phase\":0,\"reason\":\"GAME_NOT_STARTED\"}";
    private static native void startNative();
    private static native String statusNative();
    private static native boolean commandNative(int kind, int value);

    public static synchronized void attach(Activity activity) {
        if (started) return;
        started = true;
        final Context context = activity.getApplicationContext();
        status = "{\"phase\":0,\"reason\":\"VERIFYING_GAME\"}";
        new Thread(() -> {
            try {
                PackageInfo info = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
                if (!"com.YoStarEN.StellaSora".equals(info.packageName) || (android.os.Build.VERSION.SDK_INT >= 28 ? info.getLongVersionCode() : info.versionCode) != 26093014L)
                    throw new IllegalStateException("PROFILE_MISMATCH");
                verify(context.getApplicationInfo(), "lib/arm64-v8a/libil2cpp.so", "6a0bc3ecf227b668e6c8496009e443707db11d53227453094c0c9edbb8c84b64");
                verify(context.getApplicationInfo(), "assets/bin/Data/Managed/Metadata/global-metadata.dat", "5f1b6d5c8f01b600e49252dda2bc00325aedb9a4c95fdf3c941ab95736884900");
                System.loadLibrary("soravynx_game");
                startNative();
                loaded = true;
            } catch (Throwable error) {
                status = "{\"phase\":6,\"reason\":\"PROFILE_OR_LOADER_FAILED\"}";
            }
        }, "SoraVynx-init").start();
    }

    private static void verify(ApplicationInfo app, String entry, String expected) throws Exception {
        ArrayList<String> paths = new ArrayList<>();
        paths.add(app.sourceDir);
        if (app.splitSourceDirs != null) java.util.Collections.addAll(paths, app.splitSourceDirs);
        for (String path : paths) {
            try (ZipFile zip = new ZipFile(path)) {
                ZipEntry item = zip.getEntry(entry);
                if (item == null) continue;
                MessageDigest digest = MessageDigest.getInstance("SHA-256");
                try (InputStream stream = zip.getInputStream(item)) {
                    byte[] buffer = new byte[65536];
                    int count;
                    while ((count = stream.read(buffer)) != -1) digest.update(buffer, 0, count);
                }
                StringBuilder actual = new StringBuilder();
                for (byte value : digest.digest()) actual.append(String.format(java.util.Locale.ROOT, "%02x", value & 255));
                if (!expected.contentEquals(actual)) throw new IllegalStateException("HASH_MISMATCH");
                return;
            }
        }
        throw new IllegalStateException("LIBRARY_NOT_FOUND");
    }

    static String status() { return loaded ? statusNative() : status; }
    static boolean command(int kind, int value) { return loaded && commandNative(kind, value); }
    private GameBridge() {}
}
