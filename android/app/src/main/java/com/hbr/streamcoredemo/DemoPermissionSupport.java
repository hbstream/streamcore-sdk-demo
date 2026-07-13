package com.hbr.streamcoredemo;

import android.content.Context;

import androidx.annotation.NonNull;

import com.hbr.streamcore.StreamCoreAndroidPermissions;
import com.hbr.streamcore.StreamCoreCapture;
import com.hbr.streamcore.StreamCorePublisher;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

final class DemoPermissionSupport {
    private DemoPermissionSupport() {
    }

    static boolean hasCameraPreviewPermission(@NonNull Context context) {
        return missingCameraPreviewPermissions(context).length == 0;
    }

    static boolean hasMicrophoneCapturePermission(@NonNull Context context) {
        return missingMicrophoneCapturePermissions(context).length == 0;
    }

    @NonNull
    static String[] missingCameraPreviewPermissions(@NonNull Context context) {
        return missingCapturePermissions(context, cameraPreviewPermissionConfig());
    }

    @NonNull
    static String[] missingCameraAndMicrophonePermissions(@NonNull Context context) {
        final List<String> result = new ArrayList<>();
        addAllUnique(result, missingCameraPreviewPermissions(context));
        addAllUnique(result, missingMicrophoneCapturePermissions(context));
        return result.toArray(new String[0]);
    }

    static boolean hasPublisherLocalCapturePermissions(
            @NonNull Context context,
            @NonNull StreamCorePublisher.Config config) {
        return missingPublisherLocalCapturePermissions(context, config).length == 0;
    }

    @NonNull
    static String[] missingPublisherLocalCapturePermissions(
            @NonNull Context context,
            @NonNull StreamCorePublisher.Config config) {
        final List<String> permissions =
                StreamCoreAndroidPermissions.runtimePermissionsForPublisher(config);
        return toPermissionArray(
                StreamCoreAndroidPermissions.missingRuntimePermissions(context, permissions));
    }

    @NonNull
    private static String[] missingMicrophoneCapturePermissions(@NonNull Context context) {
        return missingCapturePermissions(context, microphonePermissionConfig());
    }

    @NonNull
    private static String[] missingCapturePermissions(
            @NonNull Context context,
            @NonNull StreamCoreCapture.Config config) {
        final List<String> permissions =
                StreamCoreAndroidPermissions.runtimePermissionsForCapture(config);
        return toPermissionArray(
                StreamCoreAndroidPermissions.missingRuntimePermissions(context, permissions));
    }

    @NonNull
    private static StreamCoreCapture.Config cameraPreviewPermissionConfig() {
        return StreamCoreCapture.Config.newBuilder()
                .sessionName("android_demo_permission_camera_preview")
                .sourceKind(StreamCoreCapture.SourceKind.CAMERA)
                .enableAudio(false)
                .enableVideo(true)
                .build();
    }

    @NonNull
    private static StreamCoreCapture.Config microphonePermissionConfig() {
        return StreamCoreCapture.Config.newBuilder()
                .sessionName("android_demo_permission_microphone")
                .sourceKind(StreamCoreCapture.SourceKind.MICROPHONE)
                .enableAudio(true)
                .enableVideo(false)
                .build();
    }

    private static void addAllUnique(@NonNull List<String> target, @NonNull String[] values) {
        for (String value : values) {
            if (value != null && !value.isEmpty() && !target.contains(value)) {
                target.add(value);
            }
        }
    }

    @NonNull
    private static String[] toPermissionArray(@NonNull Collection<String> permissions) {
        return permissions.toArray(new String[0]);
    }
}
