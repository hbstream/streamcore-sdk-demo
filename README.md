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

The current Windows Qt Demo is `1.6.3-demo.3` and uses the restricted `streamcore_sdk/demo` SDK build `1.6.3-demo.1` for source builds. Its built-in monochrome Processor works without an extension. To try beauty, HyperSwap/ArcFace face swap, or body shaping, [contact HBRun](https://hbrun.com/en/about/?topic=streamcore-ai-effects#contact) for the separate AI effects extension; the complete extension has no public download link. The earlier base effects archive remains only as a historical release asset and does not include face swap. The previously published `1.6.2` packages for other platforms retain their listed versions and checksums.

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

当前 Windows Qt Demo 为 `1.6.3-demo.3`；源码构建使用受限的 `streamcore_sdk/demo` SDK `1.6.3-demo.1`。内置黑白 Processor 无需扩展包即可运行。体验美颜、HyperSwap/ArcFace 换脸或身形调整，请[联系 HBRun 获取独立 AI 特效扩展包](https://hbrun.cn/about/?topic=streamcore-ai-effects#contact)；完整包不提供公开下载地址。历史 Release 保留的基础效果包不含换脸，其他平台已发布的 `1.6.2` 附件保持原版本与哈希。

### Demo 授权

可运行演示程序包中的授权仅绑定对应 Demo 进程或 App，用于验证 Standard 与 Professional 能力，并显示 `hbrun.com` 水印。授权不能移入其他进程、包名或应用中使用，也不等同于商业授权。
