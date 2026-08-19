# StreamCore SDK Android Demo

简体中文: [README.md](README.md)

This directory contains the Android sample for StreamCore SDK. It covers playback, camera and microphone capture, publishing, GB28181, and license status. The sample uses only public Android APIs and the public `com.hbr.streamcore` SDK surface.

## WHEP Playback (1.6.0)

The Player page explicitly selects Media URL or WHEP; it never guesses the protocol from endpoint text. WHEP mode exposes a masked Bearer token, an optional numeric local bind address, and an explicit HTTP switch for localhost or isolated tests. HTTPS remains the default. WHEP requires the Professional `p2_whep_player` feature. Invalid options fail before networking, and UI/status text never echoes the token, endpoint query, or bind address. Configure custom CA, ICE/TURN, relay-only, and other advanced settings through the public `StreamCorePlayer.WhepOptions` API.

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

The runnable APK embeds the encrypted `SC-LIC-ENC-v1` demo license at
`app/src/main/assets/streamcore_demo.lic`. During initialization it copies the asset into
the app-private `filesDir` and submits that private path through the public runtime API.
Users do not need to enter a registration string, and the license is never written to
shared storage. Keep the asset and default package name when building the sample.

The demo license is bound only to this sample package name and does not require the
customer's production signing certificate. It enables Standard and Professional
evaluation features with an `hbrun.com` watermark. Production integration instead
requires a commercial license for the application ID and Release signing-certificate
SHA-256, and cannot use the demo AAR or demo license.

## Dependencies

- Android Gradle Plugin, Kotlin/Java, and Android platform libraries.
- The public SDK in `app/libs/streamcore-sdk.aar`.
