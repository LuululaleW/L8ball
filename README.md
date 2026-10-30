# Amash Engine-8BP

AndroidIDE-ready Kotlin project for an access-key gated launcher for the official 8 Ball Pool app.

## App details

- App name: Amash Engine-8BP
- Android application ID: `LUCKY.HUB.LOADER`
- Kotlin namespace: `lucky.hub.loader`
- Main game: 8 Ball Pool
- Minimum Android version: Android 5.0 Lollipop (API 21)
- Key panel: `https://venomkey.com/connect`

## Startup flow

1. `SplashActivity` plays the bundled banner video behind the loading progress before key login.
2. `KeyLoginActivity` accepts or pastes a license key and verifies it with the key panel.
3. On success, `MainActivity` shows the Games, Resellers, Profile, and Settings tabs. The 8 Ball Pool card launches the installed game; if it is not installed, the card downloads the APK from `GAME_DOWNLOAD_URL` and shows progress. Android's package installer asks for confirmation, then launches the game after installation succeeds. The game runs as its own Android app. Android 8 and newer may first ask you to allow Lucky Hub to install apps in system settings.

Set `GAME_DOWNLOAD_URL` in `app/src/main/java/lucky/hub/loader/AppConfig.kt` to an authorized direct HTTPS APK link. It is blank by default, and downloads show a configuration error until a link is provided.

The banner videos, still-image fallback, game artwork, and app icon are stored in `app/src/main/res`. The splash and Games screens share one bundled video, play it silently on a loop, pause when the screen is not active, and show the still banner if a device cannot decode a video. The Settings tab also lets users refresh the GitHub version notice.

The game is installed as the separate Android package `com.miniclip.eightballpool`; this loader opens it from the Games card. A regular Android app cannot hide another package’s launcher icon, so users can remove a home-screen shortcut manually, but the game can still appear in the app drawer. Hiding it system-wide requires device-owner or system privileges.

## Key panel settings

`app/src/main/java/lucky/hub/loader/AppConfig.kt` contains `KEY_PANEL_URL` and `KEY_GAME_ID`. The game identifier is kept as `PUBG` because it is part of the supplied `key-system` request and response-signature protocol.

The client posts the entered key and device serial, then checks the response status, timestamp, expiry, and MD5 token using the supplied protocol. The Android platform's normal HTTPS certificate and hostname checks stay enabled. Only a masked key, expiry, and device serial are stored locally; the raw key is not saved. The key panel and its key database must be available for activation to succeed.

The supplied protocol includes a shared signing value in the Android client. That preserves compatibility with the provided panel, but any value embedded in an APK can be recovered by someone who decompiles it. For production licensing, signature validation should ultimately be enforced by the panel itself.

## GitHub update notice

1. The configured `GITHUB_CONFIG_URL` points to `config.json` attached to the `LUCKYHUB` GitHub release. Upload the updated JSON as that release asset.
2. If you prefer a file on the main branch, change `GITHUB_CONFIG_URL` to that file's raw GitHub URL.
3. Update the version, message, and loader APK `downloadUrl` for each loader release.

Example:

    {
      "version": "1.0.2",
      "message": "Welcome to Amash Engine-8BP.",
      "downloadUrl": "https://github.com/OWNER/REPOSITORY/releases/download/TAG/LuckyHubLoader.apk"
    }

The `downloadUrl` must point to the signed Lucky Hub loader APK for that version. Set `versionName` in `app/build.gradle` to the same version, increase `versionCode`, and sign the APK with the same key used for the installed app. After the key login succeeds, a newer loader version opens a notice with **Later** and **Update** actions. **Update** downloads the loader APK, checks that its package name and version match the app and config, and opens Android's installer. This is separate from `GAME_DOWNLOAD_URL`, which remains the 8 Ball Pool game download. If `downloadUrl` is missing, the version notice still works and the app explains that the loader APK link must be added.

## Open in AndroidIDE

Open the `LUCKY_HUB_LOADER` directory as a Gradle project. It targets SDK 34 and uses JDK 17, AGP 8.2.2, Gradle 8.2.1, and Kotlin 2.0.0. AndroidIDE can create `local.properties` for its SDK path.
