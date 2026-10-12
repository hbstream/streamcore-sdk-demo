#include "processor_frame_adapter.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <utility>
#include <vector>

#include <QRunnable>
#include <QSemaphore>

namespace demo_processor
{

ProcessorFrameAdapter::ProcessorFrameAdapter(Mode mode,
    QProcessEnvironment environment)
    : mode_(mode), worker_(std::move(environment))
{
    // The external process retains model state between frames and must never
    // be accessed by two threads at once. The monochrome path is cheap enough
    // to share the same simple serial queue in this example.
    pool_.setMaxThreadCount(1);
    pool_.setExpiryTimeout(-1);
}

ProcessorFrameAdapter::~ProcessorFrameAdapter() { Stop(); }

QString ProcessorFrameAdapter::ProbeAndWarmUp(int timeout_ms)
{
    if (mode_ != Mode::ExternalWorker) return {};
    const QString probe_error = worker_.Probe(timeout_ms);
    if (!probe_error.isEmpty()) return probe_error;

    struct Result
    {
        QSemaphore done;
        QString error;
    };
    const auto result = std::make_shared<Result>();
    pool_.start(QRunnable::create([this, result]() {
        std::vector<std::uint8_t> blank(64U * 64U * 4U, 255U);
        std::vector<std::uint8_t> output;
        if (!worker_.TransformBgra(blank, 64, 64, &output, &result->error) &&
            result->error.isEmpty())
            result->error = QString::fromUtf8("Processor model warm-up failed");
        result->done.release();
    }));
    if (!result->done.tryAcquire(1, timeout_ms))
        return QString::fromUtf8("Processor model load timed out");
    return result->error;
}

void ProcessorFrameAdapter::Callback(streamcore_video_process process,
    streamcore_video_frame_ref input, void* context)
{
    auto* adapter = static_cast<ProcessorFrameAdapter*>(context);
    if (adapter == nullptr)
    {
        FinishError(process);
        return;
    }
    adapter->Submit(process, input);
}

void ProcessorFrameAdapter::Submit(streamcore_video_process process,
    streamcore_video_frame_ref input)
{
    try
    {
        if (mode_ == Mode::ExternalWorker)
        {
            // Backpressure is intentional. Neural models can take longer than
            // one camera interval; submitting later frames into the SDK's
            // bounded ordered queue would expire useful results. Waiting here
            // lets capture frame rate adapt to actual algorithm throughput.
            QSemaphore completed;
            pool_.start(QRunnable::create([this, process, input, &completed]() {
                ProcessTask(process, input);
                completed.release();
            }));
            completed.acquire();
        }
        else
        {
            // Monochrome is independent per frame, so the SDK source callback
            // can return immediately. The task still finishes the SDK process.
            pool_.start(QRunnable::create([this, process, input]() {
                ProcessTask(process, input);
            }));
        }
    }
    catch (...)
    {
        FinishError(process);
    }
}

void ProcessorFrameAdapter::FinishError(streamcore_video_process process) noexcept
{
    streamcore_video_process_result_t result = {};
    result.action = STREAMCORE_PROCESSOR_ACTION_ERROR;
    streamcore_video_process_finish(process, &result);
}

void ProcessorFrameAdapter::ProcessTask(streamcore_video_process process,
    streamcore_video_frame_ref input) noexcept
{
    try
    {
        streamcore_frame_export_plan_t plan = {};
        streamcore_capture_video_frame_t frame = {};
        std::vector<std::uint8_t> bytes;
        const streamcore_result_t export_result =
            streamcore_video_frame_get_cpu_export_plan(
                input, STREAMCORE_VIDEO_PIXEL_FORMAT_RGB32, &plan);
        if (export_result == STREAMCORE_RESULT_OK && plan.supported != 0)
        {
            bytes.resize(plan.required_size);
            if (streamcore_video_frame_copy_as(input,
                    STREAMCORE_VIDEO_PIXEL_FORMAT_RGB32,
                    bytes.data(), bytes.size(), &frame) != STREAMCORE_RESULT_OK)
            {
                FinishError(process);
                return;
            }
        }
        else
        {
#if defined(STREAMCORE_DEMO_LEGACY_MEDIA_RGB32)
            // Compatibility for the frozen 1.6.2 Demo SDK only: file frames
            // may report hbcore's numeric RGB32 value 5. Never trust that
            // value alone; accept only an exact packed BGRA media packet.
            streamcore_video_frame_info_t info = {};
            streamcore_capture_video_frame_t view = {};
            const bool legacy_rgb32 =
                streamcore_video_frame_get_info(input, &info) == STREAMCORE_RESULT_OK &&
                streamcore_video_frame_get_cpu_view(input, &view) == STREAMCORE_RESULT_OK &&
                info.storage_kind == STREAMCORE_FRAME_STORAGE_KIND_MEDIA_PACKET &&
                info.backend_kind == STREAMCORE_FRAME_BACKEND_KIND_MEDIA_PACKET &&
                info.pixel_format == 5 && view.data != nullptr &&
                view.width > 0 && view.height > 0 &&
                static_cast<size_t>(view.width) <=
                    SIZE_MAX / static_cast<size_t>(view.height) / 4 &&
                view.size == static_cast<size_t>(view.width) *
                    static_cast<size_t>(view.height) * 4;
            if (!legacy_rgb32)
            {
                std::fprintf(stderr, "Processor RGB32 export failed: %d\n",
                    static_cast<int>(export_result));
                FinishError(process);
                return;
            }
            const auto* source = static_cast<const std::uint8_t*>(view.data);
            bytes.assign(source, source + view.size);
            frame = view;
            frame.pixel_format = STREAMCORE_VIDEO_PIXEL_FORMAT_RGB32;
#else
            std::fprintf(stderr, "Processor RGB32 export failed: %d\n",
                static_cast<int>(export_result));
            FinishError(process);
            return;
#endif
        }

        if (mode_ == Mode::ExternalWorker)
        {
            std::vector<std::uint8_t> output;
            QString error;
            if (!worker_.TransformBgra(bytes, frame.width,
                    frame.height, &output, &error))
            {
                std::fprintf(stderr, "Processor worker: %s\n",
                    error.toUtf8().constData());
                FinishError(process);
                return;
            }
            bytes = std::move(output);
        }
        else
        {
            for (size_t offset = 0; offset + 3 < bytes.size(); offset += 4)
            {
                const unsigned gray =
                    (static_cast<unsigned>(bytes[offset]) * 29U +
                     static_cast<unsigned>(bytes[offset + 1]) * 150U +
                     static_cast<unsigned>(bytes[offset + 2]) * 77U) >> 8;
                bytes[offset] = bytes[offset + 1] = bytes[offset + 2] =
                    static_cast<std::uint8_t>(gray);
            }
        }

        // create_cpu_copy takes its own copy before bytes goes out of scope.
        // The new frame transfers to the SDK only when finish(REPLACE)
        // succeeds. On any finish failure we still own and release it.
        frame.data = bytes.data();
        streamcore_video_frame_ref replacement = nullptr;
        if (streamcore_video_frame_create_cpu_copy(&frame, &replacement) !=
            STREAMCORE_RESULT_OK)
        {
            FinishError(process);
            return;
        }
        streamcore_video_process_result_t result = {};
        result.action = STREAMCORE_PROCESSOR_ACTION_REPLACE;
        result.replacement_frame = replacement;
        if (streamcore_video_process_finish(process, &result) !=
            STREAMCORE_RESULT_OK)
            streamcore_video_frame_release(replacement);
    }
    catch (...)
    {
        FinishError(process);
    }
}

void ProcessorFrameAdapter::Stop()
{
    if (mode_ == Mode::ExternalWorker)
    {
        // Queue shutdown behind every frame on the same serial pool. A lower
        // priority keeps it last even if capture had pending work when stopped.
        // This also avoids destroying QProcess from the UI thread.
        pool_.start(QRunnable::create([this]() { worker_.Stop(); }), -1);
    }
    pool_.waitForDone();
}

} // namespace demo_processor
