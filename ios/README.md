# StreamCore SDK Demo iOS

`streamcore_demo_ios` 是 StreamCore SDK 的 Apple / iOS 示例目录，定位是可阅读、可迁移的正式调用样例。

English: [README.en.md](README.en.md)

## SDK 放置方式

iOS / Xcode 工程按常见 `Frameworks` 目录放置 SDK framework：

```text
streamcore_demo_ios/
  Frameworks/StreamCoreSDK.xcframework
```

公开 GitHub 源码仓默认不提交 `Frameworks/StreamCoreSDK.xcframework`。交付包可以
预先放好 framework，让用户在 Xcode 中直接把它加入 `Frameworks, Libraries, and Embedded Content`。

正式客户包包含 iOS 真机 slice；iOS Simulator 包只用于验证，不作为正式客户 release 包交付。

## 当前内容

- `StreamCoreDemoSample.m`
  - 演示 runtime 配置、license 注册、日志配置、player / capture / publisher 的 preflight、
    runtime info 和 start 调用。
  - 只通过 `StreamCoreSDK.h` 的 Objective-C public surface 访问 SDK。
- `StreamCoreDemoViewController.h/.m`
  - 演示 iOS UIKit 分区界面、状态展示、日志分享、显示模式切换和 SDK 关键接口调用。
  - 提供播放、采集、推流三组串行操作入口：预检、开始、停止；推流的编码喂流模式额外提供
    样例包推送按钮。
  - 通过 `NSBundle.mainBundle` 查找 demo license 和 public key。
- `StreamCoreDemoAppDelegate.h/.m`、`StreamCoreDemoAppMain.m`、`StreamCoreDemoInfo.plist`
  - 提供最小 iOS app 宿主，默认 bundle id 为 `com.hbr.streamcoredemo`。
- `StreamCoreDemoLaunchScreen.storyboard`
  - 提供最小 launch screen。
- `Resources/`
  - 随示例提供 `streamcore_demo.lic` 和 `streamcore_demo_public.pem`，需要加入 Xcode target 的
    `Copy Bundle Resources`。

## 编译方式

### Xcode App Target

创建 iOS app target，加入本目录源码文件，并 embed `Frameworks/StreamCoreSDK.xcframework`。需要复制到
app bundle 的资源：

```text
Resources/streamcore_demo.lic
Resources/streamcore_demo_public.pem
StreamCoreDemoLaunchScreen.storyboard
```

### CMake / Xcode Generator

根目录 `streamcore_demo/CMakeLists.txt` 和当前目录下的 `streamcore_demo_ios/CMakeLists.txt`
现在都只消费 `Frameworks/StreamCoreSDK.xcframework`（或显式传入
`STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT`），不再依赖相邻 `streamcore_sdk` 源码树。

在 Mac 宿主上请使用 Xcode generator 和 iOS sysroot。维护中的 SDK 打包流程会自动刷新
`Frameworks/StreamCoreSDK.xcframework`，因此 demo 构建和验证始终以打包产物为准。

## 验证限制

Simulator 可以验证 app 启动、bundle 资源加载、license 状态和基础 UI wiring。摄像头、麦克风、
ReplayKit 和真实推流必须在带签名和权限的 iOS 真机上验证。

Simulator 环境可以用以下变量触发三组 session 预检：

```bash
SIMCTL_CHILD_STREAMCORE_DEMO_IOS_AUTORUN=preflight \
  xcrun simctl launch booted com.hbr.streamcoredemo
```

该入口只执行播放、采集、推流的 configure/preflight，并把结果写入界面状态和
`streamcore_demo.log`；真机上的 camera/microphone/ReplayKit/start/stop 仍应通过界面按钮验证。

## 公开边界

本示例只使用 `StreamCoreSDK.h` Objective-C 公开 wrapper 和 Apple 系统 framework，不引用
非公开 SDK 实现文件、签名密钥、客户授权生成文件或本机私有路径。
