package lucky.hub.loader

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import java.io.File
import java.util.concurrent.Executors

/** Keeps the game download alive when the dashboard is in the background. */
class GameDownloadService : Service() {
    companion object {
        const val ACTION_START = "lucky.hub.loader.action.START_GAME_DOWNLOAD"
        const val ACTION_PROGRESS = "lucky.hub.loader.action.GAME_DOWNLOAD_PROGRESS"
        const val ACTION_COMPLETE = "lucky.hub.loader.action.GAME_DOWNLOAD_COMPLETE"
        const val ACTION_FAILED = "lucky.hub.loader.action.GAME_DOWNLOAD_FAILED"
        const val EXTRA_DOWNLOADED = "downloaded_bytes"
        const val EXTRA_TOTAL = "total_bytes"
        const val EXTRA_MESSAGE = "message"
        const val EXTRA_OPEN_INSTALL = "open_game_install"

        private const val NOTIFICATION_CHANNEL_ID = "game_downloads"
        private const val NOTIFICATION_ID = 4102

        @Volatile
        private var downloadActive = false

        @Volatile
        private var currentDownloadedBytes = 0L

        @Volatile
        private var currentTotalBytes = -1L

        fun isDownloadActive(): Boolean = downloadActive

        fun currentProgress(): Pair<Long, Long> =
            currentDownloadedBytes to currentTotalBytes

        fun apkFile(context: Context): File = File(context.filesDir, "luckyhub-linked-game.apk")
    }

    private val executor = Executors.newSingleThreadExecutor()
    private val notificationManager by lazy {
        getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    }

    @Volatile
    private var downloadStarted = false

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action != ACTION_START) {
            stopSelf(startId)
            return START_NOT_STICKY
        }

        createNotificationChannel()
        startForegroundCompat(buildProgressNotification(0L, -1L))

        if (!downloadStarted) {
            downloadStarted = true
            downloadActive = true
            currentDownloadedBytes = 0L
            currentTotalBytes = -1L
            executor.execute { downloadGame() }
        }
        return START_REDELIVER_INTENT
    }

    private fun downloadGame() {
        val apk = apkFile(this)
        var lastNotificationStep = Long.MIN_VALUE
        val result = runCatching {
            ApkDownloader.download(AppConfig.GAME_DOWNLOAD_URL, apk) { downloaded, total ->
                currentDownloadedBytes = downloaded
                currentTotalBytes = total
                val step = if (total > 0L) downloaded * 100L / total else downloaded / (1024L * 1024L)
                if (step != lastNotificationStep) {
                    lastNotificationStep = step
                    notifyDownload(buildProgressNotification(downloaded, total))
                    sendDownloadBroadcast(ACTION_PROGRESS) {
                        putExtra(EXTRA_DOWNLOADED, downloaded)
                        putExtra(EXTRA_TOTAL, total)
                    }
                }
            }
            GameInstaller.validateApk(this, apk, AppConfig.GAME_PACKAGE)
            apk
        }

        downloadActive = false
        result.fold(
            onSuccess = {
                sendDownloadBroadcast(ACTION_COMPLETE)
                notifyDownload(
                    buildFinishedNotification(
                        title = "Game download complete",
                        message = "Tap to return and install the game.",
                        openInstall = true
                    )
                )
            },
            onFailure = { error ->
                apk.delete()
                sendDownloadBroadcast(ACTION_FAILED) {
                    putExtra(EXTRA_MESSAGE, error.message ?: "GAME DOWNLOAD FAILED · TAP TO RETRY.")
                }
                notifyDownload(
                    buildFinishedNotification(
                        title = "Game download failed",
                        message = error.message ?: "Tap to return and retry the download.",
                        openInstall = false
                    )
                )
            }
        )
        stopForeground(false)
        stopSelf()
    }

    private fun notifyDownload(notification: Notification) {
        runCatching { notificationManager.notify(NOTIFICATION_ID, notification) }
    }

    private fun sendDownloadBroadcast(
        action: String,
        extras: Intent.() -> Unit = {}
    ) {
        sendBroadcast(Intent(action).setPackage(packageName).apply(extras))
    }

    private fun startForegroundCompat(notification: Notification) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC)
        } else {
            startForeground(NOTIFICATION_ID, notification)
        }
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return
        val channel = NotificationChannel(
            NOTIFICATION_CHANNEL_ID,
            "Game downloads",
            NotificationManager.IMPORTANCE_LOW
        ).apply {
            description = "Shows progress while downloading a game."
            setShowBadge(false)
        }
        notificationManager.createNotificationChannel(channel)
    }

    private fun buildProgressNotification(downloaded: Long, total: Long): Notification {
        val builder = notificationBuilder()
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setContentTitle("Downloading 8 Ball Pool")
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setCategory(Notification.CATEGORY_PROGRESS)
            .setContentIntent(openDashboardPendingIntent(openInstall = false))

        if (total > 0L) {
            val percent = (downloaded * 100L / total).toInt().coerceIn(0, 100)
            builder
                .setContentText("$percent% · ${formatMegabytes(downloaded)} MB")
                .setProgress(100, percent, false)
        } else {
            builder
                .setContentText("${formatMegabytes(downloaded)} MB downloaded")
                .setProgress(0, 0, true)
        }
        return builder.build()
    }

    private fun buildFinishedNotification(
        title: String,
        message: String,
        openInstall: Boolean
    ): Notification = notificationBuilder()
        .setSmallIcon(
            if (openInstall) android.R.drawable.stat_sys_download_done
            else android.R.drawable.stat_notify_error
        )
        .setContentTitle(title)
        .setContentText(message)
        .setAutoCancel(true)
        .setOngoing(false)
        .setContentIntent(openDashboardPendingIntent(openInstall))
        .build()

    private fun notificationBuilder(): Notification.Builder =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Notification.Builder(this, NOTIFICATION_CHANNEL_ID)
        } else {
            @Suppress("DEPRECATION")
            Notification.Builder(this)
        }

    private fun openDashboardPendingIntent(openInstall: Boolean): PendingIntent {
        val intent = Intent(this, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP or
                Intent.FLAG_ACTIVITY_SINGLE_TOP
            putExtra(EXTRA_OPEN_INSTALL, openInstall)
        }
        var flags = PendingIntent.FLAG_UPDATE_CURRENT
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) flags = flags or PendingIntent.FLAG_IMMUTABLE
        return PendingIntent.getActivity(this, NOTIFICATION_ID, intent, flags)
    }

    private fun formatMegabytes(bytes: Long): Long = bytes / (1024L * 1024L)

    override fun onDestroy() {
        downloadActive = false
        executor.shutdownNow()
        super.onDestroy()
    }
}
