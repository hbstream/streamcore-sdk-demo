# StreamCore Demo Qt / Desktop

`streamcore_demo_qt` is the desktop StreamCore SDK demo. It provides a Qt 6
Widgets GUI and a small CLI snapshot entry. This directory can be published as
an independent GitHub repository.

中文: [README.md](README.md)

## SDK Placement

The Qt / CMake project may keep SDK archives for multiple platforms and extract
only the package needed by the current build:

```text
streamcore_demo_qt/
  sdk-packages/
    streamcore_sdk-1.3.3-windows_x86_64.zip
    streamcore_sdk-1.3.3-linux_x86_64.tar.gz
    streamcore_sdk-1.3.3-macos_arm64.tar.gz
  sdk/
    windows_x86_64/
      include/streamcore/streamcore_sdk.h
      lib/streamcore_sdk.lib
      bin/streamcore_sdk.dll
    linux_x86_64/
      include/streamcore/streamcore_sdk.h
      lib/libstreamcore_sdk.so
```

Public demo repositories should not commit `sdk-packages/` or `sdk/`. Internal
test packages and customer delivery packages may prebundle one or more archives.
Developers only need to extract the current platform package to
`sdk/<platform_arch>/`; CMake can then find it automatically.

## Windows Build

```powershell
cd streamcore_demo_qt
cmake -S . -B build-windows -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/windows_x86_64
cmake --build build-windows --config Release --target streamcore_demo_qt streamcore_demo_cli -- /m
```

If the SDK package has already been extracted to `sdk/windows_x86_64/`, the
`STREAMCORE_DEMO_SDK_PACKAGE_ROOT` argument can be omitted.

## ONVIF Automation

The Player page can run ONVIF discovery in automation mode:

```powershell
$env:STREAMCORE_DEMO_QT_AUTORUN = "onvif"
$env:STREAMCORE_DEMO_QT_ONVIF_BIND_IP = "192.0.2.10"
$env:STREAMCORE_DEMO_QT_ONVIF_PROBE_ENDPOINT = "239.255.255.250:3702"
.\streamcore_demo_qt.exe
```

Set `STREAMCORE_DEMO_QT_ONVIF_BIND_IP` when the host has multiple network
adapters, otherwise the operating system may send multicast probes through a
different interface.

## Linux Build

```bash
cd streamcore_demo_qt
cmake -S . -B build-linux -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/linux_x86_64
cmake --build build-linux --config Release --target streamcore_demo_qt streamcore_demo_cli -j
```

Linux requires Qt 6 Widgets, CMake, a C++17 compiler, and the normal X11 /
Wayland desktop runtime dependencies.

For a release-gate publisher proof, do not stop at a successful process start.
Run the GUI in the current real desktop session and prove the stream with an
external observer such as `ffprobe` or another SDK/demo player. If the selected
encoder backend needs dynamic providers such as x264 or OpenH264, put those
provider directories on the loader path before starting the demo; otherwise
`publisher.start` can pass while the observer receives no encoded media.

## macOS Note

The Qt/macOS C++ demo requires a macOS native C ABI SDK package extracted to
`sdk/macos_x86_64/` or `sdk/macos_arm64/`. The iOS `XCFramework` / Objective-C
package is used by `streamcore_demo_ios` and is not the package format consumed
by this Qt/C++ demo.

Camera, Microphone, and Screen Recording permission prompts are owned by the
demo app layer. The SDK capture backend only checks the permission state and
returns an error through the normal SDK error surface. Customer integrations
should follow the same rule: request permissions from the customer app before
opening SDK capture sessions. Always launch or debug through the `.app` bundle;
running `Contents/MacOS/streamcore_demo_qt` directly may be treated by macOS TCC
as a different authorization identity.

The default build uses ad-hoc signing, which is fine for one-off local testing
but may require granting Camera / Microphone / Screen Recording permission again
after rebuilds. Use a fixed bundle id and a stable signing identity for
repeatable capture tests:

```bash
cmake -S . -B build-macos \
  -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/macos_x86_64 \
  -DSTREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY="HBR StreamCore Local Development" \
  -DSTREAMCORE_DEMO_APPLE_CODE_SIGN_KEYCHAIN="$HOME/Library/Keychains/login.keychain-db"
cmake --build build-macos --config Release --target streamcore_demo_qt -j
open build-macos/streamcore_demo_qt.app --args --autorun permissions
```

If no stable signing certificate is configured, an item checked in System
Settings may belong to an older build. Remove stale `streamcore_demo_qt` entries
from Privacy settings or reset the TCC record for the bundle id, then authorize
the app bundle again.

## Public Boundary

The demo may include only `streamcore/streamcore_sdk.h` and optional public addon
headers. It must not reference non-public SDK implementation files, signing
secrets, generated license files, or machine-local paths.
