# StreamCore SDK Demo Android

`streamcore_demo_android` is the standalone Android demo for StreamCore SDK and
can be published as an independent GitHub repository.

中文: [README.md](README.md)

## 2026-05-12 Release Gate Status

- The demo consumes only the public Android AAR facade.
- The current gate refresh rebuilt the demo from the refreshed current SDK AAR,
  passed Debug/Release/lint, installed on the device, and proved camera preview,
  publisher preview, RTMP player, and RTMP publisher readback.
- Android GB28181 video-media proof is closed for the current SRS camera and
  video-file routes. GB28181 audio is not claimed because the SRS profile used
  in this batch negotiated video-only SDP.
- Speaker-output proof for Android player remains a separate proof item in the
  root function matrix.

## SDK Placement

The Android project follows the normal Gradle / Android Studio local AAR
convention:

```text
streamcore_demo_android/
  app/libs/streamcore-sdk.aar
```

You may also set `STREAMCORE_DEMO_SDK_AAR` to an absolute AAR path. Public demo
repositories should not commit `app/libs/*.aar`; delivery packages may
prebundle the AAR so the project opens and builds directly.

## Build

```powershell
cd streamcore_demo_android
.\gradlew.bat :app:assembleDebug :app:lintDebug --console=plain --no-daemon
```

Linux / macOS:

```bash
cd streamcore_demo_android
./gradlew :app:assembleDebug :app:lintDebug --console=plain --no-daemon
```

## Public Boundary

The app may use only the public `com.hbr.streamcore` SDK facade and normal
Android framework APIs. It must not reference non-public SDK implementation
files, signing secrets, generated license files, or machine-local paths.
