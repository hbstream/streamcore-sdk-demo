# StreamCore SDK .NET WinForms Demo

English: [README.md](README.md)

本目录提供基于 .NET Framework 4.5.1 的 Windows WinForms 示例，覆盖播放、推流、GB28181 与授权状态。示例只使用正式 `StreamCore.Sdk` 公共包装层，不自行声明 P/Invoke。

## WHEP 播放（1.6.0）

Player 页通过来源下拉框显式选择媒体 URL 或 WHEP。WHEP 模式提供遮蔽 Bearer Token、可选 numeric 本地绑定 IP 和隔离测试 HTTP 开关，默认仍使用 HTTPS，并要求 Professional `p2_whep_player`。`SetWhepOptions` 失败后不会继续 config、preflight 或 start；状态文本不会回显 token 或绑定地址。custom CA、ICE/TURN、relay-only 等高级参数继续使用公开 `StreamCorePlayerWhepOptions`。

## 准备 SDK

从 [HBRun 下载中心](https://hbrun.cn/downloads/) 获取 .NET SDK 包，并按 NuGet 布局解压到：

```text
packages/StreamCore.Sdk/
  lib/net451/StreamCore.Sdk.dll
  runtimes/win-x64/native/streamcore_sdk.dll
```

## 编译

```powershell
MSBuild.exe .\StreamCore.Demo.WinForms\StreamCore.Demo.WinForms.csproj /t:Clean,Build /p:Configuration=Release
```

## Demo 授权

可运行包会在程序目录下放置
`license/demo/streamcore_demo_winforms.lic`。它是 `SC-LIC-ENC-v1` 加密 Demo 授权，
示例通过 `StreamCoreRuntime.Configure` 提交文件路径；License 页也可用
`StreamCoreRuntime.RegisterLicenseText` 验证显式填写的加密授权字符串。

Demo 授权仅绑定本示例进程，用于评估 Standard 与 Professional 能力并显示
`hbrun.com` 水印，不能复制到其他应用，也不能交给正式 SDK 使用。
