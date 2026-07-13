# StreamCore SDK Desktop Demo

简体中文: [README.md](README.md)

This directory contains the Qt 6 Widgets desktop demo for Windows, Linux, and macOS. It covers publishing, playback, ONVIF, GB28181, and license status, and serves as a C/C++ integration reference.

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

The demo license is bound to this sample process. It enables Standard and Professional evaluation features with an `hbrun.com` watermark and cannot be reused by another executable.

## Dependencies

- Qt 6, platform system libraries, and the released StreamCore SDK package.
- The sample integrates through the public StreamCore SDK C API.
