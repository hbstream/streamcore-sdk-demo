package com.hbr.streamcoredemo;

import com.hbr.streamcore.StreamCoreCapture;
import com.hbr.streamcore.StreamCoreOperationStatus;

final class CapturePreviewScenarioResult {
    final String scenarioKey;
    final String lifecycleState;
    final String summary;
    final String detail;
    final StreamCoreCapture.Preflight preflight;
    final StreamCoreOperationStatus startStatus;
    final StreamCoreOperationStatus stopStatus;
    final StreamCoreOperationStatus publisherStatus;
    final StreamCoreCapture.RuntimeInfo runtimeAfterStart;

    private CapturePreviewScenarioResult(
            String scenarioKey,
            String lifecycleState,
            String summary,
            String detail,
            StreamCoreCapture.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreOperationStatus stopStatus,
            StreamCoreOperationStatus publisherStatus,
            StreamCoreCapture.RuntimeInfo runtimeAfterStart) {
        this.scenarioKey = scenarioKey;
        this.lifecycleState = lifecycleState;
        this.summary = summary;
        this.detail = detail;
        this.preflight = preflight;
        this.startStatus = startStatus;
        this.stopStatus = stopStatus;
        this.publisherStatus = publisherStatus;
        this.runtimeAfterStart = runtimeAfterStart;
    }

    static CapturePreviewScenarioResult idle(String scenarioKey) {
        return new CapturePreviewScenarioResult(
                scenarioKey,
                "idle",
                "camera preview is idle.",
                "preview target is available.",
                null,
                null,
                null,
                null,
                null);
    }

    static CapturePreviewScenarioResult starting(String scenarioKey) {
        return new CapturePreviewScenarioResult(
                scenarioKey,
                "starting",
                "camera preview is binding the TextureView and starting capture.",
                "The session is waiting for native capture start to complete.",
                null,
                null,
                null,
                null,
                null);
    }

    static CapturePreviewScenarioResult blocked(
            String scenarioKey,
            StreamCoreOperationStatus status) {
        return new CapturePreviewScenarioResult(
                scenarioKey,
                "blocked",
                status.summary,
                status.detail,
                null,
                status,
                null,
                null,
                null);
    }

    static CapturePreviewScenarioResult started(
            String scenarioKey,
            StreamCoreCapture.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreCapture.RuntimeInfo runtimeAfterStart,
            StreamCoreOperationStatus publisherStatus) {
        return new CapturePreviewScenarioResult(
                scenarioKey,
                "running",
                "camera preview is running on the visible TextureView.",
                "Stop the preview before the TextureView is destroyed.",
                preflight,
                startStatus,
                null,
                publisherStatus,
                runtimeAfterStart);
    }

    static CapturePreviewScenarioResult failed(
            String scenarioKey,
            StreamCoreCapture.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreCapture.RuntimeInfo runtimeAfterStart,
            StreamCoreOperationStatus publisherStatus) {
        return new CapturePreviewScenarioResult(
                scenarioKey,
                "start_failed",
                "camera preview did not enter the running state.",
                "Start status and runtime snapshot are shown below.",
                preflight,
                startStatus,
                null,
                publisherStatus,
                runtimeAfterStart);
    }

    CapturePreviewScenarioResult withStop(StreamCoreOperationStatus stopStatus) {
        return new CapturePreviewScenarioResult(
                scenarioKey,
                "stopped",
                stopStatus.summary,
                stopStatus.detail,
                preflight,
                startStatus,
                stopStatus,
                publisherStatus,
                runtimeAfterStart);
    }
}
