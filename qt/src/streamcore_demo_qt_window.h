/*******************************************************************************
 * streamcore_demo_qt_window.h
 * Copyright (c) 2026 HBRun. All rights reserved.
 *
 * Qt Widgets main window declaration for the StreamCore SDK demo.
 ******************************************************************************/

#ifndef _STREAMCORE_DEMO_QT_WINDOW_H_
#define _STREAMCORE_DEMO_QT_WINDOW_H_

#include "streamcore_demo_snapshot.h"

#include <QByteArray>
#include <QMainWindow>
#include <QSize>
#include <QVector>
#include <QString>

#include <atomic>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QObject;
class QPlainTextEdit;
class QPushButton;
class QResizeEvent;
class QShowEvent;
class QSlider;
class QTableWidget;
class QTimer;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

class StreamCoreDemoQtWindow : public QMainWindow
{
public:
    explicit StreamCoreDemoQtWindow(QWidget* parent = nullptr);
    ~StreamCoreDemoQtWindow() override;

private:
    enum class DemoLanguage
    {
        Auto,
        English,
        Chinese
    };

    enum class PreviewDisplayMode
    {
        StretchFill,
        AspectFit,
        AspectCrop
    };

    struct OnvifDeviceItem
    {
        QString label;
        QString device_urn;
        QString response_endpoint;
        QString service_url;
        QString profile_name;
        QString profile_token;
        QString stream_uri;
        bool resolved_stream_uri;
    };

    // 构建客户可见的 Qt Widgets 页面结构。
    void BuildUi();

    // 处理预览容器 resize，保持三种显示模式的可见几何一致。
    bool eventFilter(QObject* watched, QEvent* event) override;

    // 窗口首次显示后重新绑定一次桌面 render target，避免使用 pre-layout 尺寸。
    void showEvent(QShowEvent* event) override;

    // 窗口尺寸变化时同步调整宿主预览几何与可见状态文本。
    void resizeEvent(QResizeEvent* event) override;

    // 从 StreamCore SDK 正式 runtime 收集当前 demo 快照。
    void LoadSnapshot();

    // 更新顶部产品概览，不在主展示区直接混入授权状态。
    void PopulateOverview(const streamcore_demo_snapshot_t& snapshot);

    // 填充 License 页里的 Publisher / Player / GB28181 诊断表格。
    void PopulateSessionTable(const streamcore_demo_snapshot_t& snapshot);

    // 填充 License 标签页里的 capability / limit 合同表。
    void PopulateCapabilityTable(const streamcore_demo_snapshot_t& snapshot);

    // 根据当前 Publisher source / audio policy 刷新来源状态。
    void UpdatePublisherSourceSummary();

    // 返回当前 Publisher 分辨率下拉框所选择的目标尺寸；0x0 表示跟随来源。
    QSize SelectedPublisherResolution() const;

    // 返回当前 Publisher 分辨率用于界面摘要的文本。
    QString SelectedPublisherResolutionText() const;

    // 返回当前 Publisher 目标视频编码。
    QString SelectedPublisherVideoCodec() const;

    // 返回当前 Publisher 目标音频编码。
    QString SelectedPublisherAudioCodec() const;

    // 返回当前 Publisher 音频参数对应的编码名。
    QString SelectedPublisherAudioProfileCodec() const;

    // 返回当前 Publisher 选择的 RTMP H.265 / HEVC 发布路线。
    streamcore_publisher_rtmp_hevc_mode_t SelectedPublisherRtmpHevcMode() const;

    // 返回当前 Publisher RTMP H.265 / HEVC 路线摘要文本。
    QString PublisherRtmpHevcModeText() const;

    // 返回当前选择的预览 / 播放显示模式。
    PreviewDisplayMode SelectedPreviewDisplayMode() const;

    // 返回界面和诊断区展示用的显示模式文本。
    QString PreviewDisplayModeText() const;

