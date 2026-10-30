package lucky.hub.loader

import android.app.Activity
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.View
import android.view.inputmethod.InputMethodManager
import android.widget.EditText
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import java.util.concurrent.Executors

class KeyLoginActivity : Activity() {
    private val authExecutor = Executors.newSingleThreadExecutor()
    private lateinit var keyField: EditText
    private lateinit var loginButton: TextView
    private lateinit var loginStatus: TextView
    private lateinit var loginProgress: ProgressBar
    private lateinit var pasteButton: View
    private lateinit var getKeyButton: View
    private lateinit var languageController: LanguageController
    private var loginInProgress = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_login)
        languageController = LanguageController(this)
        languageController.bind(findViewById(R.id.keyRoot))
        window.statusBarColor = colorCompat(R.color.night_950)
        window.navigationBarColor = colorCompat(R.color.night_950)

        keyField = findViewById(R.id.accessKey)
        loginButton = findViewById(R.id.loginButton)
        loginStatus = findViewById(R.id.loginStatus)
        loginProgress = findViewById(R.id.loginProgress)
        pasteButton = findViewById(R.id.pasteButton)
        getKeyButton = findViewById(R.id.getKeyButton)

        pasteButton.setOnClickListener { pasteKey() }
        getKeyButton.setOnClickListener { openKeyChannel() }
        loginButton.setOnClickListener { activateKey() }
    }

    private fun pasteKey() {
        val clipboard = getSystemService(CLIPBOARD_SERVICE) as? ClipboardManager
        val clip: ClipData? = clipboard?.primaryClip
        if (clip == null || clip.itemCount == 0) {
            languageController.showToast("CLIPBOARD IS EMPTY.", Toast.LENGTH_SHORT)
            return
        }
        val pasted = clip.getItemAt(0).coerceToText(this)?.toString()?.trim().orEmpty()
        if (pasted.isBlank()) {
            languageController.showToast("NO KEY FOUND IN THE CLIPBOARD.", Toast.LENGTH_SHORT)
            return
        }
        keyField.setText(pasted)
        keyField.setSelection(pasted.length)
    }

    private fun activateKey() {
        if (loginInProgress) return
        val key = keyField.text.toString().trim()
        if (key.isBlank()) {
            keyField.error = "ENTER YOUR ACCESS KEY"
            keyField.requestFocus()
            return
        }

        keyField.error = null
        (getSystemService(INPUT_METHOD_SERVICE) as? InputMethodManager)
            ?.hideSoftInputFromWindow(keyField.windowToken, 0)
        keyField.clearFocus()

        loginInProgress = true
        loginStatus.setTextColor(colorCompat(R.color.signal_blue))
        loginStatus.text = "VERIFYING YOUR LICENSE…"
        loginButton.text = "CHECKING…"
        loginButton.isEnabled = false
        loginButton.alpha = 0.68f
        keyField.isEnabled = false
        pasteButton.isEnabled = false
        pasteButton.alpha = 0.65f
        getKeyButton.isEnabled = false
        getKeyButton.alpha = 0.65f
        loginProgress.visibility = View.VISIBLE

        authExecutor.execute {
            val outcome = runCatching { KeyAuthRepository.check(this, key) }
            runOnUiThread {
                if (isFinishing || isDestroyed) return@runOnUiThread
                loginInProgress = false
                loginProgress.visibility = View.GONE

                outcome.fold(
                    onSuccess = { result ->
                        when (result) {
                            is KeyAuthRepository.AuthResult.Authorized -> {
                                SessionStore.save(this, key, result.expiry, result.deviceId)
                                loginStatus.setTextColor(colorCompat(R.color.pool_green))
                                loginStatus.text = "LICENSE ACTIVE · ${result.expiry}"
                                loginButton.text = "LICENSE ACTIVE"
                                loginButton.isEnabled = false
                                loginButton.alpha = 0.85f
                                window.decorView.postDelayed({
                                    if (!isFinishing && !isDestroyed) {
                                        startActivity(Intent(this, MainActivity::class.java))
                                        finish()
                                        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out)
                                    }
                                }, 350L)
                            }
                            is KeyAuthRepository.AuthResult.Rejected -> {
                                restoreLoginInputs()
                                showFailure(result.message)
                            }
                        }
                    },
                    onFailure = { error ->
                        restoreLoginInputs()
                        showFailure(error.message ?: "Could not contact the key server.")
                    }
                )
            }
        }
    }

    private fun restoreLoginInputs() {
        loginInProgress = false
        loginButton.text = "ACTIVATE"
        loginButton.isEnabled = true
        loginButton.alpha = 1f
        keyField.isEnabled = true
        pasteButton.isEnabled = true
        pasteButton.alpha = 1f
        getKeyButton.isEnabled = true
        getKeyButton.alpha = 1f
        loginProgress.visibility = View.GONE
    }

    private fun showFailure(message: String) {
        loginStatus.setTextColor(colorCompat(R.color.accent_pink))
        loginStatus.text = message
    }

    private fun openKeyChannel() {
        try {
            startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(AppConfig.KEY_CHANNEL_URL)))
        } catch (_: Exception) {
            languageController.showToast("NO APP IS AVAILABLE TO OPEN THE KEY CHANNEL.", Toast.LENGTH_SHORT)
        }
    }

    override fun onDestroy() {
        authExecutor.shutdownNow()
        if (::languageController.isInitialized) languageController.close()
        super.onDestroy()
    }
}
