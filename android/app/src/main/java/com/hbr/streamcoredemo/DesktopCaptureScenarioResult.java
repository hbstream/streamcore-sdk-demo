package com.hbr.streamcoredemo;

import com.hbr.streamcore.StreamCoreCapture;
import com.hbr.streamcore.StreamCoreOperationStatus;
import com.hbr.streamcore.StreamCoreResultCode;

final class DesktopCaptureScenarioResult {
    final String lifecycleState;
    final String summary;
    final String detail;
    final StreamCoreCapture.Preflight preflight;
    final StreamCoreOperationStatus startStatus;
    final StreamCoreOperationStatus stopStatus;
    final StreamCoreCapture.RuntimeInfo runtimeAfterStart;
    final StreamCoreCapture.RuntimeInfo runtimeAfterStop;

    private DesktopCaptureScenarioResult(
            String lifecycleState,
            String summary,
            String detail,
            StreamCoreCapture.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreOperationStatus stopStatus,
            StreamCoreCapture.RuntimeInfo runtimeAfterStart,
            StreamCoreCapture.RuntimeInfo runtimeAfterStop) {
        this.lifecycleState = lifecycleState;
        this.summary = summary;
        this.detail = detail;
        this.preflight = preflight;
        this.startStatus = startStatus;
        this.stopStatus = stopStatus;
        this.runtimeAfterStart = runtimeAfterStart;
        this.runtimeAfterStop = runtimeAfterStop;
    }

    static DesktopCaptureScenarioResult idle() {
        return new DesktopCaptureScenarioResult(
                "idle",
                "desktop capture is idle.",
                "screen-capture permission is not active.",
                null,
                null,
                null,
                null,
                null);
    }

    static DesktopCaptureScenarioResult awaitingGrant() {
        return new DesktopCaptureScenarioResult(
                "awaiting_grant",
                "waiting for MediaProjection permission result.",
                "foreground service will start after permission is granted.",
                null,
                null,
                null,
                null,
                null);
    }

    static DesktopCaptureScenarioResult starting() {
        return new DesktopCaptureScenarioResult(
                "starting",
                "desktop capture grant received; starting ScreenCaptureService.",
                "foreground capture service is starting.",
                null,
                null,
                null,
                null,
                null);
    }

    static DesktopCaptureScenarioResult failedBeforeGrant(
            StreamCoreOperationStatus startStatus) {
        return new DesktopCaptureScenarioResult(
                "grant_launch_failed",
                startStatus.summary,
                startStatus.detail,
                null,
                startStatus,
                null,
                null,
                null);
    }

    static DesktopCaptureScenarioResult grantRejected(int resultCode, boolean hasData) {
        return new DesktopCaptureScenarioResult(
                "grant_rejected",
                "MediaProjection permission was denied or returned incomplete data.",
                "resultCode=" + resultCode + ", hasData=" + hasData,
                null,
                new StreamCoreOperationStatus(
                        StreamCoreResultCode.INVALID_ARGUMENT,
                        "desktop_capture_projection_grant_denied",
                        "desktop capture grant was not approved.",
                        "MediaProjection result was not usable."),
                null,
                null,
                null);
    }

    static DesktopCaptureScenarioResult started(
            StreamCoreCapture.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreCapture.RuntimeInfo runtimeAfterStart) {
        return new DesktopCaptureScenarioResult(
                "running",
                "desktop capture session is using the formal ScreenCaptureService path.",
                "Use the stop button to release the foreground service and current capture session.",
                preflight,
                startStatus,
                null,
                runtimeAfterStart,
                null);
    }

    static DesktopCaptureScenarioResult failedAfterStart(
            StreamCoreCapture.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreCapture.RuntimeInfo runtimeAfterStart,
            StreamCoreCapture.RuntimeInfo runtimeAfterStop) {
        return new DesktopCaptureScenarioResult(
                "start_failed",
                "desktop capture grant was received but runtime start did not succeed.",
                "Preflight and start details show whether permission, service binding, or runtime checks blocked startup.",
                preflight,
                startStatus,
                null,
                runtimeAfterStart,
                runtimeAfterStop);
    }

    DesktopCaptureScenarioResult withStop(
            StreamCoreOperationStatus stopStatus,
            StreamCoreCapture.RuntimeInfo runtimeAfterStop,
            String lifecycleState,
            String summary,
            String detail) {
        return new DesktopCaptureScenarioResult(
                lifecycleState,
                summary,
                detail,
                preflight,
                startStatus,
                stopStatus,
                runtimeAfterStart,
                runtimeAfterStop);
    }
}
