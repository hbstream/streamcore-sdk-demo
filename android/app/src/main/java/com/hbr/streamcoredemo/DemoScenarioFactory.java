package com.hbr.streamcoredemo;

import static com.hbr.streamcoredemo.DemoNetworkSupport.defaultDevelopmentRtmpUrl;
import static com.hbr.streamcoredemo.DemoOperationSupport.deferredBootstrapStatus;
import static com.hbr.streamcoredemo.DemoOperationSupport.notApplicableStatus;
import static com.hbr.streamcoredemo.DemoOperationSupport.skippedPushStatus;
import static com.hbr.streamcoredemo.PublisherValidationSupport.buildNativeTransportEncodedVideoPacket;
import static com.hbr.streamcoredemo.PublisherValidationSupport.buildNativeTransportUrl;
import static com.hbr.streamcoredemo.PublisherValidationSupport.buildValidationEncodedAudioPacket;
import static com.hbr.streamcoredemo.PublisherValidationSupport.buildValidationEncodedVideoPacket;
import static com.hbr.streamcoredemo.PublisherValidationSupport.buildValidationRawVideoFrame;
import static com.hbr.streamcoredemo.PublisherValidationSupport.pushValidationRawAudioFrames;
import static com.hbr.streamcoredemo.PublisherValidationSupport.waitForPublisherNativeTransportDrain;
import static com.hbr.streamcoredemo.PublisherValidationSupport.waitForPublisherNativeTransportWarmup;
import static com.hbr.streamcoredemo.PublisherValidationSupport.waitForPublisherValidationDrain;

import com.hbr.streamcore.StreamCoreCapture;
import com.hbr.streamcore.StreamCoreOperationStatus;
import com.hbr.streamcore.StreamCorePlayer;
import com.hbr.streamcore.StreamCorePublisher;

final class DemoScenarioFactory {
    private DemoScenarioFactory() {
    }
    static PlayerScenarioResult buildPlayerScenario() {
        final StreamCorePlayer.Session session = new StreamCorePlayer.Session();
        session.setConfig(StreamCorePlayer.Config.newBuilder()
                .sessionName("android_demo_player")
                .sourceKind(StreamCorePlayer.SourceKind.URL)
                .sourceUrl(defaultDevelopmentRtmpUrl("local_native"))
                .renderMode(StreamCorePlayer.RenderMode.AUTO)
                .build());
        final StreamCorePlayer.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus = deferredBootstrapStatus(
                "player bootstrap keeps playback start deferred.",
                "playback starts only from the player controls.");
        final StreamCorePlayer.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCorePlayer.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PlayerScenarioResult(preflight, startStatus, runtimeAfterStart, runtimeAfterStop);
    }

    static CaptureScenarioResult buildCameraCaptureScenario() {
        final StreamCoreCapture.Session session = new StreamCoreCapture.Session();
        session.setConfig(StreamCoreCapture.Config.newBuilder()
                .sessionName("android_demo_capture_camera")
                .sourceKind(StreamCoreCapture.SourceKind.CAMERA)
                .sourceId("front_camera")
                .targetFrameRate(30)
                .enableAudio(false)
                .enableVideo(true)
                .build());
        final StreamCoreCapture.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus = deferredBootstrapStatus(
                "camera capture bootstrap keeps runtime start deferred.",
                "camera capture starts only from the capture controls.");
        final StreamCoreCapture.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCoreCapture.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new CaptureScenarioResult(preflight, startStatus, runtimeAfterStart, runtimeAfterStop);
    }

    static PublisherScenarioResult buildLocalCapturePublisherScenario() {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_in_bootstrap"};
        session.setConfig(StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_local")
                .publishUrl(defaultDevelopmentRtmpUrl("local"))
                .inputKind(StreamCorePublisher.InputKind.LOCAL_CAPTURE)
                .inputBindingId("camera_main")
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary)
                .build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus = deferredBootstrapStatus(
                "local capture publisher bootstrap keeps transport start deferred.",
                "RTMP transport starts only from the publisher controls.");
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                "local_capture",
                preflight,
                startStatus,
                notApplicableStatus(),
                notApplicableStatus(),
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }

