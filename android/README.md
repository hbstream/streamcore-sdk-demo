# StreamCore SDK Android Demo

English: [README.en.md](README.en.md)

本目录提供 StreamCore SDK 的 Android 示例，覆盖播放、摄像头与麦克风采集、推流、GB28181 和授权状态查看。示例只调用公开 Android API 与 `com.hbr.streamcore` SDK 接口。

## 准备 SDK

从 [HBRun 下载中心](https://hbrun.com/zh-CN/downloads/) 获取 Android SDK 包，将 AAR 放入：

```text
app/libs/streamcore-sdk.aar
```

公开源码仓库不包含 AAR 和正式授权文件。可运行的 Demo 包可从仓库的 [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest) 下载。

## 编译

Windows：

```powershell
.\gradlew.bat :app:assembleDebug :app:lintDebug --console=plain --no-daemon
```

Linux / macOS：

```bash
./gradlew :app:assembleDebug :app:lintDebug --console=plain --no-daemon
```

使用 Android Studio 时，直接打开当前目录并等待 Gradle 同步完成。

## Demo 授权

Demo 授权仅绑定本示例的应用包名，用于体验 Standard 与 Professional 能力，并显示 `hbrun.com` 水印。更改包名后不能继续使用该授权；正式集成需要申请对应应用身份的商业授权。

## 依赖

- Android Gradle Plugin、Kotlin/Java 与 Android 系统库。
- `app/libs/streamcore-sdk.aar` 中的公开 SDK。
