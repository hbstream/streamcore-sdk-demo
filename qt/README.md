# StreamCore SDK 桌面 Demo

English: [README.en.md](README.en.md)

Publisher 页面支持 WHIP 端点和可选 Bearer Token。选择 WHIP 后，视频与音频编码固定为 H.264 与 Opus；SDK 配置或编码包参数不符合协议要求时，界面日志会原样展示 `-6 / UNSUPPORTED_MEDIA_CODEC`，以及实际编码和要求编码。

本目录提供基于 Qt 6 Widgets 的桌面演示程序，覆盖 Windows、Linux 和 macOS。界面包含推流、拉流播放、ONVIF、GB28181 与授权状态，可作为 C/C++ 集成参考。

## WHEP 播放（1.6.0）

Player 页通过来源下拉框显式选择媒体 URL 或 WHEP。WHEP 模式提供遮蔽 Bearer Token、可选 numeric 本地绑定 IP 和“仅隔离测试允许 HTTP”开关；默认使用 HTTPS，并要求 Professional `p2_whep_player`。选项校验失败后不会继续配置、预检或启动，界面只显示固定错误摘要，展示 endpoint 时会移除 userinfo、query 和 fragment。custom CA、ICE/TURN、relay-only 等高级配置继续使用公开 C API。

Qt Demo 在原有标签页之外提供独立的“AI特效接入演示”页，不改动 Publisher 页布局。页面使用紧凑的输入/效果控件和宽幅的原始/处理后双预览；输入可选本地视频或摄像头，安装独立效果包后还可选内置视频样片。该页不接收静态图片作为视频输入；换脸所需的参考头像仍可选图片。Qt 程序通过 StreamCore SDK 的 Capture Processor 接收帧、异步处理并以 `REPLACE` 交回 SDK；可选把处理后视频通过 SDK Publisher 推送至 RTMP。页面显示输入、替换、透传、错误和超时计数。SDK 不内置固定 AI 模型，未安装效果包时可直接使用内置黑白处理验证链路。

### Processor 模块接入

1. 本仓库只包含 SDK Processor 宿主、二进制帧 Worker 和无额外依赖的黑白基线；美颜、换脸、形体变形算法、模型和样片在独立的可选模块包中。源码构建无需安装 Python、OpenCV 或模型。打开该页但未安装效果包时，仍可用本地文件或摄像头验证黑白 Processor 链路。
2. 页面在未安装效果包时提供获取提示。Windows 效果扩展包解压后，`processor_pack/` 与 `runtime/python/` 应位于 Qt 可执行文件同级；重启后页面读取 `processor_pack/manifest.json`，列出效果和对应样片、参考图与强度控件。开发时也可用 `STREAMCORE_DEMO_QT_PROCESSOR_PACK` 指定清单文件。选择效果后，帧仍经过相同的 SDK Capture Processor 和可选 Publisher 路径。
3. 自定义算法可参考 [`processors/example_invert.py`](processors/example_invert.py)：实现 `create_processor()`，返回接收并返回同尺寸 `uint8`、`H×W×4` BGRA 帧的可调用对象。模型在工厂中加载一次。运行时可使用模块包内嵌的 Python/NumPy/OpenCV，或用 `STREAMCORE_DEMO_QT_PROCESSOR_PYTHON` 指定 Python；这是运行依赖，不进入 Qt 源码编译依赖。
4. 勾选推流时填写 RTMP 地址；只需双预览时可不勾选。视频文件和内置样片如果带音轨，推流自动使用该音轨；摄像头可另外勾选默认麦克风。SDK 的单个 `MEDIA_FILE` Capture 不混入独立麦克风，因此文件输入时麦克风选项禁用；音视频推流采用 H.264/AAC，预览不采集音频。Demo 用 `--probe` 预检 Python 和模块。外部 Worker 失败或超过截止时间时，SDK 按本页设置丢弃该处理帧并记录计数；内置黑白路径使用透传回退。自动化入口为 `STREAMCORE_DEMO_QT_AUTORUN=processor`，效果、来源、文件、摄像头设备和推流地址分别可由 `STREAMCORE_DEMO_QT_PROCESSOR_EFFECT`、`...SOURCE`、`...FILE`、`...CAMERA_ID`、`...URL` 设定；`...MICROPHONE=1` 在摄像头推流时勾选默认麦克风。

