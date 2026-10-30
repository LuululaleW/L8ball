package lucky.hub.loader

import android.app.Activity
import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.view.View
import android.widget.VideoView

class SplashActivity : Activity() {
    private val handler = Handler(Looper.getMainLooper())
    private lateinit var bannerVideo: LoopingBannerVideo
    private lateinit var languageController: LanguageController
    private var transitionStarted = false
    private var splashStartedAt = 0L
    private var splashPausedAt = 0L
    private var activityResumed = false
    private val splashDurationMs = 5_500L

    private val finishSplash = object : Runnable {
        override fun run() {
            if (transitionStarted || isFinishing || !activityResumed) return
            openLogin()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setBackgroundDrawableResource(R.drawable.bg_amash_splash)
        setContentView(R.layout.activity_splash)
        languageController = LanguageController(this)
        languageController.bind(findViewById(R.id.splashRoot))
        window.statusBarColor = Color.BLACK
        window.navigationBarColor = Color.BLACK
        window.decorView.systemUiVisibility = 0

        bannerVideo = LoopingBannerVideo(
            this,
            findViewById<VideoView>(R.id.splashBannerVideo),
            findViewById(R.id.splashBanner),
            R.raw.lucky_hub_video
        )

        findViewById<View>(R.id.splashLogo).apply {
            alpha = 0.15f
            scaleX = 0.88f
            scaleY = 0.88f
            animate().alpha(1f).scaleX(1f).scaleY(1f).setDuration(650L).start()
        }

        splashStartedAt = SystemClock.uptimeMillis()
    }

    override fun onResume() {
        super.onResume()
        activityResumed = true
        bannerVideo.onResume()
        if (splashPausedAt > 0L) {
            splashStartedAt += SystemClock.uptimeMillis() - splashPausedAt
            splashPausedAt = 0L
        }
        handler.removeCallbacks(finishSplash)
        val remaining = splashDurationMs - (SystemClock.uptimeMillis() - splashStartedAt)
        if (remaining <= 0L) finishSplash.run() else handler.postDelayed(finishSplash, remaining)
    }

    override fun onPause() {
        activityResumed = false
        if (::bannerVideo.isInitialized) bannerVideo.onPause()
        handler.removeCallbacks(finishSplash)
        if (splashStartedAt > 0L) splashPausedAt = SystemClock.uptimeMillis()
        super.onPause()
    }

    private fun openLogin() {
        if (transitionStarted || isFinishing || isDestroyed) return
        transitionStarted = true
        handler.removeCallbacksAndMessages(null)
        startActivity(android.content.Intent(this, KeyLoginActivity::class.java))
        finish()
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out)
    }

    override fun onDestroy() {
        handler.removeCallbacksAndMessages(null)
        if (::bannerVideo.isInitialized) bannerVideo.release()
        if (::languageController.isInitialized) languageController.close()
        super.onDestroy()
    }
}
