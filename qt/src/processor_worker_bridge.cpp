#include "processor_worker_bridge.h"

#include <cstring>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace demo_processor
{
namespace
{
constexpr std::uint32_t kMaxFrameBytes = 16U * 1024U * 1024U;
constexpr int kStartTimeoutMs = 3000;
constexpr int kFrameTimeoutMs = 30000;

QString WorkerPath(const QProcessEnvironment& environment)
{
    const QString configured = environment.value(
        QString::fromUtf8("STREAMCORE_DEMO_QT_PROCESSOR_WORKER")).trimmed();
    return configured.isEmpty() ?
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QString::fromUtf8("processors/frame_worker.py")) : configured;
}

QString PythonPath(const QProcessEnvironment& environment)
{
    const QString configured = environment.value(
        QString::fromUtf8("STREAMCORE_DEMO_QT_PROCESSOR_PYTHON")).trimmed();
    if (!configured.isEmpty()) return configured;

    const QString bundled = QDir(QCoreApplication::applicationDirPath())
        .filePath(QString::fromUtf8("runtime/python/python.exe"));
    if (QFileInfo(bundled).isFile()) return bundled;

    const QStringList search_paths = environment.value(
        QString::fromUtf8("PATH")).split(QDir::listSeparator(), Qt::SkipEmptyParts);
    QString resolved = QStandardPaths::findExecutable(
        QString::fromUtf8("python"), search_paths);
    if (resolved.isEmpty())
        resolved = QStandardPaths::findExecutable(
            QString::fromUtf8("python3"), search_paths);
    return resolved;
}

void WriteU32(char* destination, std::uint32_t value)
{
    for (int index = 0; index != 4; ++index)
        destination[index] = static_cast<char>((value >> (index * 8)) & 0xffU);
}

std::uint32_t ReadU32(const char* source)
{
    std::uint32_t result = 0;
    for (int index = 0; index != 4; ++index)
        result |= static_cast<std::uint32_t>(
            static_cast<unsigned char>(source[index])) << (index * 8);
    return result;
}

// QProcess may return a partial header or frame. Accumulate exactly the bytes
// required by the protocol; stdout is reserved for binary frames, while the
// Python worker writes diagnostics to stderr.
bool ReadUntil(QProcess* process, QByteArray* data, qsizetype required)
{
    while (data->size() < required)
    {
        data->append(process->readAllStandardOutput());
        if (data->size() >= required) return true;
        if (process->state() == QProcess::NotRunning ||
            !process->waitForReadyRead(kFrameTimeoutMs)) return false;
    }
    return true;
}

QString ProcessError(QProcess* process, const QString& fallback)
{
    const QString stderr_text = QString::fromUtf8(
        process->readAllStandardError()).trimmed();
    return stderr_text.isEmpty() ? fallback : stderr_text.left(300);
}
} // namespace

ProcessorWorkerBridge::ProcessorWorkerBridge(QProcessEnvironment environment)
    : environment_(std::move(environment)),
      python_path_(PythonPath(environment_)), worker_path_(WorkerPath(environment_)) {}

ProcessorWorkerBridge::~ProcessorWorkerBridge()
{
    // ProcessorFrameAdapter::Stop() releases the child on its owning thread.
    Q_ASSERT(process_ == nullptr);
}

QString ProcessorWorkerBridge::Probe(int timeout_ms) const
{
    if (python_path_.isEmpty())
        return QString::fromUtf8("Python executable was not found");
    if (!QFileInfo(worker_path_).isFile())
        return QString::fromUtf8("Frame worker was not found: %1").arg(worker_path_);

    QProcess probe;
    probe.setProcessEnvironment(environment_);
    // -B keeps an unpacked add-on free of generated __pycache__ files.
    probe.start(python_path_, {QString::fromUtf8("-B"),
        QString::fromUtf8("-u"), worker_path_,
        QString::fromUtf8("--probe")});
    if (!probe.waitForStarted(kStartTimeoutMs) ||
        !probe.waitForFinished(timeout_ms) ||
        probe.exitStatus() != QProcess::NormalExit || probe.exitCode() != 0)
    {
        const QString error = ProcessError(&probe,
            QString::fromUtf8("Frame worker probe failed"));
        if (probe.state() != QProcess::NotRunning)
        {
            probe.kill();
            probe.waitForFinished(300);
        }
        return error;
    }
    return {};
}

bool ProcessorWorkerBridge::TransformBgra(
    const std::vector<std::uint8_t>& input, int width, int height,
    std::vector<std::uint8_t>* output, QString* error)
{
    if (output == nullptr || error == nullptr) return false;
    output->clear();
    error->clear();
    if (width <= 0 || height <= 0 ||
        static_cast<std::uint64_t>(width) * height * 4U != input.size() ||
        input.size() > kMaxFrameBytes)
    {
        *error = QString::fromUtf8("Invalid BGRA frame dimensions or size");
        return false;
    }

    if (!process_ || process_->state() != QProcess::Running)
    {
        Stop();
        process_ = std::make_unique<QProcess>();
        process_->setProcessEnvironment(environment_);
        process_->start(python_path_, {QString::fromUtf8("-B"),
            QString::fromUtf8("-u"), worker_path_});
        if (!process_->waitForStarted(kStartTimeoutMs))
        {
            *error = ProcessError(process_.get(),
                QString::fromUtf8("Frame worker did not start"));
            Stop();
            return false;
        }
    }

    QByteArray request(16 + static_cast<qsizetype>(input.size()),
        Qt::Uninitialized);
    std::memcpy(request.data(), "SCF1", 4);
    WriteU32(request.data() + 4, static_cast<std::uint32_t>(width));
    WriteU32(request.data() + 8, static_cast<std::uint32_t>(height));
    WriteU32(request.data() + 12, static_cast<std::uint32_t>(input.size()));
    std::memcpy(request.data() + 16, input.data(), input.size());
    if (process_->write(request) != request.size())
    {
        *error = QString::fromUtf8("Frame worker rejected input");
        Stop();
        return false;
    }
    while (process_->bytesToWrite() > 0)
    {
        if (!process_->waitForBytesWritten(kFrameTimeoutMs))
        {
            *error = ProcessError(process_.get(),
                QString::fromUtf8("Frame worker input timed out"));
            Stop();
            return false;
        }
    }

    QByteArray response;
    if (!ReadUntil(process_.get(), &response, 8) ||
        std::memcmp(response.constData(), "SCO1", 4) != 0 ||
        ReadU32(response.constData() + 4) != input.size())
    {
        *error = ProcessError(process_.get(),
            QString::fromUtf8("Frame worker returned an invalid header"));
        Stop();
        return false;
    }
    const qsizetype response_size = 8 + static_cast<qsizetype>(input.size());
    if (!ReadUntil(process_.get(), &response, response_size))
    {
        *error = ProcessError(process_.get(),
            QString::fromUtf8("Frame worker output timed out"));
        Stop();
        return false;
    }
    output->assign(
        reinterpret_cast<const std::uint8_t*>(response.constData() + 8),
        reinterpret_cast<const std::uint8_t*>(response.constData() + response_size));
    return true;
}

void ProcessorWorkerBridge::Stop()
{
    if (process_ && process_->state() != QProcess::NotRunning)
    {
        process_->kill();
        process_->waitForFinished(300);
    }
    process_.reset();
}

} // namespace demo_processor
