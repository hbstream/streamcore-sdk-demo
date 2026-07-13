# StreamCore SDK iOS Demo

English: [README.en.md](README.en.md)

本目录提供 iPhone / iPad Objective-C 示例，覆盖播放、采集、推流、授权状态与常用显示设置。示例只调用 `StreamCoreSDK.h` 公开接口和 Apple 系统框架。

## 准备 SDK

从 [HBRun 下载中心](https://hbrun.com/zh-CN/downloads/) 获取 iOS SDK，将 XCFramework 放入：

```text
Frameworks/StreamCoreSDK.xcframework
```

公开源码仓库不包含 XCFramework 和正式授权文件。iOS 应用需要在 Xcode 中完成签名后安装到真机；源码可直接用于创建或集成 App Target。

## 生成 Xcode 工程

在 macOS 上执行：

```bash
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS
open build-ios/streamcore_demo_ios.xcodeproj
```

默认 Bundle ID 为 `com.hbr.streamcoredemo`。请在 Xcode 中选择开发团队和目标设备，并确认以下资源已加入 App Target：

```text
Resources/streamcore_demo.lic
Resources/streamcore_demo_public.pem
StreamCoreDemoLaunchScreen.storyboard
```

模拟器可用于查看界面、资源加载和基础接口状态；摄像头、麦克风、ReplayKit 与真实推流需要使用已签名的 iPhone 或 iPad 验证。

## Demo 授权

Demo 授权仅绑定默认示例 Bundle ID，用于体验 Standard 与 Professional 能力，并显示 `hbrun.com` 水印。更改 Bundle ID 后不能继续使用该授权。

## 依赖

- Apple 系统框架与 `Frameworks/StreamCoreSDK.xcframework`。
