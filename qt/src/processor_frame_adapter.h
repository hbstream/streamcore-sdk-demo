#pragma once

#include "processor_worker_bridge.h"
#include "streamcore/streamcore_sdk.h"

#include <vector>

#include <QThreadPool>

namespace demo_processor
{

// Converts a StreamCore Capture Processor callback into a BGRA algorithm task.
//
// This adapter owns the work queue and the optional Python worker bridge. It
// deliberately knows nothing about QWidget, source selection, preview layout,
// or effect names. Applications can replace ProcessorWorkerBridge with a native
// CPU/GPU algorithm while keeping the same SDK callback and finish contract.
//
// Lifecycle: create before streamcore_capture_set_config(), pass this object as
// video_processor_callback_context, stop/destroy the Capture first, then call
// Stop() or destroy the adapter. Every accepted SDK process is finished once.
class ProcessorFrameAdapter final
{
public:
    enum class Mode { Monochrome, ExternalWorker };

    explicit ProcessorFrameAdapter(Mode mode, QProcessEnvironment environment =
        QProcessEnvironment::systemEnvironment());
    ~ProcessorFrameAdapter();

    ProcessorFrameAdapter(const ProcessorFrameAdapter&) = delete;
    ProcessorFrameAdapter& operator=(const ProcessorFrameAdapter&) = delete;

    // Validates the configured external worker and loads its model before
    // Capture starts. The warm-up frame uses the same serial thread as later
    // video frames, so any persistent QProcess remains on its owning thread.
    // Returns an empty string on success.
    QString ProbeAndWarmUp(int timeout_ms = 30000);

    // C callback for streamcore_capture_config_t::video_processor_callback.
    static void Callback(streamcore_video_process process,
        streamcore_video_frame_ref input, void* context);

    // Drain pending finishes and close the worker on its processing thread.
    // The host must stop/destroy the Capture before calling this method.
    void Stop();

private:
    void Submit(streamcore_video_process process,
        streamcore_video_frame_ref input);
    void ProcessTask(streamcore_video_process process,
        streamcore_video_frame_ref input) noexcept;
    static void FinishError(streamcore_video_process process) noexcept;

    Mode mode_;
    QThreadPool pool_;
    ProcessorWorkerBridge worker_;
};

} // namespace demo_processor
