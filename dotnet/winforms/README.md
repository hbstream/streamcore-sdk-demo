# StreamCore SDK .NET WinForms Demo

简体中文: [README.zh-CN.md](README.zh-CN.md)

This directory contains the Windows WinForms sample for the public `StreamCore.Sdk` .NET wrapper. It covers publishing, playback, GB28181, and license status. Native imports remain inside the wrapper; the demo does not declare its own P/Invoke surface.

The Publisher page supports WHIP endpoints and an optional Bearer Token. Selecting WHIP locks the video and audio codecs to H.264 and Opus. Invalid SDK configuration or encoded-packet parameters are reported as `-6 / UNSUPPORTED_MEDIA_CODEC` with the actual and required codecs.

## WHEP Playback (1.6.0)

The Player page explicitly selects Media URL or WHEP. WHEP mode exposes a masked Bearer token, an optional numeric local bind address, and an HTTP switch limited to isolated tests. HTTPS remains the default and Professional `p2_whep_player` is required. `SetWhepOptions` failure prevents config/preflight/start, while status text never echoes the token or bind address. Use the public `StreamCorePlayerWhepOptions` object for custom CA, ICE/TURN, relay-only, and other advanced settings.

## Prepare the SDK

Download `StreamCore.Demo.Sdk.1.6.2.nupkg` from the [Demo 1.6.2 Release](https://github.com/hbstream/streamcore-sdk-demo/releases/tag/v1.6.2), verify it with `SHA256SUMS.txt` from the same release, extract it as a ZIP archive, and place its contents under this directory using the NuGet-style layout:

```text
packages/StreamCore.Sdk/
  lib/net451/StreamCore.Sdk.dll
  runtimes/win-x64/native/streamcore_sdk.dll
```

The public source repository does not include the package or a production license. It includes only the encrypted license bound to the WinForms Demo process. A production NuGet package belongs to a different trust domain and cannot replace the Demo package above. Runnable Windows demos are available from the repository [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest).

## Build

```powershell
MSBuild.exe .\StreamCore.Demo.WinForms\StreamCore.Demo.WinForms.csproj /t:Clean,Build /p:Configuration=Release
```

The current sample targets .NET Framework 4.5.1 and Windows x64.

## Demo License

The runnable package places the encrypted `SC-LIC-ENC-v1` demo license at
`license/demo/streamcore_demo_winforms.lic` below the executable directory. The sample
submits that path through `StreamCoreRuntime.Configure`; the License page can also call
`StreamCoreRuntime.RegisterLicenseText` when testing an explicitly supplied encrypted
string. The demo license is bound to the sample executable, enables Standard and
Professional evaluation features with an `hbrun.com` watermark, and is not accepted by
the production SDK.

## Dependencies

- .NET Framework, Windows system libraries, and the released `StreamCore.Sdk` package.
- The demo consumes the public wrapper and does not declare direct P/Invoke imports.
