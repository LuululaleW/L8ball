# 8BP Native Module

Native ImGui menu for **8 Ball Pool** (`com.miniclip.eightballpool`), built with
NDK, Substrate/And64InlineHook and xhook. The library (`libgdtqis.so`) is meant to
run **inside the game process** (root/Zygisk or a patched APK); the companion APK
only bundles it, shows its status and lets you extract it.

## What changed in this build

- **Login and expiry removed.** No license server, no key input, no countdown.
  The menu opens directly and reports `Lifetime`.
- Removed the key/login HTTP stack and the prebuilt curl/OpenSSL libraries.
- Added the missing Java sources (`MainActivity`, JNI `SurfaceView` bridge) so the
  project actually assembles.
- Fixed Gradle setup: no more `jcenter()`, working repository list, signed
  (debug-key) release build, correct minSdk/ABI comments.
- Fixed NDK flags (`-Werror` + `-w` conflict) and removed unused icon headers
  (~23 MB smaller).
- `get8BPbase()` now times out instead of looping forever, and hooks are skipped
  safely when the target library is not found.

## Layout

```
app/src/main/java/...            companion app + JNI bridge
app/src/main/jni/main.cpp        JNI_OnLoad, hooks, JNI exports
app/src/main/jni/menu.h          ImGui menu + ESP + overlay
app/src/main/jni/game/           reversed game structures and helpers
app/src/main/jni/include/        hook/input/obfuscation utilities + external libs
app/src/main/jni/icons/          embedded PNG headers
```

## Build

Requirements: JDK 17, Android SDK 34, NDK `24.0.8215888`, Gradle 8.5.

```bash
# local.properties must point at your SDK
echo "sdk.dir=/path/to/Android/Sdk" > local.properties
./gradlew assembleDebug
# output: app/build/outputs/apk/debug/app-debug.apk
```

Release builds use the debug signing key so the APK stays installable. Replace
the signing config in `app/build.gradle` before publishing.

The included GitHub Actions workflow (`.github/workflows/build-apk.yml`) builds a
debug APK on every push and uploads it as an artifact.

## Usage

1. Install the APK.
2. Tap **Extract libgdtqis.so**.
3. Place the library in your injector / Zygisk module and scope it to
   `com.miniclip.eightballpool`.
4. Start the game; open the menu from the floating button.

The library targets a specific game build. The inline hook offsets in
`app/src/main/jni/main.cpp` must match the installed `libmain.so` version,
otherwise the game may crash on launch.

## Notes

- Only `arm64-v8a` is built (the hook engine and prebuilt assets are 64-bit).
- `local.properties`, IDE folders and build output are git-ignored.
- Modding the game violates Miniclip's terms of service. Use at your own risk.

## License

See `LICENSE`.
