# StreamCore SDK iOS Demo

简体中文: [README.md](README.md)

This directory contains the Objective-C sample for iPhone and iPad. It covers playback, capture, publishing, license status, and common display settings. The sample uses only the public `StreamCoreSDK.h` surface and Apple system frameworks.

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
Resources/streamcore_demo_public.pem
StreamCoreDemoLaunchScreen.storyboard
```

The Simulator can be used to inspect the UI, resource loading, and basic API status. Camera, microphone, ReplayKit, and real publishing require a signed iPhone or iPad.

## Demo License

The demo license is bound to the default sample bundle identifier. It enables Standard and Professional evaluation features with an `hbrun.com` watermark. Changing the bundle identifier invalidates the demo license.

## Dependencies

- Apple system frameworks and `Frameworks/StreamCoreSDK.xcframework`.
