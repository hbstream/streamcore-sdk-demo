# StreamCore SDK Android Demo

简体中文: [README.md](README.md)

This directory contains the Android sample for StreamCore SDK. It covers playback, camera and microphone capture, publishing, GB28181, and license status. The sample uses only public Android APIs and the public `com.hbr.streamcore` SDK surface.

## Prepare the SDK

Download the Android SDK package from the [HBRun Download Center](https://hbrun.com/en/downloads/) and place the AAR at:

```text
app/libs/streamcore-sdk.aar
```

The public source repository does not contain the AAR or a production license. Runnable demo packages are available from the repository [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest).

## Build

Windows:

```powershell
.\gradlew.bat :app:assembleDebug :app:lintDebug --console=plain --no-daemon
```

Linux / macOS:

```bash
./gradlew :app:assembleDebug :app:lintDebug --console=plain --no-daemon
```

With Android Studio, open this directory and wait for Gradle sync to finish.

## Demo License

The demo license is bound to this sample package name. It enables Standard and Professional evaluation features with an `hbrun.com` watermark. Changing the package name invalidates the demo license; production integration requires a commercial license for the target application identity.

## Dependencies

- Android Gradle Plugin, Kotlin/Java, and Android platform libraries.
- The public SDK in `app/libs/streamcore-sdk.aar`.
