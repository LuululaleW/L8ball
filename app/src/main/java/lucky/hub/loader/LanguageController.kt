package lucky.hub.loader

import android.app.Activity
import android.app.AlertDialog
import android.content.Context
import android.view.View
import android.view.ViewGroup
import android.view.ViewTreeObserver
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import com.google.mlkit.common.model.DownloadConditions
import com.google.mlkit.nl.translate.TranslateLanguage
import com.google.mlkit.nl.translate.Translation
import com.google.mlkit.nl.translate.Translator
import com.google.mlkit.nl.translate.TranslatorOptions
import java.util.Locale
import java.util.concurrent.ConcurrentHashMap

class LanguageController(private val activity: Activity) {
    data class LanguageOption(val code: String, val name: String)

    private data class TranslationState(
        val language: String,
        val source: String,
        val translated: String?,
        val pending: Boolean
    )

    companion object {
        private const val PREFERENCES = "loader_preferences"
        private const val LANGUAGE_KEY = "loader_language"
        private const val ENGLISH = "en"

        fun currentLanguage(context: Context): String =
            context.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE)
                .getString(LANGUAGE_KEY, ENGLISH) ?: ENGLISH

        fun saveLanguage(context: Context, code: String) {
            context.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE)
                .edit()
                .putString(LANGUAGE_KEY, code)
                .apply()
        }

        fun supportedLanguages(): List<LanguageOption> =
            TranslateLanguage.getAllLanguages()
                .distinct()
                .map { code ->
                    val locale = Locale.forLanguageTag(code)
                    LanguageOption(code, locale.getDisplayName(locale).ifBlank { code })
                }
                .sortedBy { it.name.lowercase(Locale.getDefault()) }

        fun displayName(context: Context): String {
            val code = currentLanguage(context)
            return supportedLanguages().firstOrNull { it.code == code }?.name ?: "English"
        }

        fun showToast(context: Context, message: String, duration: Int = Toast.LENGTH_SHORT) {
            val language = currentLanguage(context)
            val appContext = context.applicationContext
            if (language == ENGLISH) {
                Toast.makeText(appContext, message, duration).show()
                return
            }
            val translator = Translation.getClient(
                TranslatorOptions.Builder()
                    .setSourceLanguage(ENGLISH)
                    .setTargetLanguage(language)
                    .build()
            )
            translator.downloadModelIfNeeded(DownloadConditions.Builder().requireWifi().build())
                .addOnSuccessListener {
                    translator.translate(message).addOnCompleteListener { task ->
                        val output = if (task.isSuccessful) task.result else message
                        Toast.makeText(appContext, output, duration).show()
                        translator.close()
                    }
                }
                .addOnFailureListener {
                    Toast.makeText(appContext, message, duration).show()
                    translator.close()
                }
        }
    }

    private var targetLanguage = currentLanguage(activity)
    private var translator: Translator? = null
    private var modelReady = false
    private var boundRoot: View? = null
    private var globalLayoutListener: ViewTreeObserver.OnGlobalLayoutListener? = null
    private val additionalRoots = mutableListOf<View>()
    private val translatedCache = ConcurrentHashMap<String, String>()

    fun bind(root: View) {
        boundRoot = root
        root.layoutDirection = if (targetLanguage in setOf("ar", "fa", "he", "ur")) {
            View.LAYOUT_DIRECTION_RTL
        } else {
            View.LAYOUT_DIRECTION_LTR
        }
        if (targetLanguage == ENGLISH) return

        activity.findViewById<TextView>(R.id.currentLanguageValue)?.text =
            "${displayName(activity)} · downloading model on Wi-Fi…"

        val options = TranslatorOptions.Builder()
            .setSourceLanguage(ENGLISH)
            .setTargetLanguage(targetLanguage)
            .build()
        val activeTranslator = Translation.getClient(options)
        translator = activeTranslator

        activeTranslator.downloadModelIfNeeded(
            DownloadConditions.Builder().requireWifi().build()
        )
            .addOnSuccessListener {
                if (!activityAlive() || targetLanguage != currentLanguage(activity)) return@addOnSuccessListener
                modelReady = true
                activity.findViewById<TextView>(R.id.currentLanguageValue)?.text = displayName(activity)
                translateAllRoots()
                val observer = root.viewTreeObserver
                if (observer.isAlive && globalLayoutListener == null) {
                    val listener = ViewTreeObserver.OnGlobalLayoutListener {
                        if (activityAlive() && modelReady) translateAllRoots()
                    }
                    globalLayoutListener = listener
                    observer.addOnGlobalLayoutListener(listener)
                }
            }
            .addOnFailureListener {
                if (!activityAlive() || targetLanguage != currentLanguage(activity)) return@addOnFailureListener
                saveLanguage(activity, ENGLISH)
                targetLanguage = ENGLISH
                Toast.makeText(
                    activity,
                    "Could not download the language model. Connect to Wi-Fi and try again.",
                    Toast.LENGTH_LONG
                ).show()
                activity.recreate()
            }
    }

    fun openLanguagePicker() {
        val options = supportedLanguages()
        val names = options.map { it.name }.toTypedArray()
        val current = currentLanguage(activity)
        val selectedIndex = options.indexOfFirst { it.code == current }.coerceAtLeast(0)

        val dialog = AlertDialog.Builder(activity)
            .setTitle("App language · ${options.size} languages")
            .setSingleChoiceItems(names, selectedIndex) { choiceDialog, which ->
                val selected = options[which]
                choiceDialog.dismiss()
                if (selected.code != current) confirmLanguageChange(selected)
            }
            .setNegativeButton("Cancel", null)
            .create()
        dialog.setOnShowListener {
            translatePromptText("App language · ${options.size} languages") { dialog.setTitle(it) }
            dialog.getButton(AlertDialog.BUTTON_NEGATIVE)?.let { button ->
                translatePromptText("Cancel") { button.text = it }
            }
        }
        dialog.show()
    }

    fun close() {
        val root = boundRoot
        val listener = globalLayoutListener
        if (root != null && listener != null && root.viewTreeObserver.isAlive) {
            root.viewTreeObserver.removeOnGlobalLayoutListener(listener)
        }
        globalLayoutListener = null
        boundRoot = null
        additionalRoots.clear()
        translator?.close()
        translator = null
        modelReady = false
    }

    fun bindAdditionalView(view: View) {
        if (additionalRoots.none { it === view }) additionalRoots += view
        if (modelReady) translateTree(view)
    }

    fun showToast(message: String, duration: Int = Toast.LENGTH_SHORT) {
        val activeTranslator = translator
        if (targetLanguage == ENGLISH || !modelReady || activeTranslator == null) {
            Toast.makeText(activity, message, duration).show()
            return
        }
        activeTranslator.translate(message)
            .addOnSuccessListener { translated ->
                if (activityAlive()) Toast.makeText(activity, translated, duration).show()
            }
            .addOnFailureListener {
                if (activityAlive()) Toast.makeText(activity, message, duration).show()
            }
    }

    private fun translateAllRoots() {
        boundRoot?.let(::translateTree)
        additionalRoots.toList().forEach(::translateTree)
    }

    private fun confirmLanguageChange(option: LanguageOption) {
        if (option.code == ENGLISH) {
            saveLanguage(activity, option.code)
            activity.recreate()
            return
        }

        val title = "Use ${option.name}?"
        val message =
            "The first use downloads an on-device language model (about 30 MB) over Wi-Fi. " +
                "After download, translations work offline."
        val dialog = AlertDialog.Builder(activity)
            .setTitle(title)
            .setMessage(message)
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Continue") { _, _ ->
                saveLanguage(activity, option.code)
                activity.recreate()
            }
            .create()
        dialog.setOnShowListener {
            translatePromptText(title) { dialog.setTitle(it) }
            dialog.findViewById<TextView>(android.R.id.message)?.let { messageView ->
                translatePromptText(message) { messageView.text = it }
            }
            dialog.getButton(AlertDialog.BUTTON_NEGATIVE)?.let { button ->
                translatePromptText("Cancel") { button.text = it }
            }
            dialog.getButton(AlertDialog.BUTTON_POSITIVE)?.let { button ->
                translatePromptText("Continue") { button.text = it }
            }
        }
        dialog.show()
    }

    private fun translatePromptText(source: String, apply: (String) -> Unit) {
        val activeTranslator = translator
        if (targetLanguage == ENGLISH || !modelReady || activeTranslator == null) {
            apply(source)
            return
        }
        activeTranslator.translate(source)
            .addOnSuccessListener { translated -> if (activityAlive()) apply(translated) }
            .addOnFailureListener { if (activityAlive()) apply(source) }
    }

    private fun translateTree(view: View) {
        if (!modelReady || !activityAlive()) return
        if (view is TextView) {
            if (view !is EditText && view.id != R.id.currentLanguageValue) {
                translateProperty(
                    view,
                    R.id.language_text_translation_state,
                    view.text?.toString().orEmpty(),
                    { view.text?.toString().orEmpty() },
                    { view.text = it }
                )
            }
            view.hint?.toString()?.let { hint ->
                translateProperty(
                    view,
                    R.id.language_hint_translation_state,
                    hint,
                    { view.hint?.toString().orEmpty() },
                    { view.hint = it }
                )
            }
            view.error?.toString()?.let { error ->
                translateProperty(
                    view,
                    R.id.language_error_translation_state,
                    error,
                    { view.error?.toString().orEmpty() },
                    { view.error = it }
                )
            }
        }
        view.contentDescription?.toString()?.let { description ->
            translateProperty(
                view,
                R.id.language_description_translation_state,
                description,
                { view.contentDescription?.toString().orEmpty() },
                { view.contentDescription = it }
            )
        }
        if (view is ViewGroup) {
            for (index in 0 until view.childCount) translateTree(view.getChildAt(index))
        }
    }

    private fun translateProperty(
        view: View,
        stateTag: Int,
        source: String,
        currentValue: () -> String,
        setValue: (String) -> Unit
    ) {
        if (source.isBlank() || source.none { it.isLetter() }) return
        val previous = view.getTag(stateTag) as? TranslationState
        if (previous?.language == targetLanguage &&
            (source == previous.translated || (previous.pending && source == previous.source))
        ) return

        val cached = translatedCache[source]
        if (cached != null) {
            view.setTag(stateTag, TranslationState(targetLanguage, source, cached, false))
            setValue(cached)
            return
        }

        val state = TranslationState(targetLanguage, source, null, true)
        view.setTag(stateTag, state)
        val activeTranslator = translator ?: return
        activeTranslator.translate(source)
            .addOnSuccessListener { translated ->
                translatedCache[source] = translated
                if (activityAlive() && view.getTag(stateTag) === state && currentValue() == source) {
                    view.setTag(stateTag, TranslationState(targetLanguage, source, translated, false))
                    setValue(translated)
                }
            }
            .addOnFailureListener {
                if (view.getTag(stateTag) === state) {
                    view.setTag(stateTag, TranslationState(targetLanguage, source, source, false))
                }
            }
    }

    private fun activityAlive(): Boolean = !activity.isFinishing && !activity.isDestroyed
}