    // 把显示模式应用到 Publisher、Player 和 GB28181 的宿主预览区域。
    void ApplyPreviewDisplayMode();

    QLabel* CreateDemoWatermarkLabel(QWidget* parent);

    void UpdateDemoWatermarkLabels();

    void UpdateDemoWatermarkLabel(
        QWidget* frame, QLabel* label, bool alignTop = false);

    // 按当前可用宽度约束预览容器高度，避免预览区被纵向拉伸成异常比例。
    void ApplyPreviewSurfaceRatios();

    // 为单个预览容器计算稳定的横向媒体高度，默认保持 16:9 风格。
    void ApplyPreviewSurfaceRatio(QWidget* frame);

    // 按源尺寸与当前显示模式调整单个预览子控件的几何。
    void ApplyPreviewDisplayModeToWidget(
        QWidget* frame,
        QWidget* content,
        const QSize& sourceSize);

    // 返回当前 Player 预览用于等比计算的源尺寸。
    QSize PlayerPreviewSourceSize() const;

    // 按当前 render target 子窗口尺寸刷新桌面播放状态文本。
    void UpdateDesktopRenderTargetStatusFromGeometry();

    // 通过 StreamCore SDK 正式 C API 刷新桌面摄像头候选项。
    void RefreshPublisherCameraDevices();

    // 按当前 Publisher 音频模式刷新麦克风 / 系统音频候选项。
    void RefreshPublisherAudioDevices();

    // 根据 source、文件路径和当前平台 source 列表刷新 Publisher 分辨率候选。
    void RefreshPublisherResolutionOptions();

    // 根据当前 Publisher source 切换摄像头设备或本地文件输入控件。
    void UpdatePublisherSourceControls();

    // Refresh transport-specific controls without hiding invalid codec choices
    // that are useful for demonstrating the SDK preflight error contract.
    void UpdatePublisherTargetControls(bool apply_codec_defaults);

    // 返回当前 Publisher 选择是否允许在运行中附加本地预览。
    bool IsPublisherPreviewEnabled() const;

    // 返回当前 Publisher 选择是否存在可运行时开关的本地预览路线。
    bool CurrentPublisherSelectionSupportsPreview() const;

    // 对正在运行的 Publisher capture 路线附加或清空本地预览 target。
    void ApplyPublisherPreviewToggle();

    // 打开桌面文件选择器，并把选中的本地媒体路径写入 Publisher 文件输入框。
    void BrowsePublisherMediaFile();

    // 打开桌面文件选择器，并把选中的音频路径写入 Publisher 音频文件输入框。
    void BrowsePublisherAudioFile();

    // 使用 StreamCore SDK 正式 publisher C API 启动当前 URL 推流配置。
    void StartPublisher();

    // macOS 下在启动真实 camera / microphone 前先触发 TCC 授权准备。
    bool EnsurePublisherCapturePermissions(
        int source_index,
        int audio_index,
        QString* error_message);

    // 停止当前 Qt Publisher 会话并释放 SDK publisher 句柄。
    void StopPublisher();

    // 根据 Publisher 会话生命周期刷新 Qt 按钮状态。
    void UpdatePublisherButtons();

    // 根据当前 Player URL 与 ONVIF 操作状态刷新桌面播放目标状态。
    void UpdatePlayerSourceSummary();

    // 返回播放页是否由用户显式选择了 WHEP；普通 HTTP(S) URL 不做协议猜测。
    bool IsPlayerWhepSelected() const;

    // 把播放页 WHEP Bearer、本地绑定和隔离测试开关写入 SDK 的深拷贝配置。
    streamcore_result_t ApplyPlayerWhepOptions(
        streamcore_player_handle player,
        QString* errorSummary) const;

    // 返回可安全展示的播放输入；WHEP endpoint 不显示 query、fragment 或凭据。
    QString PlayerDisplaySource() const;

