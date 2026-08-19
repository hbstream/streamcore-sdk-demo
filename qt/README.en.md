# StreamCore SDK Desktop Demo

简体中文: [README.md](README.md)

This directory contains the Qt 6 Widgets desktop demo for Windows, Linux, and macOS. It covers publishing, playback, ONVIF, GB28181, and license status, and serves as a C/C++ integration reference.

## WHEP Playback (1.6.0)

The Player page explicitly selects Media URL or WHEP. WHEP mode provides a masked Bearer token, an optional numeric local bind address, and an HTTP switch limited to isolated tests. HTTPS is the default and Professional `p2_whep_player` is required. Option validation failure prevents config, preflight, and start; UI diagnostics are fixed and displayed endpoints omit userinfo, query, and fragment. Use the public C API for custom CA, ICE/TURN, relay-only, and other advanced settings.

The Publisher page supports WHIP endpoints and an optional Bearer Token. Selecting WHIP locks the video and audio codecs to H.264 and Opus. The SDK still rejects incompatible configuration or encoded-packet parameters with `-6 / UNSUPPORTED_MEDIA_CODEC` and a detailed actual/required codec message.

The Publisher page also includes an asynchronous Capture Processor example. When before/after comparison is enabled, the demo applies a monochrome `REPLACE` path and shows the original and final published frames side by side. Processing finishes on a worker thread after the SDK callback returns, illustrating integration with an asynchronous application algorithm.

## Prepare the SDK

Download the target platform package from the [HBRun Download Center](https://hbrun.com/en/downloads/) and extract it to `sdk/<platform_arch>/` under this directory. For example:

```text
sdk/
  windows_x86_64/
    include/streamcore/streamcore_sdk.h
    lib/streamcore_sdk.lib
    bin/streamcore_sdk.dll
  linux_x86_64/
    include/streamcore/streamcore_sdk.h
    lib/libstreamcore_sdk.so
```

The public source repository does not include SDK binaries or a production license. Runnable desktop demos are available from the repository [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest).

## Build

Windows:

```powershell
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/windows_x86_64
cmake --build build --config Release --target streamcore_demo_qt
```

Linux:

```bash
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/linux_x86_64
cmake --build build --config Release --target streamcore_demo_qt -j
```

macOS:

```bash
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/macos_arm64
cmake --build build --config Release --target streamcore_demo_qt -j
open build/streamcore_demo_qt.app
```

Qt 6 Widgets, CMake, and a C++17 compiler are required. On macOS, the demo app requests camera, microphone, and screen-recording permissions from the operating system; launch the generated `.app` bundle.

## Demo License

Runnable packages place the matching encrypted `SC-LIC-ENC-v1` demo license under
`license/demo/` next to the application. The demo submits that file through the public
runtime configuration at startup, so no registration string needs to be entered. Keep
the default CMake deployment step when building from source. A production
`streamcore_sdk` customer license and a demo license are not interchangeable.

The demo license is bound to this sample executable, or to the `.app` bundle identifier
on macOS. It enables Standard and Professional evaluation features with an `hbrun.com`
watermark and cannot be reused by another application or platform.

## Dependencies

- Qt 6, platform system libraries, and the released StreamCore SDK package.
- The sample integrates through the public StreamCore SDK C API.
