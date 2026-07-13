# StreamCore SDK .NET WinForms Demo

English: [README.md](README.md)

本目录提供 Windows WinForms 示例，使用公开的 `StreamCore.Sdk` .NET 封装，覆盖推流、拉流播放、GB28181 与授权状态。原生接口导入统一封装在 SDK 中，Demo 不自行声明 P/Invoke。

## 准备 SDK

从 [HBRun 下载中心](https://hbrun.com/zh-CN/downloads/) 获取 .NET SDK 包，按 NuGet 常用目录结构解压到当前目录：

```text
packages/StreamCore.Sdk/
  lib/net451/StreamCore.Sdk.dll
  runtimes/win-x64/native/streamcore_sdk.dll
```

公开源码仓库不包含 SDK 包和正式授权文件。可运行的 Windows Demo 可从仓库的 [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest) 下载。

## 编译

```powershell
MSBuild.exe .\StreamCore.Demo.WinForms\StreamCore.Demo.WinForms.csproj /t:Clean,Build /p:Configuration=Release
```

当前示例面向 .NET Framework 4.5.1 和 Windows x64。

## Demo 授权

Demo 授权仅绑定本示例可执行程序，用于体验 Standard 与 Professional 能力，并显示 `hbrun.com` 水印。授权不能复制到其他应用中使用。

## 依赖

- .NET Framework、Windows 系统库和已发布的 `StreamCore.Sdk` 包。
- Demo 只调用公开 .NET 封装，不自行声明 P/Invoke。
