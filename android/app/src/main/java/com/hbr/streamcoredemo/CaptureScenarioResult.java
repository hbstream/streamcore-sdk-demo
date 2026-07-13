package com.hbr.streamcoredemo;

import com.hbr.streamcore.StreamCoreCapture;
import com.hbr.streamcore.StreamCoreOperationStatus;

final class CaptureScenarioResult {
    final StreamCoreCapture.Preflight preflight;
    final StreamCoreOperationStatus startStatus;
    final StreamCoreCapture.RuntimeInfo runtimeAfterStart;
    final StreamCoreCapture.RuntimeInfo runtimeAfterStop;

    CaptureScenarioResult(
            StreamCoreCapture.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreCapture.RuntimeInfo runtimeAfterStart,
            StreamCoreCapture.RuntimeInfo runtimeAfterStop) {
        this.preflight = preflight;
        this.startStatus = startStatus;
        this.runtimeAfterStart = runtimeAfterStart;
        this.runtimeAfterStop = runtimeAfterStop;
    }
}
