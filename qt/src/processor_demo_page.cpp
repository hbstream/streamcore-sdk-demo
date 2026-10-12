#include "processor_demo_page.h"

#include "processor_frame_adapter.h"
#include "processor_effect_catalog.h"
#if defined(Q_OS_MACOS)
#include "streamcore_demo_qt_macos_permissions.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <utility>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProcessEnvironment>
#include <QSignalBlocker>
#include <QSlider>
#include <QShowEvent>
#include <QTimer>
#include <QVariantMap>
#include <QVBoxLayout>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

ProcessorVideoWidget::ProcessorVideoWidget(QWidget* parent) : QWidget(parent)
{
    // The SDK presents directly to this native child window. Qt must not
    // erase or repaint its background between frames: doing so appears as a
    // dark flash, especially when a slow external effect lowers the frame rate.
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    setAttribute(Qt::WA_PaintOnScreen);
#endif
}

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
QPaintEngine* ProcessorVideoWidget::paintEngine() const { return nullptr; }
#endif

void ProcessorVideoWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
#if defined(Q_OS_WIN)
    // WA_NoSystemBackground prevents Qt from erasing SDK frames. Prime a new
    // HWND once, before the first SDK frame arrives, so a slow model load does
    // not reveal pixels from a previously visible native sibling window.
    // showEvent can run before the layout has assigned a nonzero client area.
    QTimer::singleShot(0, this, [this]() {
        HWND hwnd = reinterpret_cast<HWND>(winId());
        RECT rect = {};
        if (hwnd != nullptr && GetClientRect(hwnd, &rect))
        {
            HDC dc = GetDC(hwnd);
            if (dc != nullptr)
            {
                HBRUSH background = CreateSolidBrush(RGB(17, 24, 39));
                if (background != nullptr)
                {
                    FillRect(dc, &rect, background);
                    DeleteObject(background);
                }
                ReleaseDC(hwnd, dc);
            }
        }
    });
#endif
}

namespace
{
bool RenderTarget(QWidget* widget, streamcore_render_target_t* target)
{
    if (widget == nullptr || target == nullptr) return false;
    *target = {};
#if defined(Q_OS_WIN)
    target->platform_type = STREAMCORE_RENDER_PLATFORM_WINDOWS;
#elif defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    target->platform_type = STREAMCORE_RENDER_PLATFORM_APPLE;
#elif defined(Q_OS_LINUX)
    target->platform_type = STREAMCORE_RENDER_PLATFORM_LINUX;
#else
    target->platform_type = STREAMCORE_RENDER_PLATFORM_GENERIC;
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    target->target_type = STREAMCORE_RENDER_TARGET_TYPE_LAYER;
#else
    target->target_type = STREAMCORE_RENDER_TARGET_TYPE_NATIVE_WINDOW;
#endif
    target->native_handle = reinterpret_cast<void*>(widget->winId());
    target->width_hint = widget->width();
    target->height_hint = widget->height();
    return target->native_handle != nullptr;
}

} // namespace

QString ProcessorDemoPage::Tr(const char* english, const char* chinese) const
{
    return QString::fromUtf8(chinese_ ? chinese : english);
}

