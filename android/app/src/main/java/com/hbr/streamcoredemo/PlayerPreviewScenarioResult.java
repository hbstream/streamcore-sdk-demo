package com.hbr.streamcoredemo;

import com.hbr.streamcore.StreamCoreOperationStatus;
import com.hbr.streamcore.StreamCorePlayer;

final class PlayerPreviewScenarioResult {
    final String lifecycleState;
    final String summary;
    final String detail;
    final StreamCorePlayer.Preflight preflight;
    final StreamCoreOperationStatus startStatus;
    final StreamCoreOperationStatus stopStatus;
    final StreamCorePlayer.RuntimeInfo runtimeAfterStart;

    private PlayerPreviewScenarioResult(
            String lifecycleState,
            String summary,
            String detail,
            StreamCorePlayer.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreOperationStatus stopStatus,
            StreamCorePlayer.RuntimeInfo runtimeAfterStart) {
        this.lifecycleState = lifecycleState;
        this.summary = summary;
        this.detail = detail;
        this.preflight = preflight;
        this.startStatus = startStatus;
        this.stopStatus = stopStatus;
        this.runtimeAfterStart = runtimeAfterStart;
    }

    static PlayerPreviewScenarioResult idle() {
        return new PlayerPreviewScenarioResult(
                "idle",
                "player preview is idle.",
                "playback URL is editable.",
                null,
                null,
                null,
                null);
    }

    static PlayerPreviewScenarioResult starting() {
        return new PlayerPreviewScenarioResult(
                "starting",
                "player preview is binding the TextureView and starting native playback.",
                "The session is using StreamCorePlayer.Config.renderTarget().",
                null,
                null,
                null,
                null);
    }

    static PlayerPreviewScenarioResult blocked(StreamCoreOperationStatus status) {
        return new PlayerPreviewScenarioResult(
                "blocked",
                status.summary,
                status.detail,
                null,
                status,
                null,
                null);
    }

    static PlayerPreviewScenarioResult started(
            StreamCorePlayer.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCorePlayer.RuntimeInfo runtimeAfterStart) {
        return new PlayerPreviewScenarioResult(
                "running",
                "player preview is running on the visible TextureView.",
                "Stop the preview before the TextureView is destroyed.",
                preflight,
                startStatus,
                null,
                runtimeAfterStart);
    }

    static PlayerPreviewScenarioResult failed(
            StreamCorePlayer.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCorePlayer.RuntimeInfo runtimeAfterStart) {
        return new PlayerPreviewScenarioResult(
                "start_failed",
                "player preview did not enter the running state.",
                "Start status and runtime snapshot are shown below.",
                preflight,
                startStatus,
                null,
                runtimeAfterStart);
    }

    PlayerPreviewScenarioResult withStop(StreamCoreOperationStatus stopStatus) {
        return new PlayerPreviewScenarioResult(
                "stopped",
                stopStatus.summary,
                stopStatus.detail,
                preflight,
                startStatus,
                stopStatus,
                runtimeAfterStart);
    }
}
