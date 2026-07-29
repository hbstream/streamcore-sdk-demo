// StreamCore desktop Qt demo main window implementation.
//
// This file intentionally keeps the demo host wiring in one translation unit:
// it owns the visible controls, native SDK session handles, autorun entrypoints,
// and the preview/status text policy used by Windows, Linux, and macOS Qt builds.
#include "streamcore_demo_qt_window.h"

#if defined(Q_OS_MACOS)
#include "streamcore_demo_qt_macos_permissions.h"
#endif

#include "streamcore/streamcore_sdk.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include <QAbstractItemView>
#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QImageReader>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QLocale>
#include <QMetaObject>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRunnable>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QSlider>
#include <QStatusBar>
#include <QStandardPaths>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QThreadPool>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

namespace
{
constexpr qint64 kLogPackageMaxBytes = 20LL * 1024LL * 1024LL;
constexpr qint64 kLogPackageSingleFileMaxBytes = 8LL * 1024LL * 1024LL;
constexpr int kLogPackageMaxFiles = 24;

constexpr int kPublisherSourceCamera = 0;
constexpr int kPublisherSourceDesktop = 1;
constexpr int kPublisherSourceVideoFile = 2;
constexpr int kPublisherSourceImage = 3;
constexpr int kPublisherSourceNone = 4;

constexpr int kPublisherAudioNone = 0;
constexpr int kPublisherAudioMicrophone = 1;
constexpr int kPublisherAudioSystem = 2;
constexpr int kPublisherAudioFile = 3;

constexpr int kGb28181AudioNone = 0;
constexpr int kGb28181AudioMicrophone = 1;
constexpr int kGb28181AudioSystem = 2;

constexpr int kPublisherFileModeAuto = 0;
constexpr int kPublisherFileModeForceTranscode = 1;

constexpr const char* kPublisherPreviewLabelDefaultStyle =
    "QLabel { background: #111827; color: #DDE6EF; border: 0; padding: 16px; "
    "font-size: 16px; }";
constexpr const char* kPublisherPreviewLabelWarningStyle =
    "QLabel { background: #7C2D12; color: #FFF7ED; border: 2px solid #FDBA74; "
    "padding: 20px; font-size: 18px; font-weight: 700; }";
constexpr const char* kDemoWatermarkLabelStyle =
    "QLabel#demo_watermark_label { background: rgba(17, 24, 39, 165); "
    "color: #FFFFFF; border: 1px solid rgba(255, 255, 255, 70); "
    "border-radius: 6px; padding: 4px 8px; font-size: 9pt; font-weight: 600; }";
constexpr double kPreviewFrameAspectRatio = 16.0 / 9.0;
constexpr int kPreviewFrameMinHeight = 240;
constexpr int kSidebarWidth = 430;
constexpr int kRuntimeLogHeight = 150;

constexpr int kGb28181SourceCamera = 0;
constexpr int kGb28181SourceDesktop = 1;

int ComboValueOrIndex(const QComboBox* combo, int fallback)
{
    if (combo == nullptr)
    {
        return fallback;
    }

    bool ok = false;
    const int value = combo->currentData().toInt(&ok);
    return ok ? value : combo->currentIndex();
}

QString ToQString(const char* text)
{
    return QString::fromUtf8(text != nullptr ? text : "");
}

QString CapabilitySummaryText(
    const streamcore_capability_descriptor_t& item,
    bool useChinese)
{
    struct CapabilitySummary
    {
        const char* key;
        const char* english;
        const char* chinese;
    };

    static const CapabilitySummary kSummaries[] = {
        {
            "p2_core",
            "Core publishing, playback, capture, recording, mixing, and stream rendering.",
            "推流、播放、采集、录制、混流与渲染主干。"
        },
        {
            "p2_srt",
            "SRT pull and publish support.",
            "SRT 拉流和推流支持。"
        },
        {
            "p2_whip",
            "WHIP publishing with the H.264 / Opus media contract.",
            "WHIP 推流，媒体编码要求为 H.264 / Opus。"
        },
        {
            "p2_player_low_latency",
            "Low-latency buffer, no-cache, and realtime playback profile parameters.",
            "低延迟缓冲、无缓存与实时播放档位参数。"
        },
        {
            "p2_player_hardware_path",
            "GPU frame, direct-surface, and hardware render path options.",
            "GPU 帧、Direct Surface 与硬件渲染路径。"
        },
        {
            "p2_player_advanced_events",
            "Audio spectrum, audio/video frame, and playback-network status callbacks.",
            "音频频谱、音视频帧与播放网络状态回调。"
        },
        {
            "p2_publisher_advanced_transcode",
            "Advanced publish transcode strategy, target codec parameters, and hardware encoder path control.",
            "高级推流转码策略、目标编码参数与硬件编码路径控制。"
        },
        {
            "p2_capture_backend_control",
            "Explicit capture backend selection, fallback, and diagnostics.",
            "显式采集后端选择、回退与诊断。"
        },
        {
            "p2_shared_source",
            "Shared source runtime for one-source/multi-consumer capture or stream distribution.",
            "一源多消费者的共享采集或分发运行时。"
        },
        {
            "p2_gb28181",
            "GB28181 device-side signaling, platform callbacks, and packet-first media boundary.",
            "GB28181 设备侧信令、平台回调与包级媒体边界。"
        },
        {
            "p2_device_access_onvif",
            "ONVIF discovery and Profile S/T search flags.",
            "ONVIF 发现与 Profile S/T 搜索标志。"
        }
    };

    for (const CapabilitySummary& summary : kSummaries)
    {
        if (std::strcmp(item.capability_key, summary.key) == 0)
        {
            return QString::fromUtf8(useChinese ? summary.chinese : summary.english);
        }
    }
    return ToQString(item.summary);
}

QString EnvironmentText(const char* name)
{
    const QString env_value = QString::fromUtf8(qgetenv(name)).trimmed();
    if (!env_value.isEmpty())
    {
        return env_value;
    }

    QString key = QString::fromUtf8(name);
    const QString prefix = QString::fromUtf8("STREAMCORE_DEMO_QT_");
    if (key.startsWith(prefix))
    {
        key = key.mid(prefix.size());
    }
    key = key.toLower().replace(QLatin1Char('_'), QLatin1Char('-'));

    const QString short_option = QString::fromUtf8("--%1").arg(key);
    const QString full_option = QString::fromUtf8("--%1")
        .arg(QString::fromUtf8(name).toLower().replace(
            QLatin1Char('_'),
            QLatin1Char('-')));
    const QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i)
    {
        const QString arg = args.at(i);
        for (const QString& option : {short_option, full_option})
        {
            if (arg == option && i + 1 < args.size())
            {
                return args.at(i + 1).trimmed();
            }
            const QString option_prefix = option + QLatin1Char('=');
            if (arg.startsWith(option_prefix))
            {
                return arg.mid(option_prefix.size()).trimmed();
            }
        }
    }
    return QString();
}

bool EnvironmentFlag(const char* name, bool defaultValue)
{
    const QString value = EnvironmentText(name).toLower();
    if (value.isEmpty())
    {
        return defaultValue;
    }
    return value != QString::fromUtf8("0") &&
        value != QString::fromUtf8("false") &&
        value != QString::fromUtf8("no") &&
        value != QString::fromUtf8("off");
}

int EnvironmentInt(const char* name, int defaultValue)
{
    bool ok = false;
    const int value = EnvironmentText(name).toInt(&ok);
    return ok ? value : defaultValue;
}

bool SelectComboByDataOrText(QComboBox* combo, const QString& value)
{
    if (combo == nullptr || value.trimmed().isEmpty())
    {
        return false;
    }

    const QString normalized = value.trimmed().toLower();
    for (int i = 0; i < combo->count(); ++i)
    {
        const QString item_data = combo->itemData(i).toString().trimmed().toLower();
        const QString item_text = combo->itemText(i).trimmed().toLower();
        const QSize item_size = combo->itemData(i).toSize();
        const QString item_size_text = item_size.width() > 0 && item_size.height() > 0 ?
            QString::fromUtf8("%1x%2").arg(item_size.width()).arg(item_size.height()) :
            QString();
        if (item_data == normalized ||
            item_text == normalized ||
            (!item_size_text.isEmpty() && item_size_text == normalized) ||
            (!item_size_text.isEmpty() && item_text.contains(normalized)))
        {
            combo->setCurrentIndex(i);
            return true;
        }
    }
    return false;
}

bool BuildStreamCoreRenderTargetForWidget(
    QWidget* widget,
    streamcore_render_target_t* outTarget)
{
    if (widget == nullptr || outTarget == nullptr)
    {
        return false;
    }

    const WId window_id = widget->winId();
    void* native_handle = reinterpret_cast<void*>(window_id);
    if (native_handle == nullptr)
    {
        return false;
    }

    *outTarget = {};
#if defined(Q_OS_WIN)
    outTarget->platform_type = STREAMCORE_RENDER_PLATFORM_WINDOWS;
#elif defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    outTarget->platform_type = STREAMCORE_RENDER_PLATFORM_APPLE;
#elif defined(Q_OS_LINUX)
    outTarget->platform_type = STREAMCORE_RENDER_PLATFORM_LINUX;
#else
    outTarget->platform_type = STREAMCORE_RENDER_PLATFORM_GENERIC;
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    outTarget->target_type = STREAMCORE_RENDER_TARGET_TYPE_LAYER;
#else
    outTarget->target_type = STREAMCORE_RENDER_TARGET_TYPE_NATIVE_WINDOW;
#endif
    outTarget->native_handle = native_handle;
    outTarget->width_hint = widget->width();
    outTarget->height_hint = widget->height();
    return true;
}

// Extract the endpoint label used by the ONVIF device combo.
QString OnvifEndpointLabel(const streamcore_onvif_device_t& device)
{
    const QString response_endpoint = ToQString(device.response_endpoint).trimmed();
    if (!response_endpoint.isEmpty())
    {
        return response_endpoint;
    }

    const QUrl service_url(ToQString(device.service_url).trimmed());
    QString host = service_url.host();
    if (host.isEmpty())
    {
        return QString();
    }
    if (host.contains(QLatin1Char(':')) && !host.startsWith(QLatin1Char('[')))
    {
        host = QString::fromUtf8("[%1]").arg(host);
    }

    int port = service_url.port();
    if (port < 0)
    {
        const QString scheme = service_url.scheme().toLower();
        if (scheme == QString::fromUtf8("http"))
        {
            port = 80;
        }
        else if (scheme == QString::fromUtf8("https"))
        {
            port = 443;
        }
    }
    return port > 0 ? QString::fromUtf8("%1:%2").arg(host).arg(port) : host;
}

// Add ONVIF credentials to RTSP URLs when the device omits userinfo.
QString ApplyRtspCredentials(
    const QString& streamUri,
    const QString& username,
    const QString& password)
{
    if (username.isEmpty())
    {
        return streamUri;
    }

    QUrl url(streamUri);
    if (!url.isValid() ||
        url.scheme().compare(QString::fromUtf8("rtsp"), Qt::CaseInsensitive) != 0 ||
        !url.userName().isEmpty())
    {
        return streamUri;
    }

    url.setUserName(username);
    url.setPassword(password);
    return url.toString(QUrl::FullyEncoded);
}

struct ZipSourceEntry
{
    QByteArray name;
    QByteArray data;
};

struct ZipCentralEntry
{
    QByteArray name;
    quint32 crc;
    quint32 size;
    quint32 offset;
};

struct LogPackageFile
{
    QString file_path;
    QString entry_name;
    qint64 size_bytes;
    qint64 modified_ms;
};

void AppendLe16(QByteArray* out, quint16 value)
{
    out->append(static_cast<char>(value & 0xFF));
    out->append(static_cast<char>((value >> 8) & 0xFF));
}

void AppendLe32(QByteArray* out, quint32 value)
{
    out->append(static_cast<char>(value & 0xFF));
    out->append(static_cast<char>((value >> 8) & 0xFF));
    out->append(static_cast<char>((value >> 16) & 0xFF));
    out->append(static_cast<char>((value >> 24) & 0xFF));
}

quint32 Crc32(const QByteArray& data)
{
    static quint32 table[256] = {};
    static bool table_ready = false;
    if (!table_ready)
    {
        for (quint32 index = 0; index < 256; ++index)
        {
            quint32 value = index;
            for (int bit = 0; bit < 8; ++bit)
            {
                value = (value & 1U) != 0U ?
                    (0xEDB88320U ^ (value >> 1U)) :
                    (value >> 1U);
            }
            table[index] = value;
        }
        table_ready = true;
    }

    quint32 crc = 0xFFFFFFFFU;
    for (char byte : data)
    {
        crc = table[(crc ^ static_cast<unsigned char>(byte)) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}

QByteArray NormalizeZipEntryName(const QString& name)
{
    QString normalized = name;
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (normalized.contains(QString::fromUtf8("../")))
    {
        normalized.replace(QString::fromUtf8("../"), QString());
    }
    normalized.remove(QString::fromUtf8(".."));
    return normalized.toUtf8();
}

void AddStoredZipEntry(
    QByteArray* archive,
    QVector<ZipCentralEntry>* central_entries,
    const ZipSourceEntry& entry)
{
    const quint32 crc = Crc32(entry.data);
    const quint32 size = static_cast<quint32>(entry.data.size());
    const quint32 offset = static_cast<quint32>(archive->size());

    AppendLe32(archive, 0x04034B50U);
    AppendLe16(archive, 20);
    AppendLe16(archive, 0);
    AppendLe16(archive, 0);
    AppendLe16(archive, 0);
    AppendLe16(archive, 0);
    AppendLe32(archive, crc);
    AppendLe32(archive, size);
    AppendLe32(archive, size);
    AppendLe16(archive, static_cast<quint16>(entry.name.size()));
    AppendLe16(archive, 0);
    archive->append(entry.name);
    archive->append(entry.data);

    central_entries->append({entry.name, crc, size, offset});
}

bool WriteStoredZipArchive(
    const QString& target_path,
    const QVector<ZipSourceEntry>& entries,
    QString* error_message)
{
    QByteArray archive;
    QVector<ZipCentralEntry> central_entries;
    for (const ZipSourceEntry& entry : entries)
    {
        AddStoredZipEntry(&archive, &central_entries, entry);
    }

    const quint32 central_offset = static_cast<quint32>(archive.size());
    for (const ZipCentralEntry& entry : central_entries)
    {
        AppendLe32(&archive, 0x02014B50U);
        AppendLe16(&archive, 20);
        AppendLe16(&archive, 20);
        AppendLe16(&archive, 0);
        AppendLe16(&archive, 0);
        AppendLe16(&archive, 0);
        AppendLe16(&archive, 0);
        AppendLe32(&archive, entry.crc);
        AppendLe32(&archive, entry.size);
        AppendLe32(&archive, entry.size);
        AppendLe16(&archive, static_cast<quint16>(entry.name.size()));
        AppendLe16(&archive, 0);
        AppendLe16(&archive, 0);
        AppendLe16(&archive, 0);
        AppendLe16(&archive, 0);
        AppendLe32(&archive, 0);
        AppendLe32(&archive, entry.offset);
        archive.append(entry.name);
    }
    const quint32 central_size =
        static_cast<quint32>(archive.size()) - central_offset;

    AppendLe32(&archive, 0x06054B50U);
    AppendLe16(&archive, 0);
    AppendLe16(&archive, 0);
    AppendLe16(&archive, static_cast<quint16>(central_entries.size()));
    AppendLe16(&archive, static_cast<quint16>(central_entries.size()));
    AppendLe32(&archive, central_size);
    AppendLe32(&archive, central_offset);
    AppendLe16(&archive, 0);

    QFile file(target_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (error_message != nullptr)
        {
            *error_message = file.errorString();
        }
        return false;
    }
    if (file.write(archive) != archive.size())
    {
        if (error_message != nullptr)
        {
            *error_message = file.errorString();
        }
        return false;
    }
    return true;
}

QString AbsoluteLogRelatedDirectory(const QString& directory, const QString& fallback)
{
    const QString selected = directory.isEmpty() ? fallback : directory;
    const QFileInfo info(selected);
    return info.isAbsolute() ? info.absoluteFilePath() : QDir::current().absoluteFilePath(selected);
}

void CollectLogPackageFiles(
    QVector<LogPackageFile>* files,
    QSet<QString>* seen_paths,
    const QDir& directory,
    const QString& zip_prefix)
{
    if (!directory.exists())
    {
        return;
    }
    const QFileInfoList children = directory.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name);
    for (const QFileInfo& child : children)
    {
        const QString zip_name = zip_prefix + child.fileName();
        if (child.isDir())
        {
            CollectLogPackageFiles(
                files,
                seen_paths,
                QDir(child.absoluteFilePath()),
                zip_name + QString::fromUtf8("/"));
            continue;
        }
        if (!child.isFile())
        {
            continue;
        }
        QString canonical_path = child.canonicalFilePath();
        if (canonical_path.isEmpty())
        {
            canonical_path = child.absoluteFilePath();
        }
        if (seen_paths->contains(canonical_path))
        {
            continue;
        }
        seen_paths->insert(canonical_path);
        files->append({
            child.absoluteFilePath(),
            zip_name,
            child.size(),
            child.lastModified().toMSecsSinceEpoch()
        });
    }
}

void AddLogPackageFileIfPresent(
    QVector<LogPackageFile>* files,
    QSet<QString>* seen_paths,
    const QString& file_path,
    const QString& entry_name)
{
    const QFileInfo file_info(file_path);
    if (!file_info.isFile())
    {
        return;
    }
    QString canonical_path = file_info.canonicalFilePath();
    if (canonical_path.isEmpty())
    {
        canonical_path = file_info.absoluteFilePath();
    }
    if (seen_paths->contains(canonical_path))
    {
        return;
    }
    seen_paths->insert(canonical_path);
    files->append({
        file_info.absoluteFilePath(),
        entry_name,
        file_info.size(),
        file_info.lastModified().toMSecsSinceEpoch()
    });
}

int AddCappedLogPackageEntries(
    QVector<ZipSourceEntry>* entries,
    const QVector<LogPackageFile>& candidates,
    QStringList* skipped_files)
{
    int added_count = 0;
    qint64 added_bytes = 0;
    for (const LogPackageFile& candidate : candidates)
    {
        QString skip_reason;
        if (added_count >= kLogPackageMaxFiles)
        {
            skip_reason = QString::fromUtf8("skipped by file-count cap");
        }
        else if (candidate.size_bytes > kLogPackageSingleFileMaxBytes)
        {
            if (added_bytes + kLogPackageSingleFileMaxBytes > kLogPackageMaxBytes)
            {
                skip_reason = QString::fromUtf8("skipped by total-size cap for tail entry");
            }
            else
            {
                QFile file(candidate.file_path);
                if (!file.open(QIODevice::ReadOnly))
                {
                    skipped_files->append(QString::fromUtf8("%1 | open failed")
                        .arg(candidate.entry_name));
                    continue;
                }
                const qint64 start_offset =
                    std::max<qint64>(0, candidate.size_bytes - kLogPackageSingleFileMaxBytes);
                if (!file.seek(start_offset))
                {
                    skipped_files->append(QString::fromUtf8("%1 | seek failed")
                        .arg(candidate.entry_name));
                    continue;
                }
                const QString tail_entry_name =
                    candidate.entry_name + QString::fromUtf8(".tail");
                entries->append({
                    NormalizeZipEntryName(tail_entry_name),
                    file.read(kLogPackageSingleFileMaxBytes)
                });
                added_bytes += std::min(candidate.size_bytes, kLogPackageSingleFileMaxBytes);
                added_count += 1;
                skipped_files->append(QString::fromUtf8(
                    "%1 | %2 bytes | included tail-only entry %3 capped to %4 bytes")
                    .arg(candidate.entry_name)
                    .arg(candidate.size_bytes)
                    .arg(tail_entry_name)
                    .arg(kLogPackageSingleFileMaxBytes));
                continue;
            }
        }
        else if (added_bytes + candidate.size_bytes > kLogPackageMaxBytes)
        {
            skip_reason = QString::fromUtf8("skipped by total-size cap");
        }

        if (!skip_reason.isEmpty())
        {
            skipped_files->append(QString::fromUtf8("%1 | %2 bytes | %3")
                .arg(candidate.entry_name)
                .arg(candidate.size_bytes)
                .arg(skip_reason));
            continue;
        }

        QFile file(candidate.file_path);
        if (!file.open(QIODevice::ReadOnly))
        {
            skipped_files->append(QString::fromUtf8("%1 | open failed")
                .arg(candidate.entry_name));
            continue;
        }

        entries->append({NormalizeZipEntryName(candidate.entry_name), file.readAll()});
        added_bytes += candidate.size_bytes;
        added_count += 1;
    }
    return added_count;
}

void RunPublisherComparisonVideoTask(
    streamcore_video_process process,
    streamcore_video_frame_ref inputFrame,
    int processorDelayMs) noexcept
{
    try
    {
        if (processorDelayMs > 0)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(processorDelayMs));
        }

        streamcore_frame_export_plan_t export_plan = {};
        streamcore_capture_video_frame_t output_frame = {};
        streamcore_video_frame_ref replacement_frame = nullptr;
        streamcore_video_process_result_t process_result = {};
        process_result.action = STREAMCORE_PROCESSOR_ACTION_ERROR;

        if (streamcore_video_frame_get_cpu_export_plan(
                inputFrame,
                STREAMCORE_VIDEO_PIXEL_FORMAT_RGB32,
                &export_plan) != STREAMCORE_RESULT_OK ||
            export_plan.supported == 0)
        {
            streamcore_video_process_finish(process, &process_result);
            return;
        }

        std::vector<unsigned char> output_data(export_plan.required_size);
        if (streamcore_video_frame_copy_as(
                inputFrame,
                STREAMCORE_VIDEO_PIXEL_FORMAT_RGB32,
                output_data.data(),
                output_data.size(),
                &output_frame) != STREAMCORE_RESULT_OK)
        {
            streamcore_video_process_finish(process, &process_result);
            return;
        }

        const size_t complete_pixel_bytes =
            output_frame.size - (output_frame.size % 4);
        for (size_t offset = 0; offset < complete_pixel_bytes; offset += 4)
        {
            const unsigned int gray =
                (static_cast<unsigned int>(output_data[offset]) * 29U +
                 static_cast<unsigned int>(output_data[offset + 1]) * 150U +
                 static_cast<unsigned int>(output_data[offset + 2]) * 77U) >>
                8;
            output_data[offset] = static_cast<unsigned char>(gray);
            output_data[offset + 1] = static_cast<unsigned char>(gray);
            output_data[offset + 2] = static_cast<unsigned char>(gray);
        }

        output_frame.data = output_data.data();
        if (streamcore_video_frame_create_cpu_copy(
                &output_frame,
                &replacement_frame) != STREAMCORE_RESULT_OK)
        {
            streamcore_video_process_finish(process, &process_result);
            return;
        }

        process_result.action = STREAMCORE_PROCESSOR_ACTION_REPLACE;
        process_result.replacement_frame = replacement_frame;
        if (streamcore_video_process_finish(process, &process_result) !=
            STREAMCORE_RESULT_OK)
        {
            streamcore_video_frame_release(replacement_frame);
        }
    }
    catch (...)
    {
        streamcore_video_process_result_t process_result = {};
        process_result.action = STREAMCORE_PROCESSOR_ACTION_ERROR;
        streamcore_video_process_finish(process, &process_result);
    }
}

// Demo-only visual Processor. It deliberately returns from the SDK callback
// before doing the pixel work so the sample exercises the same asynchronous
// process/input lifetime contract used by inference workers.
void ProcessPublisherComparisonVideo(
    streamcore_video_process process,
    streamcore_video_frame_ref inputFrame,
    void*)
{
    try
    {
        const int processor_delay_ms = std::clamp(
            EnvironmentInt("STREAMCORE_DEMO_QT_PROCESSOR_DELAY_MS", 0),
            0,
            2000);
        QRunnable* task = QRunnable::create(
            [process, inputFrame, processor_delay_ms]() {
                RunPublisherComparisonVideoTask(
                    process,
                    inputFrame,
                    processor_delay_ms);
            });
        QThreadPool::globalInstance()->start(task);
    }
    catch (...)
    {
        streamcore_video_process_result_t process_result = {};
        process_result.action = STREAMCORE_PROCESSOR_ACTION_ERROR;
        streamcore_video_process_finish(process, &process_result);
    }
}
}

StreamCoreDemoQtWindow::StreamCoreDemoQtWindow(QWidget* parent)
    : QMainWindow(parent),
      language_combo_(nullptr),
      preview_display_mode_combo_(nullptr),
      publisher_source_combo_(nullptr),
      publisher_camera_device_combo_(nullptr),
      publisher_file_path_edit_(nullptr),
      publisher_file_browse_button_(nullptr),
      publisher_audio_combo_(nullptr),
      publisher_audio_detail_label_(nullptr),
      publisher_audio_source_combo_(nullptr),
      publisher_audio_file_label_(nullptr),
      publisher_audio_file_path_edit_(nullptr),
      publisher_audio_file_browse_button_(nullptr),
      publisher_audio_volume_slider_(nullptr),
      publisher_audio_volume_value_label_(nullptr),
      publisher_protocol_combo_(nullptr),
      publisher_url_edit_(nullptr),
      publisher_whip_bearer_token_edit_(nullptr),
      publisher_video_codec_combo_(nullptr),
      publisher_audio_codec_combo_(nullptr),
      publisher_audio_profile_combo_(nullptr),
      publisher_audio_sample_rate_combo_(nullptr),
      publisher_audio_bitrate_combo_(nullptr),
      publisher_rtmp_hevc_combo_(nullptr),
      publisher_file_mode_combo_(nullptr),
      publisher_file_mode_label_(nullptr),
      publisher_preview_toggle_(nullptr),
      publisher_processor_compare_toggle_(nullptr),
      publisher_resolution_combo_(nullptr),
      publisher_video_bitrate_edit_(nullptr),
      publisher_fps_edit_(nullptr),
      publisher_gop_edit_(nullptr),
      publisher_source_summary_label_(nullptr),
      publisher_preview_frame_(nullptr),
      publisher_original_preview_caption_(nullptr),
      publisher_preview_widget_(nullptr),
      publisher_preview_label_(nullptr),
      publisher_watermark_label_(nullptr),
      publisher_processed_preview_frame_(nullptr),
      publisher_processed_preview_column_(nullptr),
      publisher_processed_preview_widget_(nullptr),
      publisher_processed_preview_label_(nullptr),
      publisher_processed_watermark_label_(nullptr),
      publisher_status_label_(nullptr),
      publisher_video_detail_row_(nullptr),
      publisher_audio_detail_row_(nullptr),
      publisher_file_mode_row_(nullptr),
      publisher_whip_bearer_row_(nullptr),
      publisher_runtime_log_(nullptr),
      publisher_start_button_(nullptr),
      player_url_edit_(nullptr),
      player_latency_preset_combo_(nullptr),
      player_decode_mode_combo_(nullptr),
      player_render_path_combo_(nullptr),
      player_advanced_params_check_(nullptr),
      player_advanced_params_panel_(nullptr),
      player_buffer_ms_edit_(nullptr),
      player_audio_queue_edit_(nullptr),
      player_video_queue_edit_(nullptr),
      player_audio_volume_slider_(nullptr),
      player_audio_volume_value_label_(nullptr),
      player_onvif_hint_label_(nullptr),
      player_onvif_device_list_(nullptr),
      player_onvif_username_edit_(nullptr),
      player_onvif_password_edit_(nullptr),
      player_onvif_search_button_(nullptr),
      player_onvif_apply_button_(nullptr),
      player_start_button_(nullptr),
      player_status_label_(nullptr),
      player_first_frame_label_(nullptr),
      player_source_summary_label_(nullptr),
      player_runtime_log_(nullptr),
      gb28181_source_combo_(nullptr),
      gb28181_video_source_combo_(nullptr),
      gb28181_audio_combo_(nullptr),
      gb28181_audio_source_combo_(nullptr),
      gb28181_resolution_combo_(nullptr),
      gb28181_fps_edit_(nullptr),
      gb28181_audio_volume_slider_(nullptr),
      gb28181_audio_volume_value_label_(nullptr),
      gb28181_local_id_edit_(nullptr),
      gb28181_local_domain_edit_(nullptr),
      gb28181_local_port_edit_(nullptr),
      gb28181_upper_id_edit_(nullptr),
      gb28181_upper_domain_edit_(nullptr),
      gb28181_upper_password_edit_(nullptr),
      gb28181_upper_ip_edit_(nullptr),
      gb28181_upper_port_edit_(nullptr),
      gb28181_upper_transport_combo_(nullptr),
      gb28181_media_port_edit_(nullptr),
      gb28181_source_summary_label_(nullptr),
      gb28181_preview_frame_(nullptr),
      gb28181_preview_label_(nullptr),
      gb28181_watermark_label_(nullptr),
      gb28181_video_detail_row_(nullptr),
      gb28181_audio_detail_row_(nullptr),
      gb28181_runtime_log_(nullptr),
      gb28181_start_button_(nullptr),
      gb28181_poll_timer_(nullptr),
      product_title_label_(nullptr),
      product_meta_label_(nullptr),
      license_status_label_(nullptr),
      license_feature_label_(nullptr),
      log_status_label_(nullptr),
      operation_status_label_(nullptr),
      machine_id_label_(nullptr),
      share_logs_button_(nullptr),
      upload_logs_button_(nullptr),
      desktop_render_target_frame_(nullptr),
      desktop_render_target_status_label_(nullptr),
      player_watermark_label_(nullptr),
      player_preview_layout_(nullptr),
      player_fullscreen_window_(nullptr),
      session_table_(nullptr),
      capability_table_(nullptr),
      desktop_render_target_widget_(nullptr),
      active_player_(nullptr),
      active_publisher_(nullptr),
      active_publisher_capture_(nullptr),
#if STREAMCORE_DEMO_ENABLE_GB28181
      active_gb28181_(nullptr),
      active_gb28181_media_capture_(nullptr),
      active_gb28181_source_index_(0),
      active_gb28181_audio_index_(0),
      active_gb28181_width_(1280),
      active_gb28181_height_(720),
      active_gb28181_fps_(25),
      active_gb28181_source_binding_(),
      active_gb28181_source_id_(),
      active_gb28181_audio_source_id_(),
      active_gb28181_target_device_id_(),
      active_gb28181_target_channel_id_(),
      active_gb28181_target_stream_kind_(STREAMCORE_GB28181_STREAM_LIVE),
      active_gb28181_pushed_audio_packets_(0),
      active_gb28181_pushed_video_packets_(0),
#endif
      active_player_url_(),
      active_player_start_wall_time_ms_(0),
      onvif_devices_(),
      player_onvif_status_(),
      current_machine_id_(),
      log_directory_(QString::fromUtf8("streamcore/logs")),
      log_file_name_(QString::fromUtf8("streamcore_demo.log")),
      product_crash_directory_(QString::fromUtf8("streamcore/crash")),
      latest_action_(QString::fromUtf8("startup")),
      latest_status_code_(QString::fromUtf8("-")),
      latest_status_name_(QString::fromUtf8("pending")),
      latest_status_summary_(QString::fromUtf8("Runtime is loading.")),
      latest_log_zip_path_(),
      language_(StartupLanguage()),
      rebuilding_ui_(false),
      player_fullscreen_active_(false)
{
    gb28181_poll_timer_ = new QTimer(this);
    gb28181_poll_timer_->setInterval(1000);
    connect(
        gb28181_poll_timer_,
        &QTimer::timeout,
        this,
        [this]() { PollGb28181(); });
    BuildUi();
    LoadSnapshot();
    BindDesktopRenderTarget();
    ScheduleAutorunIfRequested();
    ScheduleAutomationScreenshot();
}

StreamCoreDemoQtWindow::~StreamCoreDemoQtWindow()
{
    ExitPlayerFullscreen();
    StopGb28181();
    StopPublisher();
    if (active_player_ != nullptr)
    {
        streamcore_player_stop(active_player_);
        streamcore_player_destroy(active_player_);
        active_player_ = nullptr;
    }
}

StreamCoreDemoQtWindow::DemoLanguage StreamCoreDemoQtWindow::ResolveSystemLanguage()
{
    return QLocale::system().language() == QLocale::Chinese ?
        DemoLanguage::Chinese :
        DemoLanguage::English;
}

StreamCoreDemoQtWindow::DemoLanguage StreamCoreDemoQtWindow::StartupLanguage()
{
    const QString value = EnvironmentText("STREAMCORE_DEMO_QT_LANGUAGE").toLower();
    if (value == QString::fromUtf8("en") ||
        value == QString::fromUtf8("english"))
    {
        return DemoLanguage::English;
    }
    if (value == QString::fromUtf8("zh") ||
        value == QString::fromUtf8("cn") ||
        value == QString::fromUtf8("chinese"))
    {
        return DemoLanguage::Chinese;
    }
    return DemoLanguage::Auto;
}

QString StreamCoreDemoQtWindow::UiText(const char* english, const char* chinese) const
{
    return QString::fromUtf8(
        EffectiveLanguage() == DemoLanguage::Chinese ? chinese : english);
}

StreamCoreDemoQtWindow::DemoLanguage StreamCoreDemoQtWindow::EffectiveLanguage() const
{
    return language_ == DemoLanguage::Auto ? ResolveSystemLanguage() : language_;
}

bool StreamCoreDemoQtWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event != nullptr &&
        (watched == desktop_render_target_frame_ ||
         watched == desktop_render_target_widget_) &&
        event->type() == QEvent::MouseButtonDblClick)
    {
        TogglePlayerFullscreen();
        return true;
    }
    if (event != nullptr &&
        player_fullscreen_active_ &&
        (watched == player_fullscreen_window_ ||
         watched == desktop_render_target_frame_ ||
         watched == desktop_render_target_widget_) &&
        event->type() == QEvent::KeyPress)
    {
        const QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Escape)
        {
            ExitPlayerFullscreen();
            return true;
        }
    }
    if (event != nullptr &&
        watched == player_fullscreen_window_ &&
        event->type() == QEvent::Close)
    {
        ExitPlayerFullscreen();
        event->ignore();
        return true;
    }
    if ((watched == publisher_preview_frame_ ||
         watched == publisher_processed_preview_frame_ ||
         watched == desktop_render_target_frame_ ||
         watched == gb28181_preview_frame_) &&
        event != nullptr &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Show))
    {
        ApplyPreviewDisplayMode();
        if ((watched == publisher_preview_frame_ ||
             watched == publisher_processed_preview_frame_) &&
            active_publisher_capture_ != nullptr &&
            IsPublisherPreviewEnabled())
        {
            ApplyPublisherPreviewToggle();
        }
        if (watched == desktop_render_target_frame_)
        {
            UpdateDesktopRenderTargetStatusFromGeometry();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void StreamCoreDemoQtWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        ApplyPreviewDisplayMode();
        if (active_publisher_capture_ != nullptr &&
            IsPublisherPreviewEnabled())
        {
            ApplyPublisherPreviewToggle();
        }
        BindDesktopRenderTarget();
    });
}

void StreamCoreDemoQtWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    QTimer::singleShot(0, this, [this]() {
        ApplyPreviewDisplayMode();
        if (active_publisher_capture_ != nullptr &&
            IsPublisherPreviewEnabled())
        {
            ApplyPublisherPreviewToggle();
        }
        UpdateDesktopRenderTargetStatusFromGeometry();
    });
}

StreamCoreDemoQtWindow::PreviewDisplayMode
StreamCoreDemoQtWindow::SelectedPreviewDisplayMode() const
{
    const int index = preview_display_mode_combo_ != nullptr ?
        preview_display_mode_combo_->currentIndex() :
        1;
    if (index == 0)
    {
        return PreviewDisplayMode::StretchFill;
    }
    if (index == 2)
    {
        return PreviewDisplayMode::AspectCrop;
    }
    return PreviewDisplayMode::AspectFit;
}

QString StreamCoreDemoQtWindow::PreviewDisplayModeText() const
{
    if (preview_display_mode_combo_ != nullptr &&
        preview_display_mode_combo_->currentText().trimmed().isEmpty() == false)
    {
        return preview_display_mode_combo_->currentText();
    }

    const PreviewDisplayMode mode = SelectedPreviewDisplayMode();
    if (mode == PreviewDisplayMode::StretchFill)
    {
        return UiText("stretch fill", "拉伸充满");
    }
    if (mode == PreviewDisplayMode::AspectCrop)
    {
        return UiText("aspect crop fill", "裁剪充满");
    }
    return UiText("keep aspect", "保持比例");
}

QSize StreamCoreDemoQtWindow::SelectedPublisherResolution() const
{
    if (publisher_resolution_combo_ == nullptr)
    {
        return QSize(1280, 720);
    }

    const QSize size = publisher_resolution_combo_->currentData().toSize();
    return size;
}

QString StreamCoreDemoQtWindow::SelectedPublisherResolutionText() const
{
    if (publisher_resolution_combo_ == nullptr)
    {
        return QString::fromUtf8("1280x720");
    }

    const QSize size = publisher_resolution_combo_->currentData().toSize();
    if (size.width() > 0 && size.height() > 0)
    {
        return QString::fromUtf8("%1x%2").arg(size.width()).arg(size.height());
    }
    return publisher_resolution_combo_->currentText().trimmed().isEmpty() ?
        UiText("source original", "跟随来源") :
        publisher_resolution_combo_->currentText();
}

QString StreamCoreDemoQtWindow::SelectedPublisherVideoCodec() const
{
    if (publisher_video_codec_combo_ == nullptr)
    {
        return QString::fromUtf8("h264");
    }
    const QString codec = publisher_video_codec_combo_->currentData().toString();
    return codec.isEmpty() ? QString::fromUtf8("h264") : codec;
}

QString StreamCoreDemoQtWindow::SelectedPublisherAudioCodec() const
{
    if (publisher_audio_codec_combo_ == nullptr)
    {
        return QString::fromUtf8("aac");
    }
    const QString codec = publisher_audio_codec_combo_->currentData().toString();
    return codec.isEmpty() ? QString::fromUtf8("aac") : codec;
}

QString StreamCoreDemoQtWindow::SelectedPublisherAudioProfileCodec() const
{
    const QString selected_codec = SelectedPublisherAudioCodec();
    if (publisher_audio_profile_combo_ == nullptr)
    {
        return selected_codec;
    }
    const QString profile_codec =
        publisher_audio_profile_combo_->currentData().toString();
    if (selected_codec.compare(QString::fromUtf8("opus"), Qt::CaseInsensitive) == 0)
    {
        return QString::fromUtf8("opus");
    }
    if (profile_codec.compare(QString::fromUtf8("aac"), Qt::CaseInsensitive) == 0 ||
        profile_codec.compare(QString::fromUtf8("heaac"), Qt::CaseInsensitive) == 0 ||
        profile_codec.compare(QString::fromUtf8("aac-eld"), Qt::CaseInsensitive) == 0)
    {
        return profile_codec;
    }
    return selected_codec;
}

streamcore_publisher_rtmp_hevc_mode_t
StreamCoreDemoQtWindow::SelectedPublisherRtmpHevcMode() const
{
    if (publisher_rtmp_hevc_combo_ == nullptr)
    {
        return STREAMCORE_PUBLISHER_RTMP_HEVC_MODE_AUTO_COMPAT;
    }

    bool ok = false;
    const int value = publisher_rtmp_hevc_combo_->currentData().toInt(&ok);
    if (!ok)
    {
        return STREAMCORE_PUBLISHER_RTMP_HEVC_MODE_AUTO_COMPAT;
    }
    return static_cast<streamcore_publisher_rtmp_hevc_mode_t>(value);
}

QString StreamCoreDemoQtWindow::PublisherRtmpHevcModeText() const
{
    switch (SelectedPublisherRtmpHevcMode())
    {
    case STREAMCORE_PUBLISHER_RTMP_HEVC_MODE_LEGACY_FLV_TAG:
        return UiText("legacy HEVC FLV tag", "legacy HEVC FLV tag");
    case STREAMCORE_PUBLISHER_RTMP_HEVC_MODE_ENHANCED_RTMP:
        return UiText("Enhanced RTMP", "Enhanced RTMP");
    case STREAMCORE_PUBLISHER_RTMP_HEVC_MODE_AUTO_COMPAT:
    default:
        return UiText("compatibility default", "兼容默认");
    }
}

bool StreamCoreDemoQtWindow::IsPublisherPreviewEnabled() const
{
    return publisher_preview_toggle_ != nullptr &&
        publisher_preview_toggle_->isChecked();
}

bool StreamCoreDemoQtWindow::CurrentPublisherSelectionSupportsPreview() const
{
    const int source_index = ComboValueOrIndex(
        publisher_source_combo_,
        kPublisherSourceCamera);
    const int file_mode = ComboValueOrIndex(
        publisher_file_mode_combo_,
        kPublisherFileModeAuto);
    const bool processor_compare_enabled =
        publisher_processor_compare_toggle_ != nullptr &&
        publisher_processor_compare_toggle_->isChecked();

    return source_index == kPublisherSourceCamera ||
        source_index == kPublisherSourceDesktop ||
        source_index == kPublisherSourceImage ||
        (source_index == kPublisherSourceVideoFile &&
            (file_mode == kPublisherFileModeForceTranscode ||
                processor_compare_enabled));
}

void StreamCoreDemoQtWindow::ApplyPublisherPreviewToggle()
{
    if (active_publisher_capture_ == nullptr)
    {
        UpdatePublisherSourceSummary();
        return;
    }

    streamcore_render_target_t preview_target = {};
    streamcore_render_target_t processed_preview_target = {};
    streamcore_render_target_t* preview_target_ptr = nullptr;
    streamcore_render_target_t* processed_preview_target_ptr = nullptr;
    const bool processor_compare_enabled =
        publisher_processor_compare_toggle_ != nullptr &&
        publisher_processor_compare_toggle_->isChecked();
    if (IsPublisherPreviewEnabled())
    {
        if (publisher_preview_widget_ == nullptr ||
            !BuildStreamCoreRenderTargetForWidget(
                publisher_preview_widget_,
                &preview_target))
        {
            if (publisher_status_label_ != nullptr)
            {
                publisher_status_label_->setText(UiText(
                    "Publish running, but the local preview surface is unavailable.",
                    "推流已运行，但本地预览目标当前不可用。"));
            }
            return;
        }
        preview_target_ptr = &preview_target;
        if (processor_compare_enabled)
        {
            if (publisher_processed_preview_widget_ == nullptr ||
                !BuildStreamCoreRenderTargetForWidget(
                    publisher_processed_preview_widget_,
                    &processed_preview_target))
            {
                if (publisher_status_label_ != nullptr)
                {
                    publisher_status_label_->setText(UiText(
                        "Publish running, but the processed preview surface is unavailable.",
                        "推流已运行，但处理后预览目标当前不可用。"));
                }
                return;
            }
            processed_preview_target_ptr = &processed_preview_target;
        }
    }

    const streamcore_result_t result =
        streamcore_capture_set_preview_render_target(
            active_publisher_capture_,
            preview_target_ptr);
    const streamcore_result_t processed_result =
        streamcore_capture_set_processed_preview_render_target(
            active_publisher_capture_,
            processed_preview_target_ptr);
    if ((result != STREAMCORE_RESULT_OK ||
         processed_result != STREAMCORE_RESULT_OK) &&
        publisher_status_label_ != nullptr)
    {
        publisher_status_label_->setText(UiText(
            "Publish running, but the local preview toggle could not be applied.",
            "推流已运行，但本地预览开关应用失败。"));
    }
    if (publisher_processed_preview_label_ != nullptr)
    {
        if (processed_preview_target_ptr != nullptr &&
            processed_result == STREAMCORE_RESULT_OK)
        {
            publisher_processed_preview_label_->hide();
        }
        else
        {
            publisher_processed_preview_label_->setText(UiText(
                "PROCESSED PREVIEW OFF",
                "处理后预览已关闭"));
            publisher_processed_preview_label_->show();
        }
    }
    if (publisher_preview_label_ != nullptr)
    {
        if (preview_target_ptr != nullptr && result == STREAMCORE_RESULT_OK)
        {
            publisher_preview_label_->hide();
        }
        else if (result == STREAMCORE_RESULT_OK)
        {
            publisher_preview_label_->setStyleSheet(QString::fromUtf8(
                kPublisherPreviewLabelDefaultStyle));
            publisher_preview_label_->setText(UiText(
                "LOCAL PREVIEW OFF\n\nEnable the preview toggle when you want this page to render locally.",
                "本地预览已关闭\n\n需要本地预览时，请开启预览开关。"));
            publisher_preview_label_->show();
        }
    }
    UpdatePublisherSourceSummary();
}

void StreamCoreDemoQtWindow::ApplyPreviewDisplayMode()
{
    ApplyPreviewSurfaceRatios();

    QWidget* publisher_preview_content = publisher_preview_widget_ != nullptr ?
        publisher_preview_widget_ :
        static_cast<QWidget*>(publisher_preview_label_);
    ApplyPreviewDisplayModeToWidget(
        publisher_preview_frame_,
        publisher_preview_content,
        SelectedPublisherResolution());
    if (publisher_preview_label_ != nullptr && publisher_preview_content != nullptr)
    {
        publisher_preview_label_->setGeometry(publisher_preview_content->geometry());
        publisher_preview_label_->raise();
    }
    QWidget* publisher_processed_preview_content =
        publisher_processed_preview_widget_ != nullptr ?
            publisher_processed_preview_widget_ :
            static_cast<QWidget*>(publisher_processed_preview_label_);
    ApplyPreviewDisplayModeToWidget(
        publisher_processed_preview_frame_,
        publisher_processed_preview_content,
        SelectedPublisherResolution());
    if (publisher_processed_preview_label_ != nullptr &&
        publisher_processed_preview_content != nullptr)
    {
        publisher_processed_preview_label_->setGeometry(
            publisher_processed_preview_content->geometry());
        publisher_processed_preview_label_->raise();
    }
    ApplyPreviewDisplayModeToWidget(
        desktop_render_target_frame_,
        desktop_render_target_widget_,
        PlayerPreviewSourceSize());
    ApplyPreviewDisplayModeToWidget(
        gb28181_preview_frame_,
        gb28181_preview_label_,
        QSize(1280, 720));
    UpdateDemoWatermarkLabels();
}

QLabel* StreamCoreDemoQtWindow::CreateDemoWatermarkLabel(QWidget* parent)
{
    if (parent == nullptr)
    {
        return nullptr;
    }

    QLabel* label = new QLabel(QString::fromUtf8("StreamCore Demo | hbrun.com"), parent);
    label->setObjectName(QString::fromUtf8("demo_watermark_label"));
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QString::fromUtf8(kDemoWatermarkLabelStyle));
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->hide();
    return label;
}

void StreamCoreDemoQtWindow::UpdateDemoWatermarkLabels()
{
    UpdateDemoWatermarkLabel(publisher_preview_frame_, publisher_watermark_label_);
    UpdateDemoWatermarkLabel(
        publisher_processed_preview_frame_,
        publisher_processed_watermark_label_);
    UpdateDemoWatermarkLabel(
        desktop_render_target_frame_, player_watermark_label_, true);
    UpdateDemoWatermarkLabel(gb28181_preview_frame_, gb28181_watermark_label_);
}

void StreamCoreDemoQtWindow::UpdateDemoWatermarkLabel(
    QWidget* frame, QLabel* label, bool alignTop)
{
    if (frame == nullptr || label == nullptr ||
        frame->width() <= 0 || frame->height() <= 0)
    {
        return;
    }

    label->adjustSize();
    const int margin = 10;
    const int x = frame->width() > label->width() + margin ?
        frame->width() - label->width() - margin :
        margin;
    const int y = alignTop || frame->height() <= label->height() + margin ?
        margin : frame->height() - label->height() - margin;
    label->move(x, y);
    label->show();
    label->raise();
}

void StreamCoreDemoQtWindow::ApplyPreviewSurfaceRatios()
{
    ApplyPreviewSurfaceRatio(publisher_preview_frame_);
    ApplyPreviewSurfaceRatio(publisher_processed_preview_frame_);
    ApplyPreviewSurfaceRatio(desktop_render_target_frame_);
    ApplyPreviewSurfaceRatio(gb28181_preview_frame_);
}

void StreamCoreDemoQtWindow::ApplyPreviewSurfaceRatio(QWidget* frame)
{
    if (frame == nullptr)
    {
        return;
    }
    if (player_fullscreen_active_ && frame == desktop_render_target_frame_)
    {
        frame->setMinimumSize(1, 1);
        frame->setMaximumHeight(QWIDGETSIZE_MAX);
        frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        return;
    }

    const int width = frame->width();
    if (width <= 0)
    {
        return;
    }

    const int target_height = std::max(
        static_cast<int>(width / kPreviewFrameAspectRatio + 0.5),
        kPreviewFrameMinHeight);
    if (frame->minimumHeight() == target_height &&
        frame->maximumHeight() == target_height)
    {
        return;
    }

    frame->setMinimumHeight(target_height);
    frame->setMaximumHeight(target_height);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void StreamCoreDemoQtWindow::ApplyPreviewDisplayModeToWidget(
    QWidget* frame,
    QWidget* content,
    const QSize& sourceSize)
{
    if (frame == nullptr || content == nullptr)
    {
        return;
    }

    const int frame_width = frame->width();
    const int frame_height = frame->height();
    if (frame_width <= 0 || frame_height <= 0)
    {
        return;
    }

    int target_width = frame_width;
    int target_height = frame_height;
    const int source_width = sourceSize.width() > 0 ? sourceSize.width() : 1280;
    const int source_height = sourceSize.height() > 0 ? sourceSize.height() : 720;
    const PreviewDisplayMode mode = SelectedPreviewDisplayMode();
    if (mode != PreviewDisplayMode::StretchFill)
    {
        const double frame_ratio =
            static_cast<double>(frame_width) / static_cast<double>(frame_height);
        const double source_ratio =
            static_cast<double>(source_width) / static_cast<double>(source_height);
        const bool frame_is_wider = frame_ratio > source_ratio;
        if (mode == PreviewDisplayMode::AspectFit)
        {
            if (frame_is_wider)
            {
                target_height = frame_height;
                target_width = static_cast<int>(frame_height * source_ratio + 0.5);
            }
            else
            {
                target_width = frame_width;
                target_height = static_cast<int>(frame_width / source_ratio + 0.5);
            }
        }
        else
        {
            if (frame_is_wider)
            {
                target_width = frame_width;
                target_height = static_cast<int>(frame_width / source_ratio + 0.5);
            }
            else
            {
                target_height = frame_height;
                target_width = static_cast<int>(frame_height * source_ratio + 0.5);
            }
        }
    }

    if (target_width < 1)
    {
        target_width = 1;
    }
    if (target_height < 1)
    {
        target_height = 1;
    }

    content->setGeometry(
        (frame_width - target_width) / 2,
        (frame_height - target_height) / 2,
        target_width,
        target_height);
}

QSize StreamCoreDemoQtWindow::PlayerPreviewSourceSize() const
{
    streamcore_player_runtime_info_t runtime_info = {};
    if (active_player_ != nullptr &&
        streamcore_player_get_runtime_info(active_player_, &runtime_info) ==
            STREAMCORE_RESULT_OK &&
        runtime_info.video_width > 0 &&
        runtime_info.video_height > 0)
    {
        return QSize(runtime_info.video_width, runtime_info.video_height);
    }
    return QSize(1280, 720);
}

void StreamCoreDemoQtWindow::UpdateDesktopRenderTargetStatusFromGeometry()
{
    if (desktop_render_target_status_label_ == nullptr ||
        desktop_render_target_widget_ == nullptr)
    {
        return;
    }

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    if (active_player_ != nullptr)
    {
        desktop_render_target_status_label_->setText(UiText(
            "Playing · view %1x%2 · stream resolution/bitrate pending · %3",
            "播放中 · 视图 %1x%2 · 码流分辨率/码率待统计 · %3")
                .arg(desktop_render_target_widget_->width())
                .arg(desktop_render_target_widget_->height())
                .arg(PlayerPlaybackModeSummary()));
    }
    else
    {
        desktop_render_target_status_label_->setText(UiText(
            "Ready · view %1x%2 · stream pending · %3",
            "就绪 · 视图 %1x%2 · 码流待统计 · %3")
                .arg(desktop_render_target_widget_->width())
                .arg(desktop_render_target_widget_->height())
                .arg(PlayerPlaybackModeSummary()));
    }
#endif
}

void StreamCoreDemoQtWindow::SetLanguage(DemoLanguage language)
{
    if (rebuilding_ui_ || language_ == language)
    {
        return;
    }

    ExitPlayerFullscreen();
    StopGb28181();
    StopPublisher();
    StopPlayerUrl();
    language_ = language;
    BuildUi();
    LoadSnapshot();
    BindDesktopRenderTarget();
}

void StreamCoreDemoQtWindow::BuildUi()
{
    rebuilding_ui_ = true;

    QWidget* page = new QWidget(this);
    QVBoxLayout* page_layout = new QVBoxLayout(page);
    QWidget* overview_box = new QWidget(page);
    QVBoxLayout* overview_layout = new QVBoxLayout(overview_box);
    QHBoxLayout* overview_header_layout = new QHBoxLayout();
    QVBoxLayout* overview_text_layout = new QVBoxLayout();
    const int kDesktopConfigPaneWidth = kSidebarWidth;
    QLabel* logo_label = new QLabel(page);
    QLabel* brand_contact_label = new QLabel(
        UiText("Publisher / Player / GB28181 / License", "推流 / 播放 / GB28181 / 授权"),
        page);
    QTabWidget* feature_tabs = new QTabWidget(page);
    feature_tabs->setObjectName(QString::fromUtf8("feature_tabs"));
    QWidget* publisher_page = new QWidget(feature_tabs);
    QVBoxLayout* publisher_layout = new QVBoxLayout(publisher_page);
    QHBoxLayout* publisher_content_layout = new QHBoxLayout();
    QVBoxLayout* publisher_left_layout = new QVBoxLayout();
    QVBoxLayout* publisher_preview_layout = new QVBoxLayout();
    QGroupBox* publisher_source_box =
        new QGroupBox(UiText("Source setup", "来源设置"), publisher_page);
    QVBoxLayout* publisher_source_layout = new QVBoxLayout(publisher_source_box);
    QGroupBox* publisher_publish_box =
        new QGroupBox(UiText("Publish setup", "推流设置"), publisher_page);
    QVBoxLayout* publisher_publish_layout = new QVBoxLayout(publisher_publish_box);
    QWidget* publisher_action_box = new QWidget(publisher_page);
    QVBoxLayout* publisher_action_layout = new QVBoxLayout(publisher_action_box);
    QGroupBox* publisher_runtime_box =
        new QGroupBox(UiText("Runtime summary", "运行摘要"), publisher_page);
    QVBoxLayout* publisher_runtime_layout = new QVBoxLayout(publisher_runtime_box);
    QHBoxLayout* publisher_source_row = new QHBoxLayout();
    QHBoxLayout* publisher_audio_row = new QHBoxLayout();
    QHBoxLayout* publisher_url_row = new QHBoxLayout();
    QHBoxLayout* publisher_codec_row = new QHBoxLayout();
    QHBoxLayout* publisher_output_row = new QHBoxLayout();
    QHBoxLayout* publisher_rate_row = new QHBoxLayout();
    QHBoxLayout* publisher_file_mode_row = new QHBoxLayout();
    QHBoxLayout* publisher_action_row = new QHBoxLayout();
    QWidget* player_page = new QWidget(feature_tabs);
    QVBoxLayout* player_layout = new QVBoxLayout(player_page);
    QHBoxLayout* player_content_layout = new QHBoxLayout();
    QVBoxLayout* player_left_layout = new QVBoxLayout();
    QVBoxLayout* player_preview_layout = new QVBoxLayout();
    QGroupBox* player_session_box =
        new QGroupBox(UiText("Player setup", "播放设置"), player_page);
    QVBoxLayout* player_session_layout = new QVBoxLayout(player_session_box);
    QWidget* player_tuning_box = new QWidget(player_page);
    QVBoxLayout* player_tuning_layout = new QVBoxLayout(player_tuning_box);
    QGroupBox* player_onvif_box =
        new QGroupBox(QString::fromUtf8("ONVIF"), player_page);
    QVBoxLayout* player_onvif_layout = new QVBoxLayout(player_onvif_box);
    QWidget* player_onvif_controls = new QWidget(player_onvif_box);
    QVBoxLayout* player_onvif_controls_layout =
        new QVBoxLayout(player_onvif_controls);
    QWidget* player_action_box = new QWidget(player_page);
    QVBoxLayout* player_action_layout = new QVBoxLayout(player_action_box);
    QGroupBox* player_runtime_box = new QGroupBox(
        UiText("Runtime information", "运行信息"), player_page);
    QVBoxLayout* player_runtime_layout = new QVBoxLayout(player_runtime_box);
    QHBoxLayout* player_action_row = new QHBoxLayout();
    QWidget* gb28181_page = new QWidget(feature_tabs);
    QVBoxLayout* gb28181_layout = new QVBoxLayout(gb28181_page);
    QHBoxLayout* gb28181_content_layout = new QHBoxLayout();
    QVBoxLayout* gb28181_left_layout = new QVBoxLayout();
    QVBoxLayout* gb28181_preview_layout = new QVBoxLayout();
    QGroupBox* gb28181_platform_box =
        new QGroupBox(UiText("Upper platform", "上级平台"), gb28181_page);
    QVBoxLayout* gb28181_platform_layout = new QVBoxLayout(gb28181_platform_box);
    QGroupBox* gb28181_source_box =
        new QGroupBox(UiText("Source setup", "来源设置"), gb28181_page);
    QVBoxLayout* gb28181_source_layout = new QVBoxLayout(gb28181_source_box);
    QWidget* gb28181_action_box = new QWidget(gb28181_page);
    QVBoxLayout* gb28181_action_layout = new QVBoxLayout(gb28181_action_box);
    QGroupBox* gb28181_runtime_box =
        new QGroupBox(UiText("Runtime summary", "运行摘要"), gb28181_page);
    QVBoxLayout* gb28181_runtime_layout = new QVBoxLayout(gb28181_runtime_box);
    QHBoxLayout* gb28181_platform_row = new QHBoxLayout();
    QHBoxLayout* gb28181_source_row = new QHBoxLayout();
    QHBoxLayout* gb28181_action_row = new QHBoxLayout();
    QWidget* license_page = new QWidget(feature_tabs);
    QVBoxLayout* license_layout = new QVBoxLayout(license_page);
    QWidget* logs_page = new QWidget(feature_tabs);
    QVBoxLayout* logs_layout = new QVBoxLayout(logs_page);
    QTabWidget* status_tabs = new QTabWidget(logs_page);
    QGroupBox* license_box =
        new QGroupBox(UiText("License overview", "授权概览"), license_page);
    QVBoxLayout* license_box_layout = new QVBoxLayout(license_box);
    QHBoxLayout* action_layout = new QHBoxLayout();
    QHBoxLayout* license_meta_action_layout = new QHBoxLayout();
    QGroupBox* logs_box =
        new QGroupBox(UiText("Log package and runtime", "日志包与运行状态"), logs_page);
    QVBoxLayout* logs_box_layout = new QVBoxLayout(logs_box);
    QHBoxLayout* logs_action_layout = new QHBoxLayout();
    QPushButton* refresh_button =
        new QPushButton(UiText("Refresh", "刷新"), page);
    QPushButton* copy_machine_id_button =
        new QPushButton(UiText("Copy machine ID", "复制机器码"), license_page);
    QLabel* display_mode_label =
        new QLabel(UiText("Display mode", "显示模式"), page);
    QLabel* language_label = new QLabel(UiText("Language", "语言"), page);
    QFont title_font;

    page->setObjectName(QString::fromUtf8("streamcore_demo_root"));
    overview_box->setObjectName(QString::fromUtf8("overview_bar"));
    feature_tabs->setObjectName(QString::fromUtf8("feature_tabs"));
    feature_tabs->setFocusPolicy(Qt::NoFocus);
    feature_tabs->tabBar()->setDrawBase(false);
    feature_tabs->tabBar()->setFocusPolicy(Qt::NoFocus);
    feature_tabs->tabBar()->setUsesScrollButtons(false);
    status_tabs->setObjectName(QString::fromUtf8("status_tabs"));
    page->setStyleSheet(QString::fromUtf8(
        "QWidget#streamcore_demo_root { font-size: 11pt; }"
        "QWidget#overview_bar { background: #F5F7FA; border-bottom: 1px solid #D5DDE5; }"
        "QTabWidget#feature_tabs { background: #FFFFFF; }"
        "QTabWidget#feature_tabs::pane { border: 0px; top: -1px; background: #FFFFFF; }"
        "QTabWidget#feature_tabs::tab-bar { left: 0px; top: 0px; }"
        "QTabWidget#feature_tabs QTabBar { background: #F5F7FA; margin-left: 0px; }"
        "QTabWidget#feature_tabs QTabBar::tab { background: #E8EEF4; color: #253746; border: 1px solid #CBD5DF; border-bottom: 0; border-radius: 0px; min-height: 28px; min-width: 88px; padding: 4px 10px; margin-right: 2px; font-weight: 600; }"
        "QTabWidget#feature_tabs QTabBar::tab:selected { background: #F5F7FA; color: #0F172A; border-color: #AEBBCC; border-bottom: 2px solid #2563EB; }"
        "QTabWidget#feature_tabs QTabBar::tab:hover:!selected { background: #F2F6FA; }"
        "QTabWidget#status_tabs::pane { border: 1px solid #CBD5DF; top: -1px; background: #FFFFFF; }"
        "QTabWidget#status_tabs QTabBar::tab { min-height: 24px; min-width: 72px; padding: 3px 8px; margin-right: 1px; font-weight: 500; }"
        "QLineEdit, QComboBox, QPushButton { min-height: 28px; padding: 3px 6px; font-size: 10pt; }"
        "QPushButton#publisher_start_button, QPushButton#player_start_button, QPushButton#gb28181_start_button { background: #2563EB; color: #FFFFFF; border: 1px solid #1D4ED8; border-radius: 4px; font-weight: 700; min-width: 96px; padding: 4px 14px; }"
        "QPushButton#publisher_start_button:hover, QPushButton#player_start_button:hover, QPushButton#gb28181_start_button:hover { background: #1D4ED8; }"
        "QPushButton#publisher_start_button:pressed, QPushButton#player_start_button:pressed, QPushButton#gb28181_start_button:pressed { background: #1E40AF; }"
        "QPushButton#publisher_start_button:disabled, QPushButton#player_start_button:disabled, QPushButton#gb28181_start_button:disabled { background: #94A3B8; color: #F8FAFC; border-color: #94A3B8; }"
        "QGroupBox { margin-top: 10px; padding-top: 14px; font-weight: 600; }"
        "QTableWidget { font-size: 10pt; }"));
    player_onvif_box->setObjectName(QString::fromUtf8("player_onvif_box"));
    player_onvif_box->setCheckable(false);
    player_onvif_controls->setObjectName(
        QString::fromUtf8("player_onvif_controls"));
    player_onvif_controls->setVisible(true);
    publisher_source_box->setMaximumWidth(kDesktopConfigPaneWidth);
    publisher_source_box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    publisher_publish_box->setMaximumWidth(kDesktopConfigPaneWidth);
    publisher_publish_box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    publisher_action_box->setMaximumWidth(kDesktopConfigPaneWidth);
    publisher_action_box->setMaximumHeight(40);
    publisher_action_box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    publisher_runtime_box->setMinimumHeight(kRuntimeLogHeight);
    publisher_runtime_box->setMaximumHeight(kRuntimeLogHeight + 36);
    publisher_runtime_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    player_session_box->setMaximumWidth(kDesktopConfigPaneWidth);
    player_tuning_box->setMaximumWidth(kDesktopConfigPaneWidth);
    player_onvif_box->setMaximumWidth(kDesktopConfigPaneWidth);
    player_action_box->setMaximumWidth(kDesktopConfigPaneWidth);
    player_action_box->setMaximumHeight(40);
    player_runtime_box->setMinimumHeight(160);
    player_runtime_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gb28181_platform_box->setMaximumWidth(kDesktopConfigPaneWidth);
    gb28181_source_box->setMaximumWidth(kDesktopConfigPaneWidth);
    gb28181_action_box->setMaximumWidth(kDesktopConfigPaneWidth);
    gb28181_action_box->setMaximumHeight(40);
    gb28181_runtime_box->setMinimumHeight(kRuntimeLogHeight);
    gb28181_runtime_box->setMaximumHeight(kRuntimeLogHeight + 36);
    gb28181_runtime_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    logo_label->setObjectName(QString::fromUtf8("brand_logo_label"));
    brand_contact_label->setObjectName(QString::fromUtf8("brand_contact_label"));
    refresh_button->setObjectName(QString::fromUtf8("refresh_snapshot_button"));
    copy_machine_id_button->setObjectName(
        QString::fromUtf8("copy_machine_id_button"));
    language_combo_ = new QComboBox(page);
    language_combo_->setObjectName(QString::fromUtf8("language_combo"));
    language_combo_->setEditable(false);
    language_combo_->setMinimumWidth(96);
    language_combo_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    language_combo_->addItems({
        UiText("Auto", "自动"),
        QString::fromUtf8("English"),
        QString::fromUtf8("中文")
    });
    if (language_ == DemoLanguage::Chinese)
    {
        language_combo_->setCurrentIndex(2);
    }
    else if (language_ == DemoLanguage::English)
    {
        language_combo_->setCurrentIndex(1);
    }
    else
    {
        language_combo_->setCurrentIndex(0);
    }
    preview_display_mode_combo_ = new QComboBox(page);
    preview_display_mode_combo_->setObjectName(
        QString::fromUtf8("preview_display_mode_combo"));
    preview_display_mode_combo_->setEditable(false);
    preview_display_mode_combo_->setMinimumWidth(128);
    preview_display_mode_combo_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    preview_display_mode_combo_->addItems({
        UiText("Stretch fill", "拉伸充满"),
        UiText("Keep aspect", "保持比例"),
        UiText("Crop fill", "裁剪充满")
    });
    preview_display_mode_combo_->setCurrentIndex(1);

    setWindowTitle(QString::fromUtf8("StreamCore SDK Demo"));
    setWindowIcon(QIcon(QString::fromUtf8(":/branding/streamcore_app_icon.png")));
    resize(1280, 720);

    title_font = QApplication::font();
    title_font.setPointSize(title_font.pointSize() + 2);
    title_font.setBold(true);

    product_title_label_ = new QLabel(page);
    product_title_label_->setObjectName(QString::fromUtf8("product_title_label"));
    product_title_label_->setFont(title_font);
    product_meta_label_ = new QLabel(page);
    product_meta_label_->setObjectName(QString::fromUtf8("product_meta_label"));
    product_meta_label_->setWordWrap(false);
    product_meta_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    product_meta_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    brand_contact_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    brand_contact_label->setStyleSheet(QString::fromUtf8(
        "QLabel { color: #3B536B; font-size: 9pt; }"));
    const QPixmap logo_pixmap(QString::fromUtf8(":/branding/streamcore_brand_logo.png"));
    logo_label->setFixedSize(40, 40);
    logo_label->setAlignment(Qt::AlignCenter);
    if (!logo_pixmap.isNull())
    {
        logo_label->setPixmap(logo_pixmap.scaled(
            34,
            34,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }
    license_status_label_ = new QLabel(license_page);
    license_status_label_->setObjectName(
        QString::fromUtf8("license_status_label"));
    license_status_label_->setWordWrap(true);
    license_feature_label_ = new QLabel(license_page);
    license_feature_label_->setObjectName(
        QString::fromUtf8("license_feature_label"));
    license_feature_label_->setWordWrap(true);
    log_status_label_ = new QLabel(logs_page);
    log_status_label_->setObjectName(QString::fromUtf8("log_status_label"));
    log_status_label_->setWordWrap(true);
    operation_status_label_ = new QLabel(logs_page);
    operation_status_label_->setObjectName(
        QString::fromUtf8("operation_status_label"));
    operation_status_label_->setWordWrap(true);
    operation_status_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    share_logs_button_ = new QPushButton(
        UiText("Share logs", "分享日志"),
        logs_page);
    share_logs_button_->setObjectName(QString::fromUtf8("share_logs_button"));
    upload_logs_button_ = new QPushButton(
        UiText("Upload reserved", "上传预留"),
        logs_page);
    upload_logs_button_->setObjectName(QString::fromUtf8("upload_logs_button"));
    machine_id_label_ = new QLabel(license_page);
    machine_id_label_->setObjectName(QString::fromUtf8("machine_id_label"));
    machine_id_label_->setWordWrap(true);
    machine_id_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    publisher_source_combo_ = new QComboBox(publisher_page);
    publisher_source_combo_->setObjectName(
        QString::fromUtf8("publisher_source_combo"));
    publisher_source_combo_->addItem(
        UiText("Camera", "摄像头"),
        kPublisherSourceCamera);
    publisher_source_combo_->addItem(
        UiText("Desktop capture", "桌面采集"),
        kPublisherSourceDesktop);
    publisher_source_combo_->addItem(
        UiText("Video file", "视频文件"),
        kPublisherSourceVideoFile);
    publisher_source_combo_->addItem(
        UiText("Still image", "图片"),
        kPublisherSourceImage);
    publisher_source_combo_->addItem(
        UiText("None", "无"),
        kPublisherSourceNone);

    publisher_camera_device_combo_ = new QComboBox(publisher_page);
    publisher_camera_device_combo_->setObjectName(
        QString::fromUtf8("publisher_camera_device_combo"));
    publisher_camera_device_combo_->setMinimumWidth(180);
    publisher_file_path_edit_ = new QLineEdit(publisher_page);
    publisher_file_path_edit_->setObjectName(
        QString::fromUtf8("publisher_file_path_edit"));
    publisher_file_path_edit_->setPlaceholderText(
        UiText("Select a local media file", "选择本地媒体文件"));
    publisher_file_path_edit_->setToolTip(
        UiText("Local media file used by this publisher scenario.",
               "当前推流场景使用的本地媒体文件。"));
    publisher_file_browse_button_ = new QPushButton(
        UiText("Browse", "选择"),
        publisher_page);
    publisher_file_browse_button_->setObjectName(
        QString::fromUtf8("publisher_file_browse_button"));

    publisher_audio_combo_ = new QComboBox(publisher_page);
    publisher_audio_combo_->setObjectName(
        QString::fromUtf8("publisher_audio_combo"));
    publisher_audio_combo_->addItem(
        UiText("None", "无"),
        kPublisherAudioNone);
    publisher_audio_combo_->addItem(
        UiText("Microphone", "麦克风"),
        kPublisherAudioMicrophone);
    publisher_audio_combo_->addItem(
        UiText("System audio", "系统声音"),
        kPublisherAudioSystem);
    publisher_audio_combo_->addItem(
        UiText("Audio file", "音频文件"),
        kPublisherAudioFile);
    publisher_audio_combo_->setCurrentIndex(1);
    publisher_audio_detail_label_ = new QLabel(
        UiText("Microphone source", "麦克风来源"),
        publisher_page);
    publisher_audio_detail_label_->setObjectName(
        QString::fromUtf8("publisher_audio_detail_label"));
    publisher_audio_source_combo_ = new QComboBox(publisher_page);
    publisher_audio_source_combo_->setObjectName(
        QString::fromUtf8("publisher_audio_source_combo"));
    publisher_audio_file_label_ = new QLabel(
        UiText("Audio file", "音频文件"),
        publisher_page);
    publisher_audio_file_label_->setObjectName(
        QString::fromUtf8("publisher_audio_file_label"));
    publisher_audio_file_path_edit_ = new QLineEdit(publisher_page);
    publisher_audio_file_path_edit_->setObjectName(
        QString::fromUtf8("publisher_audio_file_path_edit"));
    publisher_audio_file_path_edit_->setPlaceholderText(
        UiText("Select a local audio file", "选择本地音频文件"));
    publisher_audio_file_browse_button_ = new QPushButton(
        UiText("Browse", "选择"),
        publisher_page);
    publisher_audio_file_browse_button_->setObjectName(
        QString::fromUtf8("publisher_audio_file_browse_button"));
    publisher_audio_volume_slider_ = new QSlider(Qt::Horizontal, publisher_page);
    publisher_audio_volume_slider_->setObjectName(
        QString::fromUtf8("publisher_audio_volume_slider"));
    publisher_audio_volume_slider_->setRange(0, 100);
    publisher_audio_volume_slider_->setValue(100);
    publisher_audio_volume_slider_->setFixedWidth(128);
    publisher_audio_volume_value_label_ = new QLabel(publisher_page);
    publisher_audio_volume_value_label_->setObjectName(
        QString::fromUtf8("publisher_audio_volume_value_label"));
    publisher_audio_volume_value_label_->setMinimumWidth(44);

    publisher_protocol_combo_ = new QComboBox(publisher_page);
    publisher_protocol_combo_->setObjectName(
        QString::fromUtf8("publisher_protocol_combo"));
    publisher_protocol_combo_->addItem(QString::fromUtf8("RTMP"), QString::fromUtf8("rtmp"));
    publisher_protocol_combo_->addItem(QString::fromUtf8("RTSP"), QString::fromUtf8("rtsp"));
    publisher_protocol_combo_->addItem(QString::fromUtf8("SRT"), QString::fromUtf8("srt"));
    publisher_protocol_combo_->addItem(QString::fromUtf8("WHIP"), QString::fromUtf8("whip"));
    publisher_protocol_combo_->setMinimumWidth(96);

    publisher_url_edit_ = new QLineEdit(publisher_page);
    publisher_url_edit_->setObjectName(QString::fromUtf8("publisher_url_edit"));
    publisher_url_edit_->setText(
        QString::fromUtf8("rtmp://192.0.2.1:1935/live/desktop_demo"));
    publisher_url_edit_->setCursorPosition(0);
    publisher_url_edit_->setToolTip(
        UiText("RTMP / RTSP / SRT destination, or an HTTP(S) WHIP endpoint.",
               "RTMP / RTSP / SRT 推流地址，或 HTTP(S) WHIP 端点。"));
    publisher_whip_bearer_token_edit_ = new QLineEdit(publisher_page);
    publisher_whip_bearer_token_edit_->setObjectName(
        QString::fromUtf8("publisher_whip_bearer_token_edit"));
    publisher_whip_bearer_token_edit_->setEchoMode(QLineEdit::Password);
    publisher_whip_bearer_token_edit_->setPlaceholderText(
        UiText("Optional Bearer token", "可选 Bearer Token"));
    publisher_whip_bearer_token_edit_->setToolTip(UiText(
        "Optional WHIP Authorization bearer token. It is used in memory only.",
        "可选的 WHIP Authorization Bearer Token，仅在本次运行中使用。"));
    publisher_video_codec_combo_ = new QComboBox(publisher_page);
    publisher_video_codec_combo_->setObjectName(
        QString::fromUtf8("publisher_video_codec_combo"));
    publisher_video_codec_combo_->addItem(QString::fromUtf8("H.264"), QString::fromUtf8("h264"));
    publisher_video_codec_combo_->addItem(QString::fromUtf8("H.265 / HEVC"), QString::fromUtf8("h265"));
    publisher_video_codec_combo_->setMinimumWidth(104);
    publisher_audio_codec_combo_ = new QComboBox(publisher_page);
    publisher_audio_codec_combo_->setObjectName(
        QString::fromUtf8("publisher_audio_codec_combo"));
    publisher_audio_codec_combo_->addItem(QString::fromUtf8("AAC"), QString::fromUtf8("aac"));
    publisher_audio_codec_combo_->addItem(QString::fromUtf8("Opus"), QString::fromUtf8("opus"));
    publisher_audio_codec_combo_->setMinimumWidth(84);
    publisher_audio_profile_combo_ = new QComboBox(publisher_page);
    publisher_audio_profile_combo_->setObjectName(
        QString::fromUtf8("publisher_audio_profile_combo"));
    publisher_audio_profile_combo_->addItem(QString::fromUtf8("AAC-LC"), QString::fromUtf8("aac"));
    publisher_audio_profile_combo_->addItem(QString::fromUtf8("HE-AAC"), QString::fromUtf8("heaac"));
    publisher_audio_profile_combo_->addItem(QString::fromUtf8("AAC-ELD"), QString::fromUtf8("aac-eld"));
    publisher_audio_sample_rate_combo_ = new QComboBox(publisher_page);
    publisher_audio_sample_rate_combo_->setObjectName(
        QString::fromUtf8("publisher_audio_sample_rate_combo"));
    publisher_audio_sample_rate_combo_->addItem(QString::fromUtf8("32000"), 32000);
    publisher_audio_sample_rate_combo_->addItem(QString::fromUtf8("44100"), 44100);
    publisher_audio_sample_rate_combo_->addItem(QString::fromUtf8("48000"), 48000);
    publisher_audio_sample_rate_combo_->setCurrentIndex(2);
    publisher_audio_sample_rate_combo_->setMinimumWidth(84);
    publisher_audio_bitrate_combo_ = new QComboBox(publisher_page);
    publisher_audio_bitrate_combo_->setObjectName(
        QString::fromUtf8("publisher_audio_bitrate_combo"));
    publisher_audio_bitrate_combo_->addItem(QString::fromUtf8("64"), 64);
    publisher_audio_bitrate_combo_->addItem(QString::fromUtf8("96"), 96);
    publisher_audio_bitrate_combo_->addItem(QString::fromUtf8("128"), 128);
    publisher_audio_bitrate_combo_->addItem(QString::fromUtf8("160"), 160);
    publisher_audio_bitrate_combo_->setCurrentIndex(2);
    publisher_audio_bitrate_combo_->setMinimumWidth(76);
    publisher_rtmp_hevc_combo_ = new QComboBox(publisher_page);
    publisher_rtmp_hevc_combo_->setObjectName(
        QString::fromUtf8("publisher_rtmp_hevc_combo"));
    publisher_rtmp_hevc_combo_->addItem(
        UiText("Compatibility default", "兼容默认"),
        STREAMCORE_PUBLISHER_RTMP_HEVC_MODE_AUTO_COMPAT);
    publisher_rtmp_hevc_combo_->addItem(
        UiText("Legacy HEVC FLV", "Legacy HEVC FLV"),
        STREAMCORE_PUBLISHER_RTMP_HEVC_MODE_LEGACY_FLV_TAG);
    publisher_rtmp_hevc_combo_->addItem(
        UiText("Enhanced RTMP", "Enhanced RTMP"),
        STREAMCORE_PUBLISHER_RTMP_HEVC_MODE_ENHANCED_RTMP);
    publisher_rtmp_hevc_combo_->setMinimumWidth(164);
    publisher_file_mode_combo_ = new QComboBox(publisher_page);
    publisher_file_mode_combo_->setObjectName(
        QString::fromUtf8("publisher_file_mode_combo"));
    publisher_file_mode_combo_->addItem(
        UiText("Auto (passthrough preferred)", "自动（优先透传）"),
        kPublisherFileModeAuto);
    publisher_file_mode_combo_->addItem(
        UiText("Force transcode", "强制转码"),
        kPublisherFileModeForceTranscode);
    publisher_file_mode_combo_->setMinimumWidth(138);
    publisher_preview_toggle_ = new QCheckBox(
        UiText("Preview", "预览"),
        publisher_page);
    publisher_preview_toggle_->setObjectName(
        QString::fromUtf8("publisher_preview_toggle"));
    publisher_preview_toggle_->setChecked(
        EnvironmentFlag("STREAMCORE_DEMO_QT_PUBLISHER_PREVIEW", true));
    publisher_processor_compare_toggle_ = new QCheckBox(
        UiText("Before/after", "前后对比"),
        publisher_page);
    publisher_processor_compare_toggle_->setObjectName(
        QString::fromUtf8("publisher_processor_compare_toggle"));
    publisher_processor_compare_toggle_->setChecked(
        EnvironmentFlag(
            "STREAMCORE_DEMO_QT_PUBLISHER_PROCESSOR_COMPARE",
            false));
    publisher_processor_compare_toggle_->setToolTip(UiText(
        "Runs an asynchronous monochrome Processor and shows original and final frames side by side.",
        "运行异步黑白 Processor，并同时显示处理前与处理后的画面。"));
    publisher_resolution_combo_ = new QComboBox(publisher_page);
    publisher_resolution_combo_->setObjectName(
        QString::fromUtf8("publisher_resolution_combo"));
    publisher_resolution_combo_->setMinimumWidth(150);
    publisher_video_bitrate_edit_ = new QLineEdit(publisher_page);
    publisher_video_bitrate_edit_->setObjectName(
        QString::fromUtf8("publisher_video_bitrate_edit"));
    publisher_video_bitrate_edit_->setText(QString::fromUtf8("1200"));
    publisher_video_bitrate_edit_->setMaximumWidth(120);
    publisher_fps_edit_ = new QLineEdit(publisher_page);
    publisher_fps_edit_->setObjectName(QString::fromUtf8("publisher_fps_edit"));
    publisher_fps_edit_->setText(QString::fromUtf8("25"));
    publisher_fps_edit_->setMaximumWidth(80);
    publisher_gop_edit_ = new QLineEdit(publisher_page);
    publisher_gop_edit_->setObjectName(QString::fromUtf8("publisher_gop_edit"));
    publisher_gop_edit_->setText(QString::fromUtf8("50"));
    publisher_gop_edit_->setMaximumWidth(80);

    publisher_source_summary_label_ = new QLabel(publisher_page);
    publisher_source_summary_label_->setObjectName(
        QString::fromUtf8("publisher_source_summary_label"));
    publisher_source_summary_label_->setWordWrap(true);
    publisher_source_summary_label_->setStyleSheet(QString::fromUtf8(
        "QLabel#publisher_source_summary_label { background: #F6F8FB; border: 1px solid #D8E0E8; border-radius: 4px; padding: 6px 8px; color: #334155; font-size: 9pt; }"));
    publisher_preview_frame_ = new QWidget(publisher_page);
    publisher_preview_frame_->setObjectName(
        QString::fromUtf8("publisher_preview_frame"));
    publisher_preview_frame_->setMinimumSize(300, kPreviewFrameMinHeight);
    publisher_preview_frame_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    publisher_preview_frame_->setStyleSheet(QString::fromUtf8(
        "QWidget#publisher_preview_frame { background: #111827; border: 1px solid #2F3A45; }"));
    publisher_preview_frame_->installEventFilter(this);
    publisher_preview_widget_ = new QWidget(publisher_preview_frame_);
    publisher_preview_widget_->setObjectName(
        QString::fromUtf8("publisher_preview_widget"));
    publisher_preview_widget_->setAttribute(Qt::WA_NativeWindow);
    publisher_preview_widget_->setAutoFillBackground(true);
    publisher_preview_widget_->setStyleSheet(QString::fromUtf8(
        "QWidget#publisher_preview_widget { background: #111827; border: 0; }"));
    QPalette publisher_preview_palette = publisher_preview_widget_->palette();
    publisher_preview_palette.setColor(QPalette::Window, QColor(17, 24, 39));
    publisher_preview_widget_->setPalette(publisher_preview_palette);
    publisher_preview_label_ = new QLabel(publisher_preview_frame_);
    publisher_preview_label_->setObjectName(QString::fromUtf8("publisher_preview_label"));
    publisher_preview_label_->setAlignment(Qt::AlignCenter);
    publisher_preview_label_->setStyleSheet(QString::fromUtf8(
        kPublisherPreviewLabelDefaultStyle));
    publisher_preview_label_->setText(UiText(
        "Preview",
        "预览"));
    publisher_watermark_label_ =
        CreateDemoWatermarkLabel(publisher_preview_frame_);
    publisher_processed_preview_frame_ = new QWidget(publisher_page);
    publisher_processed_preview_frame_->setObjectName(
        QString::fromUtf8("publisher_processed_preview_frame"));
    publisher_processed_preview_frame_->setMinimumSize(
        300,
        kPreviewFrameMinHeight);
    publisher_processed_preview_frame_->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred);
    publisher_processed_preview_frame_->setStyleSheet(QString::fromUtf8(
        "QWidget#publisher_processed_preview_frame { background: #111827; "
        "border: 1px solid #2F3A45; }"));
    publisher_processed_preview_frame_->installEventFilter(this);
    publisher_processed_preview_widget_ =
        new QWidget(publisher_processed_preview_frame_);
    publisher_processed_preview_widget_->setObjectName(
        QString::fromUtf8("publisher_processed_preview_widget"));
    publisher_processed_preview_widget_->setAttribute(Qt::WA_NativeWindow);
    publisher_processed_preview_widget_->setAutoFillBackground(true);
    publisher_processed_preview_widget_->setStyleSheet(QString::fromUtf8(
        "QWidget#publisher_processed_preview_widget { background: #111827; border: 0; }"));
    QPalette publisher_processed_preview_palette =
        publisher_processed_preview_widget_->palette();
    publisher_processed_preview_palette.setColor(
        QPalette::Window,
        QColor(17, 24, 39));
    publisher_processed_preview_widget_->setPalette(
        publisher_processed_preview_palette);
    publisher_processed_preview_label_ =
        new QLabel(publisher_processed_preview_frame_);
    publisher_processed_preview_label_->setObjectName(
        QString::fromUtf8("publisher_processed_preview_label"));
    publisher_processed_preview_label_->setAlignment(Qt::AlignCenter);
    publisher_processed_preview_label_->setStyleSheet(QString::fromUtf8(
        kPublisherPreviewLabelDefaultStyle));
    publisher_processed_preview_label_->setText(UiText(
        "Processed preview",
        "处理后预览"));
    publisher_processed_watermark_label_ =
        CreateDemoWatermarkLabel(publisher_processed_preview_frame_);
    publisher_status_label_ = new QLabel(publisher_page);
    publisher_status_label_->setObjectName(
        QString::fromUtf8("publisher_status_label"));
    publisher_status_label_->setWordWrap(true);
    publisher_status_label_->setMinimumHeight(28);
    publisher_status_label_->setStyleSheet(QString::fromUtf8(
        "QLabel#publisher_status_label { color: #3B536B; padding: 4px 0; }"));
    publisher_status_label_->setText(UiText(
        "Publish idle.",
        "推流空闲。"));
    publisher_runtime_log_ = new QPlainTextEdit(publisher_page);
    publisher_runtime_log_->setObjectName(QString::fromUtf8("publisher_runtime_log"));
    publisher_runtime_log_->setReadOnly(true);
    publisher_runtime_log_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    publisher_start_button_ = new QPushButton(
        UiText("Start publish", "开始推流"),
        publisher_page);
    publisher_start_button_->setObjectName(
        QString::fromUtf8("publisher_start_button"));

    player_url_edit_ = new QLineEdit(player_page);
    player_url_edit_->setObjectName(QString::fromUtf8("player_url_edit"));
    player_url_edit_->setText(
        QString::fromUtf8("rtmp://192.0.2.1:1935/live/local_native"));
    player_url_edit_->setCursorPosition(0);
    player_latency_preset_combo_ = new QComboBox(player_page);
    player_latency_preset_combo_->setObjectName(
        QString::fromUtf8("player_latency_preset_combo"));
    player_latency_preset_combo_->setEditable(false);
    player_latency_preset_combo_->addItems({
        UiText("Compatible", "兼容模式"),
        UiText("Regular", "常规模式"),
        UiText("Low latency", "低延迟"),
        UiText("Ultra-low latency", "超低延迟")
    });
    player_latency_preset_combo_->setCurrentIndex(1);
    player_latency_preset_combo_->setMinimumWidth(132);
    player_decode_mode_combo_ = new QComboBox(player_page);
    player_decode_mode_combo_->setObjectName(
        QString::fromUtf8("player_decode_mode_combo"));
    player_decode_mode_combo_->setEditable(false);
    player_decode_mode_combo_->addItems({
        UiText("Software", "软件"),
        UiText("Hardware", "硬件")
    });
    player_decode_mode_combo_->setMinimumWidth(110);
    player_render_path_combo_ = new QComboBox(player_page);
    player_render_path_combo_->setObjectName(
        QString::fromUtf8("player_render_path_combo"));
    player_render_path_combo_->setEditable(false);
    player_render_path_combo_->addItems({
        UiText("Software frame", "软件帧"),
        UiText("GPU frame", "GPU 帧"),
        UiText("Direct surface", "Direct Surface"),
        UiText("SDK auto", "自动")
    });
    player_render_path_combo_->setMinimumWidth(168);
    player_render_path_combo_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    // 呈现路径由播放模式内部选择，不再作为面向用户的独立参数显示。
    player_render_path_combo_->setVisible(false);
    player_advanced_params_check_ = new QCheckBox(
        UiText("Advanced parameters", "高级参数"),
        player_page);
    player_advanced_params_check_->setObjectName(
        QString::fromUtf8("player_advanced_params_check"));
    player_advanced_params_check_->setChecked(false);
    player_advanced_params_panel_ = new QWidget(player_page);
    player_advanced_params_panel_->setObjectName(
        QString::fromUtf8("player_advanced_params_panel"));
    player_buffer_ms_edit_ = new QLineEdit(player_page);
    player_buffer_ms_edit_->setObjectName(QString::fromUtf8("player_buffer_ms_edit"));
    player_buffer_ms_edit_->setText(QString::fromUtf8("300"));
    player_buffer_ms_edit_->setMaximumWidth(88);
    player_buffer_ms_edit_->setToolTip(UiText(
        "Target playback buffer in milliseconds. Late frames are dropped to avoid accumulated latency.",
        "目标播放缓冲时长；出现积压时会丢弃过期帧，避免延迟累积。"));
    player_audio_queue_edit_ = new QLineEdit(player_page);
    player_audio_queue_edit_->setObjectName(
        QString::fromUtf8("player_audio_queue_edit"));
    player_audio_queue_edit_->setText(QString::fromUtf8("24"));
    player_audio_queue_edit_->setMaximumWidth(88);
    player_audio_queue_edit_->setToolTip(UiText(
        "Audio queue cap. It protects jitter buffering and is not an additional latency target.",
        "音频队列上限；用于保护抖动缓冲，不是额外延迟目标。"));
    player_video_queue_edit_ = new QLineEdit(player_page);
    player_video_queue_edit_->setObjectName(
        QString::fromUtf8("player_video_queue_edit"));
    player_video_queue_edit_->setText(QString::fromUtf8("12"));
    player_video_queue_edit_->setMaximumWidth(88);
    player_video_queue_edit_->setToolTip(UiText(
        "Video queue cap. The player catches up by dropping expired frames when needed.",
        "视频队列上限；必要时播放器会丢弃过期帧追赶当前时钟。"));
    ApplyPlayerLatencyPreset(player_latency_preset_combo_->currentIndex());
    player_audio_volume_slider_ = new QSlider(Qt::Horizontal, player_page);
    player_audio_volume_slider_->setObjectName(
        QString::fromUtf8("player_audio_volume_slider"));
    player_audio_volume_slider_->setRange(0, 100);
    player_audio_volume_slider_->setValue(100);
    player_audio_volume_slider_->setFixedWidth(128);
    player_audio_volume_value_label_ = new QLabel(player_page);
    player_audio_volume_value_label_->setObjectName(
        QString::fromUtf8("player_audio_volume_value_label"));
    player_audio_volume_value_label_->setMinimumWidth(44);
    player_onvif_hint_label_ = new QLabel(player_onvif_controls);
    player_onvif_hint_label_->setObjectName(
        QString::fromUtf8("player_onvif_hint_label"));
    player_onvif_hint_label_->setWordWrap(true);
    player_onvif_hint_label_->setVisible(false);
    player_onvif_hint_label_->setStyleSheet(QString::fromUtf8(
        "QLabel#player_onvif_hint_label { color: #6B7280; }"));
    player_onvif_device_list_ = new QListWidget(player_onvif_controls);
    player_onvif_device_list_->setObjectName(
        QString::fromUtf8("player_onvif_device_list"));
    player_onvif_device_list_->setMinimumHeight(76);
    player_onvif_device_list_->setMaximumHeight(88);
    player_onvif_username_edit_ = new QLineEdit(player_onvif_controls);
    player_onvif_username_edit_->setObjectName(
        QString::fromUtf8("player_onvif_username_edit"));
    player_onvif_username_edit_->setPlaceholderText(
        UiText("Optional user for stream URI", "解析流地址的可选用户名"));
    player_onvif_username_edit_->setMaximumWidth(220);
    player_onvif_password_edit_ = new QLineEdit(player_onvif_controls);
    player_onvif_password_edit_->setObjectName(
        QString::fromUtf8("player_onvif_password_edit"));
    player_onvif_password_edit_->setPlaceholderText(
        UiText("Optional password", "可选密码"));
    player_onvif_password_edit_->setEchoMode(QLineEdit::Password);
    player_onvif_password_edit_->setMaximumWidth(220);
    player_onvif_search_button_ = new QPushButton(
        UiText("Search ONVIF", "搜索 ONVIF"),
        player_onvif_controls);
    player_onvif_search_button_->setObjectName(
        QString::fromUtf8("player_onvif_search_button"));
    player_onvif_apply_button_ = new QPushButton(
        UiText("Resolve/use stream", "解析/使用流"),
        player_onvif_controls);
    player_onvif_apply_button_->setObjectName(
        QString::fromUtf8("player_onvif_apply_button"));
    player_onvif_apply_button_->setEnabled(false);
#if !STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
    player_onvif_apply_button_->setVisible(false);
#endif
    player_start_button_ = new QPushButton(UiText("Play", "播放"), player_page);
    player_start_button_->setObjectName(QString::fromUtf8("player_start_button"));
    player_status_label_ = new QLabel(player_page);
    player_status_label_->setObjectName(QString::fromUtf8("player_status_label"));
    player_status_label_->setWordWrap(true);
    player_status_label_->setMinimumHeight(48);
    player_status_label_->setStyleSheet(QString::fromUtf8(
        "QLabel#player_status_label { background: #F8FAFC; border: 1px solid #D8E0E8; border-radius: 4px; padding: 6px 8px; color: #334155; }"));
    player_status_label_->setText(UiText(
        "Playback idle.",
        "播放空闲。"));
    player_first_frame_label_ = new QLabel(player_page);
    player_first_frame_label_->setObjectName(
        QString::fromUtf8("player_first_frame_label"));
    player_first_frame_label_->setWordWrap(true);
    player_first_frame_label_->setMinimumHeight(48);
    player_first_frame_label_->setStyleSheet(QString::fromUtf8(
        "QLabel#player_first_frame_label { background: #F8FAFC; border: 1px solid #D8E0E8; border-radius: 4px; padding: 6px 8px; color: #334155; }"));
    player_first_frame_label_->setText(UiText(
        "First video frame: waiting for playback.",
        "首视频帧：等待播放。"));
    player_runtime_log_ = new QPlainTextEdit(player_page);
    player_runtime_log_->setObjectName(QString::fromUtf8("player_runtime_log"));
    player_runtime_log_->setReadOnly(true);
    player_source_summary_label_ = new QLabel(player_page);
    player_source_summary_label_->setObjectName(
        QString::fromUtf8("player_source_summary_label"));
    player_source_summary_label_->setWordWrap(true);
    player_source_summary_label_->setStyleSheet(QString::fromUtf8(
        "QLabel#player_source_summary_label { background: #F6F8FB; border: 1px solid #D8E0E8; border-radius: 4px; padding: 8px; color: #334155; }"));

    gb28181_source_combo_ = new QComboBox(gb28181_page);
    gb28181_source_combo_->setObjectName(
        QString::fromUtf8("gb28181_source_combo"));
    gb28181_source_combo_->addItems({
        UiText("Camera", "摄像头"),
        UiText("Desktop capture", "桌面采集")
    });
    gb28181_video_source_combo_ = new QComboBox(gb28181_page);
    gb28181_video_source_combo_->setObjectName(
        QString::fromUtf8("gb28181_video_source_combo"));
    gb28181_audio_combo_ = new QComboBox(gb28181_page);
    gb28181_audio_combo_->setObjectName(
        QString::fromUtf8("gb28181_audio_combo"));
    gb28181_audio_combo_->addItem(UiText("None", "无"), kGb28181AudioNone);
    gb28181_audio_combo_->addItem(UiText("Microphone", "麦克风"), kGb28181AudioMicrophone);
    gb28181_audio_combo_->addItem(UiText("System audio", "系统声音"), kGb28181AudioSystem);
    gb28181_audio_combo_->setCurrentIndex(1);
    gb28181_audio_source_combo_ = new QComboBox(gb28181_page);
    gb28181_audio_source_combo_->setObjectName(
        QString::fromUtf8("gb28181_audio_source_combo"));
    gb28181_resolution_combo_ = new QComboBox(gb28181_page);
    gb28181_resolution_combo_->setObjectName(
        QString::fromUtf8("gb28181_resolution_combo"));
    gb28181_fps_edit_ = new QLineEdit(gb28181_page);
    gb28181_fps_edit_->setObjectName(QString::fromUtf8("gb28181_fps_edit"));
    gb28181_fps_edit_->setText(QString::fromUtf8("25"));
    gb28181_fps_edit_->setMaximumWidth(80);
    gb28181_audio_volume_slider_ = new QSlider(Qt::Horizontal, gb28181_page);
    gb28181_audio_volume_slider_->setObjectName(
        QString::fromUtf8("gb28181_audio_volume_slider"));
    gb28181_audio_volume_slider_->setRange(0, 100);
    gb28181_audio_volume_slider_->setValue(100);
    gb28181_audio_volume_slider_->setFixedWidth(128);
    gb28181_audio_volume_value_label_ = new QLabel(gb28181_page);
    gb28181_audio_volume_value_label_->setObjectName(
        QString::fromUtf8("gb28181_audio_volume_value_label"));
    gb28181_audio_volume_value_label_->setMinimumWidth(44);
    gb28181_local_id_edit_ = new QLineEdit(gb28181_page);
    gb28181_local_id_edit_->setObjectName(QString::fromUtf8("gb28181_local_id_edit"));
    gb28181_local_id_edit_->setText(QString::fromUtf8("34020000001320000001"));
    gb28181_local_domain_edit_ = new QLineEdit(gb28181_page);
    gb28181_local_domain_edit_->setObjectName(QString::fromUtf8("gb28181_local_domain_edit"));
    gb28181_local_domain_edit_->setText(QString::fromUtf8("3402000000"));
    gb28181_local_port_edit_ = new QLineEdit(gb28181_page);
    gb28181_local_port_edit_->setObjectName(QString::fromUtf8("gb28181_local_port_edit"));
    gb28181_local_port_edit_->setText(QString::fromUtf8("5060"));
    gb28181_local_port_edit_->setMaximumWidth(88);
    gb28181_upper_id_edit_ = new QLineEdit(gb28181_page);
    gb28181_upper_id_edit_->setObjectName(QString::fromUtf8("gb28181_upper_id_edit"));
    gb28181_upper_id_edit_->setText(QString::fromUtf8("34020000002000000001"));
    gb28181_upper_domain_edit_ = new QLineEdit(gb28181_page);
    gb28181_upper_domain_edit_->setObjectName(QString::fromUtf8("gb28181_upper_domain_edit"));
    gb28181_upper_domain_edit_->setText(QString::fromUtf8("3402000000"));
    gb28181_upper_password_edit_ = new QLineEdit(gb28181_page);
    gb28181_upper_password_edit_->setObjectName(QString::fromUtf8("gb28181_upper_password_edit"));
    gb28181_upper_password_edit_->setText(QString::fromUtf8("123456"));
    gb28181_upper_ip_edit_ = new QLineEdit(gb28181_page);
    gb28181_upper_ip_edit_->setObjectName(
        QString::fromUtf8("gb28181_upper_ip_edit"));
    gb28181_upper_ip_edit_->setText(QString::fromUtf8("192.0.2.1"));
    gb28181_upper_port_edit_ = new QLineEdit(gb28181_page);
    gb28181_upper_port_edit_->setObjectName(
        QString::fromUtf8("gb28181_upper_port_edit"));
    gb28181_upper_port_edit_->setText(QString::fromUtf8("5060"));
    gb28181_upper_port_edit_->setMaximumWidth(88);
    gb28181_upper_transport_combo_ = new QComboBox(gb28181_page);
    gb28181_upper_transport_combo_->setObjectName(
        QString::fromUtf8("gb28181_upper_transport_combo"));
    gb28181_upper_transport_combo_->addItem(QString::fromUtf8("UDP"), QString::fromUtf8("udp"));
    gb28181_upper_transport_combo_->addItem(QString::fromUtf8("TCP"), QString::fromUtf8("tcp"));
    gb28181_media_port_edit_ = new QLineEdit(gb28181_page);
    gb28181_media_port_edit_->setObjectName(QString::fromUtf8("gb28181_media_port_edit"));
    gb28181_media_port_edit_->setText(QString::fromUtf8("19000"));
    gb28181_media_port_edit_->setMaximumWidth(88);
    gb28181_source_summary_label_ = new QLabel(gb28181_page);
    gb28181_source_summary_label_->setObjectName(
        QString::fromUtf8("gb28181_source_summary_label"));
    gb28181_source_summary_label_->setWordWrap(true);
    gb28181_source_summary_label_->setStyleSheet(QString::fromUtf8(
        "QLabel#gb28181_source_summary_label { background: #F6F8FB; border: 1px solid #D8E0E8; border-radius: 4px; padding: 8px; color: #334155; }"));
    gb28181_preview_frame_ = new QWidget(gb28181_page);
    gb28181_preview_frame_->setObjectName(
        QString::fromUtf8("gb28181_preview_frame"));
    gb28181_preview_frame_->setMinimumSize(520, kPreviewFrameMinHeight);
    gb28181_preview_frame_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    gb28181_preview_frame_->setStyleSheet(QString::fromUtf8(
        "QWidget#gb28181_preview_frame { background: #111827; border: 1px solid #2F3A45; }"));
    gb28181_preview_frame_->installEventFilter(this);
    gb28181_preview_label_ = new QLabel(gb28181_preview_frame_);
    gb28181_preview_label_->setObjectName(QString::fromUtf8("gb28181_preview_label"));
    gb28181_preview_label_->setAlignment(Qt::AlignCenter);
    gb28181_preview_label_->setStyleSheet(QString::fromUtf8(
        "QLabel { background: #111827; color: #DDE6EF; border: 0; }"));
    gb28181_preview_label_->setText(UiText(
        "GB28181 Preview",
        "GB28181 预览"));
    gb28181_watermark_label_ =
        CreateDemoWatermarkLabel(gb28181_preview_frame_);
    gb28181_runtime_log_ = new QPlainTextEdit(gb28181_page);
    gb28181_runtime_log_->setObjectName(QString::fromUtf8("gb28181_runtime_log"));
    gb28181_runtime_log_->setReadOnly(true);
    gb28181_start_button_ = new QPushButton(
        UiText("Start", "开始"),
        gb28181_page);
    gb28181_start_button_->setObjectName(
        QString::fromUtf8("gb28181_start_button"));

    session_table_ = new QTableWidget(page);
    session_table_->setObjectName(QString::fromUtf8("session_table"));
    session_table_->setColumnCount(7);
    session_table_->setHorizontalHeaderLabels({
        UiText("Scenario", "场景"),
        UiText("Type", "类型"),
        UiText("State", "状态"),
        UiText("Ready", "就绪"),
        UiText("Input", "输入"),
        UiText("Transcode", "转码"),
        UiText("Summary", "摘要")
    });
    session_table_->verticalHeader()->setVisible(false);
    session_table_->verticalHeader()->setDefaultSectionSize(32);
    session_table_->horizontalHeader()->setMinimumHeight(34);
    session_table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    session_table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    session_table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    session_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    session_table_->setSelectionMode(QAbstractItemView::NoSelection);

    capability_table_ = new QTableWidget(page);
    capability_table_->setObjectName(QString::fromUtf8("capability_table"));
    capability_table_->setColumnCount(5);
    capability_table_->setHorizontalHeaderLabels({
        UiText("Group", "分组"),
        QString::fromUtf8("Key"),
        UiText("Built", "构建启用"),
        UiText("License", "授权"),
        UiText("Description", "说明")
    });
    capability_table_->verticalHeader()->setVisible(false);
    capability_table_->verticalHeader()->setDefaultSectionSize(32);
    capability_table_->horizontalHeader()->setMinimumHeight(34);
    capability_table_->horizontalHeader()->setStretchLastSection(true);
    capability_table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    capability_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    capability_table_->setSelectionMode(QAbstractItemView::NoSelection);

    desktop_render_target_frame_ = new QWidget(player_page);
    desktop_render_target_frame_->setObjectName(
        QString::fromUtf8("desktop_render_target_frame"));
    desktop_render_target_frame_->setMinimumSize(520, kPreviewFrameMinHeight);
    desktop_render_target_frame_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    desktop_render_target_frame_->setStyleSheet(QString::fromUtf8(
        "QWidget#desktop_render_target_frame { background: #111827; border: 1px solid #2F3A45; }"));
    desktop_render_target_frame_->installEventFilter(this);
    desktop_render_target_widget_ = new QWidget(desktop_render_target_frame_);
    desktop_render_target_widget_->setObjectName(
        QString::fromUtf8("desktop_render_target_widget"));
    desktop_render_target_widget_->setAttribute(Qt::WA_NativeWindow);
    desktop_render_target_widget_->setAutoFillBackground(true);
    desktop_render_target_widget_->setStyleSheet(QString::fromUtf8(
        "QWidget#desktop_render_target_widget { background: #111827; border: 0; }"));
    desktop_render_target_widget_->installEventFilter(this);
    desktop_render_target_widget_->setFocusPolicy(Qt::StrongFocus);
    QPalette render_palette = desktop_render_target_widget_->palette();
    render_palette.setColor(QPalette::Window, QColor(17, 24, 39));
    desktop_render_target_widget_->setPalette(render_palette);
    player_watermark_label_ =
        CreateDemoWatermarkLabel(desktop_render_target_frame_);

    desktop_render_target_status_label_ = new QLabel(player_page);
    desktop_render_target_status_label_->setObjectName(
        QString::fromUtf8("desktop_render_target_status_label"));
    desktop_render_target_status_label_->setWordWrap(true);
    desktop_render_target_status_label_->setMinimumHeight(28);
    desktop_render_target_status_label_->setStyleSheet(QString::fromUtf8(
        "QLabel#desktop_render_target_status_label { color: #3B536B; padding: 4px 0; }"));
    desktop_render_target_status_label_->setVisible(false);

    connect(refresh_button, &QPushButton::clicked, this, [this]() {
        LoadSnapshot();
        BindDesktopRenderTarget();
        statusBar()->showMessage(
            UiText("Snapshot refreshed.", "快照已刷新。"),
            3000);
    });
    connect(copy_machine_id_button, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(current_machine_id_);
        statusBar()->showMessage(
            UiText("Machine ID copied to clipboard.", "机器码已复制到剪贴板。"),
            3000);
    });
    connect(share_logs_button_, &QPushButton::clicked, this, [this]() {
        ShareLogs();
    });
    connect(upload_logs_button_, &QPushButton::clicked, this, [this]() {
        ShowUploadReserved();
    });
    connect(language_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
            if (index == 2)
            {
                SetLanguage(DemoLanguage::Chinese);
            }
            else if (index == 1)
            {
                SetLanguage(DemoLanguage::English);
            }
            else
            {
                SetLanguage(DemoLanguage::Auto);
            }
    });
    connect(
        preview_display_mode_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            ApplyPreviewDisplayMode();
            UpdatePublisherSourceSummary();
            UpdatePlayerSourceSummary();
            UpdateGb28181SourceSummary();
            BindDesktopRenderTarget();
        });
    connect(
        publisher_source_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            RefreshPublisherCameraDevices();
            UpdatePublisherSourceControls();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_camera_device_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            RefreshPublisherResolutionOptions();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_file_path_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) {
            RefreshPublisherResolutionOptions();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_file_browse_button_,
        &QPushButton::clicked,
        this,
        [this]() { BrowsePublisherMediaFile(); });
    connect(
        publisher_audio_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            RefreshPublisherAudioDevices();
            UpdatePublisherSourceControls();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_audio_source_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_audio_file_path_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_audio_file_browse_button_,
        &QPushButton::clicked,
        this,
        [this]() { BrowsePublisherAudioFile(); });
    connect(
        publisher_audio_volume_slider_,
        &QSlider::valueChanged,
        this,
        [this](int value) {
            UpdateAudioVolumeLabels();
            if (active_publisher_capture_ != nullptr)
            {
                streamcore_capture_set_audio_volume(
                    active_publisher_capture_,
                    value);
            }
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_protocol_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            UpdatePublisherTargetControls(true);
            UpdatePublisherSourceControls();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_url_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) {
            UpdatePublisherTargetControls(false);
            UpdatePublisherSourceControls();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_whip_bearer_token_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_video_codec_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            UpdatePublisherSourceControls();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_audio_codec_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            if (publisher_audio_profile_combo_ != nullptr)
            {
                const bool use_opus =
                    SelectedPublisherAudioCodec().compare(
                        QString::fromUtf8("opus"),
                        Qt::CaseInsensitive) == 0;
                const QSignalBlocker blocker(publisher_audio_profile_combo_);
                publisher_audio_profile_combo_->clear();
                if (use_opus)
                {
                    publisher_audio_profile_combo_->addItem(
                        QString::fromUtf8("Opus"),
                        QString::fromUtf8("opus"));
                }
                else
                {
                    publisher_audio_profile_combo_->addItem(
                        QString::fromUtf8("AAC-LC"),
                        QString::fromUtf8("aac"));
                    publisher_audio_profile_combo_->addItem(
                        QString::fromUtf8("HE-AAC"),
                        QString::fromUtf8("heaac"));
                    publisher_audio_profile_combo_->addItem(
                        QString::fromUtf8("AAC-ELD"),
                        QString::fromUtf8("aac-eld"));
                }
            }
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_audio_profile_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_audio_sample_rate_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_audio_bitrate_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_rtmp_hevc_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_file_mode_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            UpdatePublisherSourceControls();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_preview_toggle_,
        &QCheckBox::toggled,
        this,
        [this](bool) {
            ApplyPublisherPreviewToggle();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_processor_compare_toggle_,
        &QCheckBox::toggled,
        this,
        [this](bool checked) {
            if (publisher_original_preview_caption_ != nullptr)
            {
                publisher_original_preview_caption_->setVisible(checked);
            }
            if (publisher_processed_preview_column_ != nullptr)
            {
                publisher_processed_preview_column_->setVisible(checked);
            }
            UpdatePublisherSourceControls();
            ApplyPreviewDisplayMode();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_resolution_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            ApplyPreviewDisplayMode();
            UpdatePublisherSourceSummary();
        });
    connect(
        publisher_video_bitrate_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_fps_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_gop_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePublisherSourceSummary(); });
    connect(
        publisher_start_button_,
        &QPushButton::clicked,
        this,
        [this]() {
            if (active_publisher_ != nullptr ||
                active_publisher_capture_ != nullptr)
            {
                StopPublisher();
                return;
            }
            StartPublisher();
        });
    connect(
        player_url_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePlayerSourceSummary(); });
    connect(
        player_latency_preset_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int index) {
            ApplyPlayerLatencyPreset(index);
            UpdatePlayerSourceSummary();
        });
    connect(
        player_decode_mode_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdatePlayerSourceSummary(); });
    connect(
        player_render_path_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdatePlayerSourceSummary(); });
    connect(
        player_advanced_params_check_,
        &QCheckBox::toggled,
        this,
        [this](bool checked) {
            player_buffer_ms_edit_->setEnabled(checked);
            player_audio_queue_edit_->setEnabled(checked);
            player_video_queue_edit_->setEnabled(checked);
        });
    connect(
        player_buffer_ms_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePlayerSourceSummary(); });
    connect(
        player_audio_queue_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePlayerSourceSummary(); });
    connect(
        player_video_queue_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdatePlayerSourceSummary(); });
    connect(
        player_audio_volume_slider_,
        &QSlider::valueChanged,
        this,
        [this](int value) {
            UpdateAudioVolumeLabels();
            if (active_player_ != nullptr)
            {
                streamcore_player_set_audio_volume(active_player_, value);
            }
            UpdatePlayerSourceSummary();
        });
    connect(
        player_onvif_search_button_,
        &QPushButton::clicked,
        this,
        [this]() { SearchOnvifDevices(); });
    connect(
        player_onvif_apply_button_,
        &QPushButton::clicked,
        this,
        [this]() { ApplySelectedOnvifDevice(); });
    connect(
        player_onvif_device_list_,
        &QListWidget::currentRowChanged,
        this,
        [this](int) { UpdateOnvifDeviceSummary(); });
    connect(
        player_onvif_device_list_,
        &QListWidget::itemDoubleClicked,
        this,
        [this]() { ApplySelectedOnvifDevice(); });
    connect(
        player_start_button_,
        &QPushButton::clicked,
        this,
        [this]() {
            if (active_player_ != nullptr)
            {
                StopPlayerUrl();
                return;
            }
            StartPlayerUrl();
        });
    connect(
        gb28181_source_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_video_source_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_audio_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_audio_source_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_resolution_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_fps_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_upper_ip_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_upper_port_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_upper_transport_combo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) { UpdateGb28181SourceSummary(); });
    connect(
        gb28181_audio_volume_slider_,
        &QSlider::valueChanged,
        this,
        [this](int) {
            UpdateAudioVolumeLabels();
            UpdateGb28181SourceSummary();
        });
    connect(
        gb28181_start_button_,
        &QPushButton::clicked,
        this,
        [this]() {
#if STREAMCORE_DEMO_ENABLE_GB28181
            if (active_gb28181_ != nullptr)
            {
                StopGb28181();
                return;
            }
#endif
            StartGb28181();
        });
    action_layout->addWidget(refresh_button);
    action_layout->addWidget(display_mode_label);
    action_layout->addWidget(preview_display_mode_combo_);
    action_layout->addSpacing(12);
    action_layout->addWidget(language_label);
    action_layout->addWidget(language_combo_);
    action_layout->setContentsMargins(0, 0, 0, 0);
    action_layout->setSpacing(8);
    license_meta_action_layout->addWidget(copy_machine_id_button);
    license_meta_action_layout->addWidget(share_logs_button_);
    license_meta_action_layout->addWidget(upload_logs_button_);
    license_meta_action_layout->addStretch(1);

    overview_layout->setContentsMargins(12, 10, 12, 10);
    overview_layout->setSpacing(0);
    overview_header_layout->setContentsMargins(0, 0, 0, 0);
    overview_header_layout->setSpacing(12);
    overview_text_layout->setContentsMargins(0, 0, 0, 0);
    overview_text_layout->setSpacing(2);
    overview_text_layout->addWidget(product_title_label_);
    overview_text_layout->addWidget(product_meta_label_);
    overview_text_layout->addWidget(brand_contact_label);
    overview_header_layout->addWidget(logo_label, 0, Qt::AlignTop);
    overview_header_layout->addLayout(overview_text_layout, 1);
    overview_header_layout->addLayout(action_layout, 0);
    overview_header_layout->setAlignment(action_layout, Qt::AlignTop | Qt::AlignRight);
    overview_layout->addLayout(overview_header_layout);

    const bool use_compact_chinese_spacing =
        EffectiveLanguage() == DemoLanguage::Chinese;
    const int kDesktopFieldLabelWidth = use_compact_chinese_spacing ? 88 : 112;
    const int kDesktopFieldRowSpacing = use_compact_chinese_spacing ? 4 : 6;
    const int kDesktopInlineSpacing = use_compact_chinese_spacing ? 6 : 8;
    const int kDesktopInlineFieldGap = use_compact_chinese_spacing ? 18 : 20;
    const int kDesktopActionButtonGap = use_compact_chinese_spacing ? 24 : 28;
    const int kDesktopSectionSpacing = 8;
    const auto createFieldRow =
        [kDesktopFieldLabelWidth, kDesktopFieldRowSpacing](QWidget* parent, const QString& labelText, QWidget* editor) -> QWidget*
    {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        QLabel* label = new QLabel(labelText, row);
        label->setWordWrap(false);
        if (kDesktopFieldLabelWidth > 0)
        {
            label->setMinimumWidth(kDesktopFieldLabelWidth);
            label->setMaximumWidth(kDesktopFieldLabelWidth);
        }
        else
        {
            label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        }
        label->setMinimumHeight(28);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        if (editor != nullptr)
        {
            editor->setSizePolicy(
                QSizePolicy::Expanding,
                editor->sizePolicy().verticalPolicy());
        }
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kDesktopFieldRowSpacing);
        layout->addWidget(label, 0, Qt::AlignVCenter);
        layout->addWidget(editor, 1, Qt::AlignVCenter);
        return row;
    };
    const auto createInlineHost = [kDesktopInlineSpacing](QWidget* parent) -> QHBoxLayout*
    {
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kDesktopInlineSpacing);
        Q_UNUSED(parent);
        return layout;
    };
    const auto createStackedField =
        [kDesktopFieldRowSpacing](QWidget* parent, const QString& labelText, QWidget* editor) -> QWidget*
    {
        QWidget* row = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(row);
        QLabel* label = new QLabel(labelText, row);
        label->setWordWrap(false);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        if (editor != nullptr)
        {
            editor->setSizePolicy(
                QSizePolicy::Expanding,
                editor->sizePolicy().verticalPolicy());
        }
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kDesktopFieldRowSpacing);
        layout->addWidget(label);
        layout->addWidget(editor);
        return row;
    };
    const auto createInlineLabel = [](QWidget* parent, const QString& text) -> QLabel*
    {
        QLabel* label = new QLabel(text, parent);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setMinimumHeight(28);
        label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        return label;
    };
    const auto addInlineLabeledWidget =
        [createInlineLabel, kDesktopInlineFieldGap](
            QHBoxLayout* layout,
            QWidget* parent,
            const QString& labelText,
            QWidget* editor,
            int editorStretch)
    {
        layout->addSpacing(kDesktopInlineFieldGap);
        layout->addWidget(createInlineLabel(parent, labelText), 0, Qt::AlignVCenter);
        layout->addWidget(editor, editorStretch, Qt::AlignVCenter);
    };
    const auto configureRuntimeLog = [](QPlainTextEdit* logView)
    {
        if (logView == nullptr)
        {
            return;
        }
        logView->setMaximumBlockCount(800);
        logView->setMinimumHeight(44);
        logView->setMaximumHeight(68);
    };

    configureRuntimeLog(publisher_runtime_log_);
    configureRuntimeLog(player_runtime_log_);
    configureRuntimeLog(gb28181_runtime_log_);
    if (player_onvif_hint_label_ != nullptr)
    {
#if STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
        player_onvif_hint_label_->setText(UiText(
            "Double-click a discovered device below to resolve and use its RTSP stream.",
            "双击下方设备即可解析并使用 RTSP 流地址。"));
#else
        player_onvif_hint_label_->setText(UiText(
            "ONVIF discovery is enabled. Enter the RTSP URL manually to start playback.",
            "已启用 ONVIF 发现；播放时请手动填写 RTSP 地址。"));
#endif
    }
    if (player_onvif_device_list_ != nullptr)
    {
        player_onvif_device_list_->addItem(
            UiText("No ONVIF devices discovered", "尚未搜索到 ONVIF 设备"));
    }

    QWidget* publisher_left_widget = new QWidget(publisher_page);
    publisher_left_widget->setFixedWidth(kSidebarWidth);
    publisher_left_layout = new QVBoxLayout(publisher_left_widget);
    publisher_left_layout->setContentsMargins(0, 0, 0, 0);
    publisher_left_layout->setSpacing(10);
    publisher_content_layout->setSpacing(12);
    publisher_preview_layout->setSpacing(8);
    publisher_runtime_box->setMinimumHeight(kRuntimeLogHeight);
    publisher_runtime_box->setMaximumHeight(kRuntimeLogHeight + 36);
    publisher_runtime_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    QWidget* publisher_video_detail_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_video_detail_layout = createInlineHost(publisher_video_detail_editor);
    publisher_video_detail_editor->setLayout(publisher_video_detail_layout);
    publisher_video_detail_layout->addWidget(publisher_camera_device_combo_, 1);
    publisher_video_detail_layout->addWidget(publisher_file_path_edit_, 2);
    publisher_video_detail_layout->addWidget(publisher_file_browse_button_);
    publisher_video_detail_row_ = createFieldRow(
        publisher_page,
        UiText("Video details", "视频设置"),
        publisher_video_detail_editor);

    QWidget* publisher_resolution_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_resolution_layout = createInlineHost(publisher_resolution_editor);
    publisher_resolution_editor->setLayout(publisher_resolution_layout);
    publisher_resolution_layout->addWidget(publisher_resolution_combo_, 1);
    addInlineLabeledWidget(
        publisher_resolution_layout,
        publisher_resolution_editor,
        UiText("FPS", "帧率"),
        publisher_fps_edit_,
        0);

    QWidget* publisher_audio_detail_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_audio_detail_layout = createInlineHost(publisher_audio_detail_editor);
    publisher_audio_detail_editor->setLayout(publisher_audio_detail_layout);
    publisher_audio_detail_layout->addWidget(publisher_audio_detail_label_);
    publisher_audio_detail_layout->addWidget(publisher_audio_source_combo_, 1);
    publisher_audio_detail_layout->addSpacing(kDesktopInlineFieldGap);
    publisher_audio_detail_layout->addWidget(publisher_audio_file_label_, 0, Qt::AlignVCenter);
    publisher_audio_detail_layout->addWidget(publisher_audio_file_path_edit_, 2);
    publisher_audio_detail_layout->addWidget(publisher_audio_file_browse_button_);
    publisher_audio_detail_row_ = createFieldRow(
        publisher_page,
        UiText("Audio details", "音频设置"),
        publisher_audio_detail_editor);

    QWidget* publisher_video_codec_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_video_codec_layout = createInlineHost(publisher_video_codec_editor);
    publisher_video_codec_editor->setLayout(publisher_video_codec_layout);
    publisher_video_codec_layout->addWidget(publisher_video_codec_combo_, 1);

    QWidget* publisher_video_rate_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_video_rate_layout = createInlineHost(publisher_video_rate_editor);
    publisher_video_rate_editor->setLayout(publisher_video_rate_layout);
    publisher_video_rate_layout->addWidget(publisher_video_bitrate_edit_, 0, Qt::AlignVCenter);
    addInlineLabeledWidget(
        publisher_video_rate_layout,
        publisher_video_rate_editor,
        QString::fromUtf8("GOP"),
        publisher_gop_edit_,
        0);
    publisher_video_rate_layout->addStretch(1);

    QWidget* publisher_audio_codec_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_audio_codec_layout = createInlineHost(publisher_audio_codec_editor);
    publisher_audio_codec_editor->setLayout(publisher_audio_codec_layout);
    publisher_audio_codec_layout->addWidget(publisher_audio_codec_combo_);
    addInlineLabeledWidget(
        publisher_audio_codec_layout,
        publisher_audio_codec_editor,
        UiText("Audio profile", "音频参数"),
        publisher_audio_profile_combo_,
        1);

    QWidget* publisher_audio_rate_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_audio_rate_layout = createInlineHost(publisher_audio_rate_editor);
    publisher_audio_rate_editor->setLayout(publisher_audio_rate_layout);
    publisher_audio_rate_layout->addWidget(publisher_audio_sample_rate_combo_, 0, Qt::AlignVCenter);
    addInlineLabeledWidget(
        publisher_audio_rate_layout,
        publisher_audio_rate_editor,
        UiText("Audio bitrate", "音频码率"),
        publisher_audio_bitrate_combo_,
        0);
    publisher_audio_rate_layout->addStretch(1);

    QWidget* publisher_hevc_preview_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_hevc_preview_layout = createInlineHost(publisher_hevc_preview_editor);
    publisher_hevc_preview_editor->setLayout(publisher_hevc_preview_layout);
    publisher_hevc_preview_layout->addWidget(publisher_rtmp_hevc_combo_, 1);
    publisher_hevc_preview_layout->addWidget(publisher_preview_toggle_);
    publisher_hevc_preview_layout->addWidget(
        publisher_processor_compare_toggle_);

    publisher_file_mode_row_ = createFieldRow(
        publisher_page,
        UiText("File transcode", "文件转码"),
        publisher_file_mode_combo_);

    QWidget* publisher_action_editor = new QWidget(publisher_page);
    QHBoxLayout* publisher_action_editor_layout = createInlineHost(publisher_action_editor);
    publisher_action_editor->setLayout(publisher_action_editor_layout);
    publisher_action_editor_layout->addWidget(publisher_start_button_);
    publisher_action_editor_layout->addSpacing(kDesktopActionButtonGap);
    publisher_action_editor_layout->addWidget(
        createInlineLabel(publisher_page, UiText("Volume", "音量")));
    publisher_action_editor_layout->addWidget(publisher_audio_volume_slider_, 1);
    publisher_action_editor_layout->addWidget(publisher_audio_volume_value_label_);
    publisher_action_editor_layout->addStretch(1);

    publisher_source_layout->setContentsMargins(10, 10, 10, 10);
    publisher_source_layout->setSpacing(kDesktopSectionSpacing);
    publisher_source_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Video", "视频"),
        publisher_source_combo_));
    publisher_source_layout->addWidget(publisher_video_detail_row_);
    publisher_source_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Resolution", "分辨率"),
        publisher_resolution_editor));
    publisher_source_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Audio", "音频"),
        publisher_audio_combo_));
    publisher_source_layout->addWidget(publisher_audio_detail_row_);

    publisher_publish_layout->setContentsMargins(10, 10, 10, 10);
    publisher_publish_layout->setSpacing(kDesktopSectionSpacing);
    publisher_publish_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Protocol", "推流协议"),
        publisher_protocol_combo_));
    publisher_publish_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Video codec", "视频编码"),
        publisher_video_codec_editor));
    publisher_publish_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Bitrate", "码率"),
        publisher_video_rate_editor));
    publisher_publish_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Audio codec", "音频编码"),
        publisher_audio_codec_editor));
    publisher_publish_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Sample rate", "采样率"),
        publisher_audio_rate_editor));
    publisher_publish_layout->addWidget(createFieldRow(
        publisher_page,
        UiText("Publish URL", "推流地址"),
        publisher_url_edit_));
    publisher_whip_bearer_row_ = createFieldRow(
        publisher_page,
        QString::fromUtf8("Bearer Token"),
        publisher_whip_bearer_token_edit_);
    publisher_publish_layout->addWidget(publisher_whip_bearer_row_);
    publisher_publish_layout->addWidget(publisher_file_mode_row_);
    publisher_publish_layout->addWidget(createFieldRow(
        publisher_page,
        QString::fromUtf8("RTMP HEVC"),
        publisher_hevc_preview_editor));

    publisher_action_layout->setContentsMargins(0, 0, 0, 0);
    publisher_action_layout->setSpacing(0);
    publisher_action_layout->addWidget(publisher_action_editor);

    publisher_runtime_layout->setContentsMargins(10, 10, 10, 10);
    publisher_runtime_layout->setSpacing(kDesktopSectionSpacing);
    publisher_runtime_layout->addWidget(publisher_source_summary_label_);
    publisher_runtime_layout->addWidget(publisher_status_label_);
    publisher_runtime_layout->addWidget(publisher_runtime_log_, 1);
    QWidget* publisher_preview_compare_widget = new QWidget(publisher_page);
    QHBoxLayout* publisher_preview_compare_layout =
        new QHBoxLayout(publisher_preview_compare_widget);
    publisher_preview_compare_layout->setContentsMargins(0, 0, 0, 0);
    publisher_preview_compare_layout->setSpacing(8);
    QWidget* publisher_original_preview_column = new QWidget(
        publisher_preview_compare_widget);
    QVBoxLayout* publisher_original_preview_layout =
        new QVBoxLayout(publisher_original_preview_column);
    publisher_original_preview_layout->setContentsMargins(0, 0, 0, 0);
    publisher_original_preview_layout->setSpacing(5);
    publisher_original_preview_caption_ = new QLabel(
        UiText("Original · before Processor", "原始画面 · Processor 前"),
        publisher_original_preview_column);
    publisher_original_preview_caption_->setStyleSheet(QString::fromUtf8(
        "QLabel { color: #475569; font-size: 9pt; font-weight: 600; }"));
    publisher_original_preview_layout->addWidget(
        publisher_original_preview_caption_);
    publisher_original_preview_caption_->setVisible(
        publisher_processor_compare_toggle_->isChecked());
    publisher_original_preview_layout->addWidget(
        publisher_preview_frame_,
        0);
    publisher_processed_preview_column_ = new QWidget(
        publisher_preview_compare_widget);
    QVBoxLayout* publisher_processed_preview_layout =
        new QVBoxLayout(publisher_processed_preview_column_);
    publisher_processed_preview_layout->setContentsMargins(0, 0, 0, 0);
    publisher_processed_preview_layout->setSpacing(5);
    QLabel* publisher_processed_preview_caption = new QLabel(
        UiText("Final · after Processor", "最终画面 · Processor 后"),
        publisher_processed_preview_column_);
    publisher_processed_preview_caption->setStyleSheet(QString::fromUtf8(
        "QLabel { color: #475569; font-size: 9pt; font-weight: 600; }"));
    publisher_processed_preview_layout->addWidget(
        publisher_processed_preview_caption);
    publisher_processed_preview_layout->addWidget(
        publisher_processed_preview_frame_,
        0);
    publisher_preview_compare_layout->addWidget(
        publisher_original_preview_column,
        1);
    publisher_preview_compare_layout->addWidget(
        publisher_processed_preview_column_,
        1);
    publisher_processed_preview_column_->setVisible(
        publisher_processor_compare_toggle_->isChecked());
    publisher_preview_layout->addWidget(
        publisher_preview_compare_widget,
        0);
    publisher_preview_layout->addWidget(publisher_runtime_box, 0);
    publisher_preview_layout->addStretch(1);
    publisher_left_layout->addWidget(publisher_source_box);
    publisher_left_layout->addWidget(publisher_publish_box);
    publisher_left_layout->addWidget(publisher_action_box);
    publisher_content_layout->addWidget(publisher_left_widget, 0);
    publisher_content_layout->addLayout(publisher_preview_layout, 1);
    publisher_content_layout->setSpacing(12);
    publisher_layout->setContentsMargins(12, 12, 12, 12);
    publisher_layout->addLayout(publisher_content_layout, 1);
    UpdatePublisherTargetControls(false);

    QWidget* player_left_widget = new QWidget(player_page);
    player_left_widget->setMinimumWidth(kSidebarWidth);
    player_left_widget->setMaximumWidth(kSidebarWidth);
    player_left_layout = new QVBoxLayout(player_left_widget);
    player_left_layout->setContentsMargins(0, 0, 0, 0);
    player_left_layout->setSpacing(10);
    player_runtime_box->setMinimumHeight(kRuntimeLogHeight);
    player_runtime_box->setMaximumHeight(QWIDGETSIZE_MAX);
    player_runtime_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    player_onvif_controls_layout->setContentsMargins(0, 0, 0, 0);
    player_onvif_controls_layout->setSpacing(6);
    QWidget* player_onvif_search_editor = new QWidget(player_onvif_controls);
    QHBoxLayout* player_onvif_search_layout = createInlineHost(player_onvif_search_editor);
    player_onvif_search_editor->setLayout(player_onvif_search_layout);
    player_onvif_search_layout->addWidget(player_onvif_search_button_);
#if STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
    player_onvif_search_layout->addWidget(player_onvif_apply_button_);
#endif
    player_onvif_search_layout->addStretch(1);
    QWidget* player_onvif_credential_editor = new QWidget(player_onvif_controls);
    QGridLayout* player_onvif_credential_layout = new QGridLayout(player_onvif_credential_editor);
    player_onvif_credential_editor->setLayout(player_onvif_credential_layout);
    player_onvif_credential_layout->setContentsMargins(0, 0, 0, 0);
    player_onvif_credential_layout->setHorizontalSpacing(kDesktopInlineSpacing);
    player_onvif_credential_layout->setVerticalSpacing(kDesktopFieldRowSpacing);
    player_onvif_credential_layout->addWidget(
        createInlineLabel(player_onvif_credential_editor, UiText("Username", "用户名")),
        0,
        0);
    player_onvif_credential_layout->addWidget(player_onvif_username_edit_, 0, 1);
    player_onvif_credential_layout->addWidget(
        createInlineLabel(player_onvif_credential_editor, UiText("Password", "密码")),
        1,
        0);
    player_onvif_credential_layout->addWidget(player_onvif_password_edit_, 1, 1);
    player_onvif_credential_layout->setColumnStretch(1, 1);
    player_onvif_controls_layout->addWidget(player_onvif_search_editor);
    player_onvif_controls_layout->addWidget(createStackedField(
        player_onvif_controls,
        UiText("Device list", "设备列表"),
        player_onvif_device_list_));
#if STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
    player_onvif_controls_layout->addWidget(createStackedField(
        player_onvif_controls,
        UiText("Credentials", "账号密码"),
        player_onvif_credential_editor));
#else
    player_onvif_credential_editor->setVisible(false);
#endif
    player_onvif_layout->setContentsMargins(8, 8, 8, 8);
    player_onvif_layout->setSpacing(6);
    player_onvif_layout->addWidget(player_onvif_controls);

    QWidget* player_decode_render_editor = new QWidget(player_page);
    QHBoxLayout* player_decode_render_layout = createInlineHost(player_decode_render_editor);
    player_decode_render_editor->setLayout(player_decode_render_layout);
    player_decode_render_layout->addWidget(player_decode_mode_combo_, 1);
    QWidget* player_queue_limits_editor = new QWidget(player_advanced_params_panel_);
    QHBoxLayout* player_queue_limits_layout = createInlineHost(
        player_queue_limits_editor);
    player_queue_limits_editor->setLayout(player_queue_limits_layout);
    player_queue_limits_layout->addWidget(createInlineLabel(
        player_queue_limits_editor, UiText("Audio", "音频")));
    player_queue_limits_layout->addWidget(
        player_audio_queue_edit_, 0, Qt::AlignVCenter);
    addInlineLabeledWidget(
        player_queue_limits_layout,
        player_queue_limits_editor,
        UiText("Video", "视频"),
        player_video_queue_edit_,
        0);
    player_queue_limits_layout->addStretch(1);
    QGridLayout* player_advanced_params_layout =
        new QGridLayout(player_advanced_params_panel_);
    player_advanced_params_layout->setContentsMargins(0, 0, 0, 0);
    player_advanced_params_layout->setHorizontalSpacing(kDesktopInlineSpacing);
    player_advanced_params_layout->setVerticalSpacing(kDesktopFieldRowSpacing);
    player_advanced_params_layout->addWidget(
        createInlineLabel(
            player_advanced_params_panel_,
            UiText("Network buffer", "网络缓冲")),
        0,
        0,
        Qt::AlignVCenter);
    player_advanced_params_layout->addWidget(
        player_buffer_ms_edit_,
        0,
        1,
        Qt::AlignLeft | Qt::AlignVCenter);
    player_advanced_params_layout->addWidget(
        createInlineLabel(player_advanced_params_panel_, QString::fromUtf8("ms")),
        0,
        2,
        Qt::AlignVCenter);
    player_advanced_params_layout->addWidget(
        createInlineLabel(
            player_advanced_params_panel_,
            UiText("Queue limits", "队列上限")),
        1,
        0,
        Qt::AlignVCenter);
    player_advanced_params_layout->addWidget(player_queue_limits_editor, 1, 1, 1, 3);
    player_advanced_params_layout->setColumnStretch(3, 1);
    player_buffer_ms_edit_->setEnabled(false);
    player_audio_queue_edit_->setEnabled(false);
    player_video_queue_edit_->setEnabled(false);
    QWidget* player_action_editor = new QWidget(player_page);
    QHBoxLayout* player_action_editor_layout = createInlineHost(player_action_editor);
    player_action_editor->setLayout(player_action_editor_layout);
    player_action_editor_layout->addWidget(player_start_button_);
    player_action_editor_layout->addSpacing(kDesktopActionButtonGap);
    player_action_editor_layout->addWidget(
        createInlineLabel(player_page, UiText("Volume", "音量")));
    player_action_editor_layout->addWidget(player_audio_volume_slider_, 1);
    player_action_editor_layout->addWidget(player_audio_volume_value_label_);
    player_action_editor_layout->addStretch(1);

    player_session_layout->setContentsMargins(8, 6, 8, 6);
    player_session_layout->setSpacing(kDesktopSectionSpacing);
    player_session_layout->addWidget(createFieldRow(
        player_page,
        QString::fromUtf8("URL"),
        player_url_edit_));
    player_tuning_layout->setContentsMargins(0, 0, 0, 0);
    player_tuning_layout->setSpacing(kDesktopSectionSpacing);
    player_tuning_layout->addWidget(createFieldRow(
        player_page,
        UiText("Playback mode", "播放模式"),
        player_latency_preset_combo_));
    player_tuning_layout->addWidget(createFieldRow(
        player_page,
        UiText("Decode", "解码"),
        player_decode_render_editor));
    player_tuning_layout->addWidget(player_advanced_params_check_);
    player_tuning_layout->addWidget(player_advanced_params_panel_);
    player_session_layout->addWidget(player_tuning_box);
    player_session_layout->addWidget(player_onvif_box);
    player_action_layout->setContentsMargins(0, 0, 0, 0);
    player_action_layout->setSpacing(0);
    player_action_layout->addWidget(player_action_editor);
    player_runtime_layout->setContentsMargins(10, 10, 10, 10);
    player_runtime_layout->setSpacing(kDesktopSectionSpacing);
    player_runtime_layout->addWidget(player_source_summary_label_);
    QHBoxLayout* player_runtime_status_layout = new QHBoxLayout();
    player_runtime_status_layout->setContentsMargins(0, 0, 0, 0);
    player_runtime_status_layout->setSpacing(kDesktopSectionSpacing);
    player_runtime_status_layout->addWidget(player_status_label_, 1);
    player_runtime_status_layout->addWidget(player_first_frame_label_, 1);
    player_runtime_layout->addLayout(player_runtime_status_layout);
    player_runtime_layout->addWidget(player_runtime_log_, 1);
    player_preview_layout_ = player_preview_layout;
    player_preview_layout_->setSpacing(12);
    player_preview_layout_->addWidget(desktop_render_target_frame_, 0);
    player_preview_layout_->addWidget(player_runtime_box, 1);
    player_left_layout->addWidget(player_session_box);
    player_left_layout->addWidget(player_action_box);
    player_left_layout->addStretch(1);
    QScrollArea* player_left_scroll = new QScrollArea(player_page);
    player_left_scroll->setObjectName(QString::fromUtf8("player_left_scroll"));
    player_left_scroll->setFrameShape(QFrame::NoFrame);
    player_left_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    player_left_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    player_left_scroll->setWidgetResizable(true);
    player_left_scroll->setMinimumWidth(kSidebarWidth);
    player_left_scroll->setMaximumWidth(kSidebarWidth);
    player_left_scroll->setStyleSheet(QString::fromUtf8(
        "QScrollArea#player_left_scroll { background: #FFFFFF; border: none; }"
        "QScrollArea#player_left_scroll > QWidget > QWidget { background: #FFFFFF; }"));
    player_left_widget->setAutoFillBackground(true);
    QPalette player_left_palette = player_left_widget->palette();
    player_left_palette.setColor(QPalette::Window, QColor(255, 255, 255));
    player_left_widget->setPalette(player_left_palette);
    player_left_scroll->viewport()->setAutoFillBackground(true);
    QPalette player_scroll_palette = player_left_scroll->viewport()->palette();
    player_scroll_palette.setColor(QPalette::Window, QColor(255, 255, 255));
    player_left_scroll->viewport()->setPalette(player_scroll_palette);
    player_left_scroll->setWidget(player_left_widget);
    player_content_layout->addWidget(player_left_scroll, 0);
    player_content_layout->addLayout(player_preview_layout, 1);
    player_content_layout->setSpacing(12);
    player_layout->setContentsMargins(12, 12, 12, 12);
    player_layout->addLayout(player_content_layout, 1);

    QWidget* gb_left_widget = new QWidget(gb28181_page);
    gb_left_widget->setFixedWidth(kSidebarWidth);
    gb28181_left_layout = new QVBoxLayout(gb_left_widget);
    gb28181_left_layout->setContentsMargins(0, 0, 0, 0);
    gb28181_left_layout->setSpacing(10);
    gb28181_runtime_box->setMinimumHeight(kRuntimeLogHeight);
    gb28181_runtime_box->setMaximumHeight(kRuntimeLogHeight + 36);
    gb28181_runtime_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QGroupBox* gb28181_local_box =
        new QGroupBox(UiText("Local SIP", "本机 SIP"), gb28181_page);
    QVBoxLayout* gb28181_local_layout = new QVBoxLayout(gb28181_local_box);
    gb28181_local_box->setMaximumWidth(kDesktopConfigPaneWidth);

    QWidget* gb_video_detail_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_video_detail_layout = createInlineHost(gb_video_detail_editor);
    gb_video_detail_editor->setLayout(gb_video_detail_layout);
    gb_video_detail_layout->addWidget(gb28181_video_source_combo_, 1);
    gb28181_video_detail_row_ = createFieldRow(
        gb28181_page,
        UiText("Video details", "视频设置"),
        gb_video_detail_editor);
    QWidget* gb_resolution_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_resolution_layout = createInlineHost(gb_resolution_editor);
    gb_resolution_editor->setLayout(gb_resolution_layout);
    gb_resolution_layout->addWidget(gb28181_resolution_combo_, 1);
    addInlineLabeledWidget(
        gb_resolution_layout,
        gb_resolution_editor,
        UiText("FPS", "帧率"),
        gb28181_fps_edit_,
        0);
    QWidget* gb_audio_detail_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_audio_detail_layout = createInlineHost(gb_audio_detail_editor);
    gb_audio_detail_editor->setLayout(gb_audio_detail_layout);
    gb_audio_detail_layout->addWidget(gb28181_audio_source_combo_, 1);
    gb28181_audio_detail_row_ = createFieldRow(
        gb28181_page,
        UiText("Audio details", "音频设置"),
        gb_audio_detail_editor);
    QWidget* gb_action_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_action_editor_layout = createInlineHost(gb_action_editor);
    gb_action_editor->setLayout(gb_action_editor_layout);
    gb_action_editor_layout->addWidget(gb28181_start_button_);
    gb_action_editor_layout->addSpacing(kDesktopActionButtonGap);
    gb_action_editor_layout->addWidget(
        createInlineLabel(gb28181_page, UiText("Volume", "音量")));
    gb_action_editor_layout->addWidget(gb28181_audio_volume_slider_, 1);
    gb_action_editor_layout->addWidget(gb28181_audio_volume_value_label_);
    gb_action_editor_layout->addStretch(1);

    QWidget* gb_local_identity_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_local_identity_layout = createInlineHost(gb_local_identity_editor);
    gb_local_identity_editor->setLayout(gb_local_identity_layout);
    gb_local_identity_layout->addWidget(gb28181_local_id_edit_, 1);

    QWidget* gb_local_auth_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_local_auth_layout = createInlineHost(gb_local_auth_editor);
    gb_local_auth_editor->setLayout(gb_local_auth_layout);
    gb_local_auth_layout->addWidget(gb28181_local_domain_edit_, 1);

    QWidget* gb_local_ports_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_local_ports_layout = createInlineHost(gb_local_ports_editor);
    gb_local_ports_editor->setLayout(gb_local_ports_layout);
    gb_local_ports_layout->addWidget(gb28181_local_port_edit_, 0, Qt::AlignVCenter);
    addInlineLabeledWidget(
        gb_local_ports_layout,
        gb_local_ports_editor,
        UiText("Media port", "媒体端口"),
        gb28181_media_port_edit_,
        0);
    gb_local_ports_layout->addStretch(1);

    QWidget* gb_upper_identity_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_upper_identity_layout = createInlineHost(gb_upper_identity_editor);
    gb_upper_identity_editor->setLayout(gb_upper_identity_layout);
    gb_upper_identity_layout->addWidget(gb28181_upper_id_edit_, 1);

    QWidget* gb_upper_domain_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_upper_domain_layout = createInlineHost(gb_upper_domain_editor);
    gb_upper_domain_editor->setLayout(gb_upper_domain_layout);
    gb_upper_domain_layout->addWidget(gb28181_upper_domain_edit_, 1);

    QWidget* gb_upper_password_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_upper_password_layout = createInlineHost(gb_upper_password_editor);
    gb_upper_password_editor->setLayout(gb_upper_password_layout);
    gb_upper_password_layout->addWidget(gb28181_upper_password_edit_, 1);
    addInlineLabeledWidget(
        gb_upper_password_layout,
        gb_upper_password_editor,
        UiText("Transport", "传输方式"),
        gb28181_upper_transport_combo_,
        1);

    QWidget* gb_upper_endpoint_editor = new QWidget(gb28181_page);
    QHBoxLayout* gb_upper_endpoint_layout = createInlineHost(gb_upper_endpoint_editor);
    gb_upper_endpoint_editor->setLayout(gb_upper_endpoint_layout);
    gb_upper_endpoint_layout->addWidget(gb28181_upper_ip_edit_, 1);
    addInlineLabeledWidget(
        gb_upper_endpoint_layout,
        gb_upper_endpoint_editor,
        UiText("Port", "端口"),
        gb28181_upper_port_edit_,
        0);

    gb28181_source_layout->setContentsMargins(10, 10, 10, 10);
    gb28181_source_layout->setSpacing(kDesktopSectionSpacing);
    gb28181_source_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("Video", "视频"),
        gb28181_source_combo_));
    gb28181_source_layout->addWidget(gb28181_video_detail_row_);
    gb28181_source_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("Resolution", "分辨率"),
        gb_resolution_editor));
    gb28181_source_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("Audio", "音频"),
        gb28181_audio_combo_));
    gb28181_source_layout->addWidget(gb28181_audio_detail_row_);

    gb28181_local_layout->setContentsMargins(10, 10, 10, 10);
    gb28181_local_layout->setSpacing(kDesktopSectionSpacing);
    gb28181_local_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("Device/channel ID", "设备/通道 ID"),
        gb_local_identity_editor));
    gb28181_local_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("SIP domain", "SIP 域"),
        gb_local_auth_editor));
    gb28181_local_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("Local ports", "本机端口"),
        gb_local_ports_editor));

    gb28181_platform_layout->setContentsMargins(10, 10, 10, 10);
    gb28181_platform_layout->setSpacing(kDesktopSectionSpacing);
    gb28181_platform_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("Upper ID", "上级 ID"),
        gb_upper_identity_editor));
    gb28181_platform_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("SIP domain", "SIP 域"),
        gb_upper_domain_editor));
    gb28181_platform_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("Auth", "认证"),
        gb_upper_password_editor));
    gb28181_platform_layout->addWidget(createFieldRow(
        gb28181_page,
        UiText("Upper SIP", "上级 SIP"),
        gb_upper_endpoint_editor));

    gb28181_action_box->setVisible(false);
    gb28181_runtime_layout->setContentsMargins(10, 10, 10, 10);
    gb28181_runtime_layout->setSpacing(kDesktopSectionSpacing);
    gb28181_runtime_layout->addWidget(gb28181_source_summary_label_);
    gb28181_runtime_layout->addWidget(gb28181_runtime_log_, 1);
    gb28181_preview_layout->setSpacing(12);
    gb28181_preview_layout->addWidget(gb28181_preview_frame_, 0);
    gb28181_preview_layout->addWidget(gb_action_editor, 0);
    gb28181_preview_layout->addWidget(gb28181_runtime_box, 0);
    gb28181_preview_layout->addStretch(1);
    gb28181_left_layout->addWidget(gb28181_source_box);
    gb28181_left_layout->addWidget(gb28181_local_box);
    gb28181_left_layout->addWidget(gb28181_platform_box);
    gb28181_left_layout->addStretch(1);
    QScrollArea* gb_left_scroll = new QScrollArea(gb28181_page);
    gb_left_scroll->setObjectName(QString::fromUtf8("gb28181_left_scroll"));
    gb_left_scroll->setFrameShape(QFrame::NoFrame);
    gb_left_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gb_left_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    gb_left_scroll->setWidgetResizable(true);
    gb_left_scroll->setMinimumWidth(kSidebarWidth);
    gb_left_scroll->setMaximumWidth(kSidebarWidth);
    gb_left_scroll->setStyleSheet(QString::fromUtf8(
        "QScrollArea#gb28181_left_scroll { background: #FFFFFF; border: none; }"
        "QScrollArea#gb28181_left_scroll > QWidget > QWidget { background: #FFFFFF; }"));
    gb_left_widget->setAutoFillBackground(true);
    QPalette gb_left_palette = gb_left_widget->palette();
    gb_left_palette.setColor(QPalette::Window, QColor(255, 255, 255));
    gb_left_widget->setPalette(gb_left_palette);
    gb_left_scroll->viewport()->setAutoFillBackground(true);
    QPalette gb_scroll_palette = gb_left_scroll->viewport()->palette();
    gb_scroll_palette.setColor(QPalette::Window, QColor(255, 255, 255));
    gb_left_scroll->viewport()->setPalette(gb_scroll_palette);
    gb_left_scroll->setWidget(gb_left_widget);
    gb28181_content_layout->addWidget(gb_left_scroll, 0);
    gb28181_content_layout->addLayout(gb28181_preview_layout, 1);
    gb28181_content_layout->setSpacing(12);
    gb28181_layout->setContentsMargins(12, 12, 12, 12);
    gb28181_layout->addLayout(gb28181_content_layout, 1);

    license_box_layout->addWidget(license_status_label_);
    license_box_layout->addWidget(license_feature_label_);
    license_box_layout->addWidget(machine_id_label_);
    license_box_layout->addWidget(log_status_label_);
    license_box_layout->addWidget(operation_status_label_);
    license_box_layout->addLayout(license_meta_action_layout);
    status_tabs->addTab(
        capability_table_,
        UiText("Capability / limits", "能力 / 限制"));
    status_tabs->addTab(
        session_table_,
        UiText("Runtime checks", "运行检查"));
    license_layout->addWidget(license_box);
    license_layout->addWidget(status_tabs, 1);

    feature_tabs->addTab(publisher_page, UiText("Publisher", "推流"));
    feature_tabs->addTab(player_page, UiText("Player", "播放"));
    feature_tabs->addTab(gb28181_page, QString::fromUtf8("GB28181"));
    feature_tabs->addTab(license_page, UiText("License", "授权"));
    feature_tabs->addTab(logs_page, UiText("Diagnostics", "诊断"));
    page_layout->setContentsMargins(0, 0, 0, 0);
    page_layout->setSpacing(0);
    page_layout->addWidget(overview_box);
    page_layout->addWidget(feature_tabs, 1);

    setCentralWidget(page);
    RefreshPublisherCameraDevices();
    RefreshPublisherResolutionOptions();
    UpdatePublisherSourceControls();
    UpdateAudioVolumeLabels();
    UpdatePublisherSourceSummary();
    UpdatePlayerSourceSummary();
    UpdateOnvifDeviceSummary();
    UpdateGb28181SourceSummary();
    UpdatePublisherButtons();
    UpdatePlayerButtons();
    UpdateGb28181Buttons();
    ApplyPreviewDisplayMode();
    QTimer::singleShot(0, this, [this]() {
        ApplyPreviewDisplayMode();
        BindDesktopRenderTarget();
    });
    rebuilding_ui_ = false;
    statusBar()->showMessage(
        UiText("Qt 6 Widgets demo is ready.", "Qt 6 Widgets 演示程序已就绪。"),
        3000);
}

void StreamCoreDemoQtWindow::RefreshPublisherCameraDevices()
{
    if (publisher_camera_device_combo_ == nullptr)
    {
        return;
    }

    const QSignalBlocker blocker(publisher_camera_device_combo_);
    const QString previous_id =
        publisher_camera_device_combo_->currentData().toString();
    publisher_camera_device_combo_->clear();
    const int source_index = ComboValueOrIndex(
        publisher_source_combo_,
        kPublisherSourceCamera);
    const bool wants_desktop = source_index == kPublisherSourceDesktop;
    const streamcore_capture_source_kind_t source_kind = wants_desktop ?
        STREAMCORE_CAPTURE_SOURCE_KIND_DESKTOP :
        STREAMCORE_CAPTURE_SOURCE_KIND_CAMERA;
    streamcore_capture_source_info_t sources[64] = {};
    size_t source_count = 0;
    const streamcore_result_t result = streamcore_capture_copy_source_list(
        source_kind,
        sources,
        sizeof(sources) / sizeof(sources[0]),
        &source_count,
        nullptr,
        0);
    int default_index = -1;
    if (result == STREAMCORE_RESULT_OK)
    {
        for (size_t i = 0; i < source_count; ++i)
        {
            const streamcore_capture_source_info_t& source = sources[i];
            if (source.source_kind != source_kind)
            {
                continue;
            }

            const QString source_id = wants_desktop ?
                QString::fromUtf8(source.display_id) :
                QString::fromUtf8(source.source_id);
            QString label = wants_desktop ?
                QString::fromUtf8(source.display_name).trimmed() :
                QString::fromUtf8(source.source_name).trimmed();
            if (label.isEmpty())
            {
                label = source_id.isEmpty() ?
                    (wants_desktop ?
                        UiText("Primary desktop", "主显示器") :
                        UiText("System default camera", "系统默认摄像头")) :
                    source_id;
            }
            if (wants_desktop &&
                source.display_width > 0 &&
                source.display_height > 0)
            {
                label += QString::fromUtf8(" · %1x%2")
                    .arg(source.display_width)
                    .arg(source.display_height);
            }

            publisher_camera_device_combo_->addItem(label, source_id);
            if (!previous_id.isEmpty() && previous_id == source_id)
            {
                publisher_camera_device_combo_->setCurrentIndex(
                    publisher_camera_device_combo_->count() - 1);
            }
            if (source.is_default != 0 && default_index < 0)
            {
                default_index = publisher_camera_device_combo_->count() - 1;
            }
        }
    }

    if (publisher_camera_device_combo_->count() == 0)
    {
        publisher_camera_device_combo_->addItem(
            wants_desktop ?
                UiText("Primary desktop", "主显示器") :
                UiText("System default camera", "系统默认摄像头"),
            QString());
    }
    else if (publisher_camera_device_combo_->currentIndex() < 0 &&
        default_index >= 0)
    {
        publisher_camera_device_combo_->setCurrentIndex(default_index);
    }
}

void StreamCoreDemoQtWindow::RefreshPublisherAudioDevices()
{
    if (publisher_audio_source_combo_ == nullptr)
    {
        return;
    }

    const int audio_index = ComboValueOrIndex(
        publisher_audio_combo_,
        kPublisherAudioMicrophone);
    const streamcore_capture_source_kind_t source_kind =
        audio_index == kPublisherAudioSystem ?
            STREAMCORE_CAPTURE_SOURCE_KIND_SYSTEM_AUDIO :
            STREAMCORE_CAPTURE_SOURCE_KIND_MICROPHONE;
    const bool needs_devices =
        audio_index == kPublisherAudioMicrophone ||
        audio_index == kPublisherAudioSystem;
    const QSignalBlocker blocker(publisher_audio_source_combo_);
    const QString previous_id =
        publisher_audio_source_combo_->currentData().toString();
    publisher_audio_source_combo_->clear();
    if (!needs_devices)
    {
        return;
    }

    streamcore_capture_source_info_t sources[32] = {};
    size_t source_count = 0;
    const streamcore_result_t result = streamcore_capture_copy_source_list(
        source_kind,
        sources,
        sizeof(sources) / sizeof(sources[0]),
        &source_count,
        nullptr,
        0);
    int default_index = -1;
    if (result == STREAMCORE_RESULT_OK)
    {
        for (size_t i = 0; i < source_count; ++i)
        {
            const streamcore_capture_source_info_t& source = sources[i];
            if (source.source_kind != source_kind)
            {
                continue;
            }
            const QString source_id = QString::fromUtf8(source.source_id);
            QString label = QString::fromUtf8(source.source_name).trimmed();
            if (label.isEmpty())
            {
                label = audio_index == kPublisherAudioSystem ?
                    UiText("Default system mix", "默认系统混音") :
                    UiText("Default microphone", "默认麦克风");
            }
            publisher_audio_source_combo_->addItem(label, source_id);
            if (!previous_id.isEmpty() && previous_id == source_id)
            {
                publisher_audio_source_combo_->setCurrentIndex(
                    publisher_audio_source_combo_->count() - 1);
            }
            if (source.is_default != 0 && default_index < 0)
            {
                default_index = publisher_audio_source_combo_->count() - 1;
            }
        }
    }

    if (publisher_audio_source_combo_->count() == 0)
    {
        publisher_audio_source_combo_->addItem(
            audio_index == kPublisherAudioSystem ?
                UiText("Default system mix", "默认系统混音") :
                UiText("Default microphone", "默认麦克风"),
            QString());
    }
    else if (publisher_audio_source_combo_->currentIndex() < 0 &&
        default_index >= 0)
    {
        publisher_audio_source_combo_->setCurrentIndex(default_index);
    }
}

void StreamCoreDemoQtWindow::RefreshPublisherResolutionOptions()
{
    if (publisher_resolution_combo_ == nullptr)
    {
        return;
    }

    const QSize previous_size =
        publisher_resolution_combo_->currentData().toSize();
    const int source_index = ComboValueOrIndex(
        publisher_source_combo_,
        kPublisherSourceCamera);
    const QString source_file_path =
        publisher_file_path_edit_ != nullptr ?
            publisher_file_path_edit_->text().trimmed() :
            QString();

    const QSignalBlocker blocker(publisher_resolution_combo_);
    publisher_resolution_combo_->clear();

    const auto add_resolution = [this](const QString& label, const QSize& size) {
        publisher_resolution_combo_->addItem(label, size);
    };
    const auto add_common_camera_presets = [&add_resolution, this]() {
        add_resolution(UiText("Camera default", "Camera default"), QSize(0, 0));
        add_resolution(QString::fromUtf8("1920x1080"), QSize(1920, 1080));
        add_resolution(QString::fromUtf8("1280x720"), QSize(1280, 720));
        add_resolution(QString::fromUtf8("640x480"), QSize(640, 480));
    };

    if (source_index == kPublisherSourceNone)
    {
        add_resolution(UiText("No video source", "No video source"), QSize(0, 0));
        publisher_resolution_combo_->setEnabled(false);
        return;
    }

    publisher_resolution_combo_->setEnabled(true);
    if (source_index == kPublisherSourceCamera)
    {
        add_common_camera_presets();
    }
    else if (source_index == kPublisherSourceDesktop)
    {
        streamcore_capture_source_info_t desktop_sources[16] = {};
        size_t desktop_source_count = 0;
        const streamcore_result_t result = streamcore_capture_copy_source_list(
            STREAMCORE_CAPTURE_SOURCE_KIND_DESKTOP,
            desktop_sources,
            sizeof(desktop_sources) / sizeof(desktop_sources[0]),
            &desktop_source_count,
            nullptr,
            0);
        if (result == STREAMCORE_RESULT_OK && desktop_source_count > 0)
        {
            for (size_t i = 0; i < desktop_source_count; ++i)
            {
                const streamcore_capture_source_info_t& source =
                    desktop_sources[i];
                if (source.display_width <= 0 || source.display_height <= 0)
                {
                    continue;
                }
                QString label = ToQString(source.display_name).trimmed();
                if (label.isEmpty())
                {
                    label = UiText("Desktop", "桌面");
                }
                add_resolution(
                    QString::fromUtf8("%1 · %2x%3")
                        .arg(label)
                        .arg(source.display_width)
                        .arg(source.display_height),
                    QSize(source.display_width, source.display_height));
            }
        }
        if (publisher_resolution_combo_->count() == 0)
        {
            add_resolution(UiText("Desktop default", "桌面默认"), QSize(0, 0));
        }
        add_resolution(QString::fromUtf8("3840x2160"), QSize(3840, 2160));
        add_resolution(QString::fromUtf8("1920x1080"), QSize(1920, 1080));
        add_resolution(QString::fromUtf8("1280x720"), QSize(1280, 720));
        add_resolution(QString::fromUtf8("960x540"), QSize(960, 540));
    }
    else if (source_index == kPublisherSourceImage)
    {
        QSize image_size;
        if (!source_file_path.isEmpty())
        {
            QImageReader reader(source_file_path);
            image_size = reader.size();
        }
        if (image_size.width() > 0 && image_size.height() > 0)
        {
            add_resolution(
                QString::fromUtf8("%1x%2")
                    .arg(image_size.width())
                    .arg(image_size.height()),
                image_size);
        }
        else
        {
            add_resolution(UiText("Image original", "Image original"), QSize(0, 0));
        }
        add_resolution(QString::fromUtf8("1920x1080"), QSize(1920, 1080));
        add_resolution(QString::fromUtf8("1280x720"), QSize(1280, 720));
    }
    else if (source_index == kPublisherSourceVideoFile)
    {
        add_resolution(
            UiText("Video file original", "Video file original"),
            QSize(0, 0));
        publisher_resolution_combo_->setEnabled(false);
    }
    else
    {
        add_resolution(UiText("Source original", "Source original"), QSize(0, 0));
    }

    if (previous_size.width() > 0 && previous_size.height() > 0)
    {
        for (int i = 0; i < publisher_resolution_combo_->count(); ++i)
        {
            if (publisher_resolution_combo_->itemData(i).toSize() == previous_size)
            {
                publisher_resolution_combo_->setCurrentIndex(i);
                break;
            }
        }
    }
}

void StreamCoreDemoQtWindow::UpdatePublisherTargetControls(
    bool apply_codec_defaults)
{
    if (publisher_protocol_combo_ == nullptr ||
        publisher_url_edit_ == nullptr)
    {
        return;
    }

    QString protocol =
        publisher_protocol_combo_->currentData().toString().toLower();
    const QString current_url = publisher_url_edit_->text().trimmed();
    if (!apply_codec_defaults)
    {
        QString detected_protocol;
        if (current_url.startsWith(QString::fromUtf8("https://"), Qt::CaseInsensitive) ||
            current_url.startsWith(QString::fromUtf8("http://"), Qt::CaseInsensitive))
        {
            detected_protocol = QString::fromUtf8("whip");
        }
        else
        {
            const int delimiter = current_url.indexOf(QString::fromUtf8("://"));
            if (delimiter > 0)
            {
                detected_protocol = current_url.left(delimiter).toLower();
            }
        }
        const int detected_index =
            publisher_protocol_combo_->findData(detected_protocol);
        if (detected_index >= 0)
        {
            const QSignalBlocker blocker(publisher_protocol_combo_);
            publisher_protocol_combo_->setCurrentIndex(detected_index);
            protocol = detected_protocol;
        }
    }
    else
    {
        bool target_matches_protocol = false;
        if (protocol == QString::fromUtf8("whip"))
        {
            target_matches_protocol =
                current_url.startsWith(QString::fromUtf8("https://"), Qt::CaseInsensitive) ||
                current_url.startsWith(QString::fromUtf8("http://"), Qt::CaseInsensitive);
        }
        else
        {
            target_matches_protocol = current_url.startsWith(
                protocol + QString::fromUtf8("://"),
                Qt::CaseInsensitive);
        }

        if (!target_matches_protocol)
        {
            QString example_url;
            if (protocol == QString::fromUtf8("whip"))
            {
                example_url = QString::fromUtf8(
                    "https://localhost:8443/whip");
            }
            else if (protocol == QString::fromUtf8("srt"))
            {
                example_url = QString::fromUtf8(
                    "srt://127.0.0.1:9000?mode=caller");
            }
            else if (protocol == QString::fromUtf8("rtsp"))
            {
                example_url = QString::fromUtf8(
                    "rtsp://127.0.0.1:8554/live/demo");
            }
            else
            {
                example_url = QString::fromUtf8(
                    "rtmp://127.0.0.1:1935/live/demo");
            }
            publisher_url_edit_->setText(example_url);
            publisher_url_edit_->setCursorPosition(0);
        }

        if (protocol == QString::fromUtf8("whip"))
        {
            const int h264_index = publisher_video_codec_combo_ != nullptr ?
                publisher_video_codec_combo_->findData(QString::fromUtf8("h264")) :
                -1;
            const int opus_index = publisher_audio_codec_combo_ != nullptr ?
                publisher_audio_codec_combo_->findData(QString::fromUtf8("opus")) :
                -1;
            if (h264_index >= 0)
            {
                publisher_video_codec_combo_->setCurrentIndex(h264_index);
            }
            if (opus_index >= 0)
            {
                publisher_audio_codec_combo_->setCurrentIndex(opus_index);
            }
        }
    }

    const bool is_whip = protocol == QString::fromUtf8("whip");
    if (is_whip)
    {
        const int h264_index = publisher_video_codec_combo_ != nullptr ?
            publisher_video_codec_combo_->findData(QString::fromUtf8("h264")) :
            -1;
        const int opus_index = publisher_audio_codec_combo_ != nullptr ?
            publisher_audio_codec_combo_->findData(QString::fromUtf8("opus")) :
            -1;
        if (h264_index >= 0 &&
            publisher_video_codec_combo_->currentIndex() != h264_index)
        {
            publisher_video_codec_combo_->setCurrentIndex(h264_index);
        }
        if (opus_index >= 0 &&
            publisher_audio_codec_combo_->currentIndex() != opus_index)
        {
            publisher_audio_codec_combo_->setCurrentIndex(opus_index);
        }
    }
    if (publisher_video_codec_combo_ != nullptr)
    {
        publisher_video_codec_combo_->setEnabled(!is_whip);
    }
    if (publisher_audio_codec_combo_ != nullptr)
    {
        publisher_audio_codec_combo_->setEnabled(!is_whip);
    }
    if (publisher_audio_profile_combo_ != nullptr)
    {
        publisher_audio_profile_combo_->setEnabled(!is_whip);
    }
    if (publisher_whip_bearer_row_ != nullptr)
    {
        publisher_whip_bearer_row_->setVisible(is_whip);
    }
    if (publisher_whip_bearer_token_edit_ != nullptr)
    {
        publisher_whip_bearer_token_edit_->setEnabled(is_whip);
    }
}

void StreamCoreDemoQtWindow::UpdatePublisherSourceControls()
{
    if (publisher_source_combo_ == nullptr ||
        publisher_camera_device_combo_ == nullptr ||
        publisher_file_path_edit_ == nullptr ||
        publisher_file_browse_button_ == nullptr ||
        publisher_audio_combo_ == nullptr ||
        publisher_audio_file_label_ == nullptr ||
        publisher_audio_file_path_edit_ == nullptr ||
        publisher_audio_file_browse_button_ == nullptr ||
        publisher_file_mode_combo_ == nullptr ||
        publisher_rtmp_hevc_combo_ == nullptr ||
        publisher_preview_toggle_ == nullptr ||
        publisher_processor_compare_toggle_ == nullptr)
    {
        return;
    }

    const int source_index = ComboValueOrIndex(
        publisher_source_combo_,
        kPublisherSourceCamera);
    const int audio_index = ComboValueOrIndex(
        publisher_audio_combo_,
        kPublisherAudioMicrophone);
    const bool is_camera = source_index == kPublisherSourceCamera;
    const bool is_desktop = source_index == kPublisherSourceDesktop;
    const bool is_video_file = source_index == kPublisherSourceVideoFile;
    const bool is_image = source_index == kPublisherSourceImage;
    const bool is_source_file = is_video_file || is_image;
    const bool uses_device_combo = is_camera || is_desktop;
    const bool needs_audio_device =
        audio_index == kPublisherAudioMicrophone ||
        audio_index == kPublisherAudioSystem;
    const bool needs_audio_file = audio_index == kPublisherAudioFile;

    if (uses_device_combo)
    {
        RefreshPublisherCameraDevices();
    }
    if (needs_audio_device)
    {
        RefreshPublisherAudioDevices();
    }

    publisher_video_detail_row_->setVisible(uses_device_combo || is_source_file);
    publisher_camera_device_combo_->setVisible(uses_device_combo);
    publisher_camera_device_combo_->setEnabled(uses_device_combo);
    publisher_file_path_edit_->setVisible(is_source_file);
    publisher_file_path_edit_->setEnabled(is_source_file);
    publisher_file_browse_button_->setVisible(is_source_file);
    publisher_file_browse_button_->setEnabled(is_source_file);

    publisher_file_mode_row_->setVisible(is_video_file);
    publisher_file_mode_combo_->setVisible(is_video_file);
    publisher_file_mode_combo_->setEnabled(is_video_file);

    if (audio_index == kPublisherAudioMicrophone)
    {
        publisher_audio_detail_label_->setText(
            UiText("Microphone", "麦克风"));
    }
    else if (audio_index == kPublisherAudioSystem)
    {
        publisher_audio_detail_label_->setText(
            UiText("System audio", "系统声音"));
    }
    else
    {
        publisher_audio_detail_label_->setText(
            UiText("Audio file", "音频文件"));
    }
    publisher_audio_detail_row_->setVisible(
        audio_index != kPublisherAudioNone &&
        !is_video_file);
    publisher_audio_source_combo_->setVisible(needs_audio_device);
    publisher_audio_source_combo_->setEnabled(needs_audio_device);

    const bool uses_rtmp = publisher_url_edit_ != nullptr &&
        publisher_url_edit_->text().trimmed().startsWith(
            QString::fromUtf8("rtmp://"),
            Qt::CaseInsensitive);
    const bool has_video_source =
        is_camera || is_desktop || is_video_file || is_image;
    publisher_rtmp_hevc_combo_->setEnabled(uses_rtmp && has_video_source);
    const bool publisher_running =
        active_publisher_ != nullptr ||
        active_publisher_capture_ != nullptr;
    publisher_processor_compare_toggle_->setEnabled(
        has_video_source && !publisher_running);
    if (!has_video_source &&
        publisher_processor_compare_toggle_->isChecked())
    {
        const QSignalBlocker processor_blocker(
            publisher_processor_compare_toggle_);
        publisher_processor_compare_toggle_->setChecked(false);
        if (publisher_processed_preview_column_ != nullptr)
        {
            publisher_processed_preview_column_->hide();
        }
    }
    UpdatePublisherTargetControls(false);
    publisher_preview_toggle_->setEnabled(CurrentPublisherSelectionSupportsPreview());

    publisher_audio_file_label_->setVisible(needs_audio_file);
    publisher_audio_file_path_edit_->setVisible(needs_audio_file);
    publisher_audio_file_path_edit_->setEnabled(needs_audio_file);
    publisher_audio_file_browse_button_->setVisible(needs_audio_file);
    publisher_audio_file_browse_button_->setEnabled(needs_audio_file);
    RefreshPublisherResolutionOptions();
}

void StreamCoreDemoQtWindow::BrowsePublisherMediaFile()
{
    if (publisher_source_combo_ == nullptr || publisher_file_path_edit_ == nullptr)
    {
        return;
    }

    QString caption = UiText("Select publisher media file", "选择推流媒体文件");
    QString filter = UiText(
        "Media files (*.mp4 *.mov *.mkv *.flv *.wav *.aac *.mp3 *.png *.jpg *.jpeg);;All files (*)",
        "媒体文件 (*.mp4 *.mov *.mkv *.flv *.wav *.aac *.mp3 *.png *.jpg *.jpeg);;所有文件 (*)");
    const int source_index = ComboValueOrIndex(
        publisher_source_combo_,
        kPublisherSourceCamera);
    if (source_index == kPublisherSourceVideoFile)
    {
        caption = UiText("Select publisher video file", "选择推流视频文件");
        filter = UiText(
            "Video files (*.mp4 *.mov *.mkv *.flv);;All files (*)",
            "视频文件 (*.mp4 *.mov *.mkv *.flv);;所有文件 (*)");
    }
    else if (source_index == kPublisherSourceImage)
    {
        caption = UiText("Select publisher image file", "选择推流图片文件");
        filter = UiText(
            "Image files (*.png *.jpg *.jpeg);;All files (*)",
            "图片文件 (*.png *.jpg *.jpeg);;所有文件 (*)");
    }

    const QString file_path = QFileDialog::getOpenFileName(
        this,
        caption,
        publisher_file_path_edit_->text().trimmed(),
        filter);
    if (!file_path.isEmpty())
    {
        publisher_file_path_edit_->setText(file_path);
    }
}

void StreamCoreDemoQtWindow::BrowsePublisherAudioFile()
{
    if (publisher_audio_file_path_edit_ == nullptr)
    {
        return;
    }

    const QString file_path = QFileDialog::getOpenFileName(
        this,
        UiText("Select publisher audio file", "选择推流音频文件"),
        publisher_audio_file_path_edit_->text().trimmed(),
        UiText(
            "Audio files (*.wav *.aac *.mp3 *.m4a);;All files (*)",
            "音频文件 (*.wav *.aac *.mp3 *.m4a);;所有文件 (*)"));
    if (!file_path.isEmpty())
    {
        publisher_audio_file_path_edit_->setText(file_path);
    }
}

bool StreamCoreDemoQtWindow::EnsurePublisherCapturePermissions(
    int source_index,
    int audio_index,
    QString* error_message)
{
#if defined(Q_OS_MACOS)
    const bool needs_screen_recording =
        source_index == kPublisherSourceDesktop;
    const bool needs_camera = source_index == kPublisherSourceCamera;
    const bool needs_microphone = audio_index == kPublisherAudioMicrophone;
    if (!needs_screen_recording && !needs_camera && !needs_microphone)
    {
        return true;
    }

    char permission_message[512] = {};
    if (!StreamCoreDemoQtRequestMacCapturePermissions(
            needs_screen_recording,
            needs_camera,
            needs_microphone,
            60000,
            permission_message,
            sizeof(permission_message)))
    {
        if (error_message != nullptr)
        {
            *error_message = ToQString(permission_message);
        }
        return false;
    }
#else
    (void)source_index;
    (void)audio_index;
    (void)error_message;
#endif
    return true;
}

void StreamCoreDemoQtWindow::StartPublisher()
{
    if (publisher_source_combo_ == nullptr ||
        publisher_audio_combo_ == nullptr ||
        publisher_url_edit_ == nullptr ||
        publisher_camera_device_combo_ == nullptr ||
        publisher_file_path_edit_ == nullptr ||
        publisher_audio_file_path_edit_ == nullptr ||
        publisher_video_codec_combo_ == nullptr ||
        publisher_audio_codec_combo_ == nullptr ||
        publisher_file_mode_combo_ == nullptr ||
        publisher_resolution_combo_ == nullptr ||
        publisher_preview_label_ == nullptr ||
        publisher_processor_compare_toggle_ == nullptr)
    {
        return;
    }

    StopPublisher();
    if (publisher_status_label_ != nullptr)
    {
        publisher_status_label_->setText(UiText(
            "Publishing is starting...",
            "正在启动推流..."));
    }

    streamcore_publisher_config_t config;
    streamcore_publisher_whip_options_t whip_options;
    streamcore_publisher_preflight_t preflight = {};
    streamcore_publisher_runtime_info_t runtime_info = {};
    streamcore_media_file_profile_t media_file_profile = {};
    streamcore_capture_config_t capture_config;
    streamcore_capture_preflight_t capture_preflight = {};
    streamcore_capture_runtime_info_t capture_runtime_info = {};
    streamcore_render_target_t preview_target = {};
    streamcore_render_target_t processed_preview_target = {};
    streamcore_publisher_handle publisher = nullptr;
    streamcore_capture_handle capture = nullptr;
    char error_text[STREAMCORE_TEXT_CAPACITY] = {};
    char media_file_probe_error[STREAMCORE_TEXT_CAPACITY] = {};
    char capture_error_text[STREAMCORE_TEXT_CAPACITY] = {};
    QByteArray session_name("qt_desktop_publisher");
    QByteArray capture_session_name("qt_publisher_capture");
    QByteArray publish_url = publisher_url_edit_->text().trimmed().toUtf8();
    QByteArray whip_bearer_token =
        publisher_whip_bearer_token_edit_ != nullptr ?
            publisher_whip_bearer_token_edit_->text().trimmed().toUtf8() :
            QByteArray();
    QByteArray input_binding;
    QByteArray camera_id;
    QByteArray desktop_display_id;
    QByteArray audio_source_id;
    QByteArray file_path = publisher_file_path_edit_->text().trimmed().toUtf8();
    QByteArray audio_file_path =
        publisher_audio_file_path_edit_->text().trimmed().toUtf8();
    QByteArray video_codec = SelectedPublisherVideoCodec().toUtf8();
    QByteArray audio_codec = SelectedPublisherAudioProfileCodec().toUtf8();
    if (EnvironmentText("STREAMCORE_DEMO_QT_AUTORUN").compare(
            QString::fromUtf8("publisher"),
            Qt::CaseInsensitive) == 0)
    {
        const QString scripted_video_codec =
            EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_VIDEO_CODEC");
        const QString scripted_audio_codec =
            EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_AUDIO_CODEC");
        if (!scripted_video_codec.isEmpty())
        {
            video_codec = scripted_video_codec.trimmed().toUtf8();
        }
        if (!scripted_audio_codec.isEmpty())
        {
            audio_codec = scripted_audio_codec.trimmed().toUtf8();
        }
    }
    QByteArray media_file_container;
    QByteArray media_file_audio_codec;
    QByteArray media_file_video_codec;
    streamcore_result_t result = STREAMCORE_RESULT_OK;
    const int source_index = ComboValueOrIndex(
        publisher_source_combo_,
        kPublisherSourceCamera);
    const int selected_audio_index = ComboValueOrIndex(
        publisher_audio_combo_,
        kPublisherAudioMicrophone);
    const int audio_index = source_index == kPublisherSourceVideoFile ?
        kPublisherAudioNone :
        selected_audio_index;
    const int file_mode = ComboValueOrIndex(
        publisher_file_mode_combo_,
        kPublisherFileModeAuto);
    const QSize selected_resolution = SelectedPublisherResolution();
    const int publisher_audio_volume_percent =
        SelectedPublisherAudioVolumePercent();
    const int publisher_audio_sample_rate =
        publisher_audio_sample_rate_combo_ != nullptr ?
            publisher_audio_sample_rate_combo_->currentData().toInt() :
            48000;
    const int publisher_audio_bitrate_kbps =
        publisher_audio_bitrate_combo_ != nullptr ?
            publisher_audio_bitrate_combo_->currentData().toInt() :
            128;
    const bool source_has_video = source_index != kPublisherSourceNone;
    const bool processor_compare_enabled =
        source_has_video &&
        publisher_processor_compare_toggle_->isChecked();
    const bool source_uses_file =
        source_index == kPublisherSourceVideoFile ||
        source_index == kPublisherSourceImage;
    const bool video_file_carries_audio =
        source_index == kPublisherSourceVideoFile;
    const bool use_capture_sink =
        source_has_video || audio_index != kPublisherAudioNone;
    bool use_capture_preview_sink = source_has_video &&
        IsPublisherPreviewEnabled();
    bool use_processed_preview_sink =
        use_capture_preview_sink &&
        processor_compare_enabled;
    const bool force_file_transcode =
        source_index == kPublisherSourceVideoFile &&
        file_mode == kPublisherFileModeForceTranscode;
    bool use_file_encoded_passthrough = false;
    if (source_index == kPublisherSourceNone &&
        audio_index == kPublisherAudioNone)
    {
        SetOperationStatus(
            QString::fromUtf8("publisher.start"),
            QString::fromUtf8("-1"),
            QString::fromUtf8("invalid_argument"),
            UiText(
                "Select at least one video or audio source before publishing.",
                "Select at least one video or audio source before publishing."));
        publisher_preview_label_->setText(UiText(
            "Publisher needs a video source, an audio source, or both.",
            "推流至少需要一个视频或音频来源。"));
        if (publisher_status_label_ != nullptr)
        {
            publisher_status_label_->setText(UiText(
                "Publish failed: no video or audio source selected.",
                "推流失败：未选择视频或音频来源。"));
        }
        UpdatePublisherButtons();
        return;
    }
    if (source_has_video && audio_index == kPublisherAudioFile &&
        !video_file_carries_audio)
    {
        const QString message = UiText(
            "One Capture session does not mix a separate audio file into this video source.",
            "单个 Capture 会话暂不把独立音频文件混入当前视频来源。");
        SetOperationStatus(
            QString::fromUtf8("publisher.start"),
            QString::fromUtf8("-1"),
            QString::fromUtf8("not_supported"),
            message);
        publisher_preview_label_->setText(message);
        if (publisher_status_label_ != nullptr)
        {
            publisher_status_label_->setText(UiText(
                "Publish failed: %1",
                "推流失败：%1").arg(message));
        }
        UpdatePublisherButtons();
        return;
    }
    QString permission_error;
    if (!EnsurePublisherCapturePermissions(
            source_index,
            audio_index,
            &permission_error))
    {
        if (permission_error.isEmpty())
        {
            permission_error = UiText(
                "Capture permission was not granted.",
                "Capture permission was not granted.");
        }
        SetOperationStatus(
            QString::fromUtf8("publisher.permission"),
            QString::fromUtf8("-1"),
            QString::fromUtf8("failed"),
            permission_error);
        publisher_preview_label_->setText(UiText(
            "Publisher capture permission is not ready.\n%1",
            "推流采集权限尚未就绪。\n%1").arg(permission_error));
        if (publisher_status_label_ != nullptr)
        {
            publisher_status_label_->setText(UiText(
                "Publish failed: permission is not ready. %1",
                "推流失败：权限尚未就绪。%1").arg(permission_error));
        }
        UpdatePublisherButtons();
        return;
    }

    if (publish_url.isEmpty())
    {
        publish_url = QByteArray("rtmp://192.0.2.1:1935/live/local_native");
    }

    streamcore_publisher_get_default_config(&config);
    streamcore_publisher_get_default_whip_options(&whip_options);
    config.session_name = session_name.constData();
    config.publish_url = publish_url.constData();
    config.allow_reconnect = 1;
    config.rtmp_hevc_mode = SelectedPublisherRtmpHevcMode();
    whip_options.bearer_token = whip_bearer_token.isEmpty() ?
        nullptr :
        whip_bearer_token.constData();
    config.transcode.audio_codec_name = audio_codec.constData();
    config.transcode.video_codec_name = video_codec.constData();
    config.transcode.target_width = selected_resolution.width() > 0 ?
        selected_resolution.width() :
        0;
    config.transcode.target_height = selected_resolution.height() > 0 ?
        selected_resolution.height() :
        0;
    config.transcode.target_video_bitrate_kbps =
        ReadBoundedInt(publisher_video_bitrate_edit_, 1200, 1, 100000);
    config.transcode.target_audio_bitrate_kbps =
        publisher_audio_bitrate_kbps > 0 ? publisher_audio_bitrate_kbps : 128;
    config.transcode.target_sample_rate =
        publisher_audio_sample_rate > 0 ? publisher_audio_sample_rate : 48000;
    config.transcode.target_channel_count = 2;
    config.transcode.target_fps =
        ReadBoundedInt(publisher_fps_edit_, 25, 1, 120);
    config.transcode.target_gop_frames =
        ReadBoundedInt(publisher_gop_edit_, 50, 1, 600);

    if (source_uses_file && file_path.isEmpty())
    {
        const QString message = source_index == kPublisherSourceImage ?
            UiText("Select a still image first.", "请先选择图片文件。") :
            UiText("Select a local video file first.", "请先选择本地视频文件。");
        publisher_preview_label_->setText(message);
        if (publisher_status_label_ != nullptr)
        {
            publisher_status_label_->setText(UiText(
                "Publish failed: %1",
                "推流失败：%1").arg(message));
        }
        SetOperationStatus(
            QString::fromUtf8("publisher.start"),
            QString::fromUtf8("-1"),
            QString::fromUtf8("invalid_argument"),
            message);
        UpdatePublisherButtons();
        return;
    }
    if (audio_index == kPublisherAudioFile && audio_file_path.isEmpty())
    {
        const QString message = UiText(
            "Select a local audio file first.",
            "请先选择本地音频文件。");
        publisher_preview_label_->setText(message);
        if (publisher_status_label_ != nullptr)
        {
            publisher_status_label_->setText(UiText(
                "Publish failed: %1",
                "推流失败：%1").arg(message));
        }
        SetOperationStatus(
            QString::fromUtf8("publisher.start"),
            QString::fromUtf8("-1"),
            QString::fromUtf8("invalid_argument"),
            message);
        UpdatePublisherButtons();
        return;
    }
    if (audio_index == kPublisherAudioSystem)
    {
        streamcore_capture_source_info_t system_audio_sources[8] = {};
        size_t system_audio_count = 0;
        const streamcore_result_t system_audio_result =
            streamcore_capture_copy_source_list(
                STREAMCORE_CAPTURE_SOURCE_KIND_SYSTEM_AUDIO,
                system_audio_sources,
                sizeof(system_audio_sources) / sizeof(system_audio_sources[0]),
                &system_audio_count,
                nullptr,
                0);
        if (system_audio_result != STREAMCORE_RESULT_OK ||
            system_audio_count == 0)
        {
            const QString message = UiText(
                "System audio capture is not available on this platform.",
                "当前平台不支持系统声音采集。");
            publisher_preview_label_->setText(message);
            if (publisher_status_label_ != nullptr)
            {
                publisher_status_label_->setText(UiText(
                    "Publish failed: %1",
                    "推流失败：%1").arg(message));
            }
            SetOperationStatus(
                QString::fromUtf8("publisher.start"),
                QString::fromUtf8("-1"),
                QString::fromUtf8("not_supported"),
                message);
            UpdatePublisherButtons();
            return;
        }
    }

    if (source_index == kPublisherSourceVideoFile)
    {
        result = streamcore_probe_media_file(
            file_path.constData(),
            &media_file_profile,
            media_file_probe_error,
            sizeof(media_file_probe_error));
        if (result != STREAMCORE_RESULT_OK)
        {
            const QString reason = ToQString(media_file_probe_error);
            const QString message = reason.isEmpty() ?
                UiText("The selected video file could not be probed.",
                    "无法探测所选视频文件。") :
                reason;
            publisher_preview_label_->setText(message);
            if (publisher_status_label_ != nullptr)
            {
                publisher_status_label_->setText(UiText(
                    "Publish failed: %1",
                    "推流失败：%1").arg(message));
            }
            SetOperationStatus(
                QString::fromUtf8("publisher.probe_media_file"),
                QString::number(result),
                QString::fromUtf8("failed"),
                message);
            UpdatePublisherButtons();
            return;
        }
        if (media_file_profile.has_audio == 0 &&
            media_file_profile.has_video == 0)
        {
            const QString message = UiText(
                "The selected media file does not contain an audio or video track.",
                "所选媒体文件不包含音频或视频轨道。");
            publisher_preview_label_->setText(message);
            if (publisher_status_label_ != nullptr)
            {
                publisher_status_label_->setText(UiText(
                    "Publish failed: %1",
                    "推流失败：%1").arg(message));
            }
            SetOperationStatus(
                QString::fromUtf8("publisher.probe_media_file"),
                QString::fromUtf8("-1"),
                QString::fromUtf8("invalid_argument"),
                message);
            UpdatePublisherButtons();
            return;
        }
        const bool is_rtmp_target =
            publish_url.trimmed().toLower().startsWith("rtmp://");
        use_file_encoded_passthrough =
            !processor_compare_enabled &&
            !force_file_transcode &&
            is_rtmp_target &&
            media_file_profile.rtmp_passthrough_supported != 0;
        if (use_file_encoded_passthrough)
        {
            use_capture_preview_sink = false;
        }
    }

    config.input_kind = STREAMCORE_PUBLISHER_INPUT_KIND_APP_RAW_FEED;
    config.input_binding_id = input_binding.constData();
    config.enable_video = source_has_video ? 1 : 0;
    config.enable_audio =
        (video_file_carries_audio || audio_index != kPublisherAudioNone) ? 1 : 0;
    if (source_index == kPublisherSourceVideoFile)
    {
        config.enable_audio = media_file_profile.has_audio != 0 ? 1 : 0;
        config.enable_video = media_file_profile.has_video != 0 ? 1 : 0;
        if (config.enable_video == 0)
        {
            use_capture_preview_sink = false;
            use_processed_preview_sink = false;
        }
        if (use_file_encoded_passthrough)
        {
            media_file_container = QByteArray(media_file_profile.container_name);
            media_file_audio_codec =
                QByteArray(media_file_profile.audio_codec_name);
            media_file_video_codec =
                QByteArray(media_file_profile.video_codec_name);
            config.input_kind =
                STREAMCORE_PUBLISHER_INPUT_KIND_APP_ENCODED_FEED;
            config.source_media_profile.container_name =
                media_file_container.constData();
            config.source_media_profile.audio_codec_name =
                config.enable_audio != 0 ?
                    media_file_audio_codec.constData() : nullptr;
            config.source_media_profile.video_codec_name =
                config.enable_video != 0 ?
                    media_file_video_codec.constData() : nullptr;
            config.source_media_profile.has_audio = config.enable_audio;
            config.source_media_profile.has_video = config.enable_video;
            config.source_media_profile.sample_rate =
                media_file_profile.sample_rate;
            config.source_media_profile.channel_count =
                media_file_profile.channel_count;
            config.source_media_profile.width = media_file_profile.width;
            config.source_media_profile.height = media_file_profile.height;
            config.source_media_profile.fps = media_file_profile.fps;
            config.transcode.audio_mode =
                STREAMCORE_PUBLISHER_TRANSCODE_MODE_REQUIRE_PASSTHROUGH;
            config.transcode.video_mode =
                STREAMCORE_PUBLISHER_TRANSCODE_MODE_REQUIRE_PASSTHROUGH;
        }
        else if (force_file_transcode)
        {
            if (config.enable_audio != 0)
            {
                config.transcode.audio_mode =
                    STREAMCORE_PUBLISHER_TRANSCODE_MODE_FORCE_TRANSCODE;
            }
            if (config.enable_video != 0)
            {
                config.transcode.video_mode =
                    STREAMCORE_PUBLISHER_TRANSCODE_MODE_FORCE_TRANSCODE;
            }
        }
    }

    if (source_index == kPublisherSourceCamera)
    {
        camera_id =
            publisher_camera_device_combo_->currentData().toString().toUtf8();
        input_binding = QByteArray("camera:") + camera_id;
        if (audio_index == kPublisherAudioMicrophone)
        {
            input_binding += QByteArray("+microphone");
        }
    }
    else if (source_index == kPublisherSourceDesktop)
    {
        if (publisher_camera_device_combo_ != nullptr)
        {
            const QByteArray selected_display =
                publisher_camera_device_combo_->currentData().toString().toUtf8();
            if (!selected_display.isEmpty())
            {
                desktop_display_id = selected_display;
            }
        }
        if (desktop_display_id.isEmpty())
        {
            streamcore_capture_source_info_t desktop_sources[16] = {};
            size_t desktop_source_count = 0;
            if (streamcore_capture_copy_source_list(
                    STREAMCORE_CAPTURE_SOURCE_KIND_DESKTOP,
                    desktop_sources,
                    sizeof(desktop_sources) / sizeof(desktop_sources[0]),
                    &desktop_source_count,
                    nullptr,
                    0) == STREAMCORE_RESULT_OK)
            {
                for (size_t i = 0; i < desktop_source_count; ++i)
                {
                    if (desktop_sources[i].display_id[0] != '\0' &&
                        desktop_sources[i].is_default != 0)
                    {
                        desktop_display_id = QByteArray(desktop_sources[i].display_id);
                        break;
                    }
                }
                if (desktop_display_id.isEmpty())
                {
                    for (size_t i = 0; i < desktop_source_count; ++i)
                    {
                        if (desktop_sources[i].display_id[0] != '\0')
                        {
                            desktop_display_id =
                                QByteArray(desktop_sources[i].display_id);
                            break;
                        }
                    }
                }
            }
        }
        if (desktop_display_id.isEmpty())
        {
            const QString message = UiText(
                "No desktop display source is available.",
                "当前没有可用的桌面显示源。");
            SetOperationStatus(
                QString::fromUtf8("publisher.start"),
                QString::fromUtf8("-1"),
                QString::fromUtf8("invalid_argument"),
                message);
            publisher_preview_label_->setText(message);
            if (publisher_status_label_ != nullptr)
            {
                publisher_status_label_->setText(UiText(
                    "Publish failed: %1",
                    "推流失败：%1").arg(message));
            }
            UpdatePublisherButtons();
            return;
        }
        input_binding = QByteArray("desktop:") + desktop_display_id;
        if (audio_index == kPublisherAudioMicrophone)
        {
            input_binding += QByteArray("+microphone");
        }
    }
    else if (source_index == kPublisherSourceVideoFile)
    {
        input_binding = QByteArray("media_file:") + file_path;
    }
    else if (source_index == kPublisherSourceImage)
    {
        input_binding = QByteArray("image_file:") + file_path;
    }
    else
    {
        input_binding = QByteArray("audio_only");
    }
    config.input_binding_id = input_binding.constData();

    result = streamcore_publisher_create_with_config(
        &config,
        &publisher,
        error_text,
        sizeof(error_text));
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_publisher_set_whip_options(
            publisher,
            &whip_options);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_publisher_preflight(
            publisher,
            &preflight,
            error_text,
            sizeof(error_text));
    }
    if (result == STREAMCORE_RESULT_OK && preflight.is_ready_to_start == 0)
    {
        result = STREAMCORE_RESULT_OPERATION_FAILED;
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        if (use_capture_preview_sink)
        {
            if (publisher_preview_widget_ == nullptr ||
                !BuildStreamCoreRenderTargetForWidget(
                    publisher_preview_widget_,
                    &preview_target))
            {
                result = STREAMCORE_RESULT_OPERATION_FAILED;
                snprintf(
                    capture_error_text,
                    sizeof(capture_error_text),
                    "%s",
                    "publisher preview surface is unavailable");
            }
        }
        if (result == STREAMCORE_RESULT_OK &&
            use_processed_preview_sink)
        {
            if (publisher_processed_preview_widget_ == nullptr ||
                !BuildStreamCoreRenderTargetForWidget(
                    publisher_processed_preview_widget_,
                    &processed_preview_target))
            {
                result = STREAMCORE_RESULT_OPERATION_FAILED;
                snprintf(
                    capture_error_text,
                    sizeof(capture_error_text),
                    "%s",
                    "publisher processed preview surface is unavailable");
            }
        }
    }
    if (result == STREAMCORE_RESULT_OK && use_capture_sink)
    {
        capture = streamcore_capture_create();
        if (capture == nullptr)
        {
            result = STREAMCORE_RESULT_OPERATION_FAILED;
            snprintf(
                capture_error_text,
                sizeof(capture_error_text),
                "%s",
                "failed to create capture session");
        }
    }
    if (result == STREAMCORE_RESULT_OK && use_capture_sink)
    {
        streamcore_capture_get_default_config(&capture_config);
        capture_config.session_name = capture_session_name.constData();
        if (publisher_audio_source_combo_ != nullptr)
        {
            audio_source_id =
                publisher_audio_source_combo_->currentData().toString().toUtf8();
        }
        if (source_index == kPublisherSourceCamera)
        {
            capture_config.source_kind = STREAMCORE_CAPTURE_SOURCE_KIND_CAMERA;
            capture_config.source_id = camera_id.isEmpty() ?
                nullptr : camera_id.constData();
        }
        else if (source_index == kPublisherSourceDesktop)
        {
            capture_config.source_kind = STREAMCORE_CAPTURE_SOURCE_KIND_DESKTOP;
            capture_config.display_id = desktop_display_id.constData();
        }
        else if (source_index == kPublisherSourceVideoFile ||
            source_index == kPublisherSourceImage)
        {
            capture_config.source_kind = STREAMCORE_CAPTURE_SOURCE_KIND_MEDIA_FILE;
            capture_config.source_id = file_path.constData();
        }
        else if (audio_index == kPublisherAudioFile)
        {
            capture_config.source_kind = STREAMCORE_CAPTURE_SOURCE_KIND_MEDIA_FILE;
            capture_config.source_id = audio_file_path.constData();
        }
        else if (audio_index == kPublisherAudioSystem)
        {
            capture_config.source_kind = STREAMCORE_CAPTURE_SOURCE_KIND_SYSTEM_AUDIO;
            capture_config.source_id = audio_source_id.isEmpty() ?
                nullptr : audio_source_id.constData();
        }
        else
        {
            capture_config.source_kind = STREAMCORE_CAPTURE_SOURCE_KIND_MICROPHONE;
            capture_config.source_id = audio_source_id.isEmpty() ?
                nullptr : audio_source_id.constData();
        }
        if (source_has_video && !video_file_carries_audio)
        {
            if (audio_index == kPublisherAudioSystem)
            {
                capture_config.audio_source_kind =
                    STREAMCORE_CAPTURE_SOURCE_KIND_SYSTEM_AUDIO;
                capture_config.audio_source_id = audio_source_id.isEmpty() ?
                    nullptr : audio_source_id.constData();
            }
            else if (audio_index == kPublisherAudioMicrophone)
            {
                capture_config.audio_source_kind =
                    STREAMCORE_CAPTURE_SOURCE_KIND_MICROPHONE;
                capture_config.audio_source_id = audio_source_id.isEmpty() ?
                    nullptr : audio_source_id.constData();
            }
        }
        capture_config.enable_video = config.enable_video;
        capture_config.enable_audio = config.enable_audio;
        capture_config.audio_volume_percent =
            publisher_audio_volume_percent;
        capture_config.preferred_width = selected_resolution.width() > 0 ?
            selected_resolution.width() :
            0;
        capture_config.preferred_height = selected_resolution.height() > 0 ?
            selected_resolution.height() :
            0;
        capture_config.preferred_fps =
            ReadBoundedInt(publisher_fps_edit_, 25, 1, 120);
        capture_config.allow_backend_fallback = EnvironmentFlag(
            "STREAMCORE_DEMO_QT_PUBLISHER_ALLOW_BACKEND_FALLBACK",
            false) ? 1 : 0;
        capture_config.preview_mode = use_capture_preview_sink ?
            STREAMCORE_CAPTURE_PREVIEW_MODE_AUTO :
            STREAMCORE_CAPTURE_PREVIEW_MODE_DISABLED;
        capture_config.sample_rate =
            publisher_audio_sample_rate > 0 ? publisher_audio_sample_rate : 48000;
        capture_config.channel_count = 2;
        if (processor_compare_enabled &&
            capture_config.enable_video != 0)
        {
            capture_config.video_processor_callback =
                ProcessPublisherComparisonVideo;
            capture_config.video_processor_callback_context = nullptr;
            capture_config.video_processor_failure_mode =
                STREAMCORE_PROCESSOR_FAILURE_MODE_PASSTHROUGH;
        }

        result = streamcore_capture_set_config(capture, &capture_config);
        if (result == STREAMCORE_RESULT_OK && use_capture_preview_sink)
        {
            result = streamcore_capture_set_preview_render_target(
                capture,
                &preview_target);
        }
        if (result == STREAMCORE_RESULT_OK &&
            use_processed_preview_sink)
        {
            result =
                streamcore_capture_set_processed_preview_render_target(
                    capture,
                    &processed_preview_target);
        }
        if (result == STREAMCORE_RESULT_OK)
        {
            result = streamcore_capture_connect_publisher(capture, publisher);
        }
        if (result == STREAMCORE_RESULT_OK)
        {
            result = streamcore_capture_preflight(
                capture,
                &capture_preflight,
                capture_error_text,
                sizeof(capture_error_text));
        }
        if (result == STREAMCORE_RESULT_OK &&
            capture_preflight.is_ready_to_start == 0)
        {
            result = STREAMCORE_RESULT_OPERATION_FAILED;
        }
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_publisher_start(
            publisher,
            error_text,
            sizeof(error_text));
    }
    if (result == STREAMCORE_RESULT_OK && use_capture_sink)
    {
        result = streamcore_capture_start(
            capture,
            capture_error_text,
            sizeof(capture_error_text));
    }
    if (publisher != nullptr)
    {
        streamcore_publisher_get_runtime_info(publisher, &runtime_info);
    }
    if (capture != nullptr)
    {
        streamcore_capture_get_runtime_info(capture, &capture_runtime_info);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        active_publisher_ = publisher;
        active_publisher_capture_ = capture;
        SetOperationStatus(
            QString::fromUtf8("publisher.start"),
            QString::number(result),
            QString::fromUtf8("ok"),
            use_capture_sink && !ToQString(capture_runtime_info.preview_summary).isEmpty() ?
                ToQString(capture_runtime_info.preview_summary) :
                ToQString(runtime_info.state_summary));
        if (publisher_status_label_ != nullptr)
        {
            publisher_status_label_->setText(UiText(
                "Publish running. %1",
                "推流已运行。%1")
                    .arg(ToQString(runtime_info.state_summary)));
        }
        if (publisher_preview_label_ != nullptr)
        {
            if (use_capture_preview_sink)
            {
                publisher_preview_label_->setStyleSheet(QString::fromUtf8(
                    kPublisherPreviewLabelDefaultStyle));
                publisher_preview_label_->hide();
            }
            else
            {
                publisher_preview_label_->show();
                const bool show_passthrough_notice =
                    source_index == kPublisherSourceVideoFile &&
                    use_file_encoded_passthrough &&
                    config.enable_video != 0;
                if (show_passthrough_notice)
                {
                    publisher_preview_label_->setStyleSheet(QString::fromUtf8(
                        kPublisherPreviewLabelWarningStyle));
                    publisher_preview_label_->setText(UiText(
                        "PASSTHROUGH ACTIVE\nNO LOCAL PREVIEW\n\nThis file is being published from encoded packets, so this page intentionally does not open a local decode path.\nVerify the published stream from an RTMP/RTSP consumer or from the runtime logs.",
                        "编码包透传已启用\n当前无本地预览\n\n当前文件正以编码包直接推流，因此本页面不会额外打开本地解码预览。\n请通过 RTMP/RTSP 拉流端或运行日志验证推流输出。"));
                }
                else
                {
                    publisher_preview_label_->setStyleSheet(QString::fromUtf8(
                        kPublisherPreviewLabelDefaultStyle));
                    publisher_preview_label_->setText(UiText(
                        "LOCAL PREVIEW OFF\n\nInput: %1\nState: %2",
                        "本地预览已关闭\n\n输入：%1\n状态：%2")
                            .arg(ToQString(runtime_info.input_identity))
                            .arg(ToQString(streamcore_session_state_name(
                                runtime_info.state))));
                }
            }
        }
        if (publisher_processed_preview_label_ != nullptr)
        {
            if (use_processed_preview_sink)
            {
                publisher_processed_preview_label_->hide();
            }
            else
            {
                publisher_processed_preview_label_->setText(UiText(
                    "PROCESSED PREVIEW OFF",
                    "处理后预览已关闭"));
                publisher_processed_preview_label_->show();
            }
        }
    }
    else
    {
        if (capture != nullptr)
        {
            streamcore_capture_stop(capture);
            streamcore_capture_destroy(capture);
        }
        if (publisher != nullptr)
        {
            streamcore_publisher_stop(publisher);
            streamcore_publisher_destroy(publisher);
        }
        const QString capture_error = ToQString(capture_error_text);
        const QString publisher_error = ToQString(error_text);
        QString reason;
        if (!capture_error.isEmpty())
        {
            reason = capture_error;
        }
        else if (!publisher_error.isEmpty())
        {
            reason = publisher_error;
        }
        else if (use_capture_sink && capture_preflight.is_ready_to_start == 0)
        {
            reason = ToQString(capture_preflight.detail);
        }
        else
        {
            reason = ToQString(preflight.detail);
        }
        const QString result_name =
            ToQString(streamcore_result_name(result));
        const QString result_detail = reason.isEmpty() ?
            result_name :
            QString::fromUtf8("%1: %2").arg(result_name, reason);
        SetOperationStatus(
            QString::fromUtf8("publisher.start"),
            QString::number(result),
            result_name,
            result_detail);
        if (publisher_status_label_ != nullptr)
        {
            publisher_status_label_->setText(UiText(
                "Publish failed [%1]: %2",
                "推流失败 [%1]：%2").arg(result_name, reason.isEmpty() ?
                    UiText("unknown error", "未知错误") :
                    reason));
        }
        if (publisher_preview_label_ != nullptr)
        {
            publisher_preview_label_->setStyleSheet(QString::fromUtf8(
                kPublisherPreviewLabelDefaultStyle));
            publisher_preview_label_->show();
        }
        publisher_preview_label_->setText(UiText(
            "Publisher start failed.\nresult=%1 (%2) ready=%3 state=%4\n%5",
            "推流启动失败。\nresult=%1 (%2) ready=%3 state=%4\n%5")
                .arg(result)
                .arg(result_name)
                .arg(use_capture_sink ?
                    capture_preflight.is_ready_to_start :
                    preflight.is_ready_to_start)
                .arg(ToQString(streamcore_session_state_name(runtime_info.state)))
                .arg(reason));
        if (publisher_processed_preview_label_ != nullptr)
        {
            publisher_processed_preview_label_->setText(UiText(
                "Processed preview is unavailable because capture did not start.",
                "采集未能启动，处理后预览不可用。"));
            publisher_processed_preview_label_->show();
        }
    }

    UpdatePublisherButtons();
}

void StreamCoreDemoQtWindow::StopPublisher()
{
    if (active_publisher_ == nullptr &&
        active_publisher_capture_ == nullptr)
    {
        UpdatePublisherButtons();
        return;
    }

    if (active_publisher_capture_ != nullptr)
    {
        SetOperationStatus(
            QString::fromUtf8("publisher.stop.video_capture"),
            QString::fromUtf8("0"),
            QString::fromUtf8("stopping"),
            QString::fromUtf8("Stopping publisher video capture."));
        streamcore_capture_stop(active_publisher_capture_);
        streamcore_capture_destroy(active_publisher_capture_);
        active_publisher_capture_ = nullptr;
    }
    if (active_publisher_ != nullptr)
    {
        SetOperationStatus(
            QString::fromUtf8("publisher.stop.publisher"),
            QString::fromUtf8("0"),
            QString::fromUtf8("stopping"),
            QString::fromUtf8("Stopping publisher runtime."));
        streamcore_publisher_stop(active_publisher_);
        streamcore_publisher_destroy(active_publisher_);
        active_publisher_ = nullptr;
    }
    if (publisher_preview_label_ != nullptr)
    {
        publisher_preview_label_->show();
        publisher_preview_label_->setStyleSheet(QString::fromUtf8(
            kPublisherPreviewLabelDefaultStyle));
        publisher_preview_label_->setText(UiText(
            "Publisher stopped.",
            "Publisher stopped."));
    }
    if (publisher_processed_preview_label_ != nullptr)
    {
        publisher_processed_preview_label_->setStyleSheet(QString::fromUtf8(
            kPublisherPreviewLabelDefaultStyle));
        publisher_processed_preview_label_->setText(UiText(
            "Processed preview stopped.",
            "处理后预览已停止。"));
        publisher_processed_preview_label_->show();
    }
    SetOperationStatus(
        QString::fromUtf8("publisher.stop"),
        QString::fromUtf8("0"),
        QString::fromUtf8("ok"),
        UiText("Publisher stopped.", "Publisher stopped."));
    if (publisher_status_label_ != nullptr)
    {
        publisher_status_label_->setText(UiText(
            "Publish stopped.",
            "推流已停止。"));
    }
    UpdatePublisherButtons();
}

void StreamCoreDemoQtWindow::UpdatePublisherButtons()
{
    const bool running = active_publisher_ != nullptr ||
        active_publisher_capture_ != nullptr;
    const bool source_audio_empty =
        ComboValueOrIndex(publisher_source_combo_, kPublisherSourceCamera) ==
            kPublisherSourceNone &&
        ComboValueOrIndex(publisher_audio_combo_, kPublisherAudioMicrophone) ==
            kPublisherAudioNone;
    if (publisher_start_button_ != nullptr)
    {
        publisher_start_button_->setEnabled(running || !source_audio_empty);
        publisher_start_button_->setText(running ?
            UiText("Stop publish", "停止推流") :
            UiText("Start publish", "开始推流"));
    }
    if (publisher_processor_compare_toggle_ != nullptr)
    {
        const int source_index = ComboValueOrIndex(
            publisher_source_combo_,
            kPublisherSourceCamera);
        publisher_processor_compare_toggle_->setEnabled(
            !running && source_index != kPublisherSourceNone);
    }
}

void StreamCoreDemoQtWindow::UpdatePublisherSourceSummary()
{
    if (publisher_source_combo_ == nullptr ||
        publisher_audio_combo_ == nullptr ||
        publisher_url_edit_ == nullptr ||
        publisher_camera_device_combo_ == nullptr ||
        publisher_file_path_edit_ == nullptr ||
        publisher_audio_file_path_edit_ == nullptr ||
        publisher_file_mode_combo_ == nullptr ||
        publisher_source_summary_label_ == nullptr)
    {
        return;
    }

    const int source_index = ComboValueOrIndex(
        publisher_source_combo_,
        kPublisherSourceCamera);
    const bool is_camera = source_index == kPublisherSourceCamera;
    const bool is_desktop = source_index == kPublisherSourceDesktop;
    const bool is_video_file = source_index == kPublisherSourceVideoFile;
    const bool is_image = source_index == kPublisherSourceImage;
    const bool is_none = source_index == kPublisherSourceNone;
    int audio_index = ComboValueOrIndex(
        publisher_audio_combo_,
        kPublisherAudioMicrophone);
    QString video_source = QString::fromUtf8("none");
    QString audio_source = QString::fromUtf8("none");
    QString source_detail = UiText("default platform source", "平台默认来源");
    const QString publisher_file_path = publisher_file_path_edit_->text().trimmed();
    const QString publisher_audio_file_path =
        publisher_audio_file_path_edit_->text().trimmed();
    const int file_mode = ComboValueOrIndex(
        publisher_file_mode_combo_,
        kPublisherFileModeAuto);
    const bool processor_compare_enabled =
        publisher_processor_compare_toggle_ != nullptr &&
        publisher_processor_compare_toggle_->isChecked();
    const bool is_rtmp_target = publisher_url_edit_->text()
        .trimmed()
        .startsWith(QString::fromUtf8("rtmp://"), Qt::CaseInsensitive);
    QString file_processing;
    bool show_passthrough_preview_notice = false;
    streamcore_capture_source_info_t system_audio_sources[8] = {};
    size_t system_audio_count = 0;
    const bool system_audio_available =
        streamcore_capture_copy_source_list(
            STREAMCORE_CAPTURE_SOURCE_KIND_SYSTEM_AUDIO,
            system_audio_sources,
            sizeof(system_audio_sources) / sizeof(system_audio_sources[0]),
            &system_audio_count,
            nullptr,
            0) == STREAMCORE_RESULT_OK &&
        system_audio_count > 0;

    const QSignalBlocker blocker(publisher_audio_combo_);
    const bool audio_none_enabled = true;
    const bool external_audio_enabled = !is_video_file;
    publisher_audio_combo_->setItemData(
        0,
        audio_none_enabled,
        Qt::UserRole - 1);
    publisher_audio_combo_->setItemData(
        1,
        external_audio_enabled,
        Qt::UserRole - 1);
    publisher_audio_combo_->setItemData(
        2,
        external_audio_enabled && system_audio_available,
        Qt::UserRole - 1);
    publisher_audio_combo_->setItemData(
        3,
        external_audio_enabled,
        Qt::UserRole - 1);

    if (is_video_file && audio_index != kPublisherAudioNone)
    {
        publisher_audio_combo_->setCurrentIndex(0);
        UpdatePublisherSourceControls();
        return;
    }
    if (audio_index == kPublisherAudioSystem && !system_audio_available)
    {
        publisher_audio_combo_->setCurrentIndex(0);
        UpdatePublisherSourceControls();
        return;
    }
    audio_index = ComboValueOrIndex(publisher_audio_combo_, kPublisherAudioNone);

    if (is_camera)
    {
        video_source = UiText("camera", "camera");
        source_detail = publisher_camera_device_combo_->currentText();
    }
    else if (is_desktop)
    {
        video_source = UiText("desktop", "桌面");
        source_detail = UiText("desktop capture", "桌面采集");
    }
    else if (is_video_file)
    {
        streamcore_media_file_profile_t profile = {};
        char probe_error[STREAMCORE_TEXT_CAPACITY] = {};
        video_source = UiText("video file", "视频文件");
        audio_source = UiText(
            "video file audio track",
            "视频文件音轨");
        source_detail = publisher_file_path.isEmpty() ?
            UiText("not selected", "未选择") :
            publisher_file_path;
        if (!publisher_file_path.isEmpty())
        {
            const QByteArray file_path_bytes = publisher_file_path.toUtf8();
            const streamcore_result_t probe_result = streamcore_probe_media_file(
                file_path_bytes.constData(),
                &profile,
                probe_error,
                sizeof(probe_error));
            if (probe_result == STREAMCORE_RESULT_OK)
            {
                video_source = profile.has_video != 0 ?
                    QString::fromUtf8(profile.video_codec_name) :
                    UiText("no video track", "无视频轨");
                audio_source = profile.has_audio != 0 ?
                    QString::fromUtf8(profile.audio_codec_name) :
                    UiText("no audio track", "无音频轨");
                if (processor_compare_enabled &&
                    profile.has_video != 0)
                {
                    file_processing = UiText(
                        "Processor + transcode",
                        "Processor 处理后转码");
                }
                else if (file_mode == kPublisherFileModeForceTranscode)
                {
                    file_processing = UiText(
                        "transcode",
                        "转码");
                }
                else if (is_rtmp_target &&
                    profile.rtmp_passthrough_supported != 0)
                {
                    show_passthrough_preview_notice =
                        !processor_compare_enabled &&
                        file_mode != kPublisherFileModeForceTranscode &&
                        profile.has_video != 0;
                    file_processing = UiText(
                        "passthrough ready",
                        "可透传");
                }
                else
                {
                    file_processing = UiText(
                        "decode + transcode",
                        "解码后转码");
                }
            }
            else
            {
                file_processing = UiText("probe failed", "探测失败");
            }
        }
        else
        {
            file_processing = UiText(
                "select a file",
                "请选择文件");
        }
    }
    else if (is_image)
    {
        video_source = UiText("still image", "图片");
        source_detail = publisher_file_path.isEmpty() ?
            UiText("not selected", "未选择") :
            publisher_file_path;
    }
    else if (is_none)
    {
        source_detail = UiText("no video source", "无视频来源");
    }

    if (!is_video_file)
    {
        if (audio_index == kPublisherAudioMicrophone)
        {
            audio_source = publisher_audio_source_combo_ != nullptr &&
                !publisher_audio_source_combo_->currentText().trimmed().isEmpty() ?
                    publisher_audio_source_combo_->currentText().trimmed() :
                    UiText("default microphone", "默认麦克风");
        }
        else if (audio_index == kPublisherAudioSystem)
        {
            audio_source = publisher_audio_source_combo_ != nullptr &&
                !publisher_audio_source_combo_->currentText().trimmed().isEmpty() ?
                    publisher_audio_source_combo_->currentText().trimmed() :
                    UiText("default system mix", "默认系统混音");
        }
        else if (audio_index == kPublisherAudioFile)
        {
            audio_source = publisher_audio_file_path.isEmpty() ?
                UiText("audio file not selected", "音频文件未选择") :
                publisher_audio_file_path;
        }
    }

    const QString validity = is_none && audio_index == kPublisherAudioNone ?
        UiText("select a source", "请选择来源") :
        UiText("ready", "就绪");
    const bool preview_route_available = CurrentPublisherSelectionSupportsPreview();
    const QString preview_policy = preview_route_available ?
        (IsPublisherPreviewEnabled() ?
            (processor_compare_enabled ?
                UiText("before/after preview", "处理前后对比") :
                UiText("preview on", "预览开启")) :
            UiText("preview off", "预览关闭")) :
        UiText("no preview route", "无预览路线");
    QString source_badge = source_detail;
    if ((is_video_file || is_image) && !publisher_file_path.isEmpty())
    {
        source_badge = QFileInfo(publisher_file_path).fileName();
    }
    if (source_badge.isEmpty())
    {
        source_badge = UiText("default source", "默认来源");
    }
    QString audio_badge = audio_source;
    if (audio_index == kPublisherAudioFile && !publisher_audio_file_path.isEmpty())
    {
        audio_badge = QFileInfo(publisher_audio_file_path).fileName();
    }
    const QString file_badge = file_processing.isEmpty() ?
        QString() :
        UiText(" | File %1", " | 文件 %1").arg(file_processing);

    publisher_source_summary_label_->setText(UiText(
        "Input: %1 | %2 | Audio %3 | %4 | %5\nOutput: %6/%7 | %8 | %9 kbps | %10 fps | GOP %11%12",
        "输入：%1 | %2 | 音频 %3 | %4 | %5\n输出：%6/%7 | %8 | %9 kbps | %10 fps | GOP %11%12")
            .arg(publisher_source_combo_->currentText())
            .arg(source_badge)
            .arg(audio_badge)
            .arg(preview_policy)
            .arg(validity)
            .arg(SelectedPublisherVideoCodec())
            .arg(SelectedPublisherAudioProfileCodec())
            .arg(SelectedPublisherResolutionText())
            .arg(ReadBoundedInt(publisher_video_bitrate_edit_, 1200, 1, 100000))
            .arg(ReadBoundedInt(publisher_fps_edit_, 25, 1, 120))
            .arg(ReadBoundedInt(publisher_gop_edit_, 50, 1, 600))
            .arg(file_badge));
    if (publisher_preview_label_ != nullptr &&
        active_publisher_ == nullptr &&
        active_publisher_capture_ == nullptr)
    {
        publisher_preview_label_->show();
        if (show_passthrough_preview_notice)
        {
            publisher_preview_label_->setStyleSheet(QString::fromUtf8(
                kPublisherPreviewLabelWarningStyle));
            publisher_preview_label_->setText(UiText(
                "PASSTHROUGH READY\nNO LOCAL PREVIEW\n\nThis file can be published from encoded packets, so this page intentionally keeps the preview area unavailable.\nStart publishing and verify the output from an RTMP/RTSP consumer or from the runtime logs.",
                "编码包透传就绪\n当前无本地预览\n\n该文件可以直接以编码包推流，因此本页面会有意保持预览区域不可用。\n开始推流后，请通过 RTMP/RTSP 拉流端或运行日志验证输出。"));
        }
        else
        {
            publisher_preview_label_->setStyleSheet(QString::fromUtf8(
                kPublisherPreviewLabelDefaultStyle));
            if (preview_route_available && !IsPublisherPreviewEnabled())
            {
                publisher_preview_label_->setText(UiText(
                    "LOCAL PREVIEW OFF\n\nEnable the preview toggle when you want this page to render locally.",
                    "本地预览已关闭\n\n需要本地预览时，请开启预览开关。"));
            }
            else if (preview_route_available)
            {
                publisher_preview_label_->setText(UiText(
                    "LOCAL PREVIEW READY\n\nStart publishing to attach the local preview surface.",
                    "本地预览已就绪\n\n开始推流后会接入本地预览画面。"));
            }
            else
            {
                publisher_preview_label_->setText(UiText(
                    "Preview",
                    "预览"));
            }
        }
    }
    if (publisher_processed_preview_label_ != nullptr &&
        processor_compare_enabled &&
        active_publisher_ == nullptr &&
        active_publisher_capture_ == nullptr)
    {
        publisher_processed_preview_label_->setStyleSheet(QString::fromUtf8(
            kPublisherPreviewLabelDefaultStyle));
        publisher_processed_preview_label_->setText(
            IsPublisherPreviewEnabled() ?
                UiText(
                    "PROCESSED PREVIEW READY\n\nThe final monochrome frame will appear here after start.",
                    "处理后预览已就绪\n\n开始后，这里会显示 Processor 输出的黑白画面。") :
                UiText(
                    "PROCESSED PREVIEW OFF\n\nEnable Preview to display both views.",
                    "处理后预览已关闭\n\n开启“预览”后可同时显示两路画面。"));
        publisher_processed_preview_label_->show();
    }
    UpdatePublisherButtons();
}

void StreamCoreDemoQtWindow::UpdatePlayerSourceSummary()
{
    if (player_url_edit_ == nullptr || player_source_summary_label_ == nullptr)
    {
        return;
    }

    QString input_text = player_url_edit_->text().trimmed();
    if (input_text.length() > 56)
    {
        input_text = input_text.left(53) + QString::fromUtf8("...");
    }
    if (input_text.isEmpty())
    {
        input_text = UiText("<enter a player URL>", "<请输入播放地址>");
    }
    QString summary = UiText(
        "Input: %1\nMode: %2",
        "输入：%1\n模式：%2")
            .arg(input_text)
            .arg(PlayerPlaybackModeSummary());
    if (!player_onvif_status_.isEmpty())
    {
        summary += QString::fromUtf8("\n") + player_onvif_status_;
    }
    player_source_summary_label_->setText(summary);
    player_source_summary_label_->setVisible(true);
    if (active_player_ != nullptr && desktop_render_target_status_label_ != nullptr)
    {
        desktop_render_target_status_label_->setText(UiText(
            "Playing · stream resolution/bitrate pending · %1",
            "播放中 · 码流分辨率/码率待统计 · %1")
                .arg(PlayerPlaybackModeSummary()));
    }
}

void StreamCoreDemoQtWindow::TogglePlayerFullscreen()
{
    if (player_fullscreen_active_)
    {
        ExitPlayerFullscreen();
        return;
    }
    EnterPlayerFullscreen();
}

void StreamCoreDemoQtWindow::EnterPlayerFullscreen()
{
    if (player_fullscreen_active_ ||
        desktop_render_target_frame_ == nullptr ||
        player_preview_layout_ == nullptr)
    {
        return;
    }

    player_fullscreen_window_ = new QWidget(nullptr);
    player_fullscreen_window_->setObjectName(
        QString::fromUtf8("player_fullscreen_window"));
    player_fullscreen_window_->setWindowTitle(UiText("Player fullscreen", "播放全屏"));
    player_fullscreen_window_->setWindowFlags(
        Qt::Window |
        Qt::FramelessWindowHint |
        Qt::WindowStaysOnTopHint);
    player_fullscreen_window_->setStyleSheet(QString::fromUtf8(
        "QWidget#player_fullscreen_window { background: #111827; }"));
    player_fullscreen_window_->installEventFilter(this);
    player_fullscreen_window_->setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* fullscreen_layout = new QVBoxLayout(player_fullscreen_window_);
    fullscreen_layout->setContentsMargins(0, 0, 0, 0);
    fullscreen_layout->setSpacing(0);

    player_preview_layout_->removeWidget(desktop_render_target_frame_);
    desktop_render_target_frame_->setParent(player_fullscreen_window_);
    fullscreen_layout->addWidget(desktop_render_target_frame_, 1);
    desktop_render_target_frame_->show();
    player_fullscreen_active_ = true;
    player_fullscreen_window_->showFullScreen();
    player_fullscreen_window_->activateWindow();
    if (desktop_render_target_widget_ != nullptr)
    {
        desktop_render_target_widget_->setFocus(Qt::OtherFocusReason);
    }

    QTimer::singleShot(0, this, [this]() {
        ApplyPreviewDisplayMode();
        RefreshActivePlayerRenderTarget();
    });
}

void StreamCoreDemoQtWindow::ExitPlayerFullscreen()
{
    if (!player_fullscreen_active_)
    {
        return;
    }

    QWidget* fullscreen_window = player_fullscreen_window_;
    player_fullscreen_window_ = nullptr;
    player_fullscreen_active_ = false;

    if (desktop_render_target_frame_ != nullptr &&
        player_preview_layout_ != nullptr)
    {
        if (fullscreen_window != nullptr && fullscreen_window->layout() != nullptr)
        {
            fullscreen_window->layout()->removeWidget(desktop_render_target_frame_);
        }
        desktop_render_target_frame_->setMinimumSize(520, kPreviewFrameMinHeight);
        desktop_render_target_frame_->setMaximumHeight(QWIDGETSIZE_MAX);
        player_preview_layout_->insertWidget(0, desktop_render_target_frame_, 0);
        desktop_render_target_frame_->show();
    }

    if (fullscreen_window != nullptr)
    {
        fullscreen_window->removeEventFilter(this);
        fullscreen_window->hide();
        fullscreen_window->deleteLater();
    }

    QTimer::singleShot(0, this, [this]() {
        ApplyPreviewDisplayMode();
        RefreshActivePlayerRenderTarget();
    });
}

bool StreamCoreDemoQtWindow::RefreshActivePlayerRenderTarget()
{
    if (active_player_ == nullptr || desktop_render_target_widget_ == nullptr)
    {
        BindDesktopRenderTarget();
        return false;
    }

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    streamcore_render_target_t render_target = {};
    if (!BuildStreamCoreRenderTargetForWidget(
            desktop_render_target_widget_,
            &render_target))
    {
        return false;
    }
    const streamcore_result_t result =
        streamcore_player_set_render_target(active_player_, &render_target);
    if (result == STREAMCORE_RESULT_OK)
    {
        UpdateDesktopRenderTargetStatusFromGeometry();
        return true;
    }
    statusBar()->showMessage(
        QString::fromUtf8("player fullscreen render-target refresh failed: %1")
            .arg(result),
        5000);
#endif
    return false;
}

void StreamCoreDemoQtWindow::ApplyPlayerLatencyPreset(int presetIndex)
{
    if (player_decode_mode_combo_ == nullptr ||
        player_render_path_combo_ == nullptr ||
        player_buffer_ms_edit_ == nullptr ||
        player_audio_queue_edit_ == nullptr ||
        player_video_queue_edit_ == nullptr)
    {
        return;
    }

    const QSignalBlocker decode_blocker(player_decode_mode_combo_);
    const QSignalBlocker render_blocker(player_render_path_combo_);
    const QSignalBlocker buffer_blocker(player_buffer_ms_edit_);
    const QSignalBlocker audio_queue_blocker(player_audio_queue_edit_);
    const QSignalBlocker video_queue_blocker(player_video_queue_edit_);

    if (presetIndex == 3)
    {
        player_decode_mode_combo_->setCurrentIndex(1);
        player_render_path_combo_->setCurrentIndex(2);
        player_buffer_ms_edit_->setText(QString::fromUtf8("0"));
        player_audio_queue_edit_->setText(QString::fromUtf8("0"));
        player_video_queue_edit_->setText(QString::fromUtf8("0"));
        return;
    }
    if (presetIndex == 2)
    {
        player_decode_mode_combo_->setCurrentIndex(1);
        player_render_path_combo_->setCurrentIndex(3);
        player_buffer_ms_edit_->setText(QString::fromUtf8("80"));
        player_audio_queue_edit_->setText(QString::fromUtf8("8"));
        player_video_queue_edit_->setText(QString::fromUtf8("4"));
        return;
    }
    if (presetIndex == 1)
    {
        player_decode_mode_combo_->setCurrentIndex(1);
        player_render_path_combo_->setCurrentIndex(3);
        player_buffer_ms_edit_->setText(QString::fromUtf8("300"));
        player_audio_queue_edit_->setText(QString::fromUtf8("24"));
        player_video_queue_edit_->setText(QString::fromUtf8("12"));
        return;
    }

    player_decode_mode_combo_->setCurrentIndex(0);
    player_render_path_combo_->setCurrentIndex(0);
    player_buffer_ms_edit_->setText(QString::fromUtf8("300"));
    player_audio_queue_edit_->setText(QString::fromUtf8("24"));
    player_video_queue_edit_->setText(QString::fromUtf8("12"));
}

streamcore_player_video_present_path_t
StreamCoreDemoQtWindow::SelectedPlayerPresentPath() const
{
    const int index = player_render_path_combo_ != nullptr ?
        player_render_path_combo_->currentIndex() :
        0;
    if (index == 1)
    {
        return STREAMCORE_PLAYER_VIDEO_PRESENT_PATH_GPU_FRAME;
    }
    if (index == 2)
    {
        return STREAMCORE_PLAYER_VIDEO_PRESENT_PATH_DIRECT_SURFACE;
    }
    if (index == 3)
    {
        return STREAMCORE_PLAYER_VIDEO_PRESENT_PATH_AUTO;
    }
    return STREAMCORE_PLAYER_VIDEO_PRESENT_PATH_SOFTWARE_FRAME;
}

bool StreamCoreDemoQtWindow::SelectedPlayerHardwareDecode() const
{
    return player_decode_mode_combo_ != nullptr &&
        player_decode_mode_combo_->currentIndex() == 1;
}

bool StreamCoreDemoQtWindow::SelectedPlayerPrefersSoftwareRenderBackend() const
{
    return SelectedPlayerPresentPath() ==
        STREAMCORE_PLAYER_VIDEO_PRESENT_PATH_SOFTWARE_FRAME;
}

QString StreamCoreDemoQtWindow::PlayerPlaybackModeSummary() const
{
    const QString preset = player_latency_preset_combo_ != nullptr ?
        player_latency_preset_combo_->currentText() :
        UiText("Regular", "常规");
    return UiText(
        "%1 / %2 / display %3 / buffer %4 ms / queue cap A/V %5/%6 / volume %7%",
        "%1 / %2 / 显示 %3 / 缓冲 %4 ms / 队列上限 A/V %5/%6 / 音量 %7%")
            .arg(preset)
            .arg(SelectedPlayerHardwareDecode() ?
                UiText("hardware decode requested", "请求硬解") :
                UiText("software decode", "软件解码"))
            .arg(PreviewDisplayModeText())
            .arg(ReadBoundedInt(player_buffer_ms_edit_, 300, -1, 10000))
            .arg(ReadBoundedInt(player_audio_queue_edit_, 24, 0, 400))
            .arg(ReadBoundedInt(player_video_queue_edit_, 12, 0, 200))
            .arg(SelectedPlayerAudioVolumePercent());
}

int StreamCoreDemoQtWindow::SelectedPublisherAudioVolumePercent() const
{
    return publisher_audio_volume_slider_ != nullptr ?
        publisher_audio_volume_slider_->value() :
        100;
}

int StreamCoreDemoQtWindow::SelectedPlayerAudioVolumePercent() const
{
    return player_audio_volume_slider_ != nullptr ?
        player_audio_volume_slider_->value() :
        100;
}

int StreamCoreDemoQtWindow::SelectedGb28181AudioVolumePercent() const
{
    return gb28181_audio_volume_slider_ != nullptr ?
        gb28181_audio_volume_slider_->value() :
        100;
}

void StreamCoreDemoQtWindow::UpdateAudioVolumeLabels()
{
    if (publisher_audio_volume_value_label_ != nullptr)
    {
        publisher_audio_volume_value_label_->setText(
            QString::fromUtf8("%1%").arg(SelectedPublisherAudioVolumePercent()));
    }
    if (player_audio_volume_value_label_ != nullptr)
    {
        player_audio_volume_value_label_->setText(
            QString::fromUtf8("%1%").arg(SelectedPlayerAudioVolumePercent()));
    }
    if (gb28181_audio_volume_value_label_ != nullptr)
    {
        gb28181_audio_volume_value_label_->setText(
            QString::fromUtf8("%1%").arg(SelectedGb28181AudioVolumePercent()));
    }
}

void StreamCoreDemoQtWindow::RouteOperationRuntimeLogLine(const QString& line)
{
    const QString action = latest_action_.trimmed().toLower();
    if (action.startsWith(QString::fromUtf8("publisher")) ||
        action.startsWith(QString::fromUtf8("qt.autorun.publisher")))
    {
        AppendRuntimeLog(publisher_runtime_log_, line);
        return;
    }
    if (action.startsWith(QString::fromUtf8("player")) ||
        action.startsWith(QString::fromUtf8("qt.autorun.player")) ||
        action.startsWith(QString::fromUtf8("qt.autorun.onvif")))
    {
        AppendRuntimeLog(player_runtime_log_, line);
        return;
    }
    if (action.startsWith(QString::fromUtf8("gb28181")) ||
        action.startsWith(QString::fromUtf8("qt.autorun.gb28181")))
    {
        AppendRuntimeLog(gb28181_runtime_log_, line);
    }
}

void StreamCoreDemoQtWindow::AppendRuntimeLog(
    QPlainTextEdit* logView,
    const QString& line)
{
    if (logView == nullptr || line.trimmed().isEmpty())
    {
        return;
    }
    logView->appendPlainText(line);
    if (QScrollBar* scroll_bar = logView->verticalScrollBar())
    {
        scroll_bar->setValue(scroll_bar->maximum());
    }
}

int StreamCoreDemoQtWindow::ReadBoundedInt(
    const QLineEdit* input,
    int defaultValue,
    int minValue,
    int maxValue) const
{
    bool ok = false;
    int value = input != nullptr ? input->text().trimmed().toInt(&ok) : defaultValue;
    if (!ok)
    {
        value = defaultValue;
    }
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

QString StreamCoreDemoQtWindow::ReadText(
    const QLineEdit* input,
    const char* defaultValue) const
{
    const QString value = input != nullptr ? input->text().trimmed() : QString();
    return value.isEmpty() ? QString::fromUtf8(defaultValue) : value;
}

void StreamCoreDemoQtWindow::SearchOnvifDevices()
{
    if (player_onvif_device_list_ == nullptr ||
        player_onvif_search_button_ == nullptr ||
        player_onvif_apply_button_ == nullptr)
    {
        return;
    }

    streamcore_onvif_discovery_config_t base_config;
    char error_text[STREAMCORE_URL_CAPACITY] = {};

    streamcore_onvif_get_default_discovery_config(&base_config);
    base_config.response_wait_ms = 1200;
    base_config.send_probe_count = 2;
    base_config.max_result_count = 32;
#if STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
    base_config.resolve_stream_uri = 0;
#endif
    const QString bind_ip =
        EnvironmentText("STREAMCORE_DEMO_QT_ONVIF_BIND_IP");
    const QString probe_endpoint =
        EnvironmentText("STREAMCORE_DEMO_QT_ONVIF_PROBE_ENDPOINT");
    const QByteArray probe_endpoint_bytes = probe_endpoint.toUtf8();
    if (!probe_endpoint_bytes.isEmpty())
    {
        base_config.probe_endpoint = probe_endpoint_bytes.constData();
        base_config.additional_probe_endpoints = "";
    }

    player_onvif_search_button_->setEnabled(false);
    player_onvif_apply_button_->setEnabled(false);
    player_onvif_status_ = UiText(
        "Searching ONVIF devices...",
        "正在搜索 ONVIF 设备...");
    UpdatePlayerSourceSummary();
    QApplication::processEvents();

    const QByteArray explicit_bind_ip = bind_ip.toUtf8();
    if (!explicit_bind_ip.isEmpty())
    {
        base_config.bind_ip = explicit_bind_ip.constData();
    }

    streamcore_onvif_device_t discovered_devices[32] = {};
    size_t discovered_count = 0;
    std::memset(error_text, 0, sizeof(error_text));
    const streamcore_result_t result =
        streamcore_onvif_discover_devices(
            &base_config,
            discovered_devices,
            sizeof(discovered_devices) / sizeof(discovered_devices[0]),
            &discovered_count,
            error_text,
            sizeof(error_text));

    onvif_devices_.clear();
    player_onvif_device_list_->clear();

    const size_t writable_count =
        discovered_count < (sizeof(discovered_devices) / sizeof(discovered_devices[0])) ?
            discovered_count :
            (sizeof(discovered_devices) / sizeof(discovered_devices[0]));
    for (size_t i = 0; i < writable_count; ++i)
    {
        const streamcore_onvif_device_t& device = discovered_devices[i];
        OnvifDeviceItem item;
        item.device_urn = ToQString(device.device_urn);
        item.response_endpoint = ToQString(device.response_endpoint);
        item.service_url = ToQString(device.service_url);
#if STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
        item.profile_name = ToQString(device.profile_name);
        item.profile_token = ToQString(device.profile_token);
        item.stream_uri = ToQString(device.stream_uri);
        item.resolved_stream_uri = device.resolved_stream_uri != 0;
#else
        item.resolved_stream_uri = false;
#endif
        item.label = OnvifEndpointLabel(device);
        if (item.label.isEmpty())
        {
            item.label = item.device_urn;
        }
        if (item.label.isEmpty())
        {
            item.label = UiText("ONVIF device", "ONVIF 设备");
        }
#if STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
        if (!item.stream_uri.isEmpty())
        {
            item.label += QString::fromUtf8(" | RTSP");
        }
#endif
        onvif_devices_.push_back(item);
        player_onvif_device_list_->addItem(item.label);
    }

    if (onvif_devices_.isEmpty())
    {
        player_onvif_device_list_->addItem(
            UiText("No ONVIF devices discovered", "尚未搜索到 ONVIF 设备"));
    }

    player_onvif_search_button_->setEnabled(true);
    player_onvif_apply_button_->setEnabled(false);

    if (result == STREAMCORE_RESULT_OK ||
        result == STREAMCORE_RESULT_BUFFER_TOO_SMALL)
    {
        player_onvif_status_ = UiText(
            "ONVIF: %1 device(s) discovered, %2 displayed.",
            "ONVIF: %1 device(s) discovered, %2 displayed.")
                .arg(static_cast<qulonglong>(discovered_count))
                .arg(onvif_devices_.size());
        if (!onvif_devices_.isEmpty())
        {
            player_onvif_device_list_->setCurrentRow(0);
            UpdateOnvifDeviceSummary();
        }
        else
        {
            UpdatePlayerSourceSummary();
        }
    }
    else
    {
        player_onvif_status_ = UiText(
            "ONVIF search failed. See status details.",
            "ONVIF search failed. See status details.");
        statusBar()->showMessage(
            QString::fromUtf8("ONVIF result=%1 | %2")
                .arg(result)
                .arg(ToQString(error_text)),
            5000);
        UpdatePlayerSourceSummary();
    }
}

void StreamCoreDemoQtWindow::ApplySelectedOnvifDevice()
{
    if (player_onvif_device_list_ == nullptr ||
        player_url_edit_ == nullptr)
    {
        return;
    }

    const int index = player_onvif_device_list_->currentRow();
    if (index < 0 || index >= onvif_devices_.size())
    {
        return;
    }

#if !STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
    player_onvif_status_ = UiText(
        "This SDK package exposes ONVIF discovery only; enter the RTSP URL manually.",
        "当前 SDK 包只提供 ONVIF 发现，请手动填写 RTSP 地址。");
    UpdatePlayerSourceSummary();
    return;
#else
    const OnvifDeviceItem& item = onvif_devices_[index];
    if (item.stream_uri.isEmpty())
    {
        OnvifDeviceItem resolved_item = item;
        if (!ResolveSelectedOnvifStreamUri(&resolved_item) ||
            resolved_item.stream_uri.isEmpty())
        {
            player_onvif_status_ = UiText(
                "ONVIF: stream URI is unresolved. Enter the ONVIF username/password and try again, or paste an RTSP URL manually.",
                "ONVIF：未能解析流地址。请填写 ONVIF 用户名/密码后重试，或手动粘贴 RTSP 地址。");
            UpdatePlayerSourceSummary();
            return;
        }

        onvif_devices_[index] = resolved_item;
        if (QListWidgetItem* item_widget =
                player_onvif_device_list_->item(index))
        {
            item_widget->setText(resolved_item.label);
        }
    }

    const OnvifDeviceItem& applied_item = onvif_devices_[index];
    const QString username = player_onvif_username_edit_ != nullptr ?
        player_onvif_username_edit_->text().trimmed() :
        QString();
    const QString password = player_onvif_password_edit_ != nullptr ?
        player_onvif_password_edit_->text() :
        QString();
    player_url_edit_->setText(
        ApplyRtspCredentials(applied_item.stream_uri, username, password));
    player_url_edit_->setCursorPosition(0);
    player_onvif_status_ = UiText(
        "ONVIF stream applied to Player URL.",
        "ONVIF 流地址已填入播放地址。");
    UpdatePlayerSourceSummary();
#endif
}

bool StreamCoreDemoQtWindow::ResolveSelectedOnvifStreamUri(
    OnvifDeviceItem* item)
{
    if (item == nullptr)
    {
        return false;
    }

#if !STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
    statusBar()->showMessage(
        UiText(
            "This SDK package exposes ONVIF discovery only; enter the RTSP URL manually.",
            "当前 SDK 包只提供 ONVIF 发现，请手动填写 RTSP 地址。"),
        5000);
    return false;
#else
    QString probe_endpoint = item->response_endpoint.trimmed();
    if (probe_endpoint.isEmpty() && !item->service_url.isEmpty())
    {
        const QUrl service_url(item->service_url);
        QString host = service_url.host();
        if (!host.isEmpty())
        {
            const int port = 3702;
            probe_endpoint = QString::fromUtf8("%1:%2").arg(host).arg(port);
        }
    }
    if (probe_endpoint.isEmpty())
    {
        return false;
    }

    streamcore_onvif_discovery_config_t config;
    streamcore_onvif_device_t devices[8];
    char error_text[STREAMCORE_URL_CAPACITY] = {};
    size_t discovered_count = 0;
    const QByteArray probe_endpoint_bytes = probe_endpoint.toUtf8();
    const QByteArray username_bytes =
        player_onvif_username_edit_ != nullptr ?
            player_onvif_username_edit_->text().trimmed().toUtf8() :
            QByteArray();
    const QByteArray password_bytes =
        player_onvif_password_edit_ != nullptr ?
            player_onvif_password_edit_->text().toUtf8() :
            QByteArray();

    streamcore_onvif_get_default_discovery_config(&config);
    config.probe_endpoint = probe_endpoint_bytes.constData();
    config.additional_probe_endpoints = "";
    config.response_wait_ms = 1200;
    config.send_probe_count = 2;
    config.max_result_count = sizeof(devices) / sizeof(devices[0]);
    config.username = username_bytes.constData();
    config.password = password_bytes.constData();
    config.resolve_stream_uri = 1;

    player_onvif_status_ = UiText(
        "Resolving ONVIF stream URI...",
        "正在解析 ONVIF 流地址...");
    UpdatePlayerSourceSummary();
    QApplication::processEvents();

    const streamcore_result_t result = streamcore_onvif_discover_devices(
        &config,
        devices,
        sizeof(devices) / sizeof(devices[0]),
        &discovered_count,
        error_text,
        sizeof(error_text));
    const size_t writable_count =
        discovered_count < (sizeof(devices) / sizeof(devices[0])) ?
            discovered_count :
            (sizeof(devices) / sizeof(devices[0]));
    for (size_t i = 0; i < writable_count; ++i)
    {
        const QString stream_uri = ToQString(devices[i].stream_uri);
        if (stream_uri.isEmpty())
        {
            continue;
        }
        item->device_urn = ToQString(devices[i].device_urn);
        item->response_endpoint = ToQString(devices[i].response_endpoint);
        item->service_url = ToQString(devices[i].service_url);
        item->profile_name = ToQString(devices[i].profile_name);
        item->profile_token = ToQString(devices[i].profile_token);
        item->stream_uri = stream_uri;
        item->resolved_stream_uri = devices[i].resolved_stream_uri != 0;
        item->label = OnvifEndpointLabel(devices[i]);
        if (item->label.isEmpty())
        {
            item->label = UiText("ONVIF device", "ONVIF 设备");
        }
        item->label += QString::fromUtf8(" | RTSP");
        return true;
    }

    if (result != STREAMCORE_RESULT_OK)
    {
        statusBar()->showMessage(
            QString::fromUtf8("ONVIF stream resolve result=%1 | %2")
                .arg(result)
                .arg(ToQString(error_text)),
            5000);
    }
    return false;
#endif
}

void StreamCoreDemoQtWindow::UpdateOnvifDeviceSummary()
{
    if (player_onvif_device_list_ == nullptr ||
        player_onvif_apply_button_ == nullptr)
    {
        return;
    }

    const int index = player_onvif_device_list_->currentRow();
    if (index < 0 || index >= onvif_devices_.size())
    {
        player_onvif_apply_button_->setEnabled(false);
        if (onvif_devices_.isEmpty())
        {
            player_onvif_status_.clear();
        }
        UpdatePlayerSourceSummary();
        return;
    }

    const OnvifDeviceItem& item = onvif_devices_[index];
    const QString profile =
        item.profile_name.isEmpty() ? item.profile_token : item.profile_name;
    const QString stream_state = item.stream_uri.isEmpty() ?
        UiText("stream unresolved", "码流未解析") :
        UiText("stream ready", "码流就绪");
    player_onvif_apply_button_->setEnabled(true);
    player_onvif_status_ = UiText(
        "ONVIF: %1 | %2 | %3",
        "ONVIF：%1 | %2 | %3")
            .arg(item.label)
            .arg(profile.isEmpty() ? QString::fromUtf8("default profile") : profile)
            .arg(stream_state);
    UpdatePlayerSourceSummary();
}

void StreamCoreDemoQtWindow::UpdateGb28181SourceSummary()
{
    if (gb28181_source_combo_ == nullptr ||
        gb28181_video_source_combo_ == nullptr ||
        gb28181_audio_combo_ == nullptr ||
        gb28181_audio_source_combo_ == nullptr ||
        gb28181_resolution_combo_ == nullptr ||
        gb28181_fps_edit_ == nullptr ||
        gb28181_upper_ip_edit_ == nullptr ||
        gb28181_upper_port_edit_ == nullptr ||
        gb28181_upper_transport_combo_ == nullptr ||
        gb28181_source_summary_label_ == nullptr)
    {
        return;
    }

    const int source_index =
        ComboValueOrIndex(gb28181_source_combo_, kGb28181SourceCamera);
    const int audio_index =
        ComboValueOrIndex(gb28181_audio_combo_, kGb28181AudioMicrophone);

    const auto refresh_video_sources = [this, source_index]() {
        QString previous_id = gb28181_video_source_combo_->currentData().toString();
        const QSignalBlocker blocker(gb28181_video_source_combo_);
        gb28181_video_source_combo_->clear();
        streamcore_capture_source_info_t sources[32] = {};
        size_t source_count = 0;
        const bool wants_desktop = source_index == kGb28181SourceDesktop;
        const streamcore_capture_source_kind_t source_kind = wants_desktop ?
            STREAMCORE_CAPTURE_SOURCE_KIND_DESKTOP :
            STREAMCORE_CAPTURE_SOURCE_KIND_CAMERA;
        const streamcore_result_t result = streamcore_capture_copy_source_list(
            source_kind,
            sources,
            sizeof(sources) / sizeof(sources[0]),
            &source_count,
            nullptr,
            0);
        int default_index = -1;
        if (result == STREAMCORE_RESULT_OK)
        {
            for (size_t i = 0; i < source_count; ++i)
            {
                const streamcore_capture_source_info_t& source = sources[i];
                if (source.source_kind != source_kind)
                {
                    continue;
                }
                const QString source_id = wants_desktop ?
                    QString::fromUtf8(source.display_id) :
                    QString::fromUtf8(source.source_id);
                QString label = wants_desktop ?
                    QString::fromUtf8(source.display_name).trimmed() :
                    QString::fromUtf8(source.source_name).trimmed();
                if (label.isEmpty())
                {
                    label = wants_desktop ?
                        UiText("Primary desktop", "主显示器") :
                        UiText("Default camera", "默认摄像头");
                }
                if (wants_desktop &&
                    source.display_width > 0 &&
                    source.display_height > 0)
                {
                    label += QString::fromUtf8(" · %1x%2")
                        .arg(source.display_width)
                        .arg(source.display_height);
                }
                gb28181_video_source_combo_->addItem(label, source_id);
                if (!previous_id.isEmpty() && previous_id == source_id)
                {
                    gb28181_video_source_combo_->setCurrentIndex(
                        gb28181_video_source_combo_->count() - 1);
                }
                if (source.is_default != 0 && default_index < 0)
                {
                    default_index = gb28181_video_source_combo_->count() - 1;
                }
            }
        }
        if (gb28181_video_source_combo_->count() == 0)
        {
            gb28181_video_source_combo_->addItem(
                wants_desktop ?
                    UiText("Primary desktop", "主显示器") :
                    UiText("Default camera", "默认摄像头"),
                QString());
        }
        else if (gb28181_video_source_combo_->currentIndex() < 0 &&
            default_index >= 0)
        {
            gb28181_video_source_combo_->setCurrentIndex(default_index);
        }
    };
    const auto refresh_audio_sources = [this, audio_index]() {
        const QSignalBlocker blocker(gb28181_audio_source_combo_);
        const QString previous_id = gb28181_audio_source_combo_->currentData().toString();
        gb28181_audio_source_combo_->clear();
        if (audio_index != kGb28181AudioMicrophone &&
            audio_index != kGb28181AudioSystem)
        {
            return;
        }
        streamcore_capture_source_info_t sources[16] = {};
        size_t source_count = 0;
        const streamcore_capture_source_kind_t source_kind =
            audio_index == kGb28181AudioSystem ?
                STREAMCORE_CAPTURE_SOURCE_KIND_SYSTEM_AUDIO :
                STREAMCORE_CAPTURE_SOURCE_KIND_MICROPHONE;
        const streamcore_result_t result = streamcore_capture_copy_source_list(
            source_kind,
            sources,
            sizeof(sources) / sizeof(sources[0]),
            &source_count,
            nullptr,
            0);
        int default_index = -1;
        if (result == STREAMCORE_RESULT_OK)
        {
            for (size_t i = 0; i < source_count; ++i)
            {
                const streamcore_capture_source_info_t& source = sources[i];
                if (source.source_kind != source_kind)
                {
                    continue;
                }
                const QString source_id = QString::fromUtf8(source.source_id);
                QString label = QString::fromUtf8(source.source_name).trimmed();
                if (label.isEmpty())
                {
                    label = audio_index == kGb28181AudioSystem ?
                        UiText("Default system mix", "默认系统混音") :
                        UiText("Default microphone", "默认麦克风");
                }
                gb28181_audio_source_combo_->addItem(label, source_id);
                if (!previous_id.isEmpty() && previous_id == source_id)
                {
                    gb28181_audio_source_combo_->setCurrentIndex(
                        gb28181_audio_source_combo_->count() - 1);
                }
                if (source.is_default != 0 && default_index < 0)
                {
                    default_index = gb28181_audio_source_combo_->count() - 1;
                }
            }
        }
        if (gb28181_audio_source_combo_->count() == 0)
        {
            gb28181_audio_source_combo_->addItem(
                audio_index == kGb28181AudioSystem ?
                    UiText("Default system mix", "默认系统混音") :
                    UiText("Default microphone", "默认麦克风"),
                QString());
        }
        else if (gb28181_audio_source_combo_->currentIndex() < 0 &&
            default_index >= 0)
        {
            gb28181_audio_source_combo_->setCurrentIndex(default_index);
        }
    };
    const auto refresh_resolutions = [this, source_index]() {
        const QSize previous_size = gb28181_resolution_combo_->currentData().toSize();
        const QSignalBlocker blocker(gb28181_resolution_combo_);
        gb28181_resolution_combo_->clear();
        const auto add_resolution = [this](const QString& label, const QSize& size) {
            gb28181_resolution_combo_->addItem(label, size);
        };
        if (source_index == kGb28181SourceDesktop)
        {
            streamcore_capture_source_info_t desktop_sources[16] = {};
            size_t desktop_source_count = 0;
            const streamcore_result_t result = streamcore_capture_copy_source_list(
                STREAMCORE_CAPTURE_SOURCE_KIND_DESKTOP,
                desktop_sources,
                sizeof(desktop_sources) / sizeof(desktop_sources[0]),
                &desktop_source_count,
                nullptr,
                0);
            if (result == STREAMCORE_RESULT_OK)
            {
                for (size_t i = 0; i < desktop_source_count; ++i)
                {
                    const streamcore_capture_source_info_t& source =
                        desktop_sources[i];
                    if (source.display_width <= 0 || source.display_height <= 0)
                    {
                        continue;
                    }
                    QString label = QString::fromUtf8(source.display_name).trimmed();
                    if (label.isEmpty())
                    {
                        label = UiText("Desktop", "桌面");
                    }
                    add_resolution(
                        QString::fromUtf8("%1 · %2x%3")
                            .arg(label)
                            .arg(source.display_width)
                            .arg(source.display_height),
                        QSize(source.display_width, source.display_height));
                }
            }
            if (gb28181_resolution_combo_->count() == 0)
            {
                add_resolution(QString::fromUtf8("1920x1080"), QSize(1920, 1080));
                add_resolution(QString::fromUtf8("1280x720"), QSize(1280, 720));
            }
        }
        else
        {
            add_resolution(UiText("Camera default", "摄像头默认"), QSize(0, 0));
            add_resolution(QString::fromUtf8("1920x1080"), QSize(1920, 1080));
            add_resolution(QString::fromUtf8("1280x720"), QSize(1280, 720));
            add_resolution(QString::fromUtf8("640x480"), QSize(640, 480));
        }
        for (int i = 0; i < gb28181_resolution_combo_->count(); ++i)
        {
            if (gb28181_resolution_combo_->itemData(i).toSize() == previous_size)
            {
                gb28181_resolution_combo_->setCurrentIndex(i);
                break;
            }
        }
        if (gb28181_resolution_combo_->currentIndex() < 0)
        {
            gb28181_resolution_combo_->setCurrentIndex(0);
        }
    };

    refresh_video_sources();
    refresh_audio_sources();
    refresh_resolutions();

    gb28181_video_detail_row_->setVisible(true);
    gb28181_audio_detail_row_->setVisible(audio_index != kGb28181AudioNone);
    gb28181_audio_source_combo_->setEnabled(audio_index != kGb28181AudioNone);

    const QString local_id = ReadText(gb28181_local_id_edit_, "34020000001320000001");
    const QString upper_ip = gb28181_upper_ip_edit_->text().trimmed();
    const QString upper_port = gb28181_upper_port_edit_->text().trimmed();
    const QString upper_transport =
        gb28181_upper_transport_combo_->currentData().toString().toUpper();
    const QString video_source_name =
        gb28181_video_source_combo_->currentText().trimmed().isEmpty() ?
            UiText("default source", "默认来源") :
            gb28181_video_source_combo_->currentText().trimmed();
    QString audio_source_name = UiText("none", "无");
    if (audio_index == kGb28181AudioMicrophone)
    {
        audio_source_name = gb28181_audio_source_combo_->currentText().trimmed().isEmpty() ?
            UiText("default microphone", "默认麦克风") :
            gb28181_audio_source_combo_->currentText().trimmed();
    }
    else if (audio_index == kGb28181AudioSystem)
    {
        audio_source_name = gb28181_audio_source_combo_->currentText().trimmed().isEmpty() ?
            UiText("default system mix", "默认系统混音") :
            gb28181_audio_source_combo_->currentText().trimmed();
    }
    const QSize resolution = gb28181_resolution_combo_->currentData().toSize();
    const QString resolution_text =
        resolution.width() > 0 && resolution.height() > 0 ?
            QString::fromUtf8("%1x%2").arg(resolution.width()).arg(resolution.height()) :
            gb28181_resolution_combo_->currentText().trimmed();
    gb28181_source_summary_label_->setText(UiText(
        "Local: %1 | Upper: %2:%3/%4\nVideo: %5 | Source: %6 | Audio: %7\nEncode: %8 | %9 fps | Volume %10% | Display %11",
        "本机：%1 | 上级：%2:%3/%4\n视频：%5 | 来源：%6 | 音频：%7\n编码：%8 | %9 fps | 音量 %10% | 显示 %11")
            .arg(local_id)
            .arg(upper_ip.isEmpty() ? QString::fromUtf8("<upper sip ip>") : upper_ip)
            .arg(upper_port.isEmpty() ? QString::fromUtf8("<sip port>") : upper_port)
            .arg(upper_transport.isEmpty() ? QString::fromUtf8("UDP") : upper_transport)
            .arg(gb28181_source_combo_->currentText())
            .arg(video_source_name)
            .arg(audio_source_name)
            .arg(resolution_text.isEmpty() ? UiText("source original", "跟随来源") : resolution_text)
            .arg(ReadBoundedInt(gb28181_fps_edit_, 25, 1, 120))
            .arg(SelectedGb28181AudioVolumePercent())
            .arg(PreviewDisplayModeText()));
}

#if STREAMCORE_DEMO_ENABLE_GB28181
void StreamCoreDemoQtWindow::OnGb28181InviteReceived(
    const streamcore_gb28181_invite_t* invite,
    void* userContext)
{
    StreamCoreDemoQtWindow* window =
        static_cast<StreamCoreDemoQtWindow*>(userContext);
    char error_text[STREAMCORE_TEXT_CAPACITY] = {};

    if (window == nullptr || invite == nullptr ||
        window->active_gb28181_ == nullptr)
    {
        return;
    }
    const streamcore_result_t result = streamcore_gb28181_reply_invite_accepted(
        window->active_gb28181_,
        invite,
        error_text,
        sizeof(error_text));
    std::fprintf(
        stderr,
        "streamcore_demo gb28181 auto_accept_invite result=%d error=%s\n",
        static_cast<int>(result),
        error_text);
}

void StreamCoreDemoQtWindow::OnGb28181SessionUpdated(
    const streamcore_gb28181_session_info_t* session,
    void* userContext)
{
    StreamCoreDemoQtWindow* window =
        static_cast<StreamCoreDemoQtWindow*>(userContext);
    if (window == nullptr || session == nullptr)
    {
        return;
    }

    const QByteArray device_id =
        session->target.device_id != nullptr ?
            QByteArray(session->target.device_id) :
            QByteArray();
    const QByteArray channel_id =
        session->target.channel_id != nullptr ?
            QByteArray(session->target.channel_id) :
            QByteArray();
    const streamcore_gb28181_stream_kind_t stream_kind =
        session->target.stream_kind;
    const streamcore_gb28181_session_state_t state = session->state;

    QMetaObject::invokeMethod(
        window,
        [window, device_id, channel_id, stream_kind, state]()
        {
            streamcore_gb28181_session_info_t session_copy = {};
            session_copy.target.device_id = device_id.constData();
            session_copy.target.channel_id = channel_id.constData();
            session_copy.target.stream_kind = stream_kind;
            session_copy.state = state;
            window->StartGb28181MediaBridge(session_copy);
        },
        Qt::QueuedConnection);
}

void StreamCoreDemoQtWindow::OnGb28181MediaRequest(
    const streamcore_gb28181_media_request_t* request,
    void* userContext)
{
    StreamCoreDemoQtWindow* window =
        static_cast<StreamCoreDemoQtWindow*>(userContext);
    if (window == nullptr || request == nullptr)
    {
        return;
    }

    window->ConfigureGb28181SourceBinding(*request);
}

void StreamCoreDemoQtWindow::OnGb28181AudioFrameRef(
    streamcore_audio_frame_ref frameRef,
    void* userContext)
{
    StreamCoreDemoQtWindow* window =
        static_cast<StreamCoreDemoQtWindow*>(userContext);
    char error_text[STREAMCORE_TEXT_CAPACITY] = {};
    if (window == nullptr || window->active_gb28181_ == nullptr ||
        frameRef == nullptr ||
        window->active_gb28181_target_device_id_.isEmpty() ||
        window->active_gb28181_target_channel_id_.isEmpty())
    {
        return;
    }

    const streamcore_gb28181_stream_target_t target =
        window->CurrentGb28181Target();
    const streamcore_result_t result =
        streamcore_gb28181_push_audio_frame_ref(
            window->active_gb28181_,
            &target,
            frameRef,
            error_text,
            sizeof(error_text));
    if (result == STREAMCORE_RESULT_OK)
    {
        ++window->active_gb28181_pushed_audio_packets_;
    }
    else if (window->active_gb28181_pushed_audio_packets_.load() < 3)
    {
        std::fprintf(
            stderr,
            "streamcore_demo gb28181 audio frame push failed result=%d error=%s\n",
            static_cast<int>(result),
            error_text);
    }
}

void StreamCoreDemoQtWindow::OnGb28181VideoFrameRef(
    streamcore_video_frame_ref frameRef,
    void* userContext)
{
    StreamCoreDemoQtWindow* window =
        static_cast<StreamCoreDemoQtWindow*>(userContext);
    char error_text[STREAMCORE_TEXT_CAPACITY] = {};
    if (window == nullptr || window->active_gb28181_ == nullptr ||
        frameRef == nullptr ||
        window->active_gb28181_target_device_id_.isEmpty() ||
        window->active_gb28181_target_channel_id_.isEmpty())
    {
        return;
    }

    const streamcore_gb28181_stream_target_t target =
        window->CurrentGb28181Target();
    const streamcore_result_t result =
        streamcore_gb28181_push_video_frame_ref(
            window->active_gb28181_,
            &target,
            frameRef,
            error_text,
            sizeof(error_text));
    if (result == STREAMCORE_RESULT_OK)
    {
        ++window->active_gb28181_pushed_video_packets_;
    }
    else if (window->active_gb28181_pushed_video_packets_.load() < 3)
    {
        std::fprintf(
            stderr,
            "streamcore_demo gb28181 video frame push failed result=%d error=%s\n",
            static_cast<int>(result),
            error_text);
    }
}

void StreamCoreDemoQtWindow::StartGb28181MediaBridge(
    const streamcore_gb28181_session_info_t& session)
{
    if (session.state != STREAMCORE_GB28181_SESSION_ACTIVE ||
        active_gb28181_ == nullptr ||
        active_gb28181_media_capture_ != nullptr)
    {
        return;
    }

    if (session.target.stream_kind == STREAMCORE_GB28181_STREAM_BROADCAST)
    {
        if (gb28181_preview_label_ != nullptr)
        {
            gb28181_preview_label_->setText(UiText(
                "GB28181 broadcast session is active.\nWaiting for downlink media packets from the upper platform.",
                "GB28181 broadcast session is active.\nWaiting for downlink media packets from the upper platform."));
        }
        return;
    }

    active_gb28181_target_device_id_ =
        session.target.device_id != nullptr ?
            QByteArray(session.target.device_id) :
            QByteArray();
    active_gb28181_target_channel_id_ =
        session.target.channel_id != nullptr ?
            QByteArray(session.target.channel_id) :
            QByteArray();
    active_gb28181_target_stream_kind_ = session.target.stream_kind;
    active_gb28181_pushed_audio_packets_.store(0);
    active_gb28181_pushed_video_packets_.store(0);

    if (active_gb28181_target_device_id_.isEmpty() ||
        active_gb28181_target_channel_id_.isEmpty())
    {
        return;
    }

    if (gb28181_preview_label_ != nullptr)
    {
        gb28181_preview_label_->setText(QString::fromUtf8(
            "GB28181 session is active.\n"
            "Media source binding is handled by the SDK media request callback."));
    }
}

void StreamCoreDemoQtWindow::ConfigureGb28181SourceBinding(
    const streamcore_gb28181_media_request_t& request)
{
    if (active_gb28181_ == nullptr)
    {
        return;
    }

    if (request.requires_local_source == 0)
    {
        streamcore_gb28181_clear_source_bindings(active_gb28181_);
        return;
    }

    streamcore_gb28181_source_binding_t binding = {};
    char error_text[STREAMCORE_TEXT_CAPACITY] = {};

    binding.target.device_id = request.device_id;
    binding.target.channel_id = request.channel_id;
    binding.target.stream_kind = request.stream_kind;
    binding.enable_audio = active_gb28181_audio_index_ != kGb28181AudioNone ? 1 : 0;
    binding.enable_video = 1;
    binding.allow_raw_frame_packet = 1;
    binding.width = active_gb28181_width_;
    binding.height = active_gb28181_height_;
    binding.fps = active_gb28181_fps_;
    binding.audio_sample_rate = request.audio_clock_rate > 0 ?
        request.audio_clock_rate :
        48000;
    binding.audio_channel_count = 2;

    if ((request.supported_source_mask &
            STREAMCORE_GB28181_MEDIA_SOURCE_MASK_LOCAL_DEVICE) == 0)
    {
        std::fprintf(
            stderr,
            "streamcore_demo gb28181 local-device binding unsupported for request=%s\n",
            request.request_key);
        return;
    }

    binding.source_kind = STREAMCORE_GB28181_MEDIA_SOURCE_LOCAL_DEVICE;
    binding.capture_source_kind =
        active_gb28181_source_index_ == kGb28181SourceDesktop ?
            STREAMCORE_CAPTURE_SOURCE_KIND_DESKTOP :
            STREAMCORE_CAPTURE_SOURCE_KIND_CAMERA;
    binding.audio_capture_source_kind =
        active_gb28181_audio_index_ == kGb28181AudioSystem ?
            STREAMCORE_CAPTURE_SOURCE_KIND_SYSTEM_AUDIO :
            STREAMCORE_CAPTURE_SOURCE_KIND_MICROPHONE;
    binding.source_id = active_gb28181_source_id_.isEmpty() ?
        nullptr :
        active_gb28181_source_id_.constData();
    if (binding.enable_audio != 0)
    {
        binding.audio_source_id = active_gb28181_audio_source_id_.isEmpty() ?
            nullptr :
            active_gb28181_audio_source_id_.constData();
    }

    const streamcore_result_t result = streamcore_gb28181_set_source_bindings(
        active_gb28181_,
        &binding,
        1,
        error_text,
        sizeof(error_text));
    std::fprintf(
        stderr,
        "streamcore_demo gb28181 source binding result=%d request=%s error=%s\n",
        static_cast<int>(result),
        request.request_key,
        error_text);
}

void StreamCoreDemoQtWindow::StopGb28181MediaBridge()
{
    if (active_gb28181_media_capture_ != nullptr)
    {
        streamcore_capture_stop(active_gb28181_media_capture_);
        streamcore_capture_destroy(active_gb28181_media_capture_);
        active_gb28181_media_capture_ = nullptr;
    }
    active_gb28181_target_device_id_.clear();
    active_gb28181_target_channel_id_.clear();
    active_gb28181_target_stream_kind_ = STREAMCORE_GB28181_STREAM_LIVE;
    active_gb28181_pushed_audio_packets_.store(0);
    active_gb28181_pushed_video_packets_.store(0);
}

streamcore_gb28181_stream_target_t StreamCoreDemoQtWindow::CurrentGb28181Target() const
{
    streamcore_gb28181_stream_target_t target = {};
    target.device_id = active_gb28181_target_device_id_.constData();
    target.channel_id = active_gb28181_target_channel_id_.constData();
    target.stream_kind = active_gb28181_target_stream_kind_;
    return target;
}
#endif

void StreamCoreDemoQtWindow::StartGb28181()
{
    if (gb28181_preview_label_ == nullptr ||
        gb28181_upper_ip_edit_ == nullptr ||
        gb28181_upper_port_edit_ == nullptr ||
        gb28181_upper_transport_combo_ == nullptr)
    {
        return;
    }

    StopGb28181();

#if STREAMCORE_DEMO_ENABLE_GB28181
    QByteArray session_name("qt_desktop_gb28181_device");
    QByteArray upper_ip = gb28181_upper_ip_edit_->text().trimmed().toUtf8();
    const int upper_port =
        ReadBoundedInt(gb28181_upper_port_edit_, 5060, 1, 65535);
    const bool upper_transport_tcp =
        gb28181_upper_transport_combo_->currentData().toString().trimmed().toLower() ==
        QString::fromUtf8("tcp");
    const streamcore_gb28181_transport_mode_t sip_transport =
        upper_transport_tcp ? STREAMCORE_GB28181_TRANSPORT_TCP :
        STREAMCORE_GB28181_TRANSPORT_UDP;
    char error_text[STREAMCORE_TEXT_CAPACITY] = {};
    streamcore_gb28181_config_t config;
    streamcore_gb28181_device_info_t device_info = {};
    streamcore_gb28181_device_status_t device_status = {};
    streamcore_gb28181_catalog_item_t catalog_item = {};
    streamcore_gb28181_runtime_info_t runtime_info = {};
    streamcore_result_t result = STREAMCORE_RESULT_OK;
    streamcore_result_t register_result = STREAMCORE_RESULT_OPERATION_FAILED;
    streamcore_gb28181_handle handle = nullptr;
    const QByteArray local_id =
        ReadText(gb28181_local_id_edit_, "34020000001320000001").toUtf8();
    const QByteArray channel_id = local_id;
    const QByteArray local_domain =
        ReadText(gb28181_local_domain_edit_, "3402000000").toUtf8();
    const QByteArray local_password =
        ReadText(gb28181_upper_password_edit_, "123456").toUtf8();
    const QByteArray local_display_name("StreamCore SDK Demo Device");
    const QByteArray local_ip("0.0.0.0");
    const int local_port = ReadBoundedInt(gb28181_local_port_edit_, 5060, 1, 65535);
    const QByteArray upper_id =
        ReadText(gb28181_upper_id_edit_, "34020000002000000001").toUtf8();
    const QByteArray upper_domain =
        ReadText(gb28181_upper_domain_edit_, "3402000000").toUtf8();
    const QByteArray upper_password = local_password;
    const QByteArray upper_display_name;
    const QByteArray media_ip("0.0.0.0");
    const int media_port = ReadBoundedInt(gb28181_media_port_edit_, 19000, 1, 65535);
    const QSize selected_resolution =
        gb28181_resolution_combo_ != nullptr ?
            gb28181_resolution_combo_->currentData().toSize() :
            QSize(1280, 720);
    active_gb28181_source_index_ =
        ComboValueOrIndex(gb28181_source_combo_, kGb28181SourceCamera);
    active_gb28181_audio_index_ =
        ComboValueOrIndex(gb28181_audio_combo_, kGb28181AudioMicrophone);
    active_gb28181_width_ = selected_resolution.width();
    active_gb28181_height_ = selected_resolution.height();
    active_gb28181_fps_ = ReadBoundedInt(gb28181_fps_edit_, 25, 1, 120);
    active_gb28181_source_id_ =
        gb28181_video_source_combo_ != nullptr ?
            gb28181_video_source_combo_->currentData().toString().toUtf8() :
            QByteArray();
    active_gb28181_audio_source_id_ =
        (gb28181_audio_source_combo_ != nullptr &&
            active_gb28181_audio_index_ != kGb28181AudioNone) ?
                gb28181_audio_source_combo_->currentData().toString().toUtf8() :
                QByteArray();
    active_gb28181_source_binding_.clear();

    if (upper_ip.isEmpty())
    {
        gb28181_preview_label_->setText(UiText(
            "Enter the upper-platform SIP IP before starting GB28181.",
            "Enter the upper-platform SIP IP before starting GB28181."));
        SetOperationStatus(
            QString::fromUtf8("gb28181.start"),
            QString::fromUtf8("-1"),
            QString::fromUtf8("invalid_argument"),
            UiText("Enter the upper-platform SIP IP.", "Enter the upper-platform SIP IP."));
        UpdateGb28181Buttons();
        return;
    }

    handle = streamcore_gb28181_create();
    if (handle == nullptr)
    {
        gb28181_preview_label_->setText(UiText(
            "GB28181 handle allocation failed.",
            "GB28181 handle allocation failed."));
        SetOperationStatus(
            QString::fromUtf8("gb28181.start"),
            QString::fromUtf8("-1"),
            QString::fromUtf8("failed"),
            UiText("GB28181 handle allocation failed.", "GB28181 handle allocation failed."));
        UpdateGb28181Buttons();
        return;
    }

    streamcore_gb28181_get_default_config(&config);
    {
        streamcore_gb28181_callbacks_t callbacks = {};
        callbacks.invite_received_callback = OnGb28181InviteReceived;
        callbacks.session_updated_callback = OnGb28181SessionUpdated;
        callbacks.media_request_callback = OnGb28181MediaRequest;
        callbacks.user_context = this;
        result = streamcore_gb28181_set_callbacks(handle, &callbacks);
    }
    config.session_name = session_name.constData();
    config.local_identity.id = local_id.constData();
    config.local_identity.domain = local_domain.constData();
    config.local_identity.password = local_password.constData();
    config.local_identity.display_name = local_display_name.constData();
    config.upper_platform_identity.id = upper_id.constData();
    config.upper_platform_identity.domain = upper_domain.constData();
    config.upper_platform_identity.password = upper_password.constData();
    config.upper_platform_identity.display_name = upper_display_name.constData();
    config.local_endpoint.ip = local_ip.constData();
    config.local_endpoint.port = local_port;
    config.local_endpoint.transport = sip_transport;
    config.upper_platform_endpoint.ip = upper_ip.constData();
    config.upper_platform_endpoint.port = upper_port;
    config.upper_platform_endpoint.transport = sip_transport;
    config.enable_digest_auth = 1;
    config.auto_reply_catalog = 1;
    config.auto_reply_device_info = 1;
    config.auto_reply_device_status = 1;
    config.default_answer.session_name = local_display_name.constData();
    config.default_answer.media_endpoint.ip = media_ip.constData();
    config.default_answer.media_endpoint.port = media_port;
    config.default_answer.media_endpoint.transport = sip_transport;
    config.default_answer.media_direction = "sendonly";

    device_info.device_name = local_display_name.constData();
    device_info.manufacturer = "HBR";
    device_info.model = "StreamCoreDemo";
    device_info.firmware = "0.1.0";
    device_status.status_text = "OK";
    device_status.is_online = 1;
    device_status.is_recording = 0;
    catalog_item.channel_id = channel_id.constData();
    catalog_item.name = local_display_name.constData();
    catalog_item.parent_id = local_id.constData();
    catalog_item.manufacturer = "HBR";
    catalog_item.model = "StreamCoreDemo";
    catalog_item.owner = "streamcore_demo";
    catalog_item.civil_code = "340200";
    catalog_item.address = "local demo";
    catalog_item.parental = 0;
    catalog_item.is_online = 1;
    catalog_item.status_text = "ON";

    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_gb28181_set_config(handle, &config);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_gb28181_set_device_info(handle, &device_info);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_gb28181_set_device_status(handle, &device_status);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_gb28181_set_catalog(handle, &catalog_item, 1);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_gb28181_start(
            handle,
            error_text,
            sizeof(error_text));
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        active_gb28181_ = handle;
        register_result = streamcore_gb28181_register(
            handle,
            error_text,
            sizeof(error_text));
        if (gb28181_poll_timer_ != nullptr)
        {
            gb28181_poll_timer_->start();
        }
        streamcore_gb28181_get_runtime_info(active_gb28181_, &runtime_info);
    }

    if (active_gb28181_ != nullptr)
    {
        SetOperationStatus(
            QString::fromUtf8("gb28181.start"),
            QString::number(register_result),
            register_result == STREAMCORE_RESULT_OK ?
                QString::fromUtf8("ok") :
                QString::fromUtf8("registered_with_warning"),
            ToQString(runtime_info.state_summary));
        gb28181_preview_label_->setText(UiText(
            "GB28181 runtime started.\nREGISTER result=%1 | registered=%2 | sessions=%3\n%4",
            "GB28181 运行时已启动。\nREGISTER result=%1 | registered=%2 | sessions=%3\n%4")
                .arg(register_result)
                .arg(runtime_info.is_registered)
                .arg(runtime_info.active_session_count)
                .arg(ToQString(runtime_info.state_summary)));
    }
    else
    {
        streamcore_gb28181_destroy(handle);
        SetOperationStatus(
            QString::fromUtf8("gb28181.start"),
            QString::number(result),
            QString::fromUtf8("failed"),
            ToQString(error_text));
        gb28181_preview_label_->setText(UiText(
            "GB28181 start failed.\nresult=%1\n%2",
            "GB28181 启动失败。\nresult=%1\n%2")
                .arg(result)
                .arg(ToQString(error_text)));
    }
#else
    gb28181_preview_label_->setText(UiText(
        "GB28181 addon is not built in this desktop demo.",
        "当前桌面 Demo 未包含 GB28181 扩展模块。"));
#endif

    UpdateGb28181Buttons();
}

void StreamCoreDemoQtWindow::StopGb28181()
{
#if STREAMCORE_DEMO_ENABLE_GB28181
    if (gb28181_poll_timer_ != nullptr)
    {
        gb28181_poll_timer_->stop();
    }
    StopGb28181MediaBridge();
    active_gb28181_source_binding_.clear();
    active_gb28181_source_index_ = kGb28181SourceCamera;
    active_gb28181_audio_index_ = kGb28181AudioMicrophone;
    active_gb28181_width_ = 1280;
    active_gb28181_height_ = 720;
    active_gb28181_fps_ = 25;
    active_gb28181_source_id_.clear();
    active_gb28181_audio_source_id_.clear();
    if (active_gb28181_ == nullptr)
    {
        UpdateGb28181Buttons();
        return;
    }

    streamcore_gb28181_stop(active_gb28181_);
    streamcore_gb28181_destroy(active_gb28181_);
    active_gb28181_ = nullptr;
    if (gb28181_preview_label_ != nullptr)
    {
        gb28181_preview_label_->setText(UiText(
            "GB28181 runtime stopped.",
            "GB28181 runtime stopped."));
    }
    SetOperationStatus(
        QString::fromUtf8("gb28181.stop"),
        QString::fromUtf8("0"),
        QString::fromUtf8("ok"),
        UiText("GB28181 runtime stopped.", "GB28181 runtime stopped."));
#endif
    UpdateGb28181Buttons();
}

void StreamCoreDemoQtWindow::PollGb28181()
{
#if STREAMCORE_DEMO_ENABLE_GB28181
    if (active_gb28181_ == nullptr || gb28181_preview_label_ == nullptr)
    {
        return;
    }

    const streamcore_result_t poll_result =
        streamcore_gb28181_poll(active_gb28181_);
    streamcore_gb28181_runtime_info_t runtime_info = {};
    const streamcore_result_t runtime_result =
        streamcore_gb28181_get_runtime_info(active_gb28181_, &runtime_info);
    gb28181_preview_label_->setText(
        QString::fromUtf8(
            "GB28181 running.\n"
            "poll=%1 runtime=%2 registered=%3 sessions=%4 "
            "bindings S/K=%5/%6 bytes S/R=%7/%8\n%9")
            .arg(poll_result)
            .arg(runtime_result)
            .arg(runtime_info.is_registered)
            .arg(runtime_info.active_session_count)
            .arg(runtime_info.active_source_binding_count)
            .arg(runtime_info.active_sink_binding_count)
            .arg(static_cast<qulonglong>(runtime_info.sent_byte_count))
            .arg(static_cast<qulonglong>(runtime_info.received_byte_count))
            .arg(ToQString(runtime_info.media_state_summary[0] != '\0' ?
                    runtime_info.media_state_summary :
                    runtime_info.state_summary)));
    return;
    gb28181_preview_label_->setText(UiText(
        "GB28181 running.\npoll=%1 runtime=%2 registered=%3 sessions=%4 bindings S/K=%5/%6 bytes S/R=%7/%8\n%9",
        "GB28181 运行中。\npoll=%1 runtime=%2 registered=%3 active sessions=%4 media A/V=%5/%6\n%7")
            .arg(poll_result)
            .arg(runtime_result)
            .arg(runtime_info.is_registered)
            .arg(runtime_info.active_session_count)
            .arg(runtime_info.active_source_binding_count)
            .arg(runtime_info.active_sink_binding_count)
            .arg(static_cast<qulonglong>(runtime_info.sent_byte_count))
            .arg(static_cast<qulonglong>(runtime_info.received_byte_count))
            .arg(ToQString(runtime_info.media_state_summary[0] != '\0' ?
                    runtime_info.media_state_summary :
                    runtime_info.state_summary)));
#endif
}

void StreamCoreDemoQtWindow::UpdateGb28181Buttons()
{
    const bool running =
#if STREAMCORE_DEMO_ENABLE_GB28181
        active_gb28181_ != nullptr;
#else
        false;
#endif
    if (gb28181_start_button_ != nullptr)
    {
        gb28181_start_button_->setEnabled(true);
        gb28181_start_button_->setText(running ?
            UiText("Stop GB28181", "停止 GB28181") :
            UiText("Start", "开始"));
    }
}

void StreamCoreDemoQtWindow::BindDesktopRenderTarget()
{
    if (desktop_render_target_status_label_ == nullptr ||
        desktop_render_target_widget_ == nullptr)
    {
        return;
    }
    ApplyPreviewDisplayMode();

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    if (active_player_ != nullptr)
    {
        desktop_render_target_status_label_->setText(UiText(
            "Playing · stream resolution/bitrate pending · %1",
            "播放中 · 码流分辨率/码率待统计 · %1")
                .arg(PlayerPlaybackModeSummary()));
        return;
    }

    streamcore_player_handle player = streamcore_player_create();
    streamcore_player_config_t config;
    streamcore_render_target_t render_target = {};
    streamcore_player_preflight_t preflight = {};
    char error_text[STREAMCORE_TEXT_CAPACITY] = {};
    streamcore_result_t config_result = STREAMCORE_RESULT_OPERATION_FAILED;
    streamcore_result_t target_result = STREAMCORE_RESULT_OPERATION_FAILED;
    streamcore_result_t preflight_result = STREAMCORE_RESULT_OPERATION_FAILED;
    const WId window_id = desktop_render_target_widget_->winId();
    void* native_handle = reinterpret_cast<void*>(window_id);
#if defined(Q_OS_WIN)
    const streamcore_render_platform_t render_platform = STREAMCORE_RENDER_PLATFORM_WINDOWS;
#elif defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    const streamcore_render_platform_t render_platform = STREAMCORE_RENDER_PLATFORM_APPLE;
#else
    const streamcore_render_platform_t render_platform = STREAMCORE_RENDER_PLATFORM_LINUX;
#endif
    const QByteArray player_url =
        player_url_edit_ != nullptr ?
            player_url_edit_->text().trimmed().toUtf8() :
            QByteArray("rtmp://192.0.2.1:1935/live/local_native");

    if (player == nullptr || native_handle == nullptr)
    {
        desktop_render_target_status_label_->setText(UiText(
            "Playback surface unavailable.",
            "Playback surface unavailable."));
        if (player != nullptr)
        {
            streamcore_player_destroy(player);
        }
        return;
    }

    streamcore_player_get_default_config(&config);
    config.session_name = "qt_desktop_render_target_probe";
    config.source_kind = STREAMCORE_PLAYER_SOURCE_KIND_URL;
    config.source_url = player_url.constData();
    config.render_target_id = "desktop_render_target_widget";
    const QString player_audio_env =
        EnvironmentText("STREAMCORE_DEMO_QT_PLAYER_AUDIO");
    config.enable_audio = player_audio_env.isEmpty() ?
        1 :
        (EnvironmentFlag("STREAMCORE_DEMO_QT_PLAYER_AUDIO", true) ? 1 : 0);
    config.audio_volume_percent = SelectedPlayerAudioVolumePercent();
    config.enable_video = 1;
    config.requested_width = desktop_render_target_widget_->width();
    config.requested_height = desktop_render_target_widget_->height();
    config.use_hardware_decode = SelectedPlayerHardwareDecode() ? 1 : 0;
    config.video_present_path = SelectedPlayerPresentPath();
    config.prefer_software_render_backend =
        SelectedPlayerPrefersSoftwareRenderBackend() ? 1 : 0;
    config.allow_video_soft_fallback = 1;
    config.audio_cache_milliseconds =
        ReadBoundedInt(player_buffer_ms_edit_, 300, -1, 10000);
    config.video_cache_milliseconds =
        ReadBoundedInt(player_buffer_ms_edit_, 300, -1, 10000);
    config.audio_max_queue_size =
        ReadBoundedInt(player_audio_queue_edit_, 24, 0, 400);
    config.video_max_queue_size =
        ReadBoundedInt(player_video_queue_edit_, 12, 0, 200);
    config.enable_realtime_profile =
        config.video_cache_milliseconds == 0 ? 1 : 0;
    config.realtime_audio_max_queue_size = config.audio_max_queue_size;
    config.realtime_video_max_queue_size = config.video_max_queue_size;

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    render_target.target_type = STREAMCORE_RENDER_TARGET_TYPE_LAYER;
#else
    render_target.target_type = STREAMCORE_RENDER_TARGET_TYPE_NATIVE_WINDOW;
#endif
    render_target.platform_type = render_platform;
    render_target.native_handle = native_handle;
    render_target.width_hint = desktop_render_target_widget_->width();
    render_target.height_hint = desktop_render_target_widget_->height();

    config_result = streamcore_player_set_config(player, &config);
    if (config_result == STREAMCORE_RESULT_OK)
    {
        target_result = streamcore_player_set_render_target(player, &render_target);
    }
    if (target_result == STREAMCORE_RESULT_OK)
    {
        preflight_result = streamcore_player_preflight(
            player,
            &preflight,
            error_text,
            sizeof(error_text));
    }
    if (preflight_result == STREAMCORE_RESULT_OK)
    {
        desktop_render_target_status_label_->setText(UiText(
            "Ready · view %1x%2 · stream pending · %3",
            "就绪 · 视图 %1x%2 · 码流待统计 · %3")
                .arg(desktop_render_target_widget_->width())
                .arg(desktop_render_target_widget_->height())
                .arg(PlayerPlaybackModeSummary()));
    }
    else
    {
        desktop_render_target_status_label_->setText(UiText(
            "Playback setup needs attention.",
            "Playback setup needs attention."));
        statusBar()->showMessage(
            QString::fromUtf8("player config=%1 set=%2 preflight=%3 error=%4")
                .arg(config_result)
                .arg(target_result)
                .arg(preflight_result)
                .arg(ToQString(error_text)),
            5000);
    }

    streamcore_player_destroy(player);
#else
    desktop_render_target_status_label_->setText(UiText(
        "Desktop playback is not enabled for this platform build.",
        "Desktop playback is not enabled for this platform build."));
#endif
}

void StreamCoreDemoQtWindow::ScheduleAutorunIfRequested()
{
    const QString autorun_mode =
        EnvironmentText("STREAMCORE_DEMO_QT_AUTORUN").toLower();
    if (autorun_mode != QString::fromUtf8("player") &&
        autorun_mode != QString::fromUtf8("media_e2e") &&
        autorun_mode != QString::fromUtf8("diagnostics") &&
        autorun_mode != QString::fromUtf8("logs") &&
        autorun_mode != QString::fromUtf8("upload") &&
        autorun_mode != QString::fromUtf8("permissions") &&
        autorun_mode != QString::fromUtf8("publisher") &&
        autorun_mode != QString::fromUtf8("onvif") &&
        autorun_mode != QString::fromUtf8("gb28181"))
    {
        return;
    }

    QTimer::singleShot(1200, this, [this]() {
        const QString mode =
            EnvironmentText("STREAMCORE_DEMO_QT_AUTORUN").toLower();
        const QString display_mode =
            EnvironmentText("STREAMCORE_DEMO_QT_DISPLAY_MODE").toLower();
        if (preview_display_mode_combo_ != nullptr)
        {
            if (display_mode == QString::fromUtf8("stretch"))
            {
                preview_display_mode_combo_->setCurrentIndex(0);
            }
            else if (display_mode == QString::fromUtf8("crop"))
            {
                preview_display_mode_combo_->setCurrentIndex(2);
            }
            else if (display_mode == QString::fromUtf8("aspect") ||
                display_mode == QString::fromUtf8("fit"))
            {
                preview_display_mode_combo_->setCurrentIndex(1);
            }
        }

        QTabWidget* feature_tabs =
            findChild<QTabWidget*>(QString::fromUtf8("feature_tabs"));
        const int quit_after_ms =
            EnvironmentInt("STREAMCORE_DEMO_QT_AUTORUN_QUIT_MS", 0);

        if (mode == QString::fromUtf8("diagnostics") ||
            mode == QString::fromUtf8("logs"))
        {
            if (feature_tabs != nullptr)
            {
                feature_tabs->setCurrentIndex(3);
            }
            const QString zip_path =
                EnvironmentText("STREAMCORE_DEMO_QT_AUTORUN_LOG_ZIP_PATH");
            if (!zip_path.isEmpty())
            {
                const QString error_message = CreateLogPackageZip(zip_path);
                if (error_message.isEmpty())
                {
                    latest_log_zip_path_ = QFileInfo(zip_path).absoluteFilePath();
                    SetOperationStatus(
                        QString::fromUtf8("qt.autorun.logs"),
                        QString::fromUtf8("0"),
                        QString::fromUtf8("ready"),
                        QString::fromUtf8("Autorun log package is ready."));
                }
                else
                {
                    SetOperationStatus(
                        QString::fromUtf8("qt.autorun.logs"),
                        QString::fromUtf8("-1"),
                        QString::fromUtf8("failed"),
                        error_message);
                }
            }
            else
            {
                SetOperationStatus(
                    QString::fromUtf8("qt.autorun.diagnostics"),
                    QString::fromUtf8("0"),
                    QString::fromUtf8("ready"),
                    UiText(
                        "License, capability, and log diagnostics loaded.",
                        "授权、能力和日志诊断已加载。"));
            }

            if (quit_after_ms > 0)
            {
                close();
                QCoreApplication::exit(0);
                QCoreApplication::quit();
                std::_Exit(0);
            }
            return;
        }

        if (mode == QString::fromUtf8("upload"))
        {
            if (feature_tabs != nullptr)
            {
                feature_tabs->setCurrentIndex(3);
            }
            ShowUploadReserved();
            if (quit_after_ms > 0)
            {
                close();
                QCoreApplication::exit(0);
                QCoreApplication::quit();
                std::_Exit(0);
            }
            return;
        }

        if (mode == QString::fromUtf8("permissions"))
        {
            if (feature_tabs != nullptr)
            {
                feature_tabs->setCurrentIndex(0);
            }
            QString permission_error;
            bool ready = true;
#if defined(Q_OS_MACOS)
            ready = false;
            if (EnvironmentText("STREAMCORE_DEMO_QT_AUTORUN").isEmpty())
            {
                permission_error = UiText(
                    "Launch the macOS app through LaunchServices with "
                    "--args --autorun permissions before requesting TCC.",
                    "Launch the macOS app through LaunchServices with --args --autorun permissions before requesting TCC.");

            }
            else
#endif
            {
#if defined(Q_OS_MACOS)
                char permission_message[512] = {};
                ready = StreamCoreDemoQtRequestMacCapturePermissions(
                    true,
                    true,
                    true,
                    60000,
                    permission_message,
                    sizeof(permission_message));
                if (!ready)
                {
                    permission_error = ToQString(permission_message);
                }
#else
                ready = EnsurePublisherCapturePermissions(
                    kPublisherSourceCamera,
                    kPublisherAudioMicrophone,
                    &permission_error);
#endif
            }
            SetOperationStatus(
                QString::fromUtf8("qt.autorun.permissions"),
                ready ? QString::fromUtf8("0") : QString::fromUtf8("-1"),
                ready ? QString::fromUtf8("ready") : QString::fromUtf8("failed"),
                ready ?
                    UiText("macOS capture permissions are ready.",
                        "macOS capture permissions are ready.") :
                    permission_error);
            if (quit_after_ms > 0)
            {
                close();
                QCoreApplication::exit(ready ? 0 : 1);
                QCoreApplication::quit();
                std::_Exit(ready ? 0 : 1);
            }
            return;
        }

        if (mode == QString::fromUtf8("publisher"))
        {
            if (feature_tabs != nullptr)
            {
                feature_tabs->setCurrentIndex(0);
            }
            const QString source =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_SOURCE").toLower();
            if (publisher_source_combo_ != nullptr)
            {
                if (source == QString::fromUtf8("desktop"))
                {
                    publisher_source_combo_->setCurrentIndex(1);
                }
                else if (source == QString::fromUtf8("media") ||
                    source == QString::fromUtf8("video"))
                {
                    publisher_source_combo_->setCurrentIndex(2);
                }
                else if (source == QString::fromUtf8("audio"))
                {
                    publisher_source_combo_->setCurrentIndex(4);
                    if (publisher_audio_combo_ != nullptr)
                    {
                        publisher_audio_combo_->setCurrentIndex(3);
                    }
                }
                else if (source == QString::fromUtf8("image") ||
                    source == QString::fromUtf8("still"))
                {
                    publisher_source_combo_->setCurrentIndex(3);
                }
                else if (source == QString::fromUtf8("none"))
                {
                    publisher_source_combo_->setCurrentIndex(4);
                }
                else
                {
                    publisher_source_combo_->setCurrentIndex(0);
                }
                RefreshPublisherCameraDevices();
                RefreshPublisherResolutionOptions();
            }
            const QString media_path =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_FILE");
            if (!media_path.isEmpty() && publisher_file_path_edit_ != nullptr)
            {
                publisher_file_path_edit_->setText(media_path);
            }
            const QString file_mode =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_FILE_MODE");
            const QString normalized_file_mode = file_mode.toLower();
            if (normalized_file_mode == QString::fromUtf8("force") ||
                normalized_file_mode == QString::fromUtf8("transcode"))
            {
                publisher_file_mode_combo_->setCurrentIndex(1);
            }
            else if (normalized_file_mode == QString::fromUtf8("auto") ||
                normalized_file_mode == QString::fromUtf8("passthrough"))
            {
                publisher_file_mode_combo_->setCurrentIndex(0);
            }
            else if (!file_mode.isEmpty())
            {
                SelectComboByDataOrText(publisher_file_mode_combo_, file_mode);
            }
            const QString camera_id =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_CAMERA_ID");
            if (!camera_id.isEmpty() && publisher_camera_device_combo_ != nullptr)
            {
                const int camera_index =
                    publisher_camera_device_combo_->findData(camera_id);
                if (camera_index >= 0)
                {
                    publisher_camera_device_combo_->setCurrentIndex(camera_index);
                }
            }
            const QString audio_path =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_AUDIO_FILE");
            if (!audio_path.isEmpty() && publisher_audio_file_path_edit_ != nullptr)
            {
                publisher_audio_file_path_edit_->setText(audio_path);
            }
            const QString publish_url =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_URL");
            if (!publish_url.isEmpty() && publisher_url_edit_ != nullptr)
            {
                publisher_url_edit_->setText(publish_url);
                publisher_url_edit_->setCursorPosition(0);
            }
            const QString whip_bearer_token =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_WHIP_BEARER_TOKEN");
            if (!whip_bearer_token.isEmpty() &&
                publisher_whip_bearer_token_edit_ != nullptr)
            {
                publisher_whip_bearer_token_edit_->setText(whip_bearer_token);
            }
            RefreshPublisherResolutionOptions();
            SelectComboByDataOrText(
                publisher_video_codec_combo_,
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_VIDEO_CODEC"));
            SelectComboByDataOrText(
                publisher_audio_codec_combo_,
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_AUDIO_CODEC"));
            const QString rtmp_hevc_mode =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_RTMP_HEVC_MODE")
                    .trimmed()
                    .toLower();
            if (!rtmp_hevc_mode.isEmpty() &&
                publisher_rtmp_hevc_combo_ != nullptr)
            {
                if (rtmp_hevc_mode == QString::fromUtf8("auto") ||
                    rtmp_hevc_mode == QString::fromUtf8("compat") ||
                    rtmp_hevc_mode == QString::fromUtf8("auto_compat"))
                {
                    publisher_rtmp_hevc_combo_->setCurrentIndex(0);
                }
                else if (rtmp_hevc_mode == QString::fromUtf8("legacy") ||
                    rtmp_hevc_mode == QString::fromUtf8("legacy_flv") ||
                    rtmp_hevc_mode == QString::fromUtf8("flv"))
                {
                    publisher_rtmp_hevc_combo_->setCurrentIndex(1);
                }
                else if (rtmp_hevc_mode == QString::fromUtf8("enhanced") ||
                    rtmp_hevc_mode == QString::fromUtf8("enhanced_rtmp"))
                {
                    publisher_rtmp_hevc_combo_->setCurrentIndex(2);
                }
                else
                {
                    SelectComboByDataOrText(
                        publisher_rtmp_hevc_combo_,
                        rtmp_hevc_mode);
                }
            }
            SelectComboByDataOrText(
                publisher_resolution_combo_,
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_RESOLUTION"));
            const QString bitrate =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_VIDEO_BITRATE");
            if (!bitrate.isEmpty() && publisher_video_bitrate_edit_ != nullptr)
            {
                publisher_video_bitrate_edit_->setText(bitrate);
            }
            const QString fps = EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_FPS");
            if (!fps.isEmpty() && publisher_fps_edit_ != nullptr)
            {
                publisher_fps_edit_->setText(fps);
            }
            const QString gop = EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_GOP");
            if (!gop.isEmpty() && publisher_gop_edit_ != nullptr)
            {
                publisher_gop_edit_->setText(gop);
            }
            const QString audio =
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_AUDIO").toLower();
            if (publisher_audio_combo_ != nullptr)
            {
                if (audio == QString::fromUtf8("none"))
                {
                    publisher_audio_combo_->setCurrentIndex(0);
                }
                else if (audio == QString::fromUtf8("microphone"))
                {
                    publisher_audio_combo_->setCurrentIndex(1);
                }
                else if (audio == QString::fromUtf8("system"))
                {
                    publisher_audio_combo_->setCurrentIndex(2);
                }
                else if (audio == QString::fromUtf8("file") ||
                    audio == QString::fromUtf8("audio-only"))
                {
                    publisher_audio_combo_->setCurrentIndex(3);
                }
            }
            if (publisher_audio_volume_slider_ != nullptr)
            {
                publisher_audio_volume_slider_->setValue(std::clamp(
                    EnvironmentInt(
                        "STREAMCORE_DEMO_QT_PUBLISHER_AUDIO_VOLUME",
                        publisher_audio_volume_slider_->value()),
                    0,
                    100));
            }
            // Source/audio selection refreshes dependent controls. Re-apply the
            // scripted codec choices last so autorun verifies the same invalid
            // codec path an interactive user can select.
            SelectComboByDataOrText(
                publisher_video_codec_combo_,
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_VIDEO_CODEC"));
            SelectComboByDataOrText(
                publisher_audio_codec_combo_,
                EnvironmentText("STREAMCORE_DEMO_QT_PUBLISHER_AUDIO_CODEC"));
            if (quit_after_ms > 0)
            {
                ScheduleAutorunQuit(quit_after_ms, mode);
            }
            SetOperationStatus(
                QString::fromUtf8("qt.autorun.publisher"),
                QString::fromUtf8("0"),
                QString::fromUtf8("starting"),
                QString::fromUtf8("Starting Qt publisher autorun path."));
            StartPublisher();
            if (quit_after_ms > 0 && active_publisher_ == nullptr)
            {
                ScheduleAutorunQuit(1000, mode);
            }
            return;
        }

        if (mode == QString::fromUtf8("gb28181"))
        {
            if (feature_tabs != nullptr)
            {
                feature_tabs->setCurrentIndex(2);
            }
            const QString upper_ip =
                EnvironmentText("STREAMCORE_DEMO_QT_GB28181_UPPER_IP");
            const QString upper_port =
                EnvironmentText("STREAMCORE_DEMO_QT_GB28181_UPPER_PORT");
            const QString upper_transport =
                EnvironmentText("STREAMCORE_DEMO_QT_GB28181_TRANSPORT");
            const QString gb_source =
                EnvironmentText("STREAMCORE_DEMO_QT_GB28181_SOURCE").toLower();
            const QString gb_media_file =
                EnvironmentText("STREAMCORE_DEMO_QT_GB28181_MEDIA_FILE");
            if (!upper_ip.isEmpty() && gb28181_upper_ip_edit_ != nullptr)
            {
                gb28181_upper_ip_edit_->setText(upper_ip);
            }
            if (!upper_port.isEmpty() && gb28181_upper_port_edit_ != nullptr)
            {
                gb28181_upper_port_edit_->setText(upper_port);
            }
            if (!upper_transport.isEmpty() && gb28181_upper_transport_combo_ != nullptr)
            {
                SelectComboByDataOrText(gb28181_upper_transport_combo_, upper_transport);
            }
            if (!gb_source.isEmpty() && gb28181_source_combo_ != nullptr)
            {
                if (gb_source.contains(QString::fromUtf8("desktop")) ||
                    gb_source.contains(QString::fromUtf8("screen")))
                {
                    gb28181_source_combo_->setCurrentIndex(
                        kGb28181SourceDesktop);
                }
                else
                {
                    gb28181_source_combo_->setCurrentIndex(
                        kGb28181SourceCamera);
                }
            }
            if (!gb_media_file.isEmpty())
            {
                SetOperationStatus(
                    QString::fromUtf8("qt.autorun.gb28181"),
                    QString::fromUtf8("-"),
                    QString::fromUtf8("ignored"),
                    QString::fromUtf8(
                        "STREAMCORE_DEMO_QT_GB28181_MEDIA_FILE is ignored because the GB28181 page now accepts only local camera or desktop sources."));
            }
            SetOperationStatus(
                QString::fromUtf8("qt.autorun.gb28181"),
                QString::fromUtf8("0"),
                QString::fromUtf8("starting"),
                QString::fromUtf8("Starting Qt GB28181 autorun path."));
            StartGb28181();
            if (quit_after_ms > 0)
            {
                ScheduleAutorunQuit(quit_after_ms, mode);
            }
            return;
        }

        if (mode == QString::fromUtf8("onvif"))
        {
            if (feature_tabs != nullptr)
            {
                feature_tabs->setCurrentIndex(1);
            }
            QGroupBox* player_onvif_box =
                findChild<QGroupBox*>(QString::fromUtf8("player_onvif_box"));
            QWidget* player_onvif_controls =
                findChild<QWidget*>(QString::fromUtf8("player_onvif_controls"));
            if (player_onvif_box != nullptr)
            {
                player_onvif_box->setChecked(true);
            }
            if (player_onvif_controls != nullptr)
            {
                player_onvif_controls->setVisible(true);
            }

            const QString onvif_username =
                EnvironmentText("STREAMCORE_DEMO_QT_ONVIF_USERNAME");
            const QString onvif_password =
                EnvironmentText("STREAMCORE_DEMO_QT_ONVIF_PASSWORD");
            if (!onvif_username.isEmpty() && player_onvif_username_edit_ != nullptr)
            {
                player_onvif_username_edit_->setText(onvif_username);
            }
            if (!onvif_password.isEmpty() && player_onvif_password_edit_ != nullptr)
            {
                player_onvif_password_edit_->setText(onvif_password);
            }

            SearchOnvifDevices();
            if (!onvif_devices_.isEmpty())
            {
                player_onvif_device_list_->setCurrentRow(0);
                ApplySelectedOnvifDevice();
            }

            const bool has_rtsp_url =
                player_url_edit_ != nullptr &&
                player_url_edit_->text().trimmed().startsWith(
                    QString::fromUtf8("rtsp"),
                    Qt::CaseInsensitive);
#if !STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI
            if (!onvif_devices_.isEmpty())
            {
                SetOperationStatus(
                    QString::fromUtf8("qt.autorun.onvif"),
                    QString::fromUtf8("0"),
                    QString::fromUtf8("ready"),
                    player_onvif_status_.isEmpty() ?
                        QString::fromUtf8("ONVIF devices discovered.") :
                        player_onvif_status_);
            }
            else
            {
                SetOperationStatus(
                    QString::fromUtf8("qt.autorun.onvif"),
                    QString::fromUtf8("-1"),
                    QString::fromUtf8("failed"),
                    player_onvif_status_.isEmpty() ?
                        QString::fromUtf8("No ONVIF devices discovered.") :
                        player_onvif_status_);
            }
#else
            if (!onvif_devices_.isEmpty() && has_rtsp_url)
            {
                SetOperationStatus(
                    QString::fromUtf8("qt.autorun.onvif"),
                    QString::fromUtf8("0"),
                    QString::fromUtf8("starting"),
                    QString::fromUtf8("ONVIF stream resolved and applied."));
                StartPlayerUrl();
            }
            else
            {
                SetOperationStatus(
                    QString::fromUtf8("qt.autorun.onvif"),
                    QString::fromUtf8("-1"),
                    QString::fromUtf8("failed"),
                    player_onvif_status_.isEmpty() ?
                        QString::fromUtf8("ONVIF stream was not resolved.") :
                        player_onvif_status_);
            }
#endif

            if (quit_after_ms > 0)
            {
                ScheduleAutorunQuit(quit_after_ms, mode);
            }
            return;
        }

        const QString player_url =
            EnvironmentText("STREAMCORE_DEMO_QT_AUTORUN_PLAYER_URL");
        const QString player_url_alias =
            player_url.isEmpty()
                ? EnvironmentText("STREAMCORE_DEMO_QT_PLAYER_URL")
                : player_url;
        if (!player_url_alias.isEmpty() && player_url_edit_ != nullptr)
        {
            player_url_edit_->setText(player_url_alias);
            player_url_edit_->setCursorPosition(0);
        }

        if (player_latency_preset_combo_ != nullptr)
        {
            const QString latency_preset =
                EnvironmentText("STREAMCORE_DEMO_QT_PLAYER_LATENCY_PRESET")
                    .trimmed()
                    .toLower();
            if (latency_preset == QString::fromUtf8("compatible") ||
                latency_preset == QString::fromUtf8("compat"))
            {
                player_latency_preset_combo_->setCurrentIndex(0);
            }
            else if (latency_preset == QString::fromUtf8("regular") ||
                latency_preset == QString::fromUtf8("normal"))
            {
                player_latency_preset_combo_->setCurrentIndex(1);
            }
            else if (latency_preset == QString::fromUtf8("low") ||
                latency_preset == QString::fromUtf8("low_latency"))
            {
                player_latency_preset_combo_->setCurrentIndex(2);
            }
            else if (latency_preset == QString::fromUtf8("ultra") ||
                latency_preset == QString::fromUtf8("ultra_low") ||
                latency_preset == QString::fromUtf8("ultra_low_latency"))
            {
                player_latency_preset_combo_->setCurrentIndex(3);
            }
        }

        if (player_decode_mode_combo_ != nullptr)
        {
            player_decode_mode_combo_->setCurrentIndex(
                EnvironmentFlag("STREAMCORE_DEMO_QT_HARDWARE_DECODE", true) ? 1 : 0);
        }
        if (player_render_path_combo_ != nullptr)
        {
            const QString render_path =
                EnvironmentText("STREAMCORE_DEMO_QT_RENDER_PATH").toLower();
            int index = 3;
            if (render_path == QString::fromUtf8("software"))
            {
                index = 0;
            }
            else if (render_path == QString::fromUtf8("gpu"))
            {
                index = 1;
            }
            else if (render_path == QString::fromUtf8("direct"))
            {
                index = 2;
            }
            player_render_path_combo_->setCurrentIndex(index);
        }
        if (player_audio_volume_slider_ != nullptr)
        {
            player_audio_volume_slider_->setValue(std::clamp(
                EnvironmentInt(
                    "STREAMCORE_DEMO_QT_PLAYER_AUDIO_VOLUME",
                    player_audio_volume_slider_->value()),
                0,
                100));
        }
        const QString buffer_ms =
            EnvironmentText("STREAMCORE_DEMO_QT_PLAYER_BUFFER_MS");
        if (!buffer_ms.isEmpty() && player_buffer_ms_edit_ != nullptr)
        {
            player_buffer_ms_edit_->setText(buffer_ms);
        }
        const QString audio_queue =
            EnvironmentText("STREAMCORE_DEMO_QT_PLAYER_AUDIO_QUEUE");
        if (!audio_queue.isEmpty() && player_audio_queue_edit_ != nullptr)
        {
            player_audio_queue_edit_->setText(audio_queue);
        }
        const QString video_queue =
            EnvironmentText("STREAMCORE_DEMO_QT_PLAYER_VIDEO_QUEUE");
        if (!video_queue.isEmpty() && player_video_queue_edit_ != nullptr)
        {
            player_video_queue_edit_->setText(video_queue);
        }
        if (player_advanced_params_check_ != nullptr &&
            (!buffer_ms.isEmpty() || !audio_queue.isEmpty() || !video_queue.isEmpty()))
        {
            player_advanced_params_check_->setChecked(true);
        }

        if (feature_tabs != nullptr)
        {
            feature_tabs->setCurrentIndex(1);
        }

        SetOperationStatus(
            QString::fromUtf8("qt.autorun.player"),
            QString::fromUtf8("0"),
            QString::fromUtf8("starting"),
            QString::fromUtf8("Starting Qt player autorun media path."));
        StartPlayerUrl();

        if (quit_after_ms > 0)
        {
            ScheduleAutorunQuit(quit_after_ms, mode);
        }
    });
}

void StreamCoreDemoQtWindow::ScheduleAutomationScreenshot()
{
    const QString screenshot_path =
        EnvironmentText("STREAMCORE_DEMO_QT_SCREENSHOT_PATH");
    if (screenshot_path.isEmpty())
    {
        return;
    }

    const int delay_ms = std::max(
        0,
        EnvironmentInt("STREAMCORE_DEMO_QT_SCREENSHOT_MS", 2600));
    QTimer::singleShot(delay_ms, this, [this, screenshot_path]() {
        const QFileInfo screenshot_info(screenshot_path);
        QDir screenshot_dir = screenshot_info.absoluteDir();
        if (!screenshot_dir.exists())
        {
            screenshot_dir.mkpath(QString::fromUtf8("."));
        }

        const bool saved = grab().save(screenshot_path, "PNG");
        SetOperationStatus(
            QString::fromUtf8("qt.autorun.screenshot"),
            saved ? QString::fromUtf8("0") : QString::fromUtf8("-1"),
            saved ? QString::fromUtf8("ready") : QString::fromUtf8("failed"),
            saved
                ? QString::fromUtf8("Automation screenshot saved.")
                : QString::fromUtf8("Automation screenshot failed."));

        if (EnvironmentFlag("STREAMCORE_DEMO_QT_SCREENSHOT_QUIT", false))
        {
            close();
            QCoreApplication::exit(saved ? 0 : 1);
            QCoreApplication::quit();
            std::_Exit(saved ? 0 : 1);
        }
    });
}

void StreamCoreDemoQtWindow::ScheduleAutorunQuit(
    int quit_after_ms,
    const QString& mode)
{
    if (quit_after_ms <= 0)
    {
        return;
    }

    QTimer::singleShot(quit_after_ms, this, [this, mode]() {
        const QString normalized_mode = mode.toLower();
        SetOperationStatus(
            QString::fromUtf8("qt.autorun.quit"),
            QString::fromUtf8("0"),
            QString::fromUtf8("stopping"),
            QString::fromUtf8("Stopping autorun session before exit."));
        if (normalized_mode == QString::fromUtf8("publisher"))
        {
            StopPublisher();
        }
        else if (normalized_mode == QString::fromUtf8("gb28181"))
        {
            StopGb28181();
        }
        else if (normalized_mode == QString::fromUtf8("player") ||
            normalized_mode == QString::fromUtf8("media_e2e") ||
            normalized_mode == QString::fromUtf8("onvif"))
        {
            StopPlayerUrl();
        }

        close();
        QCoreApplication::exit(0);
        QCoreApplication::quit();
        // autorun 只服务自动化验证；退出时已显式停止 SDK runtime，避免 macOS GUI
        // Autorun only serves automation validation after SDK runtime shutdown.
        std::_Exit(0);
    });
}

void StreamCoreDemoQtWindow::StartPlayerUrl()
{
    if (desktop_render_target_status_label_ == nullptr ||
        desktop_render_target_widget_ == nullptr ||
        player_url_edit_ == nullptr)
    {
        return;
    }
    ApplyPreviewDisplayMode();

    StopPlayerUrl();
    if (player_status_label_ != nullptr)
    {
        player_status_label_->setText(UiText(
            "Playback is starting...",
            "正在启动播放..."));
    }

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    streamcore_player_handle player = streamcore_player_create();
    streamcore_player_config_t config;
    streamcore_render_target_t render_target = {};
    streamcore_player_preflight_t preflight = {};
    streamcore_player_runtime_info_t runtime_info = {};
    char error_text[STREAMCORE_TEXT_CAPACITY] = {};
    streamcore_result_t config_result = STREAMCORE_RESULT_OPERATION_FAILED;
    streamcore_result_t target_result = STREAMCORE_RESULT_OPERATION_FAILED;
    streamcore_result_t event_callback_result =
        STREAMCORE_RESULT_OPERATION_FAILED;
    streamcore_result_t preflight_result = STREAMCORE_RESULT_OPERATION_FAILED;
    streamcore_result_t start_result = STREAMCORE_RESULT_OPERATION_FAILED;
    streamcore_result_t runtime_result = STREAMCORE_RESULT_OPERATION_FAILED;
    const WId window_id = desktop_render_target_widget_->winId();
    void* native_handle = reinterpret_cast<void*>(window_id);
#if defined(Q_OS_WIN)
    const streamcore_render_platform_t render_platform = STREAMCORE_RENDER_PLATFORM_WINDOWS;
#elif defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    const streamcore_render_platform_t render_platform = STREAMCORE_RENDER_PLATFORM_APPLE;
#else
    const streamcore_render_platform_t render_platform = STREAMCORE_RENDER_PLATFORM_LINUX;
#endif
    active_player_url_ = player_url_edit_->text().trimmed().toUtf8();
    if (active_player_url_.isEmpty())
    {
        active_player_url_ = QByteArray("rtmp://192.0.2.1:1935/live/local_native");
    }

    if (player == nullptr || native_handle == nullptr)
    {
        desktop_render_target_status_label_->setText(UiText(
            "Playback surface unavailable.",
            "Playback surface unavailable."));
        if (player_status_label_ != nullptr)
        {
            player_status_label_->setText(UiText(
                "Playback failed: surface unavailable.",
                "播放失败：渲染窗口不可用。"));
        }
        if (player != nullptr)
        {
            streamcore_player_destroy(player);
        }
        UpdatePlayerButtons();
        return;
    }

    streamcore_player_get_default_config(&config);
    config.session_name = "qt_desktop_url_player";
    config.source_kind = STREAMCORE_PLAYER_SOURCE_KIND_URL;
    config.source_url = active_player_url_.constData();
    config.render_target_id = "desktop_render_target_widget";
    const QString player_audio_env =
        EnvironmentText("STREAMCORE_DEMO_QT_PLAYER_AUDIO");
    config.enable_audio = player_audio_env.isEmpty() ?
        1 :
        (EnvironmentFlag("STREAMCORE_DEMO_QT_PLAYER_AUDIO", true) ? 1 : 0);
    config.audio_volume_percent = SelectedPlayerAudioVolumePercent();
    config.enable_video = 1;
    config.requested_width = desktop_render_target_widget_->width();
    config.requested_height = desktop_render_target_widget_->height();
    config.use_hardware_decode = SelectedPlayerHardwareDecode() ? 1 : 0;
    config.video_present_path = SelectedPlayerPresentPath();
    config.prefer_software_render_backend =
        SelectedPlayerPrefersSoftwareRenderBackend() ? 1 : 0;
    config.allow_video_soft_fallback = 1;
    config.audio_cache_milliseconds =
        ReadBoundedInt(player_buffer_ms_edit_, 300, -1, 10000);
    config.video_cache_milliseconds =
        ReadBoundedInt(player_buffer_ms_edit_, 300, -1, 10000);
    config.audio_max_queue_size =
        ReadBoundedInt(player_audio_queue_edit_, 24, 0, 400);
    config.video_max_queue_size =
        ReadBoundedInt(player_video_queue_edit_, 12, 0, 200);
    config.enable_realtime_profile =
        config.video_cache_milliseconds == 0 ? 1 : 0;
    config.realtime_audio_max_queue_size = config.audio_max_queue_size;
    config.realtime_video_max_queue_size = config.video_max_queue_size;

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    render_target.target_type = STREAMCORE_RENDER_TARGET_TYPE_LAYER;
#else
    render_target.target_type = STREAMCORE_RENDER_TARGET_TYPE_NATIVE_WINDOW;
#endif
    render_target.platform_type = render_platform;
    render_target.native_handle = native_handle;
    render_target.width_hint = desktop_render_target_widget_->width();
    render_target.height_hint = desktop_render_target_widget_->height();

    config_result = streamcore_player_set_config(player, &config);
    if (config_result == STREAMCORE_RESULT_OK)
    {
        target_result = streamcore_player_set_render_target(player, &render_target);
    }
    if (target_result == STREAMCORE_RESULT_OK)
    {
        streamcore_player_event_callback_config_t event_config = {};
        event_config.callback = &StreamCoreDemoQtWindow::OnPlayerEvent;
        event_config.user_data = this;
        event_config.enable_lifecycle_events = 1;
        event_config.enable_advanced_events = 1;
        event_callback_result =
            streamcore_player_set_event_callback(player, &event_config);
    }
    if (event_callback_result == STREAMCORE_RESULT_OK)
    {
        preflight_result = streamcore_player_preflight(
            player,
            &preflight,
            error_text,
            sizeof(error_text));
    }
    if (preflight_result == STREAMCORE_RESULT_OK)
    {
        active_player_start_wall_time_ms_ = QDateTime::currentMSecsSinceEpoch();
        if (player_first_frame_label_ != nullptr)
        {
            player_first_frame_label_->setText(UiText(
                "First video frame: waiting for SDK event.",
                "首视频帧：等待 SDK 事件。"));
        }
        start_result = streamcore_player_start(player, error_text, sizeof(error_text));
    }
    runtime_result = streamcore_player_get_runtime_info(player, &runtime_info);

    if (start_result == STREAMCORE_RESULT_OK)
    {
        active_player_ = player;
        SetOperationStatus(
            QString::fromUtf8("player.start"),
            QString::number(start_result),
            QString::fromUtf8("ok"),
            ToQString(runtime_info.state_summary));
        if (player_status_label_ != nullptr)
        {
            player_status_label_->setText(UiText(
                "Playback succeeded: %1 · %2",
                "播放成功：%1 · %2")
                    .arg(QString::fromUtf8(active_player_url_.constData()))
                    .arg(ToQString(runtime_info.state_summary)));
        }
    }
    else
    {
        active_player_start_wall_time_ms_ = 0;
        const QString player_error = !ToQString(error_text).isEmpty() ?
            ToQString(error_text) :
            ToQString(preflight.detail);
        streamcore_player_destroy(player);
        SetOperationStatus(
            QString::fromUtf8("player.start"),
            QString::number(start_result),
            QString::fromUtf8("failed"),
            player_error);
        if (player_status_label_ != nullptr)
        {
            player_status_label_->setText(UiText(
                "Playback failed: %1",
                "播放失败：%1").arg(player_error.isEmpty() ?
                    UiText("unknown error", "未知错误") :
                    player_error));
        }
    }

    if (start_result == STREAMCORE_RESULT_OK)
    {
        desktop_render_target_status_label_->setText(UiText(
            "Playing · view %1x%2 · stream resolution/bitrate pending · %3",
            "播放中 · 视图 %1x%2 · 码流分辨率/码率待统计 · %3")
            .arg(desktop_render_target_widget_->width())
            .arg(desktop_render_target_widget_->height())
            .arg(PlayerPlaybackModeSummary()));
    }
    else
    {
        desktop_render_target_status_label_->setText(UiText(
            "Playback failed. See status details.",
            "播放失败，请查看状态详情。"));
        statusBar()->showMessage(
            QString::fromUtf8("player config=%1 set=%2 event=%3 preflight=%4 start=%5 runtime=%6 state=%7 error=%8")
                .arg(config_result)
                .arg(target_result)
                .arg(event_callback_result)
                .arg(preflight_result)
                .arg(start_result)
                .arg(runtime_result)
                .arg(ToQString(streamcore_session_state_name(runtime_info.state)))
                .arg(ToQString(error_text)),
            5000);
    }
    UpdatePlayerButtons();
#else
    desktop_render_target_status_label_->setText(UiText(
        "Desktop URL playback is not enabled for this platform build.",
        "Desktop URL playback is not enabled for this platform build."));
    if (player_status_label_ != nullptr)
    {
        player_status_label_->setText(UiText(
            "Playback failed: URL playback is not enabled in this build.",
            "播放失败：当前构建未启用 URL 播放。"));
    }
    UpdatePlayerButtons();
#endif
}

void StreamCoreDemoQtWindow::StopPlayerUrl()
{
    if (active_player_ == nullptr)
    {
        UpdatePlayerButtons();
        return;
    }

    streamcore_player_stop(active_player_);
    streamcore_player_destroy(active_player_);
    active_player_ = nullptr;
    active_player_start_wall_time_ms_ = 0;
    if (desktop_render_target_status_label_ != nullptr)
    {
        desktop_render_target_status_label_->setText(UiText(
            "Desktop player stopped.",
            "桌面播放器已停止。"));
    }
    if (player_status_label_ != nullptr)
    {
        player_status_label_->setText(UiText(
            "Playback stopped.",
            "播放已停止。"));
    }
    if (player_first_frame_label_ != nullptr)
    {
        player_first_frame_label_->setText(UiText(
            "First video frame: waiting for playback.",
            "首视频帧：等待播放。"));
    }
    SetOperationStatus(
        QString::fromUtf8("player.stop"),
        QString::fromUtf8("0"),
        QString::fromUtf8("ok"),
        UiText("Desktop player stopped.", "桌面播放器已停止。"));
    UpdatePlayerButtons();
}

void StreamCoreDemoQtWindow::UpdatePlayerButtons()
{
    if (player_start_button_ != nullptr)
    {
        player_start_button_->setEnabled(true);
        player_start_button_->setText(active_player_ != nullptr ?
            UiText("Stop playback", "停止播放") :
            UiText("Play", "播放"));
    }
}

void StreamCoreDemoQtWindow::OnPlayerEvent(
    const streamcore_player_event_t* event,
    void* userData)
{
    StreamCoreDemoQtWindow* window =
        static_cast<StreamCoreDemoQtWindow*>(userData);
    if (window == nullptr || event == nullptr)
    {
        return;
    }

    const int kind = static_cast<int>(event->kind);
    const int width = event->width;
    const int height = event->height;
    const qint64 timestamp_ms = static_cast<qint64>(event->timestamp_ms);
    const QString detail = QString::fromUtf8(event->detail);
    QMetaObject::invokeMethod(
        window,
        [window, kind, width, height, timestamp_ms, detail]() {
            window->HandlePlayerEvent(
                kind,
                width,
                height,
                timestamp_ms,
                detail);
        },
        Qt::QueuedConnection);
}

void StreamCoreDemoQtWindow::HandlePlayerEvent(
    int kind,
    int width,
    int height,
    qint64 timestampMs,
    const QString& detail)
{
    const qint64 now_ms = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsed_ms = active_player_start_wall_time_ms_ > 0 ?
        std::max<qint64>(0, now_ms - active_player_start_wall_time_ms_) :
        -1;

    QString summary;
    QString action = QString::fromUtf8("player.event");
    if (kind == STREAMCORE_PLAYER_EVENT_FIRST_VIDEO_FRAME)
    {
        summary = UiText(
            "First video frame: %1 ms · %2x%3 · timestamp %4 ms",
            "首视频帧：%1 ms · %2x%3 · 时间戳 %4 ms")
            .arg(elapsed_ms >= 0 ? QString::number(elapsed_ms) : QString::fromUtf8("-"))
            .arg(width)
            .arg(height)
            .arg(timestampMs > 0 ? QString::number(timestampMs) : QString::fromUtf8("-"));
        action = QString::fromUtf8("player.first_video_frame");
        if (player_first_frame_label_ != nullptr)
        {
            player_first_frame_label_->setText(summary);
        }
    }
    else if (kind == STREAMCORE_PLAYER_EVENT_FIRST_AUDIO_FRAME)
    {
        summary = UiText(
            "First audio frame: %1 ms · timestamp %2 ms",
            "首音频帧：%1 ms · 时间戳 %2 ms")
            .arg(elapsed_ms >= 0 ? QString::number(elapsed_ms) : QString::fromUtf8("-"))
            .arg(timestampMs > 0 ? QString::number(timestampMs) : QString::fromUtf8("-"));
        action = QString::fromUtf8("player.first_audio_frame");
    }
    else
    {
        summary = detail.isEmpty() ?
            QString::fromUtf8("Player event: %1").arg(kind) :
            QString::fromUtf8("Player event: %1 · %2").arg(kind).arg(detail);
    }

    SetOperationStatus(
        action,
        QString::number(kind),
        QString::fromUtf8("event"),
        summary);
}

void StreamCoreDemoQtWindow::LoadSnapshot()
{
    streamcore_demo_snapshot_t snapshot;
    char error_text[STREAMCORE_TEXT_CAPACITY];
    const streamcore_result_t result = streamcore_demo_collect_snapshot(
        &snapshot,
        error_text,
        sizeof(error_text));

    if (result != STREAMCORE_RESULT_OK)
    {
        ShowFailure(ToQString(error_text));
        return;
    }

    PopulateOverview(snapshot);
    PopulateSessionTable(snapshot);
    PopulateCapabilityTable(snapshot);
}

void StreamCoreDemoQtWindow::PopulateOverview(
    const streamcore_demo_snapshot_t& snapshot)
{
    product_title_label_->setText(QString::fromUtf8("StreamCore SDK Demo"));
    product_meta_label_->setText(UiText(
        "Product: %1 | Version: %2 | Target: %3",
        "产品：%1 | 版本：%2 | 目标：%3")
            .arg(ToQString(snapshot.product_info.product_name))
            .arg(ToQString(snapshot.product_info.version))
            .arg(ToQString(snapshot.product_info.primary_target_name)));
    license_status_label_->setText(UiText(
        "License: %1 | configured=%2 | valid=%3 | watermark=%4",
        "授权：%1 | 已配置=%2 | 有效=%3 | 水印=%4")
            .arg(ToQString(snapshot.license_info.status_name))
            .arg(BoolText(snapshot.license_info.is_configured))
            .arg(BoolText(snapshot.license_info.is_license_valid))
            .arg(BoolText(snapshot.license_info.need_watermark)));
    license_feature_label_->setText(UiText(
        "Qt GUI licensed: %1 | max input channels: %2",
        "Qt GUI 授权: %1 | 最大输入路数 %2")
            .arg(BoolText(snapshot.qt_desktop_enabled))
            .arg(snapshot.max_input_channels));
    log_status_label_->setText(UiText(
        "Log: result=%1 | %2 | file=%3",
        "日志：result=%1 | %2 | 文件=%3")
            .arg(snapshot.log_config_result)
            .arg(ToQString(snapshot.log_info.state_summary))
            .arg(ToQString(snapshot.log_info.log_file_name)));
    machine_id_label_->setText(UiText("Machine ID: %1", "机器码：%1")
        .arg(ToQString(snapshot.machine_id)));
    current_machine_id_ = ToQString(snapshot.machine_id);
    log_directory_ = ToQString(snapshot.log_info.log_directory);
    log_file_name_ = ToQString(snapshot.log_info.log_file_name);
    product_crash_directory_ = QString::fromUtf8("streamcore/crash");
    SetOperationStatus(
        QString::fromUtf8("log.configure"),
        QString::number(snapshot.log_config_result),
        snapshot.log_config_result == STREAMCORE_RESULT_OK ?
            QString::fromUtf8("ok") :
            QString::fromUtf8("failed"),
        ToQString(snapshot.log_info.state_summary));
    license_status_label_->setToolTip(ToQString(snapshot.license_info.detail));
    license_feature_label_->setToolTip(ToQString(snapshot.license_info.summary));
}

void StreamCoreDemoQtWindow::PopulateSessionTable(
    const streamcore_demo_snapshot_t& snapshot)
{
    int row = 0;
    size_t index = 0;

    session_table_->setRowCount(static_cast<int>(3 + snapshot.publisher_case_count));

    session_table_->setItem(row, 0, new QTableWidgetItem(QString::fromUtf8("player")));
    session_table_->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8("player")));
    session_table_->setItem(row, 2, new QTableWidgetItem(ToQString(
        streamcore_session_state_name(snapshot.player_runtime.state))));
    session_table_->setItem(row, 3, new QTableWidgetItem(
        BoolText(snapshot.player_preflight.is_ready_to_start)));
    session_table_->setItem(row, 4, new QTableWidgetItem(
        ToQString(snapshot.player_runtime.source_identity)));
    session_table_->setItem(row, 5, new QTableWidgetItem(QString::fromUtf8("n/a")));
    session_table_->setItem(row, 6, new QTableWidgetItem(
        ToQString(snapshot.player_preflight.summary)));
    row += 1;

    session_table_->setItem(row, 0, new QTableWidgetItem(QString::fromUtf8("capture")));
    session_table_->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8("capture")));
    session_table_->setItem(row, 2, new QTableWidgetItem(ToQString(
        streamcore_session_state_name(snapshot.capture_runtime.state))));
    session_table_->setItem(row, 3, new QTableWidgetItem(
        BoolText(snapshot.capture_preflight.is_ready_to_start)));
    session_table_->setItem(row, 4, new QTableWidgetItem(
        ToQString(snapshot.capture_runtime.source_identity)));
    session_table_->setItem(row, 5, new QTableWidgetItem(QString::fromUtf8("n/a")));
    session_table_->setItem(row, 6, new QTableWidgetItem(
        ToQString(snapshot.capture_preflight.summary)));
    row += 1;

    session_table_->setItem(row, 0, new QTableWidgetItem(QString::fromUtf8("gb28181")));
    session_table_->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8("gb28181_device")));
    session_table_->setItem(row, 2, new QTableWidgetItem(
        snapshot.gb28181_case.addon_compiled != 0 ?
            QString::fromUtf8("configured") :
            QString::fromUtf8("not_built")));
    session_table_->setItem(row, 3, new QTableWidgetItem(
        BoolText(snapshot.gb28181_case.set_config_result == STREAMCORE_RESULT_OK &&
            snapshot.gb28181_case.runtime_info_result == STREAMCORE_RESULT_OK)));
    session_table_->setItem(row, 4, new QTableWidgetItem(
        ToQString(snapshot.gb28181_case.target_name)));
    session_table_->setItem(row, 5, new QTableWidgetItem(QString::fromUtf8("n/a")));
    session_table_->setItem(row, 6, new QTableWidgetItem(
        ToQString(snapshot.gb28181_case.summary)));
    row += 1;

    for (index = 0; index < snapshot.publisher_case_count; ++index, ++row)
    {
        const streamcore_demo_publisher_case_t& item =
            snapshot.publisher_cases[index];
        const QString summary = QString::fromUtf8("%1\ncallback=%2")
            .arg(ToQString(item.transcode_summary))
            .arg(ToQString(item.callback_summary));

        session_table_->setItem(row, 0, new QTableWidgetItem(
            ToQString(item.case_name)));
        session_table_->setItem(row, 1, new QTableWidgetItem(ToQString(
            streamcore_publisher_input_kind_name(item.runtime.input_kind))));
        session_table_->setItem(row, 2, new QTableWidgetItem(ToQString(
            streamcore_session_state_name(item.runtime.state))));
        session_table_->setItem(row, 3, new QTableWidgetItem(QString::fromUtf8(
            "%1 / policy=%2")
                .arg(BoolText(item.preflight.is_ready_to_start))
                .arg(BoolText(item.preflight.transcode_policy_satisfied))));
        session_table_->setItem(row, 4, new QTableWidgetItem(ToQString(
            item.runtime.publish_identity)));
        session_table_->setItem(row, 5, new QTableWidgetItem(summary));
        session_table_->setItem(row, 6, new QTableWidgetItem(ToQString(
            item.preflight.summary)));
    }
}

void StreamCoreDemoQtWindow::PopulateCapabilityTable(
    const streamcore_demo_snapshot_t& snapshot)
{
    size_t index = 0;

    capability_table_->setRowCount(static_cast<int>(snapshot.capability_count));
    for (index = 0; index < snapshot.capability_count; ++index)
    {
        const streamcore_capability_descriptor_t& item =
            snapshot.capabilities[index];
        capability_table_->setItem(
            static_cast<int>(index),
            0,
            new QTableWidgetItem(ToQString(
                streamcore_capability_group_name(item.group))));
        capability_table_->setItem(
            static_cast<int>(index),
            1,
            new QTableWidgetItem(ToQString(item.capability_key)));
        capability_table_->setItem(
            static_cast<int>(index),
            2,
            new QTableWidgetItem(BoolText(item.enabled_in_current_build)));
        capability_table_->setItem(
            static_cast<int>(index),
            3,
            new QTableWidgetItem(BoolText(item.requires_explicit_license)));
        capability_table_->setItem(
            static_cast<int>(index),
            4,
            new QTableWidgetItem(CapabilitySummaryText(
                item,
                EffectiveLanguage() == DemoLanguage::Chinese)));
    }
}

void StreamCoreDemoQtWindow::SetOperationStatus(
    const QString& action,
    const QString& code,
    const QString& statusName,
    const QString& summary)
{
    latest_action_ = action.isEmpty() ? QString::fromUtf8("demo") : action;
    latest_status_code_ = code.isEmpty() ? QString::fromUtf8("-") : code;
    latest_status_name_ = statusName.isEmpty() ? QString::fromUtf8("unknown") : statusName;
    latest_status_summary_ = summary.isEmpty() ? QString::fromUtf8("No details.") : summary;
    RouteOperationRuntimeLogLine(
        QString::fromUtf8("%1 | %2 | code=%3 | %4 | %5")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
            .arg(latest_action_)
            .arg(latest_status_code_)
            .arg(latest_status_name_)
            .arg(latest_status_summary_));
    AppendOperationLogLine();
    UpdateOperationStatusLabel();
}

void StreamCoreDemoQtWindow::UpdateOperationStatusLabel()
{
    if (operation_status_label_ == nullptr)
    {
        return;
    }
    operation_status_label_->setText(UiText(
        "Status: %1 | code=%2 / %3 | %4 | log=%5 | zip=%6",
        "状态：%1 | code=%2 / %3 | %4 | log=%5 | zip=%6")
            .arg(latest_action_)
            .arg(latest_status_code_)
            .arg(latest_status_name_)
            .arg(latest_status_summary_)
            .arg(CurrentLogFilePath())
            .arg(latest_log_zip_path_.isEmpty() ?
                UiText("not exported", "尚未导出") :
                latest_log_zip_path_));
}

void StreamCoreDemoQtWindow::AppendOperationLogLine()
{
    const QFileInfo log_file(CurrentLogFilePath());
    QDir directory(log_file.absolutePath());
    if (!directory.exists() && !directory.mkpath(QString::fromUtf8(".")))
    {
        return;
    }

    QFile file(log_file.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return;
    }

    const QString line = QString::fromUtf8("%1 action=%2 code=%3 status=%4 summary=%5\n")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
        .arg(latest_action_)
        .arg(latest_status_code_)
        .arg(latest_status_name_)
        .arg(latest_status_summary_);
    file.write(line.toUtf8());
}

QString StreamCoreDemoQtWindow::CurrentLogFilePath() const
{
    const QString file_name = log_file_name_.isEmpty() ?
        QString::fromUtf8("streamcore_demo.log") :
        log_file_name_;
    const QFileInfo file_info(file_name);
    if (file_info.isAbsolute())
    {
        return file_info.absoluteFilePath();
    }
    return QDir(CurrentLogDirectory()).absoluteFilePath(file_name);
}

QString StreamCoreDemoQtWindow::CurrentLogDirectory() const
{
    return AbsoluteLogRelatedDirectory(
        log_directory_,
        QString::fromUtf8("streamcore/logs"));
}

QString StreamCoreDemoQtWindow::CurrentProductCrashDirectory() const
{
    const QString crash_directory =
        EnvironmentText("STREAMCORE_DEMO_CRASH_DIRECTORY");
    if (!crash_directory.isEmpty())
    {
        return AbsoluteLogRelatedDirectory(
            crash_directory,
            QString::fromUtf8("streamcore/crash"));
    }
    return AbsoluteLogRelatedDirectory(
        product_crash_directory_,
        QString::fromUtf8("streamcore/crash"));
}

QString StreamCoreDemoQtWindow::CreateLogPackageZip(const QString& targetPath) const
{
    QVector<ZipSourceEntry> entries;
    QVector<LogPackageFile> candidates;
    QSet<QString> seen_paths;
    QStringList skipped_files;
    const QString status_text = QString::fromUtf8(
        "StreamCore SDK Demo status\n"
        "action=%1\n"
        "code=%2\n"
        "status=%3\n"
        "summary=%4\n"
        "log=%5\n"
        "log_dir=%6\n"
        "crash_dir=%7\n"
        "package_limit_files=%8\n"
        "package_limit_bytes=%9\n"
        "crash_capture_note=StreamCore SDK does not install crash handlers or expose a crash directory. "
        "Demo/server/desktop products should integrate Crashpad, Breakpad, WER/minidump, "
        "PLCrashReporter, Sentry, or another mature crash solution and write artifacts into this product directory.\n"
        "zip=%10\n")
            .arg(latest_action_)
            .arg(latest_status_code_)
            .arg(latest_status_name_)
            .arg(latest_status_summary_)
            .arg(CurrentLogFilePath())
            .arg(CurrentLogDirectory())
            .arg(CurrentProductCrashDirectory())
            .arg(kLogPackageMaxFiles)
            .arg(kLogPackageMaxBytes)
            .arg(latest_log_zip_path_.isEmpty() ?
                UiText("not exported", "尚未导出") :
                latest_log_zip_path_);
    entries.append({
        NormalizeZipEntryName(QString::fromUtf8("demo_status.txt")),
        status_text.toUtf8()
    });
    CollectLogPackageFiles(
        &candidates,
        &seen_paths,
        QDir(CurrentLogDirectory()),
        QString::fromUtf8("logs/"));
    CollectLogPackageFiles(
        &candidates,
        &seen_paths,
        QDir(CurrentProductCrashDirectory()),
        QString::fromUtf8("crash/"));
    AddLogPackageFileIfPresent(
        &candidates,
        &seen_paths,
        CurrentLogFilePath(),
        QString::fromUtf8("logs/%1").arg(QFileInfo(CurrentLogFilePath()).fileName()));
    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const LogPackageFile& left, const LogPackageFile& right) {
            return left.modified_ms > right.modified_ms;
        });
    const int added_file_count = AddCappedLogPackageEntries(
        &entries,
        candidates,
        &skipped_files);
    if (added_file_count == 0)
    {
        entries.append({
            NormalizeZipEntryName(QString::fromUtf8("logs/README.txt")),
            QByteArray("No SDK or demo log file has been created yet.\n")
        });
    }
    if (!skipped_files.isEmpty())
    {
        entries.append({
            NormalizeZipEntryName(QString::fromUtf8("skipped_files.txt")),
            QString::fromUtf8(
                "The log package is capped to %1 files and %2 bytes.\n%3\n")
                .arg(kLogPackageMaxFiles)
                .arg(kLogPackageMaxBytes)
                .arg(skipped_files.join(QString::fromUtf8("\n")))
                .toUtf8()
        });
    }

    QString error_message;
    if (!WriteStoredZipArchive(targetPath, entries, &error_message))
    {
        return error_message.isEmpty() ?
            UiText("failed to write zip", "写入 zip 失败") :
            error_message;
    }
    return QString();
}

void StreamCoreDemoQtWindow::ShareLogs()
{
    const QString default_dir = QStandardPaths::writableLocation(
        QStandardPaths::DownloadLocation).isEmpty() ?
        QDir::currentPath() :
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString default_path = QDir(default_dir).filePath(QString::fromUtf8(
        "streamcore_demo_logs_%1.zip")
            .arg(QDateTime::currentDateTime().toString(QString::fromUtf8("yyyyMMdd_HHmmss"))));
    const QString target_path = QFileDialog::getSaveFileName(
        this,
        UiText("Save log package", "保存日志包"),
        default_path,
        QString::fromUtf8("Zip (*.zip)"));
    if (target_path.isEmpty())
    {
        SetOperationStatus(
            QString::fromUtf8("logs.share"),
            QString::fromUtf8("-"),
            QString::fromUtf8("cancelled"),
            UiText("Log package export was cancelled.", "已取消导出日志包。"));
        return;
    }

    const QString error_message = CreateLogPackageZip(target_path);
    if (!error_message.isEmpty())
    {
        SetOperationStatus(
            QString::fromUtf8("logs.share"),
            QString::fromUtf8("-1"),
            QString::fromUtf8("failed"),
            error_message);
        QMessageBox::warning(
            this,
            UiText("Log package failed", "日志包导出失败"),
            error_message);
        return;
    }

    latest_log_zip_path_ = QFileInfo(target_path).absoluteFilePath();
    SetOperationStatus(
        QString::fromUtf8("logs.share"),
        QString::fromUtf8("0"),
        QString::fromUtf8("ready"),
        UiText("Log package is ready.", "日志包已就绪。"));
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(target_path).absolutePath()));
}

void StreamCoreDemoQtWindow::ShowUploadReserved()
{
    const QString message = UiText(
        "Upload is reserved because no private upload server is configured. Use Share logs to send the zip through the system file/share workflow.",
        "Upload is reserved because no private upload server is configured. Use Share logs to send the zip through the system file/share workflow.");
    SetOperationStatus(
        QString::fromUtf8("logs.upload"),
        QString::fromUtf8("-"),
        QString::fromUtf8("reserved"),
        message);
    if (EnvironmentText("STREAMCORE_DEMO_QT_AUTORUN").toLower() ==
        QString::fromUtf8("upload"))
    {
        return;
    }
    QMessageBox::information(
        this,
        UiText("Upload reserved", "上传预留"),
        message);
}

void StreamCoreDemoQtWindow::ShowFailure(const QString& message)
{
    product_title_label_->setText(QString::fromUtf8("StreamCore SDK Demo"));
    product_meta_label_->setText(UiText(
        "Startup failed.",
        "Startup failed."));
    license_status_label_->setText(UiText("License: load_failed", "授权：load_failed"));
    license_feature_label_->clear();
    log_status_label_->clear();
    machine_id_label_->clear();
    current_machine_id_.clear();
    SetOperationStatus(
        QString::fromUtf8("demo.startup"),
        QString::fromUtf8("-1"),
        QString::fromUtf8("failed"),
        message);
    session_table_->setRowCount(0);
    capability_table_->setRowCount(0);
}

QString StreamCoreDemoQtWindow::BoolText(int value)
{
    return value != 0 ? QString::fromUtf8("yes") : QString::fromUtf8("no");
}