ProcessorDemoPage::ProcessorDemoPage(bool chinese, QWidget* parent)
    : QWidget(parent), chinese_(chinese)
{
    setObjectName(QString::fromUtf8("processor_page"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(8);

    auto* controls = new QWidget(this);
    auto* control_rows = new QVBoxLayout(controls);
    control_rows->setContentsMargins(0, 0, 0, 0);
    control_rows->setSpacing(6);
    auto* primary_row = new QHBoxLayout();
    primary_row->setSpacing(8);
    control_rows->addLayout(primary_row);
    auto* advanced_layout = new QHBoxLayout();
    advanced_layout->setContentsMargins(0, 0, 0, 0);
    advanced_layout->setSpacing(8);
    advanced_row_ = new QWidget(controls);
    advanced_row_->setLayout(advanced_layout);
    control_rows->addWidget(advanced_row_);
    auto* publish_row = new QHBoxLayout();
    publish_row->setSpacing(8);
    control_rows->addLayout(publish_row);
    primary_row->addWidget(new QLabel(Tr("Input", "输入"), controls));
    source_combo_ = new QComboBox(controls);
    source_combo_->setObjectName(QString::fromUtf8("processor_source_combo"));
    source_combo_->addItem(Tr("Local video", "本地视频"),
        QString::fromUtf8("file"));
    source_combo_->addItem(Tr("Camera", "摄像头"),
        QString::fromUtf8("camera"));
    source_combo_->setMaximumWidth(150);
    primary_row->addWidget(source_combo_);
    file_edit_ = new QLineEdit(controls);
    file_edit_->setObjectName(QString::fromUtf8("processor_file_edit"));
    auto* file_browse = new QPushButton(Tr("Browse", "浏览"), controls);
    file_row_ = new QWidget(controls);
    auto* file_layout = new QHBoxLayout(file_row_);
    file_layout->setContentsMargins(0, 0, 0, 0);
    file_layout->addWidget(file_edit_, 1);
    file_layout->addWidget(file_browse);
    source_detail_label_ = new QLabel(Tr("Video", "视频"), controls);
    primary_row->addWidget(source_detail_label_);
    primary_row->addWidget(file_row_, 2);
    connect(file_browse, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this,
            Tr("Select video", "选择视频文件"), QString(),
            Tr("Video files (*.mp4 *.mov *.mkv *.flv)",
               "视频文件 (*.mp4 *.mov *.mkv *.flv)"));
        if (!path.isEmpty()) file_edit_->setText(path);
    });
    camera_row_ = new QWidget(controls);
    auto* camera_layout = new QHBoxLayout(camera_row_);
    camera_layout->setContentsMargins(0, 0, 0, 0);
    camera_combo_ = new QComboBox(camera_row_);
    camera_combo_->setObjectName(QString::fromUtf8("processor_camera_combo"));
    auto* camera_refresh = new QPushButton(Tr("Refresh", "刷新"), camera_row_);
    camera_layout->addWidget(camera_combo_, 1);
    camera_layout->addWidget(camera_refresh);
    primary_row->addWidget(camera_row_, 2);
    connect(camera_refresh, &QPushButton::clicked, this,
        [this]() { RefreshCameraDevices(); });
    publish_row->addWidget(new QLabel(Tr("Audio", "音频"), controls));
    microphone_check_ = new QCheckBox(Tr("Microphone", "麦克风"), controls);
    microphone_check_->setObjectName(QString::fromUtf8("processor_microphone_check"));
    microphone_check_->setToolTip(Tr(
        "Available for camera publishing. Video files use their own audio track and cannot mix in a microphone.",
        "仅摄像头推流可用；视频文件使用自身音轨，当前不支持混入麦克风。"));
    audio_hint_label_ = new QLabel(Tr(
        "File audio (if present)", "文件原音轨（如有）"), controls);
    audio_hint_label_->setToolTip(Tr(
        "Video publishing uses the file's audio track; microphone mixing is unavailable.",
        "视频推流自动使用文件原音轨；暂不支持混入麦克风。"));
    publish_row->addWidget(microphone_check_);
    publish_row->addWidget(audio_hint_label_);
    connect(source_combo_, &QComboBox::currentIndexChanged, this,
        [this]() {
            const bool file = source_combo_->currentData().toString() ==
                QString::fromUtf8("file");
            const bool camera = source_combo_->currentData().toString() ==
                QString::fromUtf8("camera");
            source_detail_label_->setVisible(file || camera);
            source_detail_label_->setText(file ? Tr("Video", "视频") :
                Tr("Device", "设备"));
            file_row_->setVisible(file);
            camera_row_->setVisible(camera);
            if (camera) RefreshCameraDevices();
            UpdateSourceAudioUi();
        });
    camera_row_->hide();
    UpdateSourceAudioUi();

    primary_row->addWidget(new QLabel(Tr("Effect", "特效"), controls));
    algorithm_combo_ = new QComboBox(controls);
    algorithm_combo_->setObjectName(QString::fromUtf8("processor_algorithm_combo"));
    algorithm_combo_->addItem(Tr("Monochrome (built in)", "黑白（内置）"),
        QVariantMap{{QString::fromUtf8("kind"), QString::fromUtf8("monochrome")}});
    algorithm_combo_->addItem(Tr("External Python module", "外部 Python 模块"),
        QVariantMap{{QString::fromUtf8("kind"), QString::fromUtf8("external")}});
    primary_row->addWidget(algorithm_combo_, 1);
    module_edit_ = new QLineEdit(controls);
    module_edit_->setObjectName(QString::fromUtf8("processor_module_edit"));
    module_edit_->setPlaceholderText(Tr(
        "Select an external .py module or install an effect pack",
        "选择外部 .py 模块，或安装效果模块包"));
    module_row_ = new QWidget(controls);
    auto* module_layout = new QHBoxLayout(module_row_);
    module_layout->setContentsMargins(0, 0, 0, 0);
    auto* module_browse = new QPushButton(Tr("Browse", "浏览"), module_row_);
    module_layout->addWidget(module_edit_, 1);
    module_layout->addWidget(module_browse);
    module_label_ = new QLabel(Tr("Module", "模块"), advanced_row_);
    advanced_layout->addWidget(module_label_);
    advanced_layout->addWidget(module_row_, 1);
    connect(module_browse, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this,
            Tr("Select Python module", "选择 Python 模块"), QString(),
            QString::fromUtf8("Python (*.py);;All files (*)"));
        if (!path.isEmpty()) module_edit_->setText(path);
    });
    reference_edit_ = new QLineEdit(controls);
    reference_edit_->setObjectName(QString::fromUtf8("processor_reference_edit"));
    reference_edit_->setPlaceholderText(Tr(
        "Reference face image for the selected effect",
        "所选效果使用的参考人脸图片"));
    reference_row_ = new QWidget(controls);
    auto* reference_layout = new QHBoxLayout(reference_row_);
    reference_layout->setContentsMargins(0, 0, 0, 0);
    auto* reference_browse = new QPushButton(Tr("Browse", "浏览"), reference_row_);
    reference_layout->addWidget(reference_edit_, 1);
    reference_layout->addWidget(reference_browse);
    reference_label_ = new QLabel(Tr("Reference", "参考图"), advanced_row_);
    advanced_layout->addWidget(reference_label_);
    advanced_layout->addWidget(reference_row_, 1);
    connect(reference_browse, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this,
            Tr("Select reference image", "选择参考图片"), QString(),
            Tr("Images (*.png *.jpg *.jpeg);;All files (*)",
               "图片 (*.png *.jpg *.jpeg);;所有文件 (*)"));
        if (!path.isEmpty()) reference_edit_->setText(path);
    });
    strength_slider_ = new QSlider(Qt::Horizontal, controls);
    strength_slider_->setObjectName(QString::fromUtf8("processor_strength_slider"));
    strength_slider_->setRange(0, 100);
    strength_slider_->setValue(85);
    auto* strength_row = new QWidget(controls);
    auto* strength_layout = new QHBoxLayout(strength_row);
    strength_layout->setContentsMargins(0, 0, 0, 0);
    strength_value_ = new QLabel(QString::fromUtf8("85%"), strength_row);
    strength_layout->addWidget(strength_slider_, 1);
    strength_layout->addWidget(strength_value_);
    primary_row->addWidget(new QLabel(Tr("Strength", "强度"), controls));
    primary_row->addWidget(strength_row, 1);
    connect(strength_slider_, &QSlider::valueChanged, this, [this](int value) {
        strength_value_->setText(QString::fromUtf8("%1%").arg(value));
    });
    publish_row->addWidget(new QLabel(Tr("RTMP URL", "RTMP 地址"), controls));
    url_edit_ = new QLineEdit(controls);
    url_edit_->setObjectName(QString::fromUtf8("processor_url_edit"));
    url_edit_->setPlaceholderText(QString::fromUtf8("rtmp://127.0.0.1/live/processor"));
    publish_row->addWidget(url_edit_, 1);
    publish_check_ = new QCheckBox(Tr("Publish processed video", "推流处理后视频"), controls);
    publish_check_->setObjectName(QString::fromUtf8("processor_publish_check"));
    publish_row->addWidget(publish_check_);
    auto* button_row = new QWidget(controls);
    auto* buttons = new QHBoxLayout(button_row);
    buttons->setContentsMargins(0, 0, 0, 0);
    start_button_ = new QPushButton(Tr("Start", "开始"), button_row);
    stop_button_ = new QPushButton(Tr("Stop", "停止"), button_row);
    start_button_->setMinimumWidth(72);
    stop_button_->setMinimumWidth(72);
    stop_button_->setEnabled(false);
    buttons->setSpacing(6);
    buttons->addWidget(start_button_);
    buttons->addWidget(stop_button_);
    publish_row->addWidget(button_row);
    status_label_ = new QLabel(Tr(
        "Select an input and algorithm, then press Start.",
        "选择输入和算法后，点击开始。"), this);
    status_label_->setWordWrap(true);
    status_label_->setMinimumHeight(26);
    pack_hint_label_ = new QLabel(Tr(
        "Only monochrome processing is built in. <a href=\"https://hbrun.com/en/about/?topic=streamcore-ai-effects#contact\">Contact HBRun for the optional AI effects extension</a> to try beauty, face swap, and body shaping. Extract its processor_pack/ and runtime/python/ folders beside this executable, then restart.",
        "当前仅内置黑白处理。体验美颜、换脸和身形调整，请<a href=\"https://hbrun.cn/about/?topic=streamcore-ai-effects#contact\">联系 HBRun 获取 AI 特效扩展包</a>。将包内 processor_pack/ 和 runtime/python/ 文件夹解压到本程序目录，重启后即可使用。"), this);
    pack_hint_label_->setWordWrap(true);
    pack_hint_label_->setOpenExternalLinks(true);
    pack_hint_label_->setStyleSheet(QString::fromUtf8(
        "QLabel { background: #eaf4ff; color: #17365d; padding: 7px; }"));
    stats_label_ = new QLabel(this);
    stats_label_->setWordWrap(true);
    stats_label_->setMinimumHeight(26);
    LoadEffectPack();
    connect(algorithm_combo_, &QComboBox::currentIndexChanged, this,
        [this]() { SelectAlgorithm(); });
    SelectAlgorithm();
    auto* previews = new QHBoxLayout();
    previews->setSpacing(12);
    for (int index = 0; index < 2; ++index)
    {
        auto* column = new QVBoxLayout();
        auto* caption = new QLabel(index == 0 ?
            Tr("Original", "处理前") : Tr("Processed", "处理后"), this);
        auto* video = new ProcessorVideoWidget(this);
        video->setMinimumSize(320, 250);
        column->addWidget(caption);
        column->addWidget(video, 1);
        previews->addLayout(column, 1);
        if (index == 0) original_video_ = video;
        else processed_video_ = video;
    }
    // Keep the file/effect controls and run state visible at the same time as
    // both preview surfaces. A fixed-height scroll area previously hid the
    // status and statistics below the fold at the normal 1280x930 window size.
    root->addWidget(controls);
    root->addWidget(pack_hint_label_);
    root->addWidget(status_label_);
    root->addLayout(previews, 1);
    root->addWidget(stats_label_);
    connect(start_button_, &QPushButton::clicked, this, [this]() { Start(); });
    connect(stop_button_, &QPushButton::clicked, this, [this]() { Stop(); });
    stats_timer_ = new QTimer(this);
    stats_timer_->setInterval(1000);
    connect(stats_timer_, &QTimer::timeout, this, [this]() { UpdateStats(); });
}

