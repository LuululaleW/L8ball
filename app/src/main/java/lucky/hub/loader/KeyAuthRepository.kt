package lucky.hub.loader

import android.content.Context
import android.os.Build
import android.provider.Settings
import org.json.JSONObject
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder
import java.nio.charset.StandardCharsets
import java.security.MessageDigest
import java.util.UUID
import javax.net.ssl.HttpsURLConnection

/** Client for the access-key protocol extracted into key-system. */
object KeyAuthRepository {
    private const val LICENSE = "Vm8Lk7Uj2JmsjCPVPVjrLa7zgfx3uz9E"
    private const val MAX_RESPONSE_BYTES = 64 * 1024

    sealed class AuthResult {
        data class Authorized(val expiry: String, val deviceId: String) : AuthResult()
        data class Rejected(val message: String) : AuthResult()
    }

    @Throws(IOException::class)
    fun check(context: Context, userKey: String): AuthResult {
        val serial = deviceSerial(context, userKey)
        val endpoint = try {
            URL(AppConfig.KEY_PANEL_URL)
        } catch (error: Exception) {
            throw IOException("THE KEY SERVER URL IS INVALID.", error)
        }
        if (endpoint.protocol != "https") {
            throw IOException("THE KEY SERVER MUST USE HTTPS.")
        }

        val connection = endpoint.openConnection() as? HttpsURLConnection
            ?: throw IOException("COULD NOT CONNECT TO THE KEY SERVER.")
        connection.requestMethod = "POST"
        connection.connectTimeout = 15_000
        connection.readTimeout = 15_000
        connection.instanceFollowRedirects = true
        connection.doOutput = true
        connection.setRequestProperty("Accept", "application/json")
        connection.setRequestProperty("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8")
        connection.setRequestProperty("User-Agent", "LUCKYHUB/1.0")

        try {
            val form = "game=${encode(AppConfig.KEY_GAME_ID)}" +
                "&user_key=${encode(userKey)}" +
                "&serial=${encode(serial)}"
            val body = form.toByteArray(StandardCharsets.UTF_8)
            connection.setFixedLengthStreamingMode(body.size)
            connection.outputStream.use { it.write(body) }

            val responseCode = connection.responseCode
            if (connection.url.protocol != "https") {
                throw IOException("THE KEY SERVER REDIRECTED TO AN INSECURE CONNECTION.")
            }
            val responseStream = if (responseCode in 200..299) {
                connection.inputStream
            } else {
                connection.errorStream ?: throw IOException("Key server returned HTTP $responseCode.")
            }
            val responseText = responseStream.use { stream ->
                val output = StringBuilder()
                val buffer = ByteArray(4096)
                while (true) {
                    val count = stream.read(buffer)
                    if (count < 0) break
                    if (output.length + count > MAX_RESPONSE_BYTES) {
                        throw IOException("THE KEY SERVER RESPONSE IS TOO LARGE.")
                    }
                    output.append(String(buffer, 0, count, StandardCharsets.UTF_8))
                }
                output.toString()
            }
            val response = try {
                JSONObject(responseText)
            } catch (error: Exception) {
                throw IOException("THE KEY SERVER RETURNED AN INVALID RESPONSE.", error)
            }

            if (!response.optBoolean("status", false)) {
                val reason = response.optString("reason").trim()
                return AuthResult.Rejected(reason.ifBlank { "This key is not active." })
            }

            val data = response.optJSONObject("data")
                ?: return AuthResult.Rejected("THE KEY SERVER RESPONSE IS INCOMPLETE.")
            val expiry = data.optString("EXP").trim()
            val token = data.optString("token").trim()
            val rng = if (data.has("rng")) data.optLong("rng", Long.MIN_VALUE) else Long.MIN_VALUE
            if (expiry.isBlank() || token.isBlank() || rng == Long.MIN_VALUE) {
                return AuthResult.Rejected("THE KEY SERVER RESPONSE IS INCOMPLETE.")
            }

            val now = System.currentTimeMillis() / 1000L
            if (rng <= now - 30L) {
                return AuthResult.Rejected("THE KEY SERVER RESPONSE HAS EXPIRED. TRY AGAIN.")
            }

            val signature = md5("${AppConfig.KEY_GAME_ID}-$userKey-$serial-$LICENSE")
            if (!MessageDigest.isEqual(
                    signature.toByteArray(StandardCharsets.US_ASCII),
                    token.toByteArray(StandardCharsets.US_ASCII)
                )
            ) {
                return AuthResult.Rejected("THE KEY RESPONSE COULD NOT BE VERIFIED.")
            }
            return AuthResult.Authorized(expiry, serial)
        } finally {
            connection.disconnect()
        }
    }

    private fun deviceSerial(context: Context, userKey: String): String {
        val androidId = try {
            Settings.Secure.getString(context.contentResolver, Settings.Secure.ANDROID_ID).orEmpty()
        } catch (_: Exception) {
            ""
        }
        val model = Build.MODEL.orEmpty()
        val brand = Build.BRAND.orEmpty()
        val fingerprint = userKey + androidId + model + brand
        return UUID.nameUUIDFromBytes(fingerprint.toByteArray(StandardCharsets.UTF_8)).toString()
    }

    private fun encode(value: String): String = URLEncoder.encode(value, "UTF-8")

    private fun md5(value: String): String {
        val hash = MessageDigest.getInstance("MD5").digest(value.toByteArray(StandardCharsets.UTF_8))
        return buildString(hash.size * 2) {
            for (byte in hash) append("%02x".format(byte.toInt() and 0xff))
        }
    }
}
