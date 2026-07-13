# StreamCore SDK Demo Android

`streamcore_demo_android` 是 StreamCore SDK 的 Android 示例工程，可以作为独立 GitHub 仓库发布。

English: [README.en.md](README.en.md)

## 2026-05-12 Release Gate Status / 当前上线门禁状态

- demo 只调用 Android AAR 随附的公开 Java/Kotlin facade 和平台系统 API。
- 本轮已用当前 SDK AAR 刷新 demo，完成 Debug/Release/lint、真机安装、camera
  preview、publisher preview、RTMP player 和 RTMP publisher 外部读回。
- Android GB28181 已闭合当前 SRS camera/video-file 路线的视频媒体证明；该
  SRS profile 本轮只协商 `m=video`，所以不声称 GB28181 音频已闭合。
- Android player 扬声器输出仍是根功能矩阵中的独立 proof 项，不因包构建或
  RTMP 音轨存在而自动闭合。

## SDK 放置方式

Android 工程按 Gradle / Android Studio 的常见本地 AAR 约定放置 SDK：

```text
streamcore_demo_android/
  app/libs/streamcore-sdk.aar
```

也可以通过环境变量 `STREAMCORE_DEMO_SDK_AAR` 指向本机 AAR 文件。公开源码仓默认不提交
`app/libs/*.aar`；交付包可以预先放好 AAR，让用户直接打开 Android Studio
或执行 Gradle 构建。

## 编译

```powershell
cd streamcore_demo_android
.\gradlew.bat :app:assembleDebug :app:lintDebug --console=plain --no-daemon
```

Linux / macOS:

```bash
cd streamcore_demo_android
./gradlew :app:assembleDebug :app:lintDebug --console=plain --no-daemon
```

## 当前能力

- 直接调用 StreamCore SDK Android AAR 中的正式公开接口。
- 展示 license / runtime / 日志配置、publisher、player、GB28181 占位入口、显示模式切换、日志分享和状态展示。
- 不引用非公开 SDK 实现文件、签名密钥、客户授权生成文件或本机私有路径。

## 公开边界

本工程面向客户集成示例，只能使用 `com.hbr.streamcore` 公开 facade 和 Android 系统 API。
如果需要新增能力，优先先在 SDK Android 公开接口中补齐，再让 demo 调用公开接口。