源码构建会编译该页并复制通用 Worker 脚本，无需额外编译开关。`STREAMCORE_DEMO_QT_PROCESSOR_WORKER` 可指定自定义 Worker。默认帧协议是小端 `SCF1,width,height,bytes` + BGRA 字节，返回 `SCO1,bytes` + 同尺寸 BGRA 字节，每帧上限 16 MiB。`STREAMCORE_DEMO_QT_PROCESSOR_DEADLINE_MS` 可调整 SDK 截止时间；默认 30000 ms，让低算力设备上的模型能完成一帧，实际处理帧率随算法耗时变化。

接入代码分为四个可单独阅读的部分：`src/processor_demo_page.*` 管理界面、输入与 SDK Capture/Publisher 会话；`src/processor_frame_adapter.*` 实现 SDK Processor 回调、帧导出/替换、异步队列和完成责任；`src/processor_worker_bridge.*` 是与 SDK、界面无关的可复用 Python Worker 工具类；`src/processor_effect_catalog.*` 只负责读取可选包清单并检查文件路径。每个 Worker 有自己的 `QProcessEnvironment`，切换效果不会继承上一个效果的模块或参考图。产品可沿用相同 SDK Processor 合同，以本地 C++/GPU 算法替换 Worker，或自行使用桥接类接入其他运行时。源码构建和默认运行均不要求安装扩展包。

已冻结的 1.6.2 Demo SDK 对文件解码 RGB32 帧存在底层与公开像素格式编号重叠。仅在构建时选择该旧包，Qt 才编译受限兼容分支：媒体包、格式编号 5 且数据量恰好为 `width×height×4` 时通过公开 CPU 视图读取。Windows `1.6.3-demo.1` 包修复了 SDK 边界映射，Qt 在该构建中只使用标准 CPU export/copy 接口。1.6.2 包及其哈希保持原样。

## 准备 SDK

Windows 新版获取同版本 `streamcore_sdk_1.6.3-demo.1_windows_x86_64.zip` Demo 专用 SDK 包；Linux/macOS 继续从 [Demo 1.6.2 Release](https://github.com/hbstream/streamcore-sdk-demo/releases/tag/v1.6.2) 获取对应的冻结包。校验同一 Release 提供的 SHA-256 后，解压到当前目录下的 `sdk/<platform_arch>/`。Qt 构建会核对 manifest 的版本、Demo-only 标记与产品身份，拒绝混用正式 SDK 和不匹配的 Demo 授权。示例：

```text
sdk/
  windows_x86_64/
    include/streamcore/streamcore_sdk.h
    lib/streamcore_sdk.lib
    bin/streamcore_sdk.dll
  linux_x86_64/
    include/streamcore/streamcore_sdk.h
    lib/libstreamcore_sdk.so
```

公开源码仓库不包含 SDK 二进制或正式授权。仓库内只保留绑定固定 Demo 进程或 Bundle ID 的加密 Demo 授权；Windows 新包使用 `streamcore_sdk/demo` 的独立签名与加密配置，旧平台包继续按 1.6.2 历史身份验证。可运行的桌面 Demo 和源码构建所需的 Demo SDK 包从仓库的 [Releases](https://github.com/hbstream/streamcore-sdk-demo/releases/latest) 下载。

## 编译

Windows：

```powershell
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/windows_x86_64
cmake --build build --config Release --target streamcore_demo_qt
```

Linux：

```bash
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/linux_x86_64
cmake --build build --config Release --target streamcore_demo_qt -j
```

macOS（当前发布包为 Intel x86_64）：

```bash
cmake -S . -B build -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=sdk/macos_x86_64
cmake --build build --config Release --target streamcore_demo_qt -j
open build/streamcore_demo_qt.app
```

需要 Qt 6 Widgets、CMake 和支持 C++17 的编译器。macOS 的摄像头、麦克风和屏幕录制权限由 Demo 应用向系统申请，请通过生成的 `.app` 启动。

## Demo 授权

可运行包已经在程序目录下的 `license/demo/` 放置匹配当前平台和入口的
`SC-LIC-ENC-v1` 加密 Demo 授权；程序启动时通过公开 runtime 配置提交该文件路径，
不需要用户填写注册码。自行编译时保留 CMake 默认部署步骤即可，不能把正式
`streamcore_sdk` 客户授权与 Demo 授权互换。

Demo 授权仅绑定本示例进程（macOS 绑定 `.app` 的 Bundle ID），用于体验 Standard
与 Professional 能力并显示 `hbrun.com` 水印。Windows、Linux 或 macOS 的 Demo
授权也不能复制到其他平台或其他应用中使用。

## 依赖

- Qt 6、平台系统库和已发布的 StreamCore SDK 包。
- 示例通过 StreamCore SDK 公开 C API 完成集成。
