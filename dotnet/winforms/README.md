# StreamCore SDK .NET WinForms Demo

简体中文: [README.zh-CN.md](README.zh-CN.md)

This directory contains the Windows WinForms sample for the public `StreamCore.Sdk` .NET wrapper. It covers publishing, playback, GB28181, and license status. Native imports remain inside the wrapper; the demo does not declare its own P/Invoke surface.

## Prepare the SDK

Download the .NET SDK package from the [HBRun Download Center](https://hbrun.com/en/downloads/) and extract it under this directory using the NuGet-style layout:

```text
packages/StreamCore.Sdk/
  lib/net451/StreamCore.Sdk.dll
  runtimes/win-x64/native/streamcore_sdk.dll
```

The public source repository does not include the package or a production license. Runnable Windows demos are available from the repository [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest).

## Build

```powershell
MSBuild.exe .\StreamCore.Demo.WinForms\StreamCore.Demo.WinForms.csproj /t:Clean,Build /p:Configuration=Release
```

The current sample targets .NET Framework 4.5.1 and Windows x64.

## Demo License

The demo license is bound to the sample executable. It enables Standard and Professional evaluation features with an `hbrun.com` watermark and cannot be reused by another application.

## Dependencies

- .NET Framework, Windows system libraries, and the released `StreamCore.Sdk` package.
- The demo consumes the public wrapper and does not declare direct P/Invoke imports.
