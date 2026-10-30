package lucky.hub.loader

import android.app.Activity
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageInstaller
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import java.io.File
import java.io.FileInputStream
import java.io.IOException

object GameInstaller {
    const val INSTALL_RESULT_GAME_PACKAGE = "lucky.hub.loader.GAME_PACKAGE"
    const val INSTALL_RESULT_ACTION = "lucky.hub.loader.GAME_INSTALL_RESULT"
    const val INSTALL_RESULT_MESSAGE = "lucky.hub.loader.GAME_INSTALL_MESSAGE"
    const val INSTALL_RESULT_IS_LOADER_UPDATE = "lucky.hub.loader.IS_LOADER_UPDATE"
    const val INSTALL_RESULT_SUCCESS = "lucky.hub.loader.INSTALL_SUCCESS"
    const val LOADER_UPDATE_APK_FILE_NAME = "lucky-hub-update.apk"
    const val UNKNOWN_SOURCES_REQUEST = 4821
    const val UNKNOWN_SOURCES_UPDATE_REQUEST = 4822

    fun canInstallUnknownApps(context: Context): Boolean =
        Build.VERSION.SDK_INT < Build.VERSION_CODES.O ||
            context.packageManager.canRequestPackageInstalls()

    fun requestUnknownSourcesPermission(
        activity: Activity,
        requestCode: Int = UNKNOWN_SOURCES_REQUEST
    ) {
        val intent = Intent(
            android.provider.Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
            Uri.parse("package:${activity.packageName}")
        )
        activity.startActivityForResult(intent, requestCode)
    }

    @Throws(IOException::class)
    fun validateApk(
        context: Context,
        apkFile: File,
        expectedPackage: String,
        expectedVersionName: String? = null
    ) {
        if (!apkFile.isFile || apkFile.length() == 0L) {
            throw IOException("THE DOWNLOADED APK IS MISSING OR EMPTY.")
        }
        val archive = context.packageManager.getPackageArchiveInfo(apkFile.absolutePath, 0)
            ?: throw IOException("THE DOWNLOAD IS NOT A VALID ANDROID APK.")
        if (archive.packageName != expectedPackage) {
            throw IOException("THE DOWNLOADED APK IS NOT THE EXPECTED APP PACKAGE.")
        }
        if (expectedVersionName != null && archive.versionName != expectedVersionName) {
            throw IOException("THE LOADER APK VERSION DOES NOT MATCH THE UPDATE CONFIG.")
        }
    }

    @Throws(Exception::class)
    fun install(context: Context, apkFile: File, expectedPackage: String) {
        validateApk(context, apkFile, expectedPackage)

        val installer = context.packageManager.packageInstaller
        val params = PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL)
        params.setAppPackageName(expectedPackage)
        val sessionId = installer.createSession(params)
        var committed = false
        try {
            val session = installer.openSession(sessionId)
            try {
                FileInputStream(apkFile).use { input ->
                    session.openWrite("game.apk", 0, apkFile.length()).use { output ->
                        input.copyTo(output)
                        session.fsync(output)
                    }
                }

                val resultIntent = Intent(context, GameInstallResultReceiver::class.java)
                    .putExtra(INSTALL_RESULT_GAME_PACKAGE, expectedPackage)
                    .putExtra(INSTALL_RESULT_IS_LOADER_UPDATE, expectedPackage == context.packageName)
                var pendingIntentFlags = PendingIntent.FLAG_UPDATE_CURRENT
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    pendingIntentFlags = pendingIntentFlags or PendingIntent.FLAG_MUTABLE
                }
                val statusReceiver = PendingIntent.getBroadcast(
                    context,
                    sessionId,
                    resultIntent,
                    pendingIntentFlags
                )
                session.commit(statusReceiver.intentSender)
                committed = true
            } finally {
                session.close()
            }
        } finally {
            if (!committed) installer.abandonSession(sessionId)
        }
    }
}