ProcessorDemoPage::~ProcessorDemoPage() { Stop(); }

void ProcessorDemoPage::SetStatus(const QString& message)
{
    status_label_->setText(message);
    std::fprintf(stderr, "Processor page: %s\n", message.toUtf8().constData());
}

void ProcessorDemoPage::LoadEffectPack()
{
    const demo_processor::EffectPack pack =
        demo_processor::ProcessorEffectCatalog::Load(chinese_);
    if (pack.state == demo_processor::EffectPack::State::Missing) return;
    if (pack.state == demo_processor::EffectPack::State::Invalid)
    {
        pack_hint_label_->setText(Tr(
            "An effect pack was found, but its manifest, module, or sample files are incomplete. Extract the complete add-on beside this executable and restart the demo.",
            "检测到效果扩展包，但清单、模块或内置素材不完整。请将完整扩展包解压到本程序目录，再重启 Demo。"));
        return;
    }

    pack_module_path_ = pack.module_path;
    const int first_effect = algorithm_combo_->count();
    for (const demo_processor::EffectDescription& effect : pack.effects)
    {
        QVariantMap data;
        data.insert(QString::fromUtf8("kind"), QString::fromUtf8("pack"));
        data.insert(QString::fromUtf8("effect_id"), effect.id);
        data.insert(QString::fromUtf8("module"), pack.module_path);
        data.insert(QString::fromUtf8("sample"), effect.sample_path);
        data.insert(QString::fromUtf8("reference"), effect.reference_path);
        data.insert(QString::fromUtf8("strength"), effect.strength);
        algorithm_combo_->addItem(effect.name, data);
    }
    pack_hint_label_->hide();
    source_combo_->insertItem(0, Tr("Included sample", "内置素材"),
        QString::fromUtf8("sample"));
    source_combo_->setCurrentIndex(0);
    algorithm_combo_->setCurrentIndex(first_effect);
}

