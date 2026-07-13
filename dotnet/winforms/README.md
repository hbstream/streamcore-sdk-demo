# StreamCore Demo .NET WinForms

中文: [README.zh-CN.md](README.zh-CN.md)

This directory contains the Windows .NET Framework WinForms sample for the
official `StreamCore.Sdk` wrapper.

The demo does not declare direct P/Invoke calls. Native C imports stay inside
`StreamCore.Sdk`, and the UI calls only the public runtime / player / capture /
publisher / GB28181 helper classes.

## Current Scope

The current WinForms layout now follows the same top-level tab split as the Qt
demo:

- `Publisher`: local-capture, media-file, still-image, and audio-only publish
  entry points; prominent `NO LOCAL PREVIEW` notice for file passthrough and
  still-image routes; a narrower dark preview surface aligned with the Qt demo
- `Player`: URL playback, preflight, start/stop, software or hardware decode,
  software/GPU/direct/auto present path, and WinForms `Panel.Handle` render
  target
- `GB28181`: runtime start/register/keepalive/unregister controls, device and
  catalog metadata, source-binding selection, and session/status logging
- `License`: product info, machine id, runtime/license state, demo-license
  bootstrap, and manual encrypted license registration

This closes demo-side page parity and host wiring for the current public `.NET`
wrapper. Real-media publishing and GB28181 validation should still be tested in
the target project environment.

The project targets .NET Framework 4.5.1 because the current Windows build host
already has that reference assembly and Visual Studio MSBuild available. The
executable is forced to x64 so it matches the current Windows SDK native
package architecture.

## SDK Placement

The WinForms demo consumes only packaged `.NET` SDK artifacts. It does not
reference the monorepo SDK project or any repo build-output directory. Use the
classic NuGet-style `packages/` layout:

Before native runtime DLLs are copied, the build removes stale FFmpeg and
runtime DLL names from the output directory, then copies the matching SDK
release `bin/` dependencies such as `avcodec-62`, `avformat-62`, and
`avutil-60`.

```text
streamcore_demo_dotnet_winforms/
  packages/StreamCore.Sdk/lib/net451/StreamCore.Sdk.dll
  packages/StreamCore.Sdk/runtimes/win-x64/native/streamcore_sdk.dll
```

Public demo repositories should not commit `packages/`. Delivery packages may
prebundle this directory so Visual Studio /
MSBuild can build directly. If a package is extracted to a different local
directory, override only `StreamCoreSdkBinary` and
`StreamCoreNativeRuntimeDir` to point at that packaged layout.

## Build

```powershell
cd streamcore_demo_dotnet_winforms
MSBuild.exe .\StreamCore.Demo.WinForms\StreamCore.Demo.WinForms.csproj /t:Clean,Build /p:Configuration=Release
```

## Public Boundary

The WinForms demo must not declare direct P/Invoke calls. Native C imports stay
inside `StreamCore.Sdk`; the demo consumes only the public wrapper surface.