    static PublisherScenarioResult buildRawFeedPublisherScenario(boolean validationHost) {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {
                validationHost ? "not_triggered_in_runtime_check" : "not_triggered_in_bootstrap"
        };
        final StreamCorePublisher.Config.Builder builder = StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_raw")
                .publishUrl(defaultDevelopmentRtmpUrl("raw"))
                .inputKind(StreamCorePublisher.InputKind.APP_RAW_FEED)
                .inputBindingId("raw_feed")
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary);
        if (validationHost) {
            builder.androidTransportPolicy(
                    StreamCorePublisher.AndroidTransportPolicy.FORCE_VALIDATION_BACKEND);
        }
        session.setConfig(builder.build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus;
        final StreamCoreOperationStatus audioPushStatus;
        final StreamCoreOperationStatus videoPushStatus;
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart;
        final String scenarioKey;
        if (validationHost) {
            // 模拟器上显式跑一轮 start + push + stop，确保校验后端的正式链路真实闭合。
            scenarioKey = "raw_feed_runtime_check";
            startStatus = session.start();
            if (startStatus.isOk()) {
                audioPushStatus = pushValidationRawAudioFrames(session);
                videoPushStatus = session.pushRawVideoFrame(buildValidationRawVideoFrame());
                waitForPublisherValidationDrain();
            } else {
                audioPushStatus = skippedPushStatus(
                        "publisher raw-feed skipped audio push after start failure.",
                        "startStatus=" + startStatus.statusName);
                videoPushStatus = skippedPushStatus(
                        "publisher raw-feed skipped video push after start failure.",
                        "startStatus=" + startStatus.statusName);
            }
            runtimeAfterStart = session.getRuntimeInfo();
        } else {
            scenarioKey = "raw_feed_contract";
            startStatus = deferredBootstrapStatus(
                    "raw-feed publisher bootstrap keeps transport start deferred.",
                    "raw-feed publisher is ready for explicit start.");
            audioPushStatus = notApplicableStatus();
            videoPushStatus = notApplicableStatus();
            runtimeAfterStart = session.getRuntimeInfo();
        }
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                scenarioKey,
                preflight,
                startStatus,
                audioPushStatus,
                videoPushStatus,
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }

    static PublisherScenarioResult buildEncodedFeedForcedScenario(boolean validationHost) {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {
                validationHost ? "not_triggered_in_runtime_check" : "not_triggered_in_bootstrap"
        };
        final StreamCorePublisher.Config.Builder builder = StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_encoded")
                .publishUrl(defaultDevelopmentRtmpUrl("encoded"))
                .inputKind(StreamCorePublisher.InputKind.APP_ENCODED_FEED)
                .inputBindingId("encoded_feed")
                .sourceMediaProfile(StreamCorePublisher.SourceMediaProfile.newBuilder()
                        .containerName("flv")
                        .audioCodecName("aac")
                        .videoCodecName("h264")
                        .hasAudio(true)
                        .hasVideo(true)
                        .build())
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary);
        if (validationHost) {
            builder.androidTransportPolicy(
                    StreamCorePublisher.AndroidTransportPolicy.FORCE_VALIDATION_BACKEND);
        } else {
            builder.transcodeOptions(StreamCorePublisher.TranscodeOptions.newBuilder()
                    .audioMode(StreamCorePublisher.TranscodeMode.FORCE_TRANSCODE)
                    .videoMode(StreamCorePublisher.TranscodeMode.FORCE_TRANSCODE)
                    .targetAudioCodecName("aac")
                    .targetVideoCodecName("h264")
                    .build());
        }
        session.setConfig(builder.build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus;
        final StreamCoreOperationStatus audioPushStatus;
        final StreamCoreOperationStatus videoPushStatus;
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart;
        final String scenarioKey;
        if (validationHost) {
            scenarioKey = "encoded_feed_runtime_check";
            startStatus = session.start();
            if (startStatus.isOk()) {
                audioPushStatus = session.pushEncodedAudioPacket(buildValidationEncodedAudioPacket());
                videoPushStatus = session.pushEncodedVideoPacket(buildValidationEncodedVideoPacket());
                waitForPublisherValidationDrain();
            } else {
                audioPushStatus = skippedPushStatus(
                        "publisher encoded-feed skipped audio push after start failure.",
                        "startStatus=" + startStatus.statusName);
                videoPushStatus = skippedPushStatus(
                        "publisher encoded-feed skipped video push after start failure.",
                        "startStatus=" + startStatus.statusName);
            }
            runtimeAfterStart = session.getRuntimeInfo();
        } else {
            scenarioKey = "encoded_feed_forced";
            startStatus = deferredBootstrapStatus(
                    "encoded-feed publisher bootstrap keeps transport start deferred.",
                    "encoded-feed publisher is ready for explicit start.");
            audioPushStatus = notApplicableStatus();
            videoPushStatus = notApplicableStatus();
            runtimeAfterStart = session.getRuntimeInfo();
        }
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                scenarioKey,
                preflight,
                startStatus,
                audioPushStatus,
                videoPushStatus,
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }

