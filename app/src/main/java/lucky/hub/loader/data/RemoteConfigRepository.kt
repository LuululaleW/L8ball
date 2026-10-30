package lucky.hub.loader.data

import org.json.JSONObject
import java.io.IOException
import java.net.URL
import javax.net.ssl.HttpsURLConnection

object RemoteConfigRepository {
    private const val MAX_CONFIG_BYTES = 16 * 1024

    @Throws(IOException::class)
    fun fetch(configUrl: String): RemoteConfig {
        val url = try {
            URL(configUrl)
        } catch (error: Exception) {
            throw IOException("THE GITHUB CONFIG URL IS INVALID.", error)
        }
        if (url.protocol != "https") {
            throw IOException("THE GITHUB CONFIG MUST USE HTTPS.")
        }

        val connection = url.openConnection() as? HttpsURLConnection
            ?: throw IOException("COULD NOT CONNECT TO THE GITHUB CONFIG.")
        connection.connectTimeout = 8_000
        connection.readTimeout = 8_000
        connection.requestMethod = "GET"
        connection.instanceFollowRedirects = true
        connection.useCaches = false
        connection.setRequestProperty("Accept", "application/json")
        connection.setRequestProperty("Cache-Control", "no-cache")
        connection.setRequestProperty("User-Agent", "LuckyHubLoader/1.0")

        try {
            val responseCode = connection.responseCode
            if (responseCode !in 200..299) {
                throw IOException("GitHub returned HTTP $responseCode.")
            }
            if (connection.contentLength > MAX_CONFIG_BYTES) {
                throw IOException("The config file is too large.")
            }

            val body = connection.inputStream.bufferedReader(Charsets.UTF_8).use { reader ->
                val result = StringBuilder()
                val buffer = CharArray(1024)
                while (true) {
                    val count = reader.read(buffer)
                    if (count < 0) break
                    result.append(buffer, 0, count)
                    if (result.length > MAX_CONFIG_BYTES) {
                        throw IOException("THE CONFIG FILE IS TOO LARGE.")
                    }
                }
                result.toString()
            }

            val json = try {
                JSONObject(body)
            } catch (error: Exception) {
                throw IOException("THE GITHUB CONFIG IS NOT VALID JSON.", error)
            }
            val version = json.optString("version", json.optString("latestVersion")).trim()
            if (version.isEmpty() || version.split('.').any { it.toIntOrNull() == null }) {
                throw IOException("THE GITHUB CONFIG IS MISSING A VALID NUMERIC VERSION.")
            }
            val downloadUrl = json.optString("downloadUrl", json.optString("apkUrl")).trim()
                .takeIf { it.isNotEmpty() }
            if (downloadUrl != null) {
                val apkUrl = try {
                    URL(downloadUrl)
                } catch (error: Exception) {
                    throw IOException("THE LOADER UPDATE LINK IS INVALID.", error)
                }
                if (apkUrl.protocol != "https" || apkUrl.host.isBlank()) {
                    throw IOException("THE LOADER UPDATE LINK MUST USE HTTPS.")
                }
            }
            return RemoteConfig(
                version = version,
                message = json.optString("message", "").trim(),
                downloadUrl = downloadUrl
            )
        } finally {
            connection.disconnect()
        }
    }
}
