package lucky.hub.loader

import android.app.Activity
import android.app.AlertDialog
import android.content.BroadcastReceiver
import android.content.ClipData
import android.content.Context
import android.content.ClipboardManager
import android.content.Intent
import android.content.IntentFilter
import android.graphics.drawable.ColorDrawable
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import android.widget.VideoView
import java.io.File
import lucky.hub.loader.data.RemoteConfig
import lucky.hub.loader.data.RemoteConfigRepository
import java.util.concurrent.Executors

class MainActivity : Activity() {
    companion object {
        private const val NOTIFICATION_PERMISSION_REQUEST = 930
        private const val STATE_SELECTED_PAGE = "selected_page"
    }

    private val configExecutor = Executors.newSingleThreadExecutor()
    private val gameExecutor = Executors.newSingleThreadExecutor()
    private val updateExecutor = Executors.newSingleThreadExecutor()
    private lateinit var gamesPage: View
    private lateinit var resellersPage: View
    private lateinit var profilePage: View
    private lateinit var settingsPage: View
    private lateinit var eightBallStatus: TextView
    private lateinit var configStatus: TextView
    private lateinit var updateMessage: TextView
    private lateinit var refreshButton: TextView
    private lateinit var gameDownloadProgress: ProgressBar
    private lateinit var bannerVideo: LoopingBannerVideo
    private lateinit var languageController: LanguageController
    private var selectedPage = "games"
    private var activityResumed = false
    private var gameDownloadReceiverRegistered = false
    private var gameActionInProgress = false
    private var gameDownloadRunning = false
    private var installSessionStarted = false
    private var awaitingUnknownSourcesPermission = false
    private var pendingGameApk: File? = null
    private var pendingLoaderUpdateApk: File? = null
    private var pendingLoaderUpdateConfig: RemoteConfig? = null
    private var loaderUpdateDownloadRunning = false
    private var loaderUpdateInstallSessionStarted = false
    private var awaitingLoaderUpdatePermission = false
    private var installResultReceiverRegistered = false
    private val installResultReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action != GameInstaller.INSTALL_RESULT_ACTION || isFinishing || isDestroyed) return
            val message = intent.getStringExtra(GameInstaller.INSTALL_RESULT_MESSAGE)
                ?: "INSTALLATION WAS NOT COMPLETED."
            if (intent.getBooleanExtra(GameInstaller.INSTALL_RESULT_IS_LOADER_UPDATE, false)) {
                loaderUpdateDownloadRunning = false
                loaderUpdateInstallSessionStarted = false
                awaitingLoaderUpdatePermission = false
                pendingLoaderUpdateApk = null
                refreshButton.isEnabled = true
                setGameButtonEnabled(true)
                configStatus.text = if (intent.getBooleanExtra(GameInstaller.INSTALL_RESULT_SUCCESS, false)) {
                    "UPDATE INSTALLED"
                } else {
                    "UPDATE INSTALL FAILED"
                }
                updateMessage.text = message
            } else {
                finishGameAction(message)
            }
        }
    }

    private val gameDownloadReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (isFinishing || isDestroyed) return
            when (intent.action) {
                GameDownloadService.ACTION_PROGRESS -> {
                    if (gameDownloadRunning) {
                        showGameDownloadProgress(
                            intent.getLongExtra(GameDownloadService.EXTRA_DOWNLOADED, 0L),
                            intent.getLongExtra(GameDownloadService.EXTRA_TOTAL, -1L)
                        )
                    }
                }

                GameDownloadService.ACTION_COMPLETE -> {
                    gameDownloadRunning = false
                    gameActionInProgress = true
                    pendingGameApk = GameDownloadService.apkFile(this@MainActivity)
                    gameDownloadProgress.visibility = View.GONE
                    setGameButtonEnabled(false)
                    eightBallStatus.text = "DOWNLOAD COMPLETE · PREPARING INSTALL…"
                    if (activityResumed) pendingGameApk?.let(::installDownloadedGame)
                }

                GameDownloadService.ACTION_FAILED -> finishGameAction(
                    intent.getStringExtra(GameDownloadService.EXTRA_MESSAGE)
                        ?: "DOWNLOAD FAILED · TAP TO RETRY."
                )
            }
        }
    }

    private var configRequestRunning = false
    private var updateDialogShownFor: String? = null
    private var updateDialogShownDownloadUrl: String? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (!SessionStore.hasActiveSession(this)) {
            startActivity(Intent(this, KeyLoginActivity::class.java))
            finish()
            return
        }

        setContentView(R.layout.activity_main)
        findViewById<TextView>(R.id.currentLanguageValue).text = LanguageController.displayName(this)
        languageController = LanguageController(this)
        languageController.bind(findViewById(R.id.mainRoot))
        findViewById<View>(R.id.changeLanguageButton).setOnClickListener {
            languageController.openLanguagePicker()
        }
        window.statusBarColor = colorCompat(R.color.night_950)
        window.navigationBarColor = colorCompat(R.color.night_950)

        gamesPage = findViewById(R.id.gamesPage)
        resellersPage = findViewById(R.id.resellersPage)
        profilePage = findViewById(R.id.profilePage)
        settingsPage = findViewById(R.id.settingsPage)
        eightBallStatus = findViewById(R.id.eightBallStatus)
        configStatus = findViewById(R.id.configStatus)
        updateMessage = findViewById(R.id.updateMessage)
        refreshButton = findViewById(R.id.refreshButton)
        gameDownloadProgress = findViewById(R.id.gameDownloadProgress)
        bannerVideo = LoopingBannerVideo(
            this,
            findViewById<VideoView>(R.id.dashboardBannerVideo),
            findViewById(R.id.dashboardBannerFallback),
            R.raw.lucky_hub_video
        )
        registerInstallResultReceiver()
        registerGameDownloadReceiver()

        findViewById<TextView>(R.id.versionLabel).text = "INSTALLED  ${BuildConfig.VERSION_NAME}"
        findViewById<TextView>(R.id.profileKey).text = SessionStore.maskedKey(this)
        findViewById<TextView>(R.id.profileExpiry).text = SessionStore.expiry(this)
        findViewById<TextView>(R.id.licenseExpiry).text = "Expires · ${SessionStore.expiry(this)}"

        findViewById<View>(R.id.eightBallButton).setOnClickListener {
            playOrDownloadGame()
        }
        findViewById<View>(R.id.tabGames).setOnClickListener { showPage("games") }
        findViewById<View>(R.id.tabResellers).setOnClickListener { showPage("resellers") }
        findViewById<View>(R.id.tabProfile).setOnClickListener { showPage("profile") }
        findViewById<View>(R.id.tabSettings).setOnClickListener { showPage("settings") }
        findViewById<View>(R.id.openChannelButton).setOnClickListener { openKeyChannel() }
        findViewById<View>(R.id.copyDeviceButton).setOnClickListener { copyDeviceId() }
        findViewById<View>(R.id.logoutButton).setOnClickListener { logOut() }
        findViewById<View>(R.id.settingsLogoutButton).setOnClickListener { logOut() }
        refreshButton.setOnClickListener { checkRemoteConfig(showFeedback = true) }
        findViewById<View>(R.id.mainRoot).apply {
            alpha = 0f
            translationY = 8f
            animate().alpha(1f).translationY(0f).setDuration(300L).start()
        }
        showPage(savedInstanceState?.getString(STATE_SELECTED_PAGE) ?: "games")
        updateGameStates()
        if (GameDownloadService.isDownloadActive()) {
            adoptRunningGameDownload()
        } else if (intent.getBooleanExtra(GameDownloadService.EXTRA_OPEN_INSTALL, false)) {
            resumeGameInstallFromNotification()
        }
        checkRemoteConfig(showFeedback = false)
    }

    override fun onResume() {
        super.onResume()
        activityResumed = true
        if (::bannerVideo.isInitialized) bannerVideo.onResume()
        if (::eightBallStatus.isInitialized) {
            if (installSessionStarted) {
                installSessionStarted = false
                gameActionInProgress = false
                pendingGameApk = null
                gameDownloadProgress.visibility = View.GONE
                setGameButtonEnabled(true)
                updateGameStates()
            } else if (!gameActionInProgress) {
                updateGameStates()
            }
            if (GameDownloadService.isDownloadActive() && !gameActionInProgress) {
                adoptRunningGameDownload()
            }
            if (gameActionInProgress && !gameDownloadRunning && !installSessionStarted &&
                !awaitingUnknownSourcesPermission
            ) {
                pendingGameApk?.let(::installDownloadedGame)
            }
        }
        if (pendingLoaderUpdateApk != null && !loaderUpdateDownloadRunning &&
            !loaderUpdateInstallSessionStarted && !awaitingLoaderUpdatePermission
        ) {
            pendingLoaderUpdateConfig?.let { config ->
                pendingLoaderUpdateApk?.let { apk -> installLoaderUpdate(apk, config.version) }
            }
        }
    }

    override fun onPause() {
        activityResumed = false
        if (::bannerVideo.isInitialized) bannerVideo.onPause()
        super.onPause()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putString(STATE_SELECTED_PAGE, selectedPage)
        super.onSaveInstanceState(outState)
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        if (intent.getBooleanExtra(GameDownloadService.EXTRA_OPEN_INSTALL, false)) {
            resumeGameInstallFromNotification()
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == NOTIFICATION_PERMISSION_REQUEST &&
            grantResults.firstOrNull() != android.content.pm.PackageManager.PERMISSION_GRANTED
        ) {
            languageController.showToast(
                "ALLOW NOTIFICATIONS TO SEE GAME DOWNLOAD PROGRESS.",
                Toast.LENGTH_LONG
            )
        }
    }

    @Deprecated("Deprecated in Android, retained for the install permission result on API 21+")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == GameInstaller.UNKNOWN_SOURCES_UPDATE_REQUEST) {
            awaitingLoaderUpdatePermission = false
            val config = pendingLoaderUpdateConfig
            if (config == null) {
                finishLoaderUpdate("UPDATE CONFIGURATION WAS LOST · CHECK AGAIN.")
            } else if (GameInstaller.canInstallUnknownApps(this)) {
                val readyApk = pendingLoaderUpdateApk
                if (readyApk != null) installLoaderUpdate(readyApk, config.version)
                else beginLoaderUpdateDownload(config)
            } else {
                finishLoaderUpdate("ALLOW APP UPDATES IN ANDROID SETTINGS, THEN TRY AGAIN.")
            }
            return
        }
        if (requestCode != GameInstaller.UNKNOWN_SOURCES_REQUEST) return

        awaitingUnknownSourcesPermission = false
        val apk = pendingGameApk
        if (apk == null) {
            finishGameAction("DOWNLOAD AGAIN TO INSTALL THE GAME.")
        } else if (GameInstaller.canInstallUnknownApps(this)) {
            installDownloadedGame(apk)
        } else {
            finishGameAction("ALLOW GAME INSTALLS IN SETTINGS, THEN TRY AGAIN.")
        }
    }

    override fun onDestroy() {
        if (installResultReceiverRegistered) {
            unregisterReceiver(installResultReceiver)
            installResultReceiverRegistered = false
        }
        if (gameDownloadReceiverRegistered) {
            unregisterReceiver(gameDownloadReceiver)
            gameDownloadReceiverRegistered = false
        }
        configExecutor.shutdownNow()
        gameExecutor.shutdownNow()
        updateExecutor.shutdownNow()
        if (::bannerVideo.isInitialized) bannerVideo.release()
        if (::languageController.isInitialized) languageController.close()
        super.onDestroy()
    }

    private fun registerGameDownloadReceiver() {
        val filter = IntentFilter().apply {
            addAction(GameDownloadService.ACTION_PROGRESS)
            addAction(GameDownloadService.ACTION_COMPLETE)
            addAction(GameDownloadService.ACTION_FAILED)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(gameDownloadReceiver, filter, Context.RECEIVER_NOT_EXPORTED)
        } else {
            @Suppress("DEPRECATION")
            registerReceiver(gameDownloadReceiver, filter)
        }
        gameDownloadReceiverRegistered = true
    }

    private fun registerInstallResultReceiver() {
        val filter = IntentFilter(GameInstaller.INSTALL_RESULT_ACTION)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(installResultReceiver, filter, Context.RECEIVER_NOT_EXPORTED)
        } else {
            @Suppress("DEPRECATION")
            registerReceiver(installResultReceiver, filter)
        }
        installResultReceiverRegistered = true
    }

    private fun updateGameStates() {
        if (gameActionInProgress) return
        val linkedGameInstalled = GameLauncher.isInstalled(this, AppConfig.GAME_PACKAGE)
        val downloadReady = GameDownloadService.apkFile(this).let { it.isFile && it.length() > 0L }
        eightBallStatus.text = when {
            linkedGameInstalled -> "GAME INSTALLED · TAP TO PLAY"
            downloadReady -> "DOWNLOAD READY · TAP TO INSTALL"
            else -> "TAP TO DOWNLOAD 8 BALL POOL"
        }
        eightBallStatus.setTextColor(
            colorCompat(if (downloadReady || linkedGameInstalled) R.color.pool_green else R.color.signal_blue)
        )
        findViewById<View>(R.id.eightBallButton).contentDescription = when {
            linkedGameInstalled -> "PLAY 8 BALL POOL"
            downloadReady -> "INSTALL THE DOWNLOADED GAME"
            else -> "DOWNLOAD AND INSTALL 8 BALL POOL"
        }
        findViewById<TextView>(R.id.eightBallButtonIcon).text = if (linkedGameInstalled) "▶" else "↓"
    }

    private fun playOrDownloadGame() {
        if (gameActionInProgress) return
        if (GameLauncher.isInstalled(this, AppConfig.GAME_PACKAGE)) {
            if (!GameLauncher.launchDownloadedGame(this, AppConfig.GAME_PACKAGE)) {
                languageController.showToast("Android could not open 8 Ball Pool.", Toast.LENGTH_LONG)
            }
            return
        }
        downloadLinkedGame()
    }

    private fun downloadLinkedGame() {
        if (gameActionInProgress) return
        if (GameDownloadService.isDownloadActive()) {
            adoptRunningGameDownload()
            return
        }

        val existingApk = GameDownloadService.apkFile(this)
        if (existingApk.isFile && existingApk.length() > 0L) {
            prepareDownloadedApk(existingApk, retryIfInvalid = true)
        } else {
            beginGameDownload()
        }
    }

    private fun beginGameDownload() {
        gameActionInProgress = true
        gameDownloadRunning = true
        installSessionStarted = false
        awaitingUnknownSourcesPermission = false
        pendingGameApk = null
        setGameButtonEnabled(false)
        gameDownloadProgress.visibility = View.VISIBLE
        gameDownloadProgress.isIndeterminate = true
        gameDownloadProgress.progress = 0
        eightBallStatus.text = "PREPARING GAME DOWNLOAD…"
        eightBallStatus.setTextColor(colorCompat(R.color.signal_blue))

        try {
            val serviceIntent = Intent(this, GameDownloadService::class.java)
                .setAction(GameDownloadService.ACTION_START)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                startForegroundService(serviceIntent)
            } else {
                startService(serviceIntent)
            }
            requestNotificationPermissionIfNeeded()
        } catch (error: Exception) {
            finishGameAction("COULD NOT START THE GAME DOWNLOAD · TAP TO RETRY.")
            languageController.showToast(
                error.message ?: "Could not start the game download.",
                Toast.LENGTH_LONG
            )
        }
    }

    private fun requestNotificationPermissionIfNeeded() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS) !=
            android.content.pm.PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(
                arrayOf(android.Manifest.permission.POST_NOTIFICATIONS),
                NOTIFICATION_PERMISSION_REQUEST
            )
        }
    }

    private fun adoptRunningGameDownload() {
        gameActionInProgress = true
        gameDownloadRunning = true
        pendingGameApk = null
        setGameButtonEnabled(false)
        gameDownloadProgress.visibility = View.VISIBLE
        val (downloaded, total) = GameDownloadService.currentProgress()
        showGameDownloadProgress(downloaded, total)
    }

    private fun prepareDownloadedApk(apkFile: File, retryIfInvalid: Boolean) {
        gameActionInProgress = true
        gameDownloadRunning = false
        installSessionStarted = false
        pendingGameApk = null
        setGameButtonEnabled(false)
        gameDownloadProgress.visibility = View.VISIBLE
        gameDownloadProgress.isIndeterminate = true
        eightBallStatus.text = "CHECKING DOWNLOADED GAME…"
        eightBallStatus.setTextColor(colorCompat(R.color.signal_blue))

        gameExecutor.execute {
            val result = runCatching {
                GameInstaller.validateApk(this, apkFile, AppConfig.GAME_PACKAGE)
                apkFile
            }
            runOnUiThread {
                if (isFinishing || isDestroyed) return@runOnUiThread
                result.fold(
                    onSuccess = { downloadedApk ->
                        pendingGameApk = downloadedApk
                        gameDownloadProgress.visibility = View.GONE
                        eightBallStatus.text = "DOWNLOAD COMPLETE · PREPARING INSTALL…"
                        if (activityResumed) installDownloadedGame(downloadedApk)
                    },
                    onFailure = {
                        apkFile.delete()
                        if (retryIfInvalid) {
                            beginGameDownload()
                        } else {
                            finishGameAction("DOWNLOADED GAME IS INVALID · TAP TO RETRY.")
                        }
                    }
                )
            }
        }
    }

    private fun resumeGameInstallFromNotification() {
        if (installSessionStarted || gameDownloadRunning || GameDownloadService.isDownloadActive()) return
        val apk = GameDownloadService.apkFile(this)
        if (!apk.isFile || apk.length() == 0L) {
            updateGameStates()
            return
        }
        if (gameActionInProgress && pendingGameApk != null) {
            if (!awaitingUnknownSourcesPermission) installDownloadedGame(pendingGameApk!!)
            return
        }
        prepareDownloadedApk(apk, retryIfInvalid = false)
    }

    private fun showGameDownloadProgress(downloaded: Long, total: Long) {
        if (total > 0) {
            val percent = ((downloaded.toDouble() / total.toDouble()) * 100.0)
                .toInt().coerceIn(0, 100)
            gameDownloadProgress.isIndeterminate = false
            gameDownloadProgress.progress = percent
            eightBallStatus.text = "DOWNLOADING GAME · $percent%"
        } else {
            gameDownloadProgress.isIndeterminate = true
            val downloadedMb = downloaded / (1024L * 1024L)
            eightBallStatus.text = "DOWNLOADING GAME · ${downloadedMb} MB"
        }
        eightBallStatus.setTextColor(colorCompat(R.color.signal_blue))
    }

    private fun installDownloadedGame(apkFile: File) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
            !GameInstaller.canInstallUnknownApps(this)
        ) {
            gameDownloadProgress.visibility = View.GONE
            eightBallStatus.text = "ALLOW THIS APP TO INSTALL THE DOWNLOADED GAME."
            awaitingUnknownSourcesPermission = true
            try {
                GameInstaller.requestUnknownSourcesPermission(this)
            } catch (_: Exception) {
                finishGameAction("ENABLE GAME INSTALLS IN ANDROID SETTINGS.")
            }
            return
        }

        awaitingUnknownSourcesPermission = false
        installSessionStarted = true
        gameDownloadProgress.visibility = View.VISIBLE
        gameDownloadProgress.isIndeterminate = true
        eightBallStatus.text = "INSTALLING DOWNLOADED GAME…"
        eightBallStatus.setTextColor(colorCompat(R.color.signal_blue))
        gameExecutor.execute {
            val result = runCatching {
                GameInstaller.install(applicationContext, apkFile, AppConfig.GAME_PACKAGE)
            }
            runOnUiThread {
                if (isFinishing || isDestroyed) return@runOnUiThread
                result.fold(
                    onSuccess = {
                        if (gameActionInProgress && installSessionStarted) {
                            gameDownloadProgress.visibility = View.GONE
                            eightBallStatus.text = "CONFIRM INSTALLATION TO LAUNCH THE DOWNLOADED GAME."
                        }
                    },
                    onFailure = { error ->
                        installSessionStarted = false
                        finishGameAction("INSTALL FAILED · TAP TO DOWNLOAD AGAIN.")
                        languageController.showToast(
                            error.message ?: "Could not start the game installer.",
                            Toast.LENGTH_LONG
                        )
                    }
                )
            }
        }
    }

    private fun setGameButtonEnabled(enabled: Boolean) {
        findViewById<View>(R.id.eightBallButton).apply {
            isEnabled = enabled
            alpha = if (enabled) 1f else 0.55f
        }
    }

    private fun finishGameAction(message: String) {
        gameActionInProgress = false
        gameDownloadRunning = false
        installSessionStarted = false
        awaitingUnknownSourcesPermission = false
        pendingGameApk = null
        gameDownloadProgress.visibility = View.GONE
        setGameButtonEnabled(true)
        eightBallStatus.text = message
        eightBallStatus.setTextColor(colorCompat(R.color.accent_pink))
    }

    private fun showPage(page: String) {
        selectedPage = page
        if (::bannerVideo.isInitialized) bannerVideo.setPageActive(page == "games")
        gamesPage.visibility = if (page == "games") View.VISIBLE else View.GONE
        resellersPage.visibility = if (page == "resellers") View.VISIBLE else View.GONE
        profilePage.visibility = if (page == "profile") View.VISIBLE else View.GONE
        settingsPage.visibility = if (page == "settings") View.VISIBLE else View.GONE

        val tabs = listOf(
            Triple(R.id.tabGames, R.id.tabGamesIcon, R.id.tabGamesLabel) to "games",
            Triple(R.id.tabResellers, R.id.tabResellersIcon, R.id.tabResellersLabel) to "resellers",
            Triple(R.id.tabProfile, R.id.tabProfileIcon, R.id.tabProfileLabel) to "profile",
            Triple(R.id.tabSettings, R.id.tabSettingsIcon, R.id.tabSettingsLabel) to "settings"
        )
        for ((ids, name) in tabs) {
            val selected = name == page
            val color = colorCompat(if (selected) R.color.accent_pink else R.color.ink_secondary)
            findViewById<ImageView>(ids.second).setColorFilter(color)
            findViewById<TextView>(ids.third).apply {
                setTextColor(color)
                setTypeface(null, if (selected) android.graphics.Typeface.BOLD else android.graphics.Typeface.NORMAL)
            }
            findViewById<View>(ids.first).isSelected = selected
        }
    }

    private fun copyDeviceId() {
        val deviceId = SessionStore.deviceId(this)
        if (deviceId.isBlank()) {
            languageController.showToast("Device ID is not available.", Toast.LENGTH_SHORT)
            return
        }
        val clipboard = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
        clipboard.setPrimaryClip(ClipData.newPlainText("Device ID", deviceId))
        languageController.showToast("Device ID copied.", Toast.LENGTH_SHORT)
    }

    private fun openKeyChannel() {
        try {
            startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(AppConfig.KEY_CHANNEL_URL)))
        } catch (_: Exception) {
            languageController.showToast("No app is available to open the key channel.", Toast.LENGTH_SHORT)
        }
    }

    private fun logOut() {
        SessionStore.clear(this)
        startActivity(Intent(this, KeyLoginActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
        })
    }

    private fun checkRemoteConfig(showFeedback: Boolean) {
        if (configRequestRunning) return
        if (AppConfig.GITHUB_CONFIG_URL.contains("YOUR_GITHUB_USERNAME") ||
            AppConfig.GITHUB_CONFIG_URL.contains("YOUR_REPOSITORY")
        ) {
            configStatus.text = "UPDATE CONFIG LINK NEEDED"
            updateMessage.text = "Add your raw GitHub config.json link in AppConfig.kt to enable version notices."
            if (showFeedback) {
                languageController.showToast("Add your GitHub config link in AppConfig.kt.", Toast.LENGTH_SHORT)
            }
            return
        }

        configRequestRunning = true
        configStatus.text = "CHECKING UPDATE NOTICE…"
        refreshButton.isEnabled = false

        configExecutor.execute {
            val result = runCatching { RemoteConfigRepository.fetch(AppConfig.GITHUB_CONFIG_URL) }
            runOnUiThread {
                configRequestRunning = false
                if (isFinishing || isDestroyed) return@runOnUiThread
                refreshButton.isEnabled = true
                result.fold(
                    onSuccess = { config -> showRemoteConfig(config, showFeedback) },
                    onFailure = { error ->
                        configStatus.text = "UPDATE NOTICE UNAVAILABLE"
                        updateMessage.text = "Could not check the update notice. Check your connection and try again."
                        if (showFeedback) {
                            languageController.showToast(error.message ?: "Update check failed.", Toast.LENGTH_SHORT)
                        }
                    }
                )
            }
        }
    }

    private fun showRemoteConfig(config: RemoteConfig, showFeedback: Boolean) {
        val localVersion = BuildConfig.VERSION_NAME
        val isNewerVersion = compareVersions(config.version, localVersion) > 0
        configStatus.text = if (isNewerVersion) {
            "NEW VERSION · ${config.version}"
        } else {
            "LATEST VERSION · ${config.version}"
        }
        updateMessage.text = config.message.ifBlank {
            if (isNewerVersion) "A NEWER LUCKY HUB 8BP LOADER VERSION IS AVAILABLE." else "YOU ARE UP TO DATE."
        }
        if (isNewerVersion && config.downloadUrl.isNullOrBlank()) {
            updateMessage.append("\n\nADD the loader APK as downloadUrl in the GitHub config.")
        }

        if (isNewerVersion && (showFeedback || updateDialogShownFor != config.version ||
                updateDialogShownDownloadUrl != config.downloadUrl)
        ) {
            updateDialogShownFor = config.version
            updateDialogShownDownloadUrl = config.downloadUrl
            showUpdateNoticeDialog(config.version, updateMessage.text.toString(), config)
        } else if (showFeedback) {
            languageController.showToast("Version info refreshed.", Toast.LENGTH_SHORT)
        }
    }

    private fun showUpdateNoticeDialog(version: String, message: String, config: RemoteConfig) {
        val notice = layoutInflater.inflate(R.layout.dialog_update_notice, null, false)
        notice.findViewById<TextView>(R.id.updateNoticeVersion).text = "VERSION $version"
        notice.findViewById<TextView>(R.id.updateNoticeMessage).text = message

        val dialog = AlertDialog.Builder(this).setView(notice).create()
        val updateButton = notice.findViewById<TextView>(R.id.updateNoticeUpdate)
        updateButton.isEnabled = !config.downloadUrl.isNullOrBlank()
        if (!updateButton.isEnabled) {
            updateButton.text = "APK LINK MISSING"
            updateButton.alpha = 0.55f
        }
        notice.findViewById<View>(R.id.updateNoticeLater).setOnClickListener { dialog.dismiss() }
        updateButton.setOnClickListener {
            dialog.dismiss()
            downloadLoaderUpdateFromNotice(config)
        }
        languageController.bindAdditionalView(notice)
        dialog.setOnShowListener {
            val density = resources.displayMetrics.density
            val width = (resources.displayMetrics.widthPixels - (32f * density).toInt())
                .coerceAtLeast((280f * density).toInt())
                .coerceAtMost((420f * density).toInt())
            dialog.window?.apply {
                setBackgroundDrawable(ColorDrawable(android.graphics.Color.TRANSPARENT))
                setDimAmount(0.72f)
                setLayout(width, ViewGroup.LayoutParams.WRAP_CONTENT)
            }
        }
        dialog.show()
    }

    private fun downloadLoaderUpdateFromNotice(config: RemoteConfig) {
        if (config.downloadUrl.isNullOrBlank()) {
            finishLoaderUpdate("ADD the loader APK downloadUrl to the GitHub config first.")
            return
        }
        if (loaderUpdateDownloadRunning || loaderUpdateInstallSessionStarted) return
        if (GameDownloadService.isDownloadActive() || gameActionInProgress) {
            languageController.showToast("FINISH THE GAME DOWNLOAD OR INSTALL BEFORE UPDATING.", Toast.LENGTH_SHORT)
            return
        }

        pendingLoaderUpdateConfig = config
        showPage("settings")
        refreshButton.isEnabled = false
        setGameButtonEnabled(false)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && !GameInstaller.canInstallUnknownApps(this)) {
            awaitingLoaderUpdatePermission = true
            configStatus.text = "ALLOW LOADER UPDATES"
            updateMessage.text = "Allow Lucky Hub to install its update, then return here."
            try {
                GameInstaller.requestUnknownSourcesPermission(
                    this,
                    GameInstaller.UNKNOWN_SOURCES_UPDATE_REQUEST
                )
            } catch (error: Exception) {
                awaitingLoaderUpdatePermission = false
                finishLoaderUpdate(error.message ?: "ENABLE APP UPDATES IN ANDROID SETTINGS.")
            }
            return
        }
        beginLoaderUpdateDownload(config)
    }

    private fun beginLoaderUpdateDownload(config: RemoteConfig) {
        val downloadUrl = config.downloadUrl ?: run {
            finishLoaderUpdate("ADD the loader APK downloadUrl to the GitHub config first.")
            return
        }
        if (loaderUpdateDownloadRunning) return
        val apk = File(cacheDir, GameInstaller.LOADER_UPDATE_APK_FILE_NAME)
        pendingLoaderUpdateConfig = config
        pendingLoaderUpdateApk = null
        loaderUpdateDownloadRunning = true
        loaderUpdateInstallSessionStarted = false
        awaitingLoaderUpdatePermission = false
        refreshButton.isEnabled = false
        setGameButtonEnabled(false)
        configStatus.text = "DOWNLOADING LOADER UPDATE…"
        updateMessage.text = "Preparing version ${config.version}."

        updateExecutor.execute {
            var lastProgressStep = Int.MIN_VALUE
            val result = runCatching {
                ApkDownloader.download(downloadUrl, apk) { downloaded, total ->
                    val step = if (total > 0L) {
                        (downloaded * 100L / total).toInt().coerceIn(0, 100)
                    } else {
                        (downloaded / (1024L * 1024L)).toInt()
                    }
                    if (step != lastProgressStep) {
                        lastProgressStep = step
                        runOnUiThread {
                            if (!isFinishing && !isDestroyed) {
                                val progress = if (total > 0L) "$step%" else "$step MB"
                                updateMessage.text = "Downloading loader update · $progress"
                            }
                        }
                    }
                }
                GameInstaller.validateApk(this, apk, packageName, config.version)
                apk
            }
            runOnUiThread {
                loaderUpdateDownloadRunning = false
                if (isFinishing || isDestroyed) return@runOnUiThread
                result.fold(
                    onSuccess = { downloadedApk ->
                        pendingLoaderUpdateApk = downloadedApk
                        configStatus.text = "UPDATE READY TO INSTALL"
                        updateMessage.text = "Confirm the Android installer to update Lucky Hub."
                        installLoaderUpdate(downloadedApk, config.version)
                    },
                    onFailure = { error ->
                        apk.delete()
                        finishLoaderUpdate(error.message ?: "LOADER UPDATE DOWNLOAD FAILED · TRY AGAIN.")
                    }
                )
            }
        }
    }

    private fun installLoaderUpdate(apk: File, version: String) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && !GameInstaller.canInstallUnknownApps(this)) {
            awaitingLoaderUpdatePermission = true
            try {
                GameInstaller.requestUnknownSourcesPermission(
                    this,
                    GameInstaller.UNKNOWN_SOURCES_UPDATE_REQUEST
                )
            } catch (error: Exception) {
                awaitingLoaderUpdatePermission = false
                finishLoaderUpdate(error.message ?: "ENABLE APP UPDATES IN ANDROID SETTINGS.")
            }
            return
        }

        awaitingLoaderUpdatePermission = false
        loaderUpdateInstallSessionStarted = true
        configStatus.text = "INSTALLING LOADER UPDATE…"
        updateMessage.text = "Confirm the Android installer to install version $version."
        updateExecutor.execute {
            val result = runCatching {
                GameInstaller.install(applicationContext, apk, packageName)
            }
            runOnUiThread {
                if (isFinishing || isDestroyed) return@runOnUiThread
                result.fold(
                    onSuccess = {
                        configStatus.text = "CONFIRM LOADER UPDATE"
                        updateMessage.text = "Finish the installation in Android to update Lucky Hub."
                    },
                    onFailure = { error ->
                        loaderUpdateInstallSessionStarted = false
                        finishLoaderUpdate(error.message ?: "COULD NOT OPEN THE LOADER INSTALLER.")
                    }
                )
            }
        }
    }

    private fun finishLoaderUpdate(message: String) {
        loaderUpdateDownloadRunning = false
        loaderUpdateInstallSessionStarted = false
        awaitingLoaderUpdatePermission = false
        pendingLoaderUpdateApk = null
        refreshButton.isEnabled = true
        setGameButtonEnabled(true)
        configStatus.text = "LOADER UPDATE"
        updateMessage.text = message
    }

    private fun compareVersions(first: String, second: String): Int {
        val firstParts = first.split('.', '-', '+').map {
            it.takeWhile(Char::isDigit).toIntOrNull() ?: 0
        }
        val secondParts = second.split('.', '-', '+').map {
            it.takeWhile(Char::isDigit).toIntOrNull() ?: 0
        }
        val count = maxOf(firstParts.size, secondParts.size)
        for (index in 0 until count) {
            val left = firstParts.getOrElse(index) { 0 }
            val right = secondParts.getOrElse(index) { 0 }
            if (left != right) return left.compareTo(right)
        }
        return 0
    }
}
