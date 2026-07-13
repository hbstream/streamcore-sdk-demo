package com.hbr.streamcoredemo;

import com.hbr.streamcore.StreamCoreOperationStatus;
import com.hbr.streamcore.StreamCorePlayer;

final class PlayerScenarioResult {
    final StreamCorePlayer.Preflight preflight;
    final StreamCoreOperationStatus startStatus;
    final StreamCorePlayer.RuntimeInfo runtimeAfterStart;
    final StreamCorePlayer.RuntimeInfo runtimeAfterStop;

    PlayerScenarioResult(
            StreamCorePlayer.Preflight preflight,
            StreamCoreOperationStatus startStatus,
            StreamCorePlayer.RuntimeInfo runtimeAfterStart,
            StreamCorePlayer.RuntimeInfo runtimeAfterStop) {
        this.preflight = preflight;
        this.startStatus = startStatus;
        this.runtimeAfterStart = runtimeAfterStart;
        this.runtimeAfterStop = runtimeAfterStop;
    }
}