    // 在当前 Player 渲染区域和独立全屏窗口之间切换，不改变播放会话本身。
    void TogglePlayerFullscreen();

    // 把 Player 渲染区域提升到独立全屏窗口。
    void EnterPlayerFullscreen();

    // 把 Player 渲染区域还原到 Player 标签页。
    void ExitPlayerFullscreen();

    // 播放区域几何变化后把当前 SDK render target 重新提交给正在运行的播放器。
    bool RefreshActivePlayerRenderTarget();

    // 使用 StreamCore SDK 正式 ONVIF C API 搜索当前网络里的设备。
    void SearchOnvifDevices();

    // 将 ONVIF 下拉框中选中的设备流地址应用到播放器 URL。
    void ApplySelectedOnvifDevice();

    // 对选中设备执行带可选凭据的 RTSP stream URI 解析。
    bool ResolveSelectedOnvifStreamUri(OnvifDeviceItem* item);

    // 根据当前 ONVIF 搜索结果刷新播放器页设备状态。
    void UpdateOnvifDeviceSummary();

    // 根据当前 GB28181 source 刷新 INVITE live source 状态。
    void UpdateGb28181SourceSummary();

    // 使用 StreamCore SDK GB28181 addon 启动设备侧运行时并发送 REGISTER。
    void StartGb28181();

    // 停止 GB28181 运行时、轮询定时器和 SDK addon 句柄。
    void StopGb28181();

    // 周期性推进 GB28181 运行时定时器和控制逻辑。
    void PollGb28181();

    // 根据 GB28181 会话生命周期刷新 Qt 按钮状态。
    void UpdateGb28181Buttons();

    // 绑定桌面播放器渲染目标并显示 SDK 调用结果。
    void BindDesktopRenderTarget();

    // 用当前 Player URL 启动 Qt 桌面播放器会话。
    void StartPlayerUrl();

    // 按环境变量启动可重复的 UI 媒体端到端验证流程。
    void ScheduleAutorunIfRequested();

    // 按环境变量保存当前窗口截图，供桌面端布局自动化验证使用。
    void ScheduleAutomationScreenshot();

    // 在 autorun 验证退出前停止对应 SDK runtime，避免后台 publisher 或播放器残留。
    void ScheduleAutorunQuit(int quit_after_ms, const QString& mode);

    // 停止当前 Qt 桌面播放器会话并释放 SDK 句柄。
    void StopPlayerUrl();

    // 根据播放器会话生命周期刷新 Qt 按钮状态。
    void UpdatePlayerButtons();

    // SDK 播放器事件回调入口，用于展示首帧耗时和专业事件状态。
    static void OnPlayerEvent(
        const streamcore_player_event_t* event,
        void* userData);

    // 处理 SDK 播放器事件，显示首帧耗时和专业事件状态。
    void HandlePlayerEvent(
        int kind,
        int width,
        int height,
        qint64 timestampMs,
        const QString& detail);

    // 将播放页预设映射为可见的低延迟参数，便于验证标准/专业版播放能力差异。
    void ApplyPlayerLatencyPreset(int presetIndex);

    // 根据当前播放器控件生成 StreamCore SDK 播放策略配置。
    streamcore_player_video_present_path_t SelectedPlayerPresentPath() const;

    // 返回播放器是否请求硬件解码。
    bool SelectedPlayerHardwareDecode() const;

    // 返回 software-frame 路线是否优先使用软件渲染后端。
    bool SelectedPlayerPrefersSoftwareRenderBackend() const;

    // 生成播放主界面使用的简短能力状态文本。
    QString PlayerPlaybackModeSummary() const;

    // 返回当前 Publisher 采集音量百分比。
    int SelectedPublisherAudioVolumePercent() const;

    // 返回当前 Player 本地播放音量百分比。
    int SelectedPlayerAudioVolumePercent() const;

    // 返回当前 GB28181 采集音量百分比。
    int SelectedGb28181AudioVolumePercent() const;

