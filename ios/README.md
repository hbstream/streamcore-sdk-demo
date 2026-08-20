# StreamCore SDK iOS Demo

English: [README.en.md](README.en.md)

本目录提供 iPhone / iPad Objective-C 示例，覆盖播放、采集、推流、授权状态与常用显示设置。示例只调用 `StreamCoreSDK.h` 公开接口和 Apple 系统框架。

## WHEP 播放（1.6.0）

Player 页通过分段控件显式选择媒体 URL 或 WHEP。WHEP 模式提供遮蔽 Bearer Token、可选 numeric 本地绑定 IP 与隔离测试 HTTP 开关，默认仍使用 HTTPS，并要求 Professional `p2_whep_player`。`configureWhepOptions:` 失败时不会继续 preflight/start；界面展示 endpoint 时会移除 userinfo、query 和 fragment。custom CA、ICE/TURN、relay-only 等高级参数可通过 `HBRStreamCorePlayerWhepOptions` 正式接口设置。

## 准备 SDK

从 [Demo 1.6.2 Release](https://github.com/hbstream/streamcore-sdk-demo/releases/tag/v1.6.2) 获取 `streamcore_demo_sdk_1.6.2_ios_arm64.tgz`；仅构建模拟器时获取对应 simulator 包。用同一 Release 的 `SHA256SUMS.txt` 校验并解压后，将 XCFramework 放入：

```text
Frameworks/StreamCoreSDK.xcframework
```

公开源码仓库不包含 XCFramework 或正式授权，只保留绑定默认 Demo Bundle ID 的加密授权。正式 XCFramework 与 Demo 授权域不同，不能替代上述 Demo SDK。iOS 应用需要在 Xcode 中完成签名后安装到真机；源码可直接用于创建或集成 App Target。

## 生成 Xcode 工程

在 macOS 上执行：

```bash
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS
open build-ios/streamcore_demo_ios.xcodeproj
```

默认 Bundle ID 为 `com.hbr.streamcoredemo`。请在 Xcode 中选择开发团队和目标设备，并确认以下资源已加入 App Target：

```text
Resources/streamcore_demo.lic
StreamCoreDemoLaunchScreen.storyboard
```

模拟器可用于查看界面、资源加载和基础接口状态；摄像头、麦克风、ReplayKit 与真实推流需要使用已签名的 iPhone 或 iPad 验证。

## Demo 授权

`Resources/streamcore_demo.lic` 是随示例提供的 `SC-LIC-ENC-v1` 加密 Demo 授权。
App 从 `NSBundle.mainBundle` 取得只读资源路径，并通过公开 runtime 配置提交；用户
不需要填写注册码。该授权只绑定默认 Demo Bundle ID，不要求用户提供正式应用的
Team ID，用于体验 Standard 与 Professional 能力并显示 `hbrun.com` 水印。

更改 Bundle ID 后不能继续使用。正式集成需要申请“Bundle ID + Team ID”商业授权，
并使用正式 XCFramework；Demo 授权与正式授权不能互换。

## 依赖

- Apple 系统框架与 `Frameworks/StreamCoreSDK.xcframework`。