void ProcessorDemoPage::SelectAlgorithm()
{
    const QVariantMap data = algorithm_combo_->currentData().toMap();
    const QString kind = data.value(QString::fromUtf8("kind")).toString();
    const bool packed = kind == QString::fromUtf8("pack");
    if (packed)
    {
        module_edit_->setText(data.value(QString::fromUtf8("module")).toString());
        sample_path_ = data.value(QString::fromUtf8("sample")).toString();
        reference_edit_->setText(data.value(QString::fromUtf8("reference")).toString());
        strength_slider_->setValue(std::lround(data.value(QString::fromUtf8("strength"))
            .toDouble() * 100.0));
        SetStatus(Tr("Included sample and effect are ready. Press Start.",
            "内置素材和效果已准备好，点击开始即可查看。"));
    }
    else if (kind == QString::fromUtf8("external") &&
        module_edit_->text() == pack_module_path_)
    {
        module_edit_->clear();
        reference_edit_->clear();
    }
    const bool needs_reference = packed &&
        !data.value(QString::fromUtf8("reference")).toString().isEmpty();
    const bool show_module = kind == QString::fromUtf8("external");
    const bool show_reference = needs_reference || show_module;
    module_label_->setVisible(show_module);
    module_row_->setVisible(show_module);
    reference_label_->setVisible(show_reference);
    reference_row_->setVisible(show_reference);
    advanced_row_->setVisible(show_module || show_reference);
}

void ProcessorDemoPage::RefreshCameraDevices()
{
    const QString previous_id = camera_combo_->currentData().toString();
    const QSignalBlocker blocker(camera_combo_);
    camera_combo_->clear();
    streamcore_capture_source_info_t sources[32] = {};
    size_t source_count = 0;
    const streamcore_result_t result = streamcore_capture_copy_source_list(
        STREAMCORE_CAPTURE_SOURCE_KIND_CAMERA, sources,
        sizeof(sources) / sizeof(sources[0]), &source_count, nullptr, 0);
    if (result != STREAMCORE_RESULT_OK)
    {
        camera_combo_->addItem(Tr("Camera enumeration failed", "摄像头枚举失败"));
        camera_combo_->setEnabled(false);
        return;
    }
    int default_index = -1;
    for (size_t index = 0; index < source_count; ++index)
    {
        if (sources[index].source_kind != STREAMCORE_CAPTURE_SOURCE_KIND_CAMERA)
            continue;
        const QString id = QString::fromUtf8(sources[index].source_id);
        const QString name = QString::fromUtf8(sources[index].source_name);
        camera_combo_->addItem(name.isEmpty() ? id : name, id);
        if (id == previous_id && !id.isEmpty())
            default_index = camera_combo_->count() - 1;
        else if (default_index < 0 && sources[index].is_default != 0)
            default_index = camera_combo_->count() - 1;
    }
    if (camera_combo_->count() == 0)
    {
        camera_combo_->addItem(Tr("No camera found", "未发现摄像头"));
        camera_combo_->setEnabled(false);
    }
    else
    {
        camera_combo_->setEnabled(true);
        camera_combo_->setCurrentIndex(std::max(0, default_index));
    }
}