    static PublisherScenarioResult buildRawFeedNativeTransportScenario() {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_in_native_transport"};
        session.setConfig(StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_raw_native")
                .publishUrl(buildNativeTransportUrl("raw_native"))
                .inputKind(StreamCorePublisher.InputKind.APP_RAW_FEED)
                .inputBindingId("raw_feed_native")
                .androidTransportPolicy(
                        StreamCorePublisher.AndroidTransportPolicy.FORCE_NATIVE_TRANSPORT)
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary)
                .build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus = session.start();
        final StreamCoreOperationStatus audioPushStatus;
        final StreamCoreOperationStatus videoPushStatus;
        if (startStatus.isOk()) {
            waitForPublisherNativeTransportWarmup();
            audioPushStatus = pushValidationRawAudioFrames(session);
            videoPushStatus = session.pushRawVideoFrame(buildValidationRawVideoFrame());
            waitForPublisherNativeTransportDrain();
        } else {
            audioPushStatus = skippedPushStatus(
                    "publisher raw native-transport regression skipped audio push after start failure.",
                    "startStatus=" + startStatus.statusName);
            videoPushStatus = skippedPushStatus(
                    "publisher raw native-transport regression skipped video push after start failure.",
                    "startStatus=" + startStatus.statusName);
        }
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                "raw_feed_native_transport",
                preflight,
                startStatus,
                audioPushStatus,
                videoPushStatus,
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }

    static PublisherScenarioResult buildEncodedFeedNativeTransportScenario() {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_in_native_transport"};
        session.setConfig(StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_encoded_native")
                .publishUrl(buildNativeTransportUrl("encoded_native"))
                .inputKind(StreamCorePublisher.InputKind.APP_ENCODED_FEED)
                .inputBindingId("encoded_feed_native")
                .androidTransportPolicy(
                        StreamCorePublisher.AndroidTransportPolicy.FORCE_NATIVE_TRANSPORT)
                .sourceMediaProfile(StreamCorePublisher.SourceMediaProfile.newBuilder()
                        .containerName("annexb")
                        .audioCodecName("aac")
                        .videoCodecName("h264")
                        .hasAudio(true)
                        .hasVideo(true)
                        .audioFormat(48000, 2)
                        .videoFormat(64, 64, 25)
                        .build())
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary)
                .build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus = session.start();
        final StreamCoreOperationStatus audioPushStatus;
        final StreamCoreOperationStatus videoPushStatus;
        if (startStatus.isOk()) {
            waitForPublisherNativeTransportWarmup();
            audioPushStatus = session.pushEncodedAudioPacket(buildValidationEncodedAudioPacket());
            videoPushStatus = session.pushEncodedVideoPacket(buildNativeTransportEncodedVideoPacket());
            waitForPublisherNativeTransportDrain();
        } else {
            audioPushStatus = skippedPushStatus(
                    "publisher encoded native-transport regression skipped audio push after start failure.",
                    "startStatus=" + startStatus.statusName);
            videoPushStatus = skippedPushStatus(
                    "publisher encoded native-transport regression skipped video push after start failure.",
                    "startStatus=" + startStatus.statusName);
        }
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                "encoded_feed_native_transport",
                preflight,
                startStatus,
                audioPushStatus,
                videoPushStatus,
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }

    static PublisherScenarioResult buildMediaFilePassthroughScenario() {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_in_bootstrap"};
        session.setConfig(StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_file")
                .publishUrl(defaultDevelopmentRtmpUrl("file"))
                .inputKind(StreamCorePublisher.InputKind.APP_ENCODED_FEED)
                .inputBindingId("file:/sdcard/Movies/demo.mp4")
                .sourceMediaProfile(StreamCorePublisher.SourceMediaProfile.newBuilder()
                        .containerName("mp4")
                        .audioCodecName("aac")
                        .videoCodecName("h264")
                        .hasAudio(true)
                        .hasVideo(true)
                        .build())
                .transcodeOptions(StreamCorePublisher.TranscodeOptions.newBuilder()
                        .audioMode(StreamCorePublisher.TranscodeMode.AUTO)
                        .videoMode(StreamCorePublisher.TranscodeMode.AUTO)
                        .targetAudioCodecName("aac")
                        .targetVideoCodecName("h264")
                        .build())
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary)
                .build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus = deferredBootstrapStatus(
                "media-file publisher bootstrap keeps transport start deferred.",
                "media-file publisher is ready for explicit start.");
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                "media_file_passthrough",
                preflight,
                startStatus,
                notApplicableStatus(),
                notApplicableStatus(),
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }

    static PublisherScenarioResult buildStillImageScenario() {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_in_bootstrap"};
        session.setConfig(StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_image")
                .publishUrl(defaultDevelopmentRtmpUrl("image"))
                .inputKind(StreamCorePublisher.InputKind.APP_RAW_FEED)
                .inputBindingId("image:/sdcard/Pictures/demo.jpg")
                .sourceMediaProfile(StreamCorePublisher.SourceMediaProfile.newBuilder()
                        .containerName("raw")
                        .audioCodecName("")
                        .videoCodecName("raw")
                        .hasAudio(false)
                        .hasVideo(true)
                        .videoFormat(1280, 720, 25)
                        .build())
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary)
                .build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus = deferredBootstrapStatus(
                "still-image publisher bootstrap keeps transport start deferred.",
                "still-image publisher is ready for explicit start.");
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                "still_image_fixed_fps",
                preflight,
                startStatus,
                notApplicableStatus(),
                notApplicableStatus(),
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }

}
