package com.hbr.streamcoredemo;

import androidx.annotation.NonNull;

import com.hbr.streamcore.StreamCoreOperationStatus;
import com.hbr.streamcore.StreamCoreResultCode;

final class DemoOperationSupport {
    private DemoOperationSupport() {
    }

    @NonNull
    static StreamCoreOperationStatus notApplicableStatus() {
        return new StreamCoreOperationStatus(
                StreamCoreResultCode.OK,
                "not_applicable",
                "not applicable",
                "this scenario does not call the corresponding formal feed entry.");
    }

    @NonNull
    static StreamCoreOperationStatus skippedPushStatus(
            @NonNull String summary,
            @NonNull String detail) {
        return new StreamCoreOperationStatus(
                StreamCoreResultCode.OK,
                "push_skipped",
                summary,
                detail);
    }

    @NonNull
    static StreamCoreOperationStatus deferredBootstrapStatus(
            @NonNull String summary,
            @NonNull String detail) {
        return new StreamCoreOperationStatus(
                StreamCoreResultCode.OK,
                "bootstrap_deferred",
                summary,
                detail);
    }

    @NonNull
    static StreamCoreOperationStatus buildStatus(
            int resultCode,
            @NonNull String statusName,
            @NonNull String summary,
            @NonNull String detail) {
        return new StreamCoreOperationStatus(resultCode, statusName, summary, detail);
    }

    @NonNull
    static String safeMessage(RuntimeException failure) {
        if (failure == null || failure.getMessage() == null) {
            return "runtime exception";
        }
        return failure.getMessage();
    }
}
