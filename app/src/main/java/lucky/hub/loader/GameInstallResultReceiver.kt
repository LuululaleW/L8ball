package lucky.hub.loader

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import java.io.File
import android.content.pm.PackageInstaller

class GameInstallResultReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        val isLoaderUpdate = intent.getBooleanExtra(GameInstaller.INSTALL_RESULT_IS_LOADER_UPDATE, false)
        val packageName = intent.getStringExtra(GameInstaller.INSTALL_RESULT_GAME_PACKAGE)
            ?: if (isLoaderUpdate) context.packageName else AppConfig.GAME_PACKAGE
        when (intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE)) {
            PackageInstaller.STATUS_PENDING_USER_ACTION -> {
                @Suppress("DEPRECATION")
                val confirmationIntent = intent.getParcelableExtra(Intent.EXTRA_INTENT) as? Intent
                if (confirmationIntent == null) {
                    reportFailure(context, "Could not open the install confirmation.", isLoaderUpdate)
                } else {
                    confirmationIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    context.startActivity(confirmationIntent)
                }
            }

            PackageInstaller.STATUS_SUCCESS -> {
                if (isLoaderUpdate) {
                    File(context.cacheDir, GameInstaller.LOADER_UPDATE_APK_FILE_NAME).delete()
                    context.sendBroadcast(
                        Intent(GameInstaller.INSTALL_RESULT_ACTION)
                            .setPackage(context.packageName)
                            .putExtra(GameInstaller.INSTALL_RESULT_IS_LOADER_UPDATE, true)
                            .putExtra(GameInstaller.INSTALL_RESULT_SUCCESS, true)
                            .putExtra(GameInstaller.INSTALL_RESULT_MESSAGE, "Lucky Hub update installed.")
                    )
                } else {
                    GameDownloadService.apkFile(context).delete()
                }
                if (!isLoaderUpdate && !GameLauncher.launchDownloadedGame(context, packageName)) {
                    reportFailure(context, "Game installed, but Android could not launch it.")
                }
            }

            else -> {
                val statusMessage = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE)
                    ?.takeIf { it.isNotBlank() } ?: "Game installation was not completed."
                reportFailure(context, statusMessage, isLoaderUpdate)
            }
        }
    }

    private fun reportFailure(context: Context, message: String, isLoaderUpdate: Boolean = false) {
        LanguageController.showToast(context, message, android.widget.Toast.LENGTH_LONG)
        context.sendBroadcast(
            Intent(GameInstaller.INSTALL_RESULT_ACTION)
                .setPackage(context.packageName)
                .putExtra(GameInstaller.INSTALL_RESULT_IS_LOADER_UPDATE, isLoaderUpdate)
                .putExtra(GameInstaller.INSTALL_RESULT_SUCCESS, false)
                .putExtra(GameInstaller.INSTALL_RESULT_MESSAGE, message)
        )
    }
}