    // 刷新音量滑块旁的百分比文本。
    void UpdateAudioVolumeLabels();

    // 根据 action 前缀把状态行路由到对应页面的滚动运行日志。
    void RouteOperationRuntimeLogLine(const QString& line);

    // 向指定运行日志窗口追加新行并保持滚动到末尾。
    void AppendRuntimeLog(QPlainTextEdit* logView, const QString& line);

    // 从输入框读取有边界的整数参数。
    int ReadBoundedInt(
        const QLineEdit* input,
        int defaultValue,
        int minValue,
        int maxValue) const;

    // 从输入框读取文本参数，空值回退到默认值。
    QString ReadText(const QLineEdit* input, const char* defaultValue) const;

    // 切换 Qt demo 语言并重建页面。
    void SetLanguage(DemoLanguage language);

    // 返回当前语言下的界面文本。
    QString UiText(const char* english, const char* chinese) const;

    // 返回当前生效的界面语言；Auto 时跟随系统语言。
    DemoLanguage EffectiveLanguage() const;

    // 根据系统语言选择默认 demo 语言。
    static DemoLanguage ResolveSystemLanguage();
    static DemoLanguage StartupLanguage();

    // 在快照装载失败时把错误导向 License 标签页诊断区。
    void ShowFailure(const QString& message);

    // 将 C API 的 int 布尔值转为稳定展示文本。
    void SetOperationStatus(
        const QString& action,
        const QString& code,
        const QString& statusName,
        const QString& summary);

    void UpdateOperationStatusLabel();

    // 把最近一次操作状态追加到 SDK log 回报的日志文件，供后续分享包带出。
    void AppendOperationLogLine();

    // 返回当前日志文件完整路径，优先使用 SDK log 生效目录。
    QString CurrentLogFilePath() const;

    // 返回当前 SDK log 日志目录，支持相对路径转为工作目录下绝对路径。
    QString CurrentLogDirectory() const;

    // 返回当前产品级 crash 目录，供日志包收集成熟 crash 方案已生成的 dump/report。
    QString CurrentProductCrashDirectory() const;

    // 生成带大小与数量上限的日志 zip；返回空字符串表示成功，否则为错误说明。
    QString CreateLogPackageZip(const QString& targetPath) const;

    void ShareLogs();

    void ShowUploadReserved();

    static QString BoolText(int value);

