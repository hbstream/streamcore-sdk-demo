# StreamCore SDK Demo

<p>
  <a href="#english">English</a> |
  <a href="#简体中文">简体中文</a>
</p>

## English

This repository contains public HBRun StreamCore SDK demo projects for desktop, mobile, and .NET applications. The samples use public SDK APIs only and cover playback, capture, publishing, recording, SRT, WHIP (WebRTC ingest), ONVIF, and GB28181 workflows. The Qt demo also shows asynchronous Capture Processor integration with side-by-side original and processed previews.

### Project Layout

| Directory | Platform and contents |
| --- | --- |
| [`qt/`](qt/) | Qt Widgets desktop demo for Windows, Linux, and macOS |
| [`android/`](android/) | Android playback, capture, and publishing samples |
| [`ios/`](ios/) | Objective-C integration sample for iPhone and iPad |
| [`dotnet/winforms/`](dotnet/winforms/) | Windows WinForms demo using the public .NET wrapper |

### Downloads

Runnable demos, SDK packages, and codec add-ons are available from the official download channels:

- [HBRun Download Center](https://hbrun.com/en/downloads)
- [StreamCore SDK Product Page](https://hbrun.com/en/products/streamcore-sdk)
- [Latest StreamCore SDK Release](https://github.com/hbstream/hbrun-downloads/releases?q=streamcore-sdk&expanded=true)
- [Runnable Demo Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest)

Each platform README links to the matching Demo SDK build package and shows its exact placement. The repository includes only encrypted licenses bound to the fixed Demo identities. It does not include SDK binaries or production license files, and production SDK packages are intentionally not accepted by these Demo projects.

### Demo License

Runnable demo packages include a license bound to the matching demo process or app. It enables Standard and Professional evaluation features with an `hbrun.com` watermark. It cannot be reused by another process, package, or application and is not a production license.

## 简体中文

本仓库提供 HBRun StreamCore SDK 的公开演示工程，覆盖桌面端、移动端和 .NET。示例仅调用公开 SDK API，可用于验证播放、采集、推流、录像、SRT、WHIP（WebRTC 推流）、ONVIF 和 GB28181 等能力。Qt Demo 还提供异步 Capture Processor 示例，可同时查看处理前与处理后的画面。

### 工程目录

| 目录 | 平台与内容 |
| --- | --- |
| [`qt/`](qt/) | Qt Widgets 桌面演示工程，覆盖 Windows、Linux 和 macOS |
| [`android/`](android/) | Android 播放、采集与推流示例 |
| [`ios/`](ios/) | iPhone / iPad Objective-C 接入示例 |
| [`dotnet/winforms/`](dotnet/winforms/) | Windows WinForms 演示工程，使用公开 .NET 封装 |

### 下载与运行

无需编译即可体验的演示程序、各平台 SDK 包和编解码增强包可从以下入口获取：

- [HBRun 下载中心](https://hbrun.cn/downloads)
- [StreamCore SDK 产品页](https://hbrun.cn/products/streamcore-sdk)
- [最新 StreamCore SDK Release](https://github.com/hbstream/hbrun-downloads/releases?q=streamcore-sdk&expanded=true)
- [演示程序 Release](https://github.com/hbstream/streamcore-sdk-demo/releases/latest)

各平台目录的 README 提供匹配的 Demo SDK 构建包、校验入口和准确放置位置。仓库只携带绑定固定 Demo 身份的加密授权，不包含 SDK 二进制或正式授权；这些 Demo 工程也会明确拒绝正式 SDK 包，避免混用授权域。

### Demo 授权

可运行演示程序包中的授权仅绑定对应 Demo 进程或 App，用于验证 Standard 与 Professional 能力，并显示 `hbrun.com` 水印。授权不能移入其他进程、包名或应用中使用，也不等同于商业授权。
