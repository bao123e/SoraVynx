/* SPDX-License-Identifier: AGPL-3.0-or-later */
package com.soravynx.bridge;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import org.json.JSONObject;

/** Manifest requires a signature permission, shared only with the companion APK. */
public final class ControlReceiver extends BroadcastReceiver {
    @Override public void onReceive(Context context, Intent intent) {
        if (!"com.soravynx.android.CONTROL".equals(intent.getAction())) return;
        try {
            int kind = intent.getIntExtra("kind", -2);
            boolean accepted = kind == -2 || GameBridge.command(kind, intent.getIntExtra("value", 0));
            JSONObject result = new JSONObject(GameBridge.status());
            result.put("accepted", accepted);
            setResultCode(ActivityResult.OK);
            setResultData(result.toString());
        } catch (Exception ignored) {
            setResultData("{\"phase\":6,\"reason\":\"BRIDGE_FAILED\",\"accepted\":false}");
        }
    }
    private static final class ActivityResult { static final int OK = -1; }
}
