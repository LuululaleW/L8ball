package lucky.hub.loader

import android.content.Context

/** Reads a color resource using an API available on Android 5.0 and newer. */
@Suppress("DEPRECATION")
fun Context.colorCompat(resourceId: Int): Int = resources.getColor(resourceId)