    QComboBox* language_combo_;
    QComboBox* preview_display_mode_combo_;
    QComboBox* publisher_source_combo_;
    QComboBox* publisher_camera_device_combo_;
    QLineEdit* publisher_file_path_edit_;
    QPushButton* publisher_file_browse_button_;
    QComboBox* publisher_audio_combo_;
    QLabel* publisher_audio_detail_label_;
    QComboBox* publisher_audio_source_combo_;
    QLabel* publisher_audio_file_label_;
    QLineEdit* publisher_audio_file_path_edit_;
    QPushButton* publisher_audio_file_browse_button_;
    QSlider* publisher_audio_volume_slider_;
    QLabel* publisher_audio_volume_value_label_;
    QComboBox* publisher_protocol_combo_;
    QLineEdit* publisher_url_edit_;
    QLineEdit* publisher_whip_bearer_token_edit_;
    QComboBox* publisher_video_codec_combo_;
    QComboBox* publisher_audio_codec_combo_;
    QComboBox* publisher_audio_profile_combo_;
    QComboBox* publisher_audio_sample_rate_combo_;
    QComboBox* publisher_audio_bitrate_combo_;
    QComboBox* publisher_rtmp_hevc_combo_;
    QComboBox* publisher_file_mode_combo_;
    QLabel* publisher_file_mode_label_;
    QCheckBox* publisher_preview_toggle_;
    QCheckBox* publisher_processor_compare_toggle_;
    QComboBox* publisher_resolution_combo_;
    QLineEdit* publisher_video_bitrate_edit_;
    QLineEdit* publisher_fps_edit_;
    QLineEdit* publisher_gop_edit_;
    QLabel* publisher_source_summary_label_;
    QWidget* publisher_preview_frame_;
    QLabel* publisher_original_preview_caption_;
    QWidget* publisher_preview_widget_;
    QLabel* publisher_preview_label_;
    QLabel* publisher_watermark_label_;
    QWidget* publisher_processed_preview_frame_;
    QWidget* publisher_processed_preview_column_;
    QWidget* publisher_processed_preview_widget_;
    QLabel* publisher_processed_preview_label_;
    QLabel* publisher_processed_watermark_label_;
    QLabel* publisher_status_label_;
    QWidget* publisher_video_detail_row_;
    QWidget* publisher_audio_detail_row_;
    QWidget* publisher_file_mode_row_;
    QWidget* publisher_whip_bearer_row_;
    QPlainTextEdit* publisher_runtime_log_;
    QPushButton* publisher_start_button_;
    QLineEdit* player_url_edit_;
    QComboBox* player_source_kind_combo_;
    QWidget* player_whep_options_panel_;
    QLineEdit* player_whep_bearer_token_edit_;
    QLineEdit* player_whep_local_bind_ip_edit_;
    QCheckBox* player_whep_allow_insecure_http_check_;
    QComboBox* player_latency_preset_combo_;
    QComboBox* player_decode_mode_combo_;
    QComboBox* player_render_path_combo_;
    QCheckBox* player_advanced_params_check_;
    QWidget* player_advanced_params_panel_;
    QLineEdit* player_buffer_ms_edit_;
    QLineEdit* player_audio_queue_edit_;
    QLineEdit* player_video_queue_edit_;
    QSlider* player_audio_volume_slider_;
    QLabel* player_audio_volume_value_label_;
    QLabel* player_onvif_hint_label_;
    QListWidget* player_onvif_device_list_;
    QLineEdit* player_onvif_username_edit_;
    QLineEdit* player_onvif_password_edit_;
    QPushButton* player_onvif_search_button_;
    QPushButton* player_onvif_apply_button_;
    QPushButton* player_start_button_;
    QLabel* player_status_label_;
    QLabel* player_first_frame_label_;
    QLabel* player_source_summary_label_;
    QPlainTextEdit* player_runtime_log_;
    QComboBox* gb28181_source_combo_;
    QComboBox* gb28181_video_source_combo_;
    QComboBox* gb28181_audio_combo_;
    QComboBox* gb28181_audio_source_combo_;
    QComboBox* gb28181_resolution_combo_;
    QLineEdit* gb28181_fps_edit_;
    QSlider* gb28181_audio_volume_slider_;
    QLabel* gb28181_audio_volume_value_label_;
    QLineEdit* gb28181_local_id_edit_;
    QLineEdit* gb28181_local_domain_edit_;
    QLineEdit* gb28181_local_port_edit_;
    QLineEdit* gb28181_upper_id_edit_;
    QLineEdit* gb28181_upper_domain_edit_;
    QLineEdit* gb28181_upper_password_edit_;
    QLineEdit* gb28181_upper_ip_edit_;
    QLineEdit* gb28181_upper_port_edit_;
    QComboBox* gb28181_upper_transport_combo_;
    QLineEdit* gb28181_media_port_edit_;
    QLabel* gb28181_source_summary_label_;
    QWidget* gb28181_preview_frame_;
    QLabel* gb28181_preview_label_;
    QLabel* gb28181_watermark_label_;
    QWidget* gb28181_video_detail_row_;
    QWidget* gb28181_audio_detail_row_;
    QPlainTextEdit* gb28181_runtime_log_;
    QPushButton* gb28181_start_button_;
    QTimer* gb28181_poll_timer_;
    QLabel* product_title_label_;
    QLabel* product_meta_label_;
    QLabel* license_status_label_;
    QLabel* license_feature_label_;
    QLabel* log_status_label_;
    QLabel* operation_status_label_;
    QLabel* machine_id_label_;
    QPushButton* share_logs_button_;
    QPushButton* upload_logs_button_;
    QWidget* desktop_render_target_frame_;
    QLabel* desktop_render_target_status_label_;
    QLabel* player_watermark_label_;
    QVBoxLayout* player_preview_layout_;
    QWidget* player_fullscreen_window_;
    QTableWidget* session_table_;
    QTableWidget* capability_table_;
    QWidget* desktop_render_target_widget_;
    streamcore_player_handle active_player_;
    streamcore_publisher_handle active_publisher_;
    streamcore_capture_handle active_publisher_capture_;
#if STREAMCORE_DEMO_ENABLE_GB28181
    // 接收上级 INVITE，自动应答后记录后续媒体发送目标。
    static void OnGb28181InviteReceived(
        const streamcore_gb28181_invite_t* invite,
        void* userContext);

