# StreamCore SDK Desktop Demo

简体中文: [README.md](README.md)

This directory contains the Qt 6 Widgets desktop demo for Windows, Linux, and macOS. It covers publishing, playback, ONVIF, GB28181, and license status, and serves as a C/C++ integration reference.

## WHEP Playback (1.6.0)

The Player page explicitly selects Media URL or WHEP. WHEP mode provides a masked Bearer token, an optional numeric local bind address, and an HTTP switch limited to isolated tests. HTTPS is the default and Professional `p2_whep_player` is required. Option validation failure prevents config, preflight, and start; UI diagnostics are fixed and displayed endpoints omit userinfo, query, and fragment. Use the public C API for custom CA, ICE/TURN, relay-only, and other advanced settings.

The Publisher page supports WHIP endpoints and an optional Bearer Token. Selecting WHIP locks the video and audio codecs to H.264 and Opus. The SDK still rejects incompatible configuration or encoded-packet parameters with `-6 / UNSUPPORTED_MEDIA_CODEC` and a detailed actual/required codec message.

The Qt demo includes a separate AI Effects Integration Demo tab without changing the Publisher page. Compact input and effect controls sit above wide original and processed previews. Input can be a local video or camera, and an installed effect pack can provide sample videos. Still images are not video inputs on this tab; a face-swap reference may still be an image. The Qt app receives frames through the StreamCore SDK Capture Processor, finishes with `REPLACE` on a worker thread, and can publish the processed video to RTMP. The tab reports input, replaced, passthrough, error, and overrun counts. The SDK does not bundle fixed AI models; without an effect pack, the built-in monochrome processor still demonstrates the media path.

### Processor module integration

1. This repository contains only the SDK Processor host, binary frame worker, and dependency-free monochrome baseline. Beauty, face swap, body-shape effects, models, and samples live in a separate optional module pack. Building the Qt source needs no Python, OpenCV, or model installation. Without the pack, local files and cameras still exercise the monochrome Processor path.
2. Without an effect pack, the page shows how to request one. Extract the Windows add-on so `processor_pack/` and `runtime/python/` sit beside the Qt executable, then restart. The page reads `processor_pack/manifest.json` to list effects, samples, reference images, and strength. For development, `STREAMCORE_DEMO_QT_PROCESSOR_PACK` can point to a manifest. Every effect uses the same SDK Capture Processor and optional Publisher path.
3. For your own algorithm, use [`processors/example_invert.py`](processors/example_invert.py) as a template. Implement `create_processor()` returning a callable from `uint8` `H×W×4` BGRA to the same shape and type. Load models once in the factory. Python/NumPy/OpenCV can be supplied at runtime by the optional pack or through `STREAMCORE_DEMO_QT_PROCESSOR_PYTHON`; they are not Qt build dependencies.
4. Publishing is optional. When publishing a video file or included sample with an audio track, that track is used automatically. Camera publishing may include the default microphone via a checkbox. The single `MEDIA_FILE` Capture cannot mix in a separate microphone, so the checkbox is disabled for file input. The output uses H.264/AAC; preview-only sessions do not capture audio. The demo probes the Python module before starting. Failed or late external-worker frames use the page's SDK drop policy and appear in the counters; the built-in monochrome path uses passthrough on failure. Autorun uses `STREAMCORE_DEMO_QT_AUTORUN=processor`; effect, source, file, camera ID, and publish URL can be set through the corresponding `STREAMCORE_DEMO_QT_PROCESSOR_*` environment variables. `...MICROPHONE=1` selects the default microphone for camera publishing.

Source builds always compile this tab and copy the generic worker without an extra build option. `STREAMCORE_DEMO_QT_PROCESSOR_WORKER` selects a custom worker. The default protocol is little-endian `SCF1,width,height,bytes` plus BGRA bytes, with `SCO1,bytes` plus same-size BGRA bytes in reply. Each frame is limited to 16 MiB. `STREAMCORE_DEMO_QT_PROCESSOR_DEADLINE_MS` controls the SDK deadline (30000 ms by default so slower hardware can finish a frame); effective frame rate follows algorithm throughput.

The integration has four focused pieces: `src/processor_demo_page.*` owns the UI and SDK Capture/Publisher sessions; `src/processor_frame_adapter.*` owns the SDK callback, frame conversion/replacement, serial queue, and finish responsibility; `src/processor_worker_bridge.*` is a reusable Python worker utility independent of the SDK and UI; `src/processor_effect_catalog.*` reads and validates the optional pack manifest. Each worker receives its own `QProcessEnvironment`, so switching effects cannot carry an old module or reference image into the next session. Applications can keep the SDK Processor contract and replace the worker with native C++/GPU code, or reuse the bridge for another runtime. Source builds and the default demo require no effect pack.

The frozen 1.6.2 Demo SDK has overlapping internal and public pixel-format values for decoded file RGB32 frames. The narrow CPU-view workaround is compiled only when this older package is selected. The Windows `1.6.3-demo.1` SDK fixes the format mapping at its source boundary, so its Qt build uses only the standard CPU export/copy API. The 1.6.2 package and checksums remain unchanged.

## Prepare the SDK

For the new Windows build, obtain the matching Demo-only `streamcore_sdk_1.6.3-demo.1_windows_x86_64.zip` SDK package. Linux and macOS continue to use the frozen packages from the [Demo 1.6.2 Release](https://github.com/hbstream/streamcore-sdk-demo/releases/tag/v1.6.2). Verify the SHA-256 from the same release and extract the package to `sdk/<platform_arch>/` here. CMake checks the manifest version, Demo-only flag, and product identity; it rejects production SDK packages and mismatched Demo licenses. For example:

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

The public source repository does not include SDK binaries or production licenses. It contains only encrypted Demo licenses bound to the fixed sample identities. The new Windows package uses separate signing and encryption material for the internal `streamcore_sdk/demo` edition; legacy platform packages retain their 1.6.2 identity. Runnable demos and matching Demo SDK packages are available from the repository [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest).

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

macOS (the current package targets Intel x86_64):

```bash
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/macos_x86_64
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
