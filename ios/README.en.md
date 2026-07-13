# StreamCore SDK Demo iOS

`streamcore_demo_ios` is the Apple/iOS demo for StreamCore SDK. It is designed
as a customer-readable and reusable integration sample.

中文: [README.md](README.md)

## SDK Placement

The iOS / Xcode project follows the common local `Frameworks` directory layout:

```text
streamcore_demo_ios/
  Frameworks/StreamCoreSDK.xcframework
```

Public demo repositories should not commit `Frameworks/StreamCoreSDK.xcframework`.
Delivery packages may prebundle it so users can add it directly to Xcode
`Frameworks, Libraries, and Embedded Content`.

The formal customer package contains the iOS device slice. iOS Simulator builds
are for validation only and are not part of the formal customer release package.

## Build Options

### Xcode App Target

Create an iOS app target with bundle id `com.hbr.streamcoredemo`, add the source
files in this directory, and embed `Frameworks/StreamCoreSDK.xcframework`.

The following files must be copied into the app bundle:

```text
Resources/streamcore_demo.lic
Resources/streamcore_demo_public.pem
StreamCoreDemoLaunchScreen.storyboard
```

### CMake / Xcode Generator

The root `streamcore_demo/CMakeLists.txt` and the local
`streamcore_demo_ios/CMakeLists.txt` are both package-first entrypoints. They
consume only `Frameworks/StreamCoreSDK.xcframework` (or an explicit
`STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT` override) and do not depend on the sibling
`streamcore_sdk` source tree.

Use the Xcode generator and an iOS sysroot on the Mac host. The maintained SDK
packaging flow refreshes `Frameworks/StreamCoreSDK.xcframework` automatically,
so demo builds and validation stay on packaged artifacts.

## Validation

Simulator validation can prove app startup, bundle resource loading, license
status, and basic UI wiring. The UIKit controller exposes serial controls for
player, capture, and publisher sessions: preflight, start, and stop. The encoded
publisher path also has a sample packet push button.

Internal automation can trigger the three session preflights with:

```bash
SIMCTL_CHILD_STREAMCORE_DEMO_IOS_AUTORUN=preflight \
  xcrun simctl launch booted com.hbr.streamcoredemo
```

The autorun path only executes configure/preflight and writes the result to the
visible status area and `streamcore_demo.log`. Camera, microphone, ReplayKit,
real start/stop, and real publishing must still be verified on a signed physical
iOS device.

## Public Boundary

The sample may use only the public `StreamCoreSDK.h` Objective-C wrapper and
ordinary Apple frameworks. It must not reference non-public SDK implementation
files, signing secrets, generated license files, or machine-local paths.