void ProcessorDemoPage::UpdateSourceAudioUi()
{
    // MEDIA_FILE already owns both tracks. The SDK ignores audio_source_kind
    // for this source, so disable the microphone option instead of implying
    // that the current Capture path can mix it into file audio.
    const bool camera = source_combo_->currentData().toString() ==
        QString::fromUtf8("camera");
    microphone_check_->setEnabled(camera);
    if (!camera) microphone_check_->setChecked(false);
    audio_hint_label_->setVisible(!camera);
}

void ProcessorDemoPage::StartFromEnvironment()
{
    const QString source = qEnvironmentVariable(
        "STREAMCORE_DEMO_QT_PROCESSOR_SOURCE").trimmed().toLower();
    const QString file = qEnvironmentVariable("STREAMCORE_DEMO_QT_PROCESSOR_FILE");
    if (!file.isEmpty()) file_edit_->setText(file);
    const QString requested_source = source == QString::fromUtf8("camera") ? source :
        (!file.isEmpty() || source == QString::fromUtf8("file")) ?
        QString::fromUtf8("file") : source;
    const int source_index = source_combo_->findData(requested_source);
    if (source_index >= 0) source_combo_->setCurrentIndex(source_index);
    const QString camera_id = qEnvironmentVariable(
        "STREAMCORE_DEMO_QT_PROCESSOR_CAMERA_ID");
    if (!camera_id.isEmpty())
    {
        const int index = camera_combo_->findData(camera_id);
        if (index >= 0) camera_combo_->setCurrentIndex(index);
    }
    url_edit_->setText(qEnvironmentVariable("STREAMCORE_DEMO_QT_PROCESSOR_URL"));
    const QString effect = qEnvironmentVariable(
        "STREAMCORE_DEMO_QT_PROCESSOR_EFFECT").trimmed().toLower();
    for (int index = 0; index < algorithm_combo_->count(); ++index)
    {
        if (algorithm_combo_->itemData(index).toMap().value(
                QString::fromUtf8("effect_id")).toString() == effect &&
            !effect.isEmpty())
        {
            algorithm_combo_->setCurrentIndex(index);
            break;
        }
    }
    const QString module = qEnvironmentVariable("STREAMCORE_DEMO_QT_PROCESSOR_MODULE");
    const QString reference = qEnvironmentVariable(
        "STREAMCORE_DEMO_QT_PROCESSOR_REFERENCE_IMAGE");
    const QString algorithm = qEnvironmentVariable(
        "STREAMCORE_DEMO_QT_PROCESSOR_ALGORITHM").trimmed().toLower();
    if (algorithm == QString::fromUtf8("monochrome"))
        algorithm_combo_->setCurrentIndex(0);
    else if (algorithm == QString::fromUtf8("external"))
        algorithm_combo_->setCurrentIndex(1);
    if (!module.isEmpty()) module_edit_->setText(module);
    if (!reference.isEmpty()) reference_edit_->setText(reference);
    bool strength_ok = false;
    const int strength = qEnvironmentVariableIntValue(
        "STREAMCORE_DEMO_QT_PROCESSOR_STRENGTH", &strength_ok);
    if (strength_ok) strength_slider_->setValue(std::clamp(strength, 0, 100));
    const QString microphone = qEnvironmentVariable(
        "STREAMCORE_DEMO_QT_PROCESSOR_MICROPHONE").trimmed().toLower();
    microphone_check_->setChecked(
        source_combo_->currentData().toString() == QString::fromUtf8("camera") &&
        (microphone == QString::fromUtf8("1") ||
         microphone == QString::fromUtf8("true")));
    publish_check_->setChecked(!url_edit_->text().isEmpty());
    Start();
}

