/* SPDX-License-Identifier: AGPL-3.0-or-later */
package com.soravynx.android

import android.app.Activity
import android.content.*
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Handler
import android.os.Looper
import com.facebook.react.bridge.*
import com.facebook.react.ReactPackage
import com.facebook.react.uimanager.ViewManager
import org.json.JSONObject
import java.util.concurrent.atomic.AtomicBoolean

class GameModule(private val context: ReactApplicationContext) : ReactContextBaseJavaModule(context) {
  companion object { const val GAME = "com.YoStarEN.StellaSora"; const val PERMISSION = "com.soravynx.android.CONTROL" }
  private var picker: Promise? = null
  private val preferences = context.getSharedPreferences("appearance", Context.MODE_PRIVATE)
  private val handler = Handler(Looper.getMainLooper())
  override fun getName() = "SoraVynxGame"

  init {
    context.addActivityEventListener(object : BaseActivityEventListener() {
      override fun onActivityResult(activity: Activity, requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode != 8401) return
        val promise = picker ?: return
        picker = null
        val uri = data?.data
        if (resultCode != Activity.RESULT_OK || uri == null) { promise.resolve(null); return }
        try {
          context.contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
          val old = preferences.getString("image", null)
          preferences.edit().putString("image", uri.toString()).apply()
          if (old != null && old != uri.toString()) releaseUri(old)
          promise.resolve(uri.toString())
        } catch (error: Exception) { promise.reject("IMAGE_ACCESS", "Không thể giữ quyền đọc ảnh đã chọn.", error) }
      }
    })
  }
  private fun releaseUri(value: String) {
    try { context.contentResolver.releasePersistableUriPermission(Uri.parse(value), Intent.FLAG_GRANT_READ_URI_PERMISSION) } catch (_: Exception) { }
  }
  private fun gameIssue(): String? {
    return try {
      val game = context.packageManager.getPackageInfo(GAME, PackageManager.GET_SIGNATURES)
      val own = context.packageManager.getPackageInfo(context.packageName, PackageManager.GET_SIGNATURES)
      if (game.versionCode != 26093014) "WRONG_GAME_VERSION"
      else if (game.signatures?.singleOrNull() == null || own.signatures?.singleOrNull() == null || game.signatures?.singleOrNull() != own.signatures?.singleOrNull()) "ORIGINAL_OR_DIFFERENT_SIGNATURE"
      else {
        context.packageManager.getReceiverInfo(ComponentName(GAME, "com.soravynx.bridge.ControlReceiver"), 0)
        null
      }
    } catch (_: PackageManager.NameNotFoundException) { "PATCHED_GAME_NOT_INSTALLED" }
  }
  private fun exchange(kind: Int, value: Int, promise: Promise) {
    val issue = gameIssue()
    if (issue != null) { promise.resolve(JSONObject().put("phase", -1).put("reason", issue).toString()); return }
    val complete = AtomicBoolean(false)
    val timeout = Runnable {
      if (complete.compareAndSet(false, true)) promise.resolve("{\"phase\":-1,\"reason\":\"GAME_NOT_RESPONDING\"}")
    }
    handler.postDelayed(timeout, 5000)
    val intent = Intent(PERMISSION).setComponent(ComponentName(GAME, "com.soravynx.bridge.ControlReceiver"))
      .putExtra("kind", kind).putExtra("value", value).addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES)
    try {
      context.sendOrderedBroadcast(intent, PERMISSION, object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
          if (complete.compareAndSet(false, true)) {
            handler.removeCallbacks(timeout)
            promise.resolve(resultData ?: "{\"phase\":-1,\"reason\":\"GAME_BRIDGE_UNAVAILABLE\"}")
          }
        }
      }, handler, Activity.RESULT_CANCELED, null, null)
    } catch (error: Exception) {
      handler.removeCallbacks(timeout)
      if (complete.compareAndSet(false, true)) promise.reject("GAME_CONNECTION", "Không nối được game đã tích hợp.", error)
    }
  }
  @ReactMethod fun getStatus(promise: Promise) = exchange(-2, 0, promise)
  @ReactMethod fun command(kind: Int, value: Int, promise: Promise) {
    val valid = kind == -1 || kind in 0..8 && value in 0..1 || kind == 10 && value in 2..50 || kind == 11 && value in 1..10 || kind == 12 && value in 2..100
    if (!valid) { promise.reject("INVALID_CONTROL", "Giá trị điều khiển ngoài giới hạn."); return }
    exchange(kind, value, promise)
  }
  @ReactMethod fun launchGame(promise: Promise) {
    val issue = gameIssue()
    if (issue != null) { promise.reject(issue, "Cần cài game tích hợp và menu trong cùng bộ build."); return }
    val intent = context.packageManager.getLaunchIntentForPackage(GAME)
    if (intent == null) { promise.reject("NO_GAME_ACTIVITY", "Không tìm thấy màn hình game."); return }
    context.startActivity(intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
    promise.resolve(null)
  }
  @ReactMethod fun appearance(promise: Promise) {
    promise.resolve(JSONObject().put("image", preferences.getString("image", null))
      .put("imageOpacity", preferences.getInt("imageOpacity", 60))
      .put("panelOpacity", preferences.getInt("panelOpacity", 85))
      .put("buttonOpacity", preferences.getInt("buttonOpacity", 85)).toString())
  }
  @ReactMethod fun setOpacity(key: String, value: Int, promise: Promise) {
    if (key !in listOf("imageOpacity", "panelOpacity", "buttonOpacity") || value !in 0..100) {
      promise.reject("INVALID_OPACITY", "Opacity phải từ 0 đến 100."); return
    }
    preferences.edit().putInt(key, value).apply(); promise.resolve(null)
  }
  @ReactMethod fun pickBackground(promise: Promise) {
    val activity = currentActivity
    if (activity == null || picker != null) { promise.reject("PICKER_BUSY", "Chưa thể mở bộ chọn ảnh."); return }
    picker = promise
    try {
      activity.startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
        type = "image/*"; addCategory(Intent.CATEGORY_OPENABLE)
        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
      }, 8401)
    } catch (error: Exception) { picker = null; promise.reject("PICKER_FAILED", "Không mở được bộ chọn ảnh.", error) }
  }
  @ReactMethod fun removeBackground(promise: Promise) {
    preferences.getString("image", null)?.let { releaseUri(it) }
    preferences.edit().remove("image").apply(); promise.resolve(null)
  }
}

class GamePackage : ReactPackage {
  override fun createNativeModules(context: ReactApplicationContext): List<NativeModule> = listOf(GameModule(context))
  override fun createViewManagers(context: ReactApplicationContext): List<ViewManager<*, *>> = emptyList()
}
