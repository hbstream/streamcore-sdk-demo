# StreamCore SDK iOS Demo

简体中文: [README.md](README.md)

This directory contains the Objective-C sample for iPhone and iPad. It covers playback, capture, publishing, license status, and common display settings. The sample uses only the public `StreamCoreSDK.h` surface and Apple system frameworks.

## WHEP Playback (1.6.0)

The Player page explicitly selects Media URL or WHEP. WHEP mode provides a masked Bearer token, an optional numeric local bind address, and an HTTP switch for isolated tests. HTTPS remains the default and Professional `p2_whep_player` is required. A failed `configureWhepOptions:` call prevents preflight/start, and displayed endpoints omit userinfo, query, and fragment. Use `HBRStreamCorePlayerWhepOptions` for custom CA, ICE/TURN, relay-only, and other advanced settings.

## Prepare the SDK

Download the iOS SDK from the [HBRun Download Center](https://hbrun.com/en/downloads/) and place the XCFramework at:

```text
Frameworks/StreamCoreSDK.xcframework
```

The public source repository does not contain the XCFramework or a production license. iOS apps must be signed in Xcode before installation on a physical device.

## Generate the Xcode Project

Run on macOS:

```bash
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS
open build-ios/streamcore_demo_ios.xcodeproj
```

The default bundle identifier is `com.hbr.streamcoredemo`. Select a development team and target device in Xcode, and ensure these resources belong to the app target:

```text
Resources/streamcore_demo.lic
StreamCoreDemoLaunchScreen.storyboard
```

The Simulator can be used to inspect the UI, resource loading, and basic API status. Camera, microphone, ReplayKit, and real publishing require a signed iPhone or iPad.

## Demo License

`Resources/streamcore_demo.lic` is the encrypted `SC-LIC-ENC-v1` demo license shipped
with the sample. The app resolves its read-only path from `NSBundle.mainBundle` and
submits it through the public runtime configuration, so no registration string needs to
be entered. It is bound only to the default demo bundle identifier and does not require
the customer's production Team ID.

Changing the bundle identifier invalidates the demo license. Production integration
requires a commercial license for the Bundle ID and Team ID together with the production
XCFramework. Demo and production licenses are not interchangeable.

## Dependencies

- Apple system frameworks and `Frameworks/StreamCoreSDK.xcframework`.