bool ProcessorDemoPage::Start()
{
    Stop();
    const QString source = source_combo_->currentData().toString();
    const bool camera_source = source == QString::fromUtf8("camera");
    const QString path = source == QString::fromUtf8("sample") ? sample_path_ :
        QFileInfo(file_edit_->text().trimmed()).absoluteFilePath();
    if (!camera_source && !QFileInfo(path).isFile())
    {
        SetStatus(Tr("Select an existing video file.",
            "请选择存在的视频文件。"));
        return false;
    }
    if (!camera_source)
    {
        // The frozen 1.6.2 media probe may report a JPEG as a video track.
        // Keep this page's input contract explicit before asking the SDK.
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix != QString::fromUtf8("mp4") &&
            suffix != QString::fromUtf8("mov") &&
            suffix != QString::fromUtf8("mkv") &&
            suffix != QString::fromUtf8("flv"))
        {
            SetStatus(Tr("Select an MP4, MOV, MKV, or FLV video file.",
                "请选择 MP4、MOV、MKV 或 FLV 视频文件。"));
            return false;
        }
    }
    streamcore_media_file_profile_t file_profile = {};
    if (!camera_source)
    {
        const QByteArray probe_path = QDir::toNativeSeparators(path).toUtf8();
        char probe_error[STREAMCORE_TEXT_CAPACITY] = {};
        const streamcore_result_t probe_result = streamcore_probe_media_file(
            probe_path.constData(), &file_profile, probe_error,
            sizeof(probe_error));
        if (probe_result != STREAMCORE_RESULT_OK ||
            file_profile.is_still_image != 0 || file_profile.has_video == 0)
        {
            SetStatus(Tr("Select a readable video file with a video track; still images are not supported.",
                "请选择可读取且包含视频轨道的视频文件；此页不支持静态图片。"));
            return false;
        }
    }
    if (camera_source && !camera_combo_->isEnabled())
    {
        SetStatus(Tr("No camera is available. Refresh the device list.",
            "没有可用摄像头，请刷新设备列表。"));
        return false;
    }
#if defined(Q_OS_MACOS)
    if (camera_source)
    {
        char permission_error[512] = {};
        if (!StreamCoreDemoQtRequestMacCapturePermissions(false, true,
                publish_check_->isChecked() && microphone_check_->isChecked(),
                60000, permission_error, sizeof(permission_error)))
        {
            SetStatus(QString::fromUtf8(permission_error));
            return false;
        }
    }
