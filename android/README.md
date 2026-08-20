# StreamCore SDK Android Demo

English: [README.en.md](README.en.md)

本目录提供 StreamCore SDK 的 Android 示例，覆盖播放、摄像头与麦克风采集、推流、GB28181 和授权状态查看。示例只调用公开 Android API 与 `com.hbr.streamcore` SDK 接口。

## WHEP 播放（1.6.0）

Player 页通过“来源类型”显式选择媒体 URL 或 WHEP，不根据地址文本猜测协议。WHEP 模式可填写遮蔽显示的 Bearer Token、可选 numeric 本地绑定 IP，并可为 localhost/隔离测试显式允许 HTTP；默认仍强制 HTTPS。WHEP 属于 Professional 的 `p2_whep_player` 能力，配置失败时 Demo 会在联网前停止，且状态区不会回显 token、endpoint query 或绑定地址。custom CA、ICE/TURN 与 relay-only 等高级项请通过 SDK 正式 `StreamCorePlayer.WhepOptions` 接口配置。

## 准备 SDK

从 [Demo 1.6.2 Release](https://github.com/hbstream/streamcore-sdk-demo/releases/tag/v1.6.2) 下载 `streamcore_demo_sdk_1.6.2_android_multi.aar`，用同一 Release 的 `SHA256SUMS.txt` 校验后，将其重命名并放入：

```text
app/libs/streamcore-sdk.aar
```

公开源码仓库不包含 AAR 或正式授权，只保留绑定默认 Demo 包名的加密授权。正式 AAR 与 Demo 授权域不同，不能替代上述 Demo AAR。可运行的 Demo 包也可从仓库的 [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest) 下载。

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

可运行 APK 已把 `SC-LIC-ENC-v1` 加密 Demo 授权放入
`app/src/main/assets/streamcore_demo.lic`。应用首次初始化时把它复制到自身
`filesDir`，再通过公开 runtime 配置提交私有文件路径；用户不需要填写注册码，
授权也不会写入公共存储。自行编译时必须保留该 asset 和默认包名。

Demo 授权仅绑定本示例包名，不要求用户提交正式应用的 Release 签名证书，
用于体验 Standard 与 Professional 能力并显示 `hbrun.com` 水印。更改包名后不能
继续使用；正式集成需要申请“applicationId + Release 签名证书 SHA-256”商业授权，
不能继续使用 Demo 授权或 Demo AAR。

## 依赖

- Android Gradle Plugin、Kotlin/Java 与 Android 系统库。
- `app/libs/streamcore-sdk.aar` 中的公开 SDK。