    // 监听 GB28181 会话 ACTIVE 状态，触发当前 demo source 到 RTP/PS 的媒体桥。
    static void OnGb28181SessionUpdated(
        const streamcore_gb28181_session_info_t* session,
        void* userContext);

    // 在 GB28181 session active 后按当前 UI source 写入 SDK source binding。
    static void OnGb28181MediaRequest(
        const streamcore_gb28181_media_request_t* request,
        void* userContext);

    // 把 capture 音频 frame ref 转发给当前 GB28181 活动会话。
    static void OnGb28181AudioFrameRef(
        streamcore_audio_frame_ref frameRef,
        void* userContext);

    // 把 capture 视频 frame ref 转发给当前 GB28181 活动会话。
    static void OnGb28181VideoFrameRef(
        streamcore_video_frame_ref frameRef,
        void* userContext);

    // 使用当前 GB28181 source 启动 capture -> GB28181 媒体发送桥。
    void StartGb28181MediaBridge(
        const streamcore_gb28181_session_info_t& session);

    // 通过 SDK source binding 把当前 demo source 接入 GB28181 active request。
    void ConfigureGb28181SourceBinding(
        const streamcore_gb28181_media_request_t& request);

    // 停止当前 GB28181 媒体发送桥并清理目标快照。
    void StopGb28181MediaBridge();

    // 返回当前 GB28181 目标路由的 C 结构视图。
    streamcore_gb28181_stream_target_t CurrentGb28181Target() const;

    streamcore_gb28181_handle active_gb28181_;
    streamcore_capture_handle active_gb28181_media_capture_;
    int active_gb28181_source_index_;
    int active_gb28181_audio_index_;
    int active_gb28181_width_;
    int active_gb28181_height_;
    int active_gb28181_fps_;
    QByteArray active_gb28181_source_binding_;
    QByteArray active_gb28181_source_id_;
    QByteArray active_gb28181_audio_source_id_;
    QByteArray active_gb28181_target_device_id_;
    QByteArray active_gb28181_target_channel_id_;
    streamcore_gb28181_stream_kind_t active_gb28181_target_stream_kind_;
    std::atomic<qint64> active_gb28181_pushed_audio_packets_;
    std::atomic<qint64> active_gb28181_pushed_video_packets_;
#endif
    QByteArray active_player_url_;
    qint64 active_player_start_wall_time_ms_;
    QVector<OnvifDeviceItem> onvif_devices_;
    QString player_onvif_status_;
    QString current_machine_id_;
    QString log_directory_;
    QString log_file_name_;
    QString product_crash_directory_;
    QString latest_action_;
    QString latest_status_code_;
    QString latest_status_name_;
    QString latest_status_summary_;
    QString latest_log_zip_path_;
    bool runtime_ready_;
    DemoLanguage language_;
    bool rebuilding_ui_;
    bool player_fullscreen_active_;
};

#endif // _STREAMCORE_DEMO_QT_WINDOW_H_