#endif
    const QVariantMap selected_effect = algorithm_combo_->currentData().toMap();
    const QString effect_kind = selected_effect.value(
        QString::fromUtf8("kind")).toString();
    const bool external = effect_kind != QString::fromUtf8("monochrome");
    QProcessEnvironment worker_environment = QProcessEnvironment::systemEnvironment();
    if (external)
    {
        const QString module = module_edit_->text().trimmed();
        const QString reference = reference_edit_->text().trimmed();
        const bool needs_reference = effect_kind == QString::fromUtf8("pack") &&
            !selected_effect.value(QString::fromUtf8("reference")).toString().isEmpty();
        if (!QFileInfo(module).isFile() ||
            (needs_reference && !QFileInfo(reference).isFile()))
        {
            SetStatus(Tr("Processor module or reference image was not found.",
                "找不到处理模块或参考图片。"));
            return false;
        }
        // The generic worker reads one session-specific environment. Clear
        // inherited choices first so switching effects cannot reuse a stale
        // face-swap reference or effect ID from an earlier session.
        worker_environment.remove(QString::fromUtf8("STREAMCORE_PROCESSOR_EFFECT"));
        worker_environment.remove(QString::fromUtf8("STREAMCORE_PROCESSOR_SOURCE_IMAGE"));
        worker_environment.insert(QString::fromUtf8("STREAMCORE_DEMO_QT_PROCESSOR_MODULE"), module);
        if (effect_kind == QString::fromUtf8("pack"))
            worker_environment.insert(QString::fromUtf8("STREAMCORE_PROCESSOR_EFFECT"),
                selected_effect.value(QString::fromUtf8("effect_id")).toString());
        worker_environment.insert(QString::fromUtf8("STREAMCORE_PROCESSOR_STRENGTH"),
            QString::number(strength_slider_->value() / 100.0, 'f', 2));
        if (!reference.isEmpty())
            worker_environment.insert(QString::fromUtf8("STREAMCORE_PROCESSOR_SOURCE_IMAGE"), reference);
    }
    auto frame_adapter = std::make_unique<demo_processor::ProcessorFrameAdapter>(
        external ? demo_processor::ProcessorFrameAdapter::Mode::ExternalWorker :
            demo_processor::ProcessorFrameAdapter::Mode::Monochrome,
        std::move(worker_environment));
    if (external)
    {
        SetStatus(Tr("Loading processor model...", "正在加载处理模型……"));
        QCoreApplication::processEvents();
        const QString warmup_error = frame_adapter->ProbeAndWarmUp();
        if (!warmup_error.isEmpty())
        {
            SetStatus(warmup_error);
            return false;
        }
    }

    const QByteArray file = QDir::toNativeSeparators(path).toUtf8();
    const QByteArray camera_id = camera_combo_->currentData().toString().toUtf8();
    QByteArray binding = camera_source ?
        QByteArray("camera:") + camera_id : QByteArray("media_file:") + file;
    const QByteArray url = url_edit_->text().trimmed().toUtf8();
    const QByteArray capture_name("qt_processor_capture");
    const QByteArray publisher_name("qt_processor_publisher");
    const bool publish = publish_check_->isChecked();
    const bool microphone = publish && camera_source && microphone_check_->isChecked();
    const bool file_audio = publish && !camera_source && file_profile.has_audio != 0;
    const bool publish_audio = microphone || file_audio;
    if (microphone) binding += QByteArray("+microphone");
    if (publish && !url.startsWith("rtmp://"))
    {
        SetStatus(Tr("Enter an RTMP URL or disable publishing.",
            "请输入 RTMP 地址，或关闭推流。"));
        return false;
    }

    streamcore_result_t result = STREAMCORE_RESULT_OK;
    char error[STREAMCORE_TEXT_CAPACITY] = {};
    char capture_error[STREAMCORE_TEXT_CAPACITY] = {};
    streamcore_publisher_preflight_t pub_preflight = {};
    streamcore_capture_preflight_t cap_preflight = {};
    streamcore_render_target_t original_target = {};
    streamcore_render_target_t processed_target = {};
    if (publish)
    {
        streamcore_publisher_config_t config;
        streamcore_publisher_get_default_config(&config);
        config.session_name = publisher_name.constData();
        config.publish_url = url.constData();
        config.input_kind = STREAMCORE_PUBLISHER_INPUT_KIND_APP_RAW_FEED;
        config.input_binding_id = binding.constData();
        config.enable_video = 1;
        config.enable_audio = publish_audio ? 1 : 0;
        if (publish_audio)
        {
            config.transcode.audio_codec_name = "aac";
            config.transcode.audio_mode =
                STREAMCORE_PUBLISHER_TRANSCODE_MODE_FORCE_TRANSCODE;
            config.transcode.target_audio_bitrate_kbps = 128;
            config.transcode.target_sample_rate = 48000;
            config.transcode.target_channel_count = 2;
        }
        config.transcode.video_codec_name = "h264";
        config.transcode.video_mode =
            STREAMCORE_PUBLISHER_TRANSCODE_MODE_FORCE_TRANSCODE;
        config.transcode.target_video_bitrate_kbps = 1200;
        config.transcode.target_fps = camera_source ? 15 :
            source == QString::fromUtf8("sample") ? 10 : 25;
        config.transcode.target_gop_frames =
            source == QString::fromUtf8("sample") ? 10 :
            camera_source ? 30 : 50;
        result = streamcore_publisher_create_with_config(
            &config, &publisher_, error, sizeof(error));
        if (result == STREAMCORE_RESULT_OK)
            result = streamcore_publisher_preflight(
                publisher_, &pub_preflight, error, sizeof(error));
        if (result == STREAMCORE_RESULT_OK && pub_preflight.is_ready_to_start == 0)
            result = STREAMCORE_RESULT_OPERATION_FAILED;
    }
    if (result == STREAMCORE_RESULT_OK &&
        (!RenderTarget(original_video_, &original_target) ||
         !RenderTarget(processed_video_, &processed_target)))
        result = STREAMCORE_RESULT_OPERATION_FAILED;
    if (result == STREAMCORE_RESULT_OK)
    {
        capture_ = streamcore_capture_create();
        if (capture_ == nullptr) result = STREAMCORE_RESULT_OPERATION_FAILED;
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        // The adapter must stay alive from set_config through Capture destroy.
        // The callback may finish asynchronously after it returns to the SDK.
        streamcore_capture_config_t config;
        streamcore_capture_get_default_config(&config);
        config.session_name = capture_name.constData();
        config.source_kind = camera_source ?
            STREAMCORE_CAPTURE_SOURCE_KIND_CAMERA :
            STREAMCORE_CAPTURE_SOURCE_KIND_MEDIA_FILE;
        config.source_id = camera_source ?
            (camera_id.isEmpty() ? nullptr : camera_id.constData()) :
            file.constData();
        config.enable_video = 1;
        config.enable_audio = publish_audio ? 1 : 0;
        if (microphone)
            config.audio_source_kind = STREAMCORE_CAPTURE_SOURCE_KIND_MICROPHONE;
        config.preferred_fps = camera_source ? 15 :
            source == QString::fromUtf8("sample") ? 10 : 25;
        if (camera_source)
        {
            config.preferred_width = 640;
            config.preferred_height = 480;
        }
        config.preview_mode = STREAMCORE_CAPTURE_PREVIEW_MODE_AUTO;
        config.video_processor_callback = demo_processor::ProcessorFrameAdapter::Callback;
        config.video_processor_callback_context = frame_adapter.get();
        config.video_processor_failure_mode =
            external ? STREAMCORE_PROCESSOR_FAILURE_MODE_DROP :
            STREAMCORE_PROCESSOR_FAILURE_MODE_PASSTHROUGH;
        bool deadline_ok = false;
        const int deadline = qEnvironmentVariableIntValue(
            "STREAMCORE_DEMO_QT_PROCESSOR_DEADLINE_MS", &deadline_ok);
        config.video_processor_max_duration_ms = deadline_ok ?
            std::clamp(deadline, 1, 60000) : 30000;
        result = streamcore_capture_set_config(capture_, &config);
        if (result == STREAMCORE_RESULT_OK)
            result = streamcore_capture_set_preview_render_target(
                capture_, &original_target);
        if (result == STREAMCORE_RESULT_OK)
            result = streamcore_capture_set_processed_preview_render_target(
                capture_, &processed_target);
        if (result == STREAMCORE_RESULT_OK && publish)
            result = streamcore_capture_connect_publisher(capture_, publisher_);
        if (result == STREAMCORE_RESULT_OK)
            result = streamcore_capture_preflight(capture_, &cap_preflight,
                capture_error, sizeof(capture_error));
        if (result == STREAMCORE_RESULT_OK && cap_preflight.is_ready_to_start == 0)
            result = STREAMCORE_RESULT_OPERATION_FAILED;
    }
    if (result == STREAMCORE_RESULT_OK && publish)
        result = streamcore_publisher_start(publisher_, error, sizeof(error));
    if (result == STREAMCORE_RESULT_OK)
        result = streamcore_capture_start(capture_, capture_error,
            sizeof(capture_error));
    if (result != STREAMCORE_RESULT_OK)
    {
        const QString detail = capture_error[0] ? QString::fromUtf8(capture_error) :
            error[0] ? QString::fromUtf8(error) :
            capture_ ? QString::fromUtf8(cap_preflight.detail) :
            QString::fromUtf8(pub_preflight.detail);
        Stop();
        SetStatus(Tr("Start failed", "启动失败") +
            QString::fromUtf8(" [%1]: %2").arg(result).arg(detail));
        return false;
    }
    frame_adapter_ = std::move(frame_adapter);
    SetStatus(publish ? Tr("Processing and publishing.", "正在处理并推流。") :
        Tr("Processing preview.", "正在处理预览。"));
    start_button_->setEnabled(false);
    stop_button_->setEnabled(true);
    stats_last_replaced_ = 0;
    stats_clock_.start();
    stats_timer_->start();
    return true;
}

