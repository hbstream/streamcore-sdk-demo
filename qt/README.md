# StreamCore SDK 桌面 Demo

English: [README.en.md](README.en.md)

Publisher 页面支持 WHIP 端点和可选 Bearer Token。选择 WHIP 后，视频与音频编码固定为 H.264 与 Opus；SDK 配置或编码包参数不符合协议要求时，界面日志会原样展示 `-6 / UNSUPPORTED_MEDIA_CODEC`，以及实际编码和要求编码。

本目录提供基于 Qt 6 Widgets 的桌面演示程序，覆盖 Windows、Linux 和 macOS。界面包含推流、拉流播放、ONVIF、GB28181 与授权状态，可作为 C/C++ 集成参考。

## WHEP 播放（1.6.0）

Player 页通过来源下拉框显式选择媒体 URL 或 WHEP。WHEP 模式提供遮蔽 Bearer Token、可选 numeric 本地绑定 IP 和“仅隔离测试允许 HTTP”开关；默认使用 HTTPS，并要求 Professional `p2_whep_player`。选项校验失败后不会继续配置、预检或启动，界面只显示固定错误摘要，展示 endpoint 时会移除 userinfo、query 和 fragment。custom CA、ICE/TURN、relay-only 等高级配置继续使用公开 C API。

Publisher 页面还提供异步 Capture Processor 示例：勾选处理前后对比后，Demo 会以黑白处理演示 `REPLACE` 路径，并同时显示原始画面与最终推流画面。示例回调返回后由工作线程完成处理，用于说明上层异步算法的接入方式。

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

可运行包已经在程序目录下的 `license/demo/` 放置匹配当前平台和入口的
`SC-LIC-ENC-v1` 加密 Demo 授权；程序启动时通过公开 runtime 配置提交该文件路径，
不需要用户填写注册码。自行编译时保留 CMake 默认部署步骤即可，不能把正式
`streamcore_sdk` 客户授权与 Demo 授权互换。

Demo 授权仅绑定本示例进程（macOS 绑定 `.app` 的 Bundle ID），用于体验 Standard
与 Professional 能力并显示 `hbrun.com` 水印。Windows、Linux 或 macOS 的 Demo
授权也不能复制到其他平台或其他应用中使用。

## 依赖

- Qt 6、平台系统库和已发布的 StreamCore SDK 包。
- 示例通过 StreamCore SDK 公开 C API 完成集成。
