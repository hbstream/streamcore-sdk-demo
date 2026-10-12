#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <QProcessEnvironment>
#include <QString>

class QProcess;

namespace demo_processor
{

// Reusable, SDK-independent bridge between BGRA frames and a local algorithm.
//
// The host chooses a Python executable and the generic frame_worker.py script.
// That script imports an effect module exposing create_processor(). The effect
// module and its dependencies are optional runtime files; this C++ class has no
// compile-time dependency on Python, NumPy, OpenCV, or any model.
//
// Threading contract: Probe() may run on the UI thread before capture starts.
// TransformBgra() and Stop() must run serially on the SAME long-lived worker
// thread. QProcess is created there and must also be destroyed there. A caller
// should drain its work queue, call Stop() on that thread, then destroy this
// object. ProcessorFrameAdapter shows one way to honor that contract.
class ProcessorWorkerBridge final
{
public:
    // Capture an environment for this one worker. Algorithm selection and
    // model paths cannot leak from a previous effect or another session.
    explicit ProcessorWorkerBridge(QProcessEnvironment environment =
        QProcessEnvironment::systemEnvironment());
    ~ProcessorWorkerBridge();

    ProcessorWorkerBridge(const ProcessorWorkerBridge&) = delete;
    ProcessorWorkerBridge& operator=(const ProcessorWorkerBridge&) = delete;

    // Runs frame_worker.py --probe in a short-lived process. The module path
    // and effect options come from this bridge's captured environment.
    // Returns an empty string on success, otherwise a displayable error.
    // A module without probe_processor() may initialize its model here, so
    // callers should allow the same startup budget as model warm-up.
    QString Probe(int timeout_ms = 30000) const;

    // Sends one tightly packed BGRA frame and receives one same-size BGRA
    // frame. The worker protocol is little-endian SCF1(width,height,length)
    // followed by pixels, with SCO1(length) and pixels in the response.
    // Frames over 16 MiB and malformed responses are rejected. On failure,
    // the child is stopped so the next call starts a fresh process.
    bool TransformBgra(const std::vector<std::uint8_t>& input,
        int width, int height, std::vector<std::uint8_t>* output,
        QString* error);

    // Run on the TransformBgra() thread after all submitted frames finish.
    void Stop();

private:
    QProcessEnvironment environment_;
    QString python_path_;
    QString worker_path_;
    std::unique_ptr<QProcess> process_;
};

} // namespace demo_processor
