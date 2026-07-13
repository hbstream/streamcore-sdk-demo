# StreamCore Demo Qt / Desktop

`streamcore_demo_qt` 是 StreamCore SDK 的桌面端示例，提供 Qt 6 Widgets GUI 和一个命令行快照入口。
该目录可以作为独立 GitHub 仓库发布。

English: [README.en.md](README.en.md)

## SDK 放置方式

Qt / CMake 工程允许同时保存多个平台 SDK 压缩包，并按当前平台解压使用：

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

公开 GitHub 源码仓默认不提交 `sdk-packages/` 和 `sdk/`；交付包可以预先放好一个
或多个压缩包。开发者只需要把当前平台的包解压到 `sdk/<platform_arch>/`，CMake 就会自动识别。

## Windows 编译

```powershell
cd streamcore_demo_qt
cmake -S . -B build-windows -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/windows_x86_64
cmake --build build-windows --config Release --target streamcore_demo_qt streamcore_demo_cli -- /m
```

如果 SDK 已按当前平台解压到 `sdk/windows_x86_64/`，可以省略 `STREAMCORE_DEMO_SDK_PACKAGE_ROOT`。

## Linux 编译

```bash
cd streamcore_demo_qt
cmake -S . -B build-linux -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/linux_x86_64
cmake --build build-linux --config Release --target streamcore_demo_qt streamcore_demo_cli -j
```

Linux 需要系统已安装 Qt 6 Widgets、CMake、C++17 编译器和基础 X11 / Wayland 运行依赖。

Release gate 的 Linux publisher 证明不能只看进程启动或 `publisher.start`。
必须在当前真实桌面会话中运行 GUI，并用外部 `ffprobe` 或另一端 SDK/demo
player 证明媒体已到达。若编码后端依赖 x264/OpenH264 这类动态 provider，
启动 demo 前必须把 provider 目录放入 loader path，否则可能出现 start 成功但
外部观察端没有编码媒体的 `partial` 结果。

## macOS 说明

Qt/macOS C++ demo 需要 macOS native C ABI SDK 包，并解压到 `sdk/macos_x86_64/` 或
`sdk/macos_arm64/`。iOS 的 `XCFramework` / Objective-C 包用于 `streamcore_demo_ios`，不能直接作为本
Qt/C++ demo 的 SDK 包。

macOS 摄像头、麦克风和屏幕录制权限由 demo 应用层触发，SDK 后端只检查权限状态并通过错误码返回；
客户集成时也应由客户 app 负责申请权限。请始终通过 `.app` 启动或调试，不要直接运行
`Contents/MacOS/streamcore_demo_qt`，否则 TCC 可能把它视为不同的授权身份。

默认构建会使用 ad-hoc 签名，适合一次性本地调试，但反复重新编译后 macOS 可能要求重新授权。需要稳定
复测 Camera / Microphone / Screen Recording 时，应使用固定 bundle id 和稳定签名身份：

```bash
cmake -S . -B build-macos \
  -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/macos_x86_64 \
  -DSTREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY="HBR StreamCore Local Development" \
  -DSTREAMCORE_DEMO_APPLE_CODE_SIGN_KEYCHAIN="$HOME/Library/Keychains/login.keychain-db"
cmake --build build-macos --config Release --target streamcore_demo_qt -j
open build-macos/streamcore_demo_qt.app --args --autorun permissions
```

如果没有配置稳定签名证书，系统设置里看到的勾选项可能属于旧构建。遇到这种情况，删除隐私设置中的旧
`streamcore_demo_qt` 项或重置对应 bundle id 的 TCC 记录后，再通过 `.app` 授权一次。

## 接口边界

本 demo 只 include `streamcore/streamcore_sdk.h` 和可选公开 addon 头，不引用
非公开 SDK 实现文件、签名密钥、客户授权生成文件或本机私有路径。
