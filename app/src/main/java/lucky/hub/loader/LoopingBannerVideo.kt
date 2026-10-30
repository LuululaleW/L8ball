package lucky.hub.loader

import android.content.Context
import android.media.MediaPlayer
import android.net.Uri
import android.view.View
import android.widget.VideoView

/** Plays a bundled banner silently and keeps its still image visible if decoding fails. */
class LoopingBannerVideo(
    context: Context,
    private val videoView: VideoView,
    private val fallbackView: View,
    videoResourceId: Int
) {
    private var activityResumed = false
    private var pageActive = true
    private var prepared = false
    private var failed = false

    init {
        fallbackView.visibility = View.VISIBLE
        videoView.setOnPreparedListener { player: MediaPlayer ->
            player.isLooping = true
            player.setVolume(0f, 0f)
            prepared = true
            updatePlayback()
        }
        videoView.setOnInfoListener { _, what, _ ->
            if (what == MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START) {
                fallbackView.visibility = View.GONE
            }
            false
        }
        videoView.setOnErrorListener { _, _, _ ->
            failed = true
            prepared = false
            videoView.visibility = View.GONE
            fallbackView.visibility = View.VISIBLE
            true
        }

        val uri = Uri.parse("android.resource://${context.packageName}/$videoResourceId")
        videoView.setVideoURI(uri)
    }

    fun onResume() {
        activityResumed = true
        updatePlayback()
    }

    fun onPause() {
        activityResumed = false
        pauseIfPlaying()
    }

    fun setPageActive(active: Boolean) {
        pageActive = active
        updatePlayback()
    }

    fun release() {
        activityResumed = false
        pageActive = false
        prepared = false
        videoView.stopPlayback()
    }

    private fun updatePlayback() {
        if (failed || !prepared) return
        if (activityResumed && pageActive) {
            videoView.start()
        } else {
            pauseIfPlaying()
        }
    }

    private fun pauseIfPlaying() {
        if (videoView.isPlaying) videoView.pause()
    }
}