void ProcessorDemoPage::Stop()
{
    if (stats_timer_ != nullptr) stats_timer_->stop();
    stats_clock_.invalidate();
    // Stop and destroy Capture first. Its callback context points into
    // frame_adapter_, whose queued finishes and child process are drained only
    // after no new callbacks can arrive.
    if (capture_ != nullptr)
    {
        streamcore_capture_stop(capture_);
        streamcore_capture_destroy(capture_);
        capture_ = nullptr;
    }
    if (publisher_ != nullptr)
    {
        streamcore_publisher_stop(publisher_);
        streamcore_publisher_destroy(publisher_);
        publisher_ = nullptr;
    }
    frame_adapter_.reset();
    if (start_button_ != nullptr) start_button_->setEnabled(true);
    if (stop_button_ != nullptr) stop_button_->setEnabled(false);
}

void ProcessorDemoPage::UpdateStats()
{
    if (capture_ == nullptr) return;
    streamcore_capture_runtime_info_t info = {};
    if (streamcore_capture_get_runtime_info(capture_, &info) !=
        STREAMCORE_RESULT_OK) return;
    const qint64 elapsed_ms = stats_clock_.isValid() ? stats_clock_.restart() : 0;
    const qint64 replaced = info.video_processor_replaced_frame_count;
    if (replaced > 0 && stats_last_replaced_ == 0)
        SetStatus(publisher_ != nullptr ?
            Tr("Processing and publishing.", "正在处理并推流。") :
            Tr("Processing preview.", "正在处理预览。"));
    const double output_fps = elapsed_ms > 0 ?
        1000.0 * static_cast<double>(replaced - stats_last_replaced_) /
            static_cast<double>(elapsed_ms) : 0.0;
    stats_last_replaced_ = replaced;
    stats_label_->setText(Tr("Output %1 fps (auto) | Input %2 | Replaced %3 | Dropped %4 | Error %5 | Overrun %6",
        "输出 %1 帧/秒（自动）| 输入 %2 | 已处理 %3 | 丢帧 %4 | 错误 %5 | 超时 %6")
        .arg(output_fps, 0, 'f', 1)
        .arg(info.video_processor_input_frame_count)
        .arg(info.video_processor_replaced_frame_count)
        .arg(info.video_processor_dropped_frame_count)
        .arg(info.video_processor_error_count)
        .arg(info.video_processor_overrun_count));
    std::fprintf(stderr,
        "Processor stats: input=%lld replaced=%lld dropped=%lld pass=%lld error=%lld overrun=%lld\n",
        static_cast<long long>(info.video_processor_input_frame_count),
        static_cast<long long>(info.video_processor_replaced_frame_count),
        static_cast<long long>(info.video_processor_dropped_frame_count),
        static_cast<long long>(info.video_processor_passthrough_frame_count),
        static_cast<long long>(info.video_processor_error_count),
        static_cast<long long>(info.video_processor_overrun_count));
    if (publisher_ != nullptr)
    {
        streamcore_publisher_runtime_info_t publisher_info = {};
        if (streamcore_publisher_get_runtime_info(publisher_,
                &publisher_info) == STREAMCORE_RESULT_OK)
        {
            std::fprintf(stderr, "Processor publisher: raw=%d state=%s\n",
                publisher_info.accepted_raw_input,
                publisher_info.state_summary);
        }
    }
}
