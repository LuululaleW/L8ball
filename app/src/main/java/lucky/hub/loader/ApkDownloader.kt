package lucky.hub.loader

import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.InterruptedIOException
import java.net.HttpURLConnection
import java.net.URL
import javax.net.ssl.HttpsURLConnection

/** Downloads the configured APK over HTTPS without handing its URL to a browser. */
object ApkDownloader {
    private const val MAX_APK_BYTES = 1_500_000_000L
    private const val MAX_REDIRECTS = 5
    private val REDIRECT_CODES = setOf(
        HttpURLConnection.HTTP_MOVED_PERM,
        HttpURLConnection.HTTP_MOVED_TEMP,
        HttpURLConnection.HTTP_SEE_OTHER,
        307,
        308
    )

    @Throws(IOException::class)
    fun download(
        downloadUrl: String,
        destination: File,
        onProgress: (downloadedBytes: Long, totalBytes: Long) -> Unit
    ) {
        if (downloadUrl.isBlank()) {
            throw IOException("THE APK DOWNLOAD LINK HAS NOT BEEN CONFIGURED.")
        }
        var currentUrl = try {
            URL(downloadUrl)
        } catch (error: Exception) {
            throw IOException("THE APK DOWNLOAD LINK IS INVALID.", error)
        }
        if (currentUrl.protocol != "https") {
            throw IOException("THE APK DOWNLOAD LINK MUST USE HTTPS.")
        }

        val parent = destination.parentFile ?: throw IOException("APK DOWNLOAD LOCATION IS UNAVAILABLE.")
        if (!parent.exists() && !parent.mkdirs()) {
            throw IOException("COULD NOT CREATE THE APK DOWNLOAD LOCATION.")
        }
        val temporaryFile = File(parent, destination.name + ".part")
        temporaryFile.delete()

        try {
            var redirectCount = 0
            while (true) {
                val connection = currentUrl.openConnection() as? HttpsURLConnection
                    ?: throw IOException("THE APK DOWNLOAD MUST STAY ON HTTPS.")
                connection.connectTimeout = 15_000
                connection.readTimeout = 30_000
                connection.requestMethod = "GET"
                connection.instanceFollowRedirects = false
                connection.setRequestProperty("User-Agent", "LuckyHubLoader/1.0")
                connection.setRequestProperty("Accept", "application/vnd.android.package-archive")

                try {
                    val responseCode = connection.responseCode
                    if (responseCode in REDIRECT_CODES) {
                        if (redirectCount++ >= MAX_REDIRECTS) {
                            throw IOException("THE APK DOWNLOAD REDIRECTED TOO MANY TIMES.")
                        }
                        val location = connection.getHeaderField("Location")
                            ?: throw IOException("THE APK DOWNLOAD REDIRECT IS INVALID.")
                        currentUrl = URL(currentUrl, location)
                        if (currentUrl.protocol != "https") {
                            throw IOException("THE APK DOWNLOAD REDIRECT MUST USE HTTPS.")
                        }
                        continue
                    }
                    if (responseCode !in 200..299) {
                        throw IOException("APK DOWNLOAD FAILED (HTTP $responseCode).")
                    }

                    val totalBytes = connection.getHeaderField("Content-Length")
                        ?.toLongOrNull() ?: -1L
                    if (totalBytes > MAX_APK_BYTES) {
                        throw IOException("THE APK IS TOO LARGE TO DOWNLOAD.")
                    }

                    var downloadedBytes = 0L
                    onProgress(0L, totalBytes)
                    connection.inputStream.use { input ->
                        FileOutputStream(temporaryFile).use { output ->
                            val buffer = ByteArray(64 * 1024)
                            while (true) {
                                if (Thread.currentThread().isInterrupted) {
                                    throw InterruptedIOException("APK DOWNLOAD WAS CANCELLED.")
                                }
                                val count = input.read(buffer)
                                if (count < 0) break
                                downloadedBytes += count
                                if (downloadedBytes > MAX_APK_BYTES) {
                                    throw IOException("THE APK IS TOO LARGE TO DOWNLOAD.")
                                }
                                output.write(buffer, 0, count)
                                onProgress(downloadedBytes, totalBytes)
                            }
                            output.fd.sync()
                        }
                    }
                    if (downloadedBytes == 0L) {
                        throw IOException("THE APK DOWNLOAD WAS EMPTY.")
                    }
                    if (totalBytes >= 0 && downloadedBytes != totalBytes) {
                        throw IOException("THE APK DOWNLOAD DID NOT FINISH.")
                    }

                    if (destination.exists() && !destination.delete()) {
                        throw IOException("COULD NOT SAVE THE DOWNLOADED APK.")
                    }
                    if (!temporaryFile.renameTo(destination)) {
                        throw IOException("COULD NOT SAVE THE DOWNLOADED APK.")
                    }
                    onProgress(downloadedBytes, if (totalBytes > 0) totalBytes else downloadedBytes)
                    return
                } finally {
                    connection.disconnect()
                }
            }
        } catch (error: Exception) {
            temporaryFile.delete()
            if (error is IOException) throw error
            throw IOException("COULD NOT DOWNLOAD THE APK.", error)
        }
    }
}
