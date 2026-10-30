package lucky.hub.loader

import android.content.Context

/** Stores only the masked key and server-provided session details, never the entered key itself. */
object SessionStore {
    private const val PREFS = "amash_license_session"
    private const val ACTIVE = "active"
    private const val MASKED_KEY = "masked_key"
    private const val EXPIRY = "expiry"
    private const val DEVICE_ID = "device_id"

    fun save(context: Context, key: String, expiry: String, deviceId: String) {
        val masked = if (key.length >= 4) "••••" + key.takeLast(4) else "••••"
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putBoolean(ACTIVE, true)
            .putString(MASKED_KEY, masked)
            .putString(EXPIRY, expiry)
            .putString(DEVICE_ID, deviceId)
            .apply()
    }

    fun hasActiveSession(context: Context): Boolean =
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getBoolean(ACTIVE, false)

    fun maskedKey(context: Context): String =
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .getString(MASKED_KEY, "••••") ?: "••••"

    fun expiry(context: Context): String =
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .getString(EXPIRY, "Active") ?: "Active"

    fun deviceId(context: Context): String =
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .getString(DEVICE_ID, "") ?: ""

    fun clear(context: Context) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().clear().apply()
    }
}
