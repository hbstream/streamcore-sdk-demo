package com.hbr.streamcoredemo;

import com.hbr.streamcore.StreamCoreOperationStatus;
import com.hbr.streamcore.StreamCorePublisher;

final class PublisherScenarioResult {
    final String scenarioKey;
    final StreamCorePublisher.Preflight preflight;
    final StreamCoreOperationStatus startStatus;
    final StreamCoreOperationStatus audioPushStatus;
    final StreamCoreOperationStatus videoPushStatus;
    final StreamCorePublisher.RuntimeInfo runtimeAfterStart;
    final StreamCorePublisher.RuntimeInfo runtimeAfterStop;
    final String callbackMessage;

    PublisherScenarioResult(
            String scenarioKey,
            StreamCorePublisher.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCoreOperationStatus audioPushStatus,
            StreamCoreOperationStatus videoPushStatus,
            StreamCorePublisher.RuntimeInfo runtimeAfterStart,
            StreamCorePublisher.RuntimeInfo runtimeAfterStop,
            String callbackMessage) {
        this.scenarioKey = scenarioKey;
        this.preflight = preflight;
        this.startStatus = startStatus;
        this.audioPushStatus = audioPushStatus;
        this.videoPushStatus = videoPushStatus;
        this.runtimeAfterStart = runtimeAfterStart;
        this.runtimeAfterStop = runtimeAfterStop;
        this.callbackMessage = callbackMessage;
    }
}
