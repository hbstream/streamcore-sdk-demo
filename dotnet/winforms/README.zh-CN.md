# StreamCore Demo .NET WinForms

English: [README.md](README.md)

本目录是 StreamCore SDK 的 Windows `.NET Framework WinForms` 示例，演示 C#
宿主程序如何只依赖官方 `StreamCore.Sdk` wrapper，而不直接声明 P/Invoke。

native C 导入统一收口在 `StreamCore.Sdk` 中；本示例只调用公开的
runtime / player / capture / publisher / GB28181 helper 类。

## 当前范围

当前 WinForms 主界面已经按 Qt demo 的顶层布局收口为四个标签页：

- `Publisher`
  - 本地采集、媒体文件、静态图片和纯音频推流入口
  - 视频文件透传和静态图片路径会在预览区内明确显示醒目的
    `NO LOCAL PREVIEW / 当前无本地预览`，不再留下空白预览区
  - 预览区使用与 Qt demo 接近的窄幅深色提示面，避免默认窗口右侧视频区域过宽
- `Player`
  - URL 播放、preflight、start/stop
  - software / hardware decode
  - software / GPU / direct / auto present path
  - WinForms `Panel.Handle` 作为 Windows render target
- `GB28181`
  - runtime start/register/keepalive/unregister
  - Device / Catalog 元数据
  - source binding 选择和会话状态日志
- `License`
  - product info、machine id、runtime/license 状态
  - demo license bootstrap
  - 手工注册加密 license 字符串

该示例展示 `.NET` demo 的页面组织、官方 wrapper 调用和宿主接线。真实推流、
播放和 GB28181 流程仍应在目标项目环境中完成验证。

当前工程目标框架仍是 `.NET Framework 4.5.1`，因为本机 Windows 构建宿主已具备
该 reference assembly 和 Visual Studio MSBuild。可执行文件固定为 `x64`，以匹配
当前 Windows SDK native runtime 包。

## SDK 放置方式

WinForms demo 现在只消费打包后的 `.NET` SDK，不再引用 monorepo 里的 SDK 工程或
repo build 输出。默认使用传统 `packages/` 布局：

构建输出会在复制 native runtime 前清理旧 FFmpeg DLL，再从匹配的
SDK release `bin/` 补入 `avcodec-62` / `avformat-62` / `avutil-60` 等运行依赖，
避免输出目录混入旧 `avcodec-61` 系列导致无法直接运行。

```text
streamcore_demo_dotnet_winforms/
  packages/StreamCore.Sdk/lib/net451/StreamCore.Sdk.dll
  packages/StreamCore.Sdk/runtimes/win-x64/native/streamcore_sdk.dll
```

公开 demo 仓库默认不提交 `packages/`。交付包可以预先放好该目录，
让 Visual Studio / MSBuild 直接构建。如果本机把包解压到了其他目录，只允许通过
MSBuild 属性覆盖 `StreamCoreSdkBinary` 和 `StreamCoreNativeRuntimeDir` 指向那份
已打包布局。

## 编译

```powershell
cd streamcore_demo_dotnet_winforms
MSBuild.exe .\StreamCore.Demo.WinForms\StreamCore.Demo.WinForms.csproj /t:Clean,Build /p:Configuration=Release
```

## 接口边界

WinForms demo 不能直接声明 P/Invoke。native C 导入必须继续留在
`StreamCore.Sdk` 中，demo 只消费公开 wrapper surface。
