# StreamCore SDK 桌面 Demo

English: [README.en.md](README.en.md)

Publisher 页面支持 WHIP 端点和可选 Bearer Token。选择 WHIP 后，视频与音频编码固定为 H.264 与 Opus；SDK 配置或编码包参数不符合协议要求时，界面日志会原样展示 `-6 / UNSUPPORTED_MEDIA_CODEC`，以及实际编码和要求编码。

本目录提供基于 Qt 6 Widgets 的桌面演示程序，覆盖 Windows、Linux 和 macOS。界面包含推流、拉流播放、ONVIF、GB28181 与授权状态，可作为 C/C++ 集成参考。

## 准备 SDK

从 [HBRun 下载中心](https://hbrun.com/zh-CN/downloads/) 获取目标平台 SDK 包，解压到当前目录下的 `sdk/<platform_arch>/`。例如：

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

公开源码仓库不包含 SDK 二进制和正式授权文件。可运行的桌面 Demo 可从仓库的 [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest) 下载。

## 编译

Windows：

```powershell
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/windows_x86_64
cmake --build build --config Release --target streamcore_demo_qt
```

Linux：

```bash
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/linux_x86_64
cmake --build build --config Release --target streamcore_demo_qt -j
```

macOS：

```bash
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/macos_arm64
cmake --build build --config Release --target streamcore_demo_qt -j
open build/streamcore_demo_qt.app
```

需要 Qt 6 Widgets、CMake 和支持 C++17 的编译器。macOS 的摄像头、麦克风和屏幕录制权限由 Demo 应用向系统申请，请通过生成的 `.app` 启动。

## Demo 授权

Demo 授权仅绑定本示例进程，用于体验 Standard 与 Professional 能力，并显示 `hbrun.com` 水印。授权不能复制到其他可执行程序中使用。

## 依赖

- Qt 6、平台系统库和已发布的 StreamCore SDK 包。
- 示例通过 StreamCore SDK 公开 C API 完成集成。
