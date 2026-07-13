package com.hbr.streamcoredemo;

import static com.hbr.streamcoredemo.DemoNetworkSupport.DEFAULT_DEVELOPMENT_RTMP_HOST;
import static com.hbr.streamcoredemo.DemoNetworkSupport.EMULATOR_HOST_LOOPBACK;
import static com.hbr.streamcoredemo.DemoNetworkSupport.RTMP_PORT;
import static com.hbr.streamcoredemo.DemoNetworkSupport.RTMP_REACHABILITY_TIMEOUT_MS;
import static com.hbr.streamcoredemo.DemoNetworkSupport.isTcpEndpointReachable;

import android.os.Build;

import com.hbr.streamcore.StreamCoreOperationStatus;
import com.hbr.streamcore.StreamCorePublisher;
import com.hbr.streamcore.StreamCoreResultCode;

import java.util.Locale;

final class PublisherValidationSupport {
    private PublisherValidationSupport() {
    }

    static boolean shouldRunPublisherValidationScenario() {
        return containsAnyIgnoreCase(Build.FINGERPRINT, "generic", "emulator", "sdk_gphone", "vbox")
                || containsAnyIgnoreCase(
                Build.MODEL,
                "sdk",
                "emulator",
                "android sdk built for",
                "aosp")
                || containsAnyIgnoreCase(Build.PRODUCT, "sdk", "emulator", "sdk_gphone", "sdk_phone")
                || containsAnyIgnoreCase(Build.MANUFACTURER, "genymotion")
                || containsAnyIgnoreCase(Build.BRAND, "generic", "android");
    }

    static String resolvePublisherNativeTransportHost() {
        if (shouldRunPublisherValidationScenario()
                && isTcpEndpointReachable(
                EMULATOR_HOST_LOOPBACK,
                RTMP_PORT,
                RTMP_REACHABILITY_TIMEOUT_MS)) {
            return EMULATOR_HOST_LOOPBACK;
        }
        if (isTcpEndpointReachable(
                DEFAULT_DEVELOPMENT_RTMP_HOST,
                RTMP_PORT,
                RTMP_REACHABILITY_TIMEOUT_MS)) {
            return DEFAULT_DEVELOPMENT_RTMP_HOST;
        }
        return null;
    }

    static String buildNativeTransportUrl(String streamName) {
        final String host = resolvePublisherNativeTransportHost();
        return buildNativeTransportUrlForHost(streamName, host);
    }

    static void waitForPublisherValidationDrain() {
        sleepQuietly(250L);
    }

    static void waitForPublisherNativeTransportWarmup() {
        sleepQuietly(600L);
    }

    static void waitForPublisherNativeTransportDrain() {
        sleepQuietly(1200L);
    }

    static void waitForPublisherLocalCaptureTransportDrain() {
        sleepQuietly(9000L);
    }

    static StreamCoreOperationStatus pushValidationRawAudioFrames(
            StreamCorePublisher.Session session) {
        StreamCoreOperationStatus lastStatus = new StreamCoreOperationStatus(
                StreamCoreResultCode.NOT_ENABLED,
                "not_applicable",
                "not applicable",
                "");
        for (int i = 0; i < 3; ++i) {
            lastStatus = session.pushRawAudioFrame(buildValidationRawAudioFrame(i * 24L));
            if (!lastStatus.isOk()) {
                return lastStatus;
            }
        }
        return lastStatus;
    }

    static StreamCorePublisher.RawVideoFrame buildValidationRawVideoFrame() {
        final int width = 64;
        final int height = 64;
        final int bytesPerPixel = 4;
        final byte[] frameData = new byte[width * height * bytesPerPixel];
        for (int i = 0; i + 3 < frameData.length; i += bytesPerPixel) {
            frameData[i] = 0x20;
            frameData[i + 1] = 0x40;
            frameData[i + 2] = (byte) 0x80;
            frameData[i + 3] = (byte) 0xFF;
        }
        return StreamCorePublisher.RawVideoFrame.newBuilder()
                .data(frameData)
                .width(width)
                .height(height)
                .strideBytes(width * bytesPerPixel)
                .pixelFormat(StreamCorePublisher.VideoPixelFormat.RGB32)
                .timestampMs(33L)
                .build();
    }

    static StreamCorePublisher.EncodedPacket buildValidationEncodedAudioPacket() {
        return StreamCorePublisher.EncodedPacket.newBuilder()
                .data(new byte[] {(byte) 0x12, (byte) 0x10, 0x56, (byte) 0xE5})
                .codecName("aac")
                .keyFrame(false)
                .timestampMs(0L)
                .build();
    }

    static StreamCorePublisher.EncodedPacket buildValidationEncodedVideoPacket() {
        return StreamCorePublisher.EncodedPacket.newBuilder()
                .data(new byte[] {0x00, 0x00, 0x00, 0x01, 0x65, (byte) 0x88, (byte) 0x84, 0x21})
                .codecName("h264")
                .keyFrame(true)
                .timestampMs(33L)
                .build();
    }

    static StreamCorePublisher.EncodedPacket buildNativeTransportEncodedVideoPacket() {
        return StreamCorePublisher.EncodedPacket.newBuilder()
                .data(new byte[] {
                        0x00, 0x00, 0x00, 0x01,
                        0x67, 0x42, (byte) 0xC0, 0x1E, (byte) 0xDA, 0x02, (byte) 0x80, 0x2D,
                        (byte) 0xD0, (byte) 0x80, 0x00, 0x00, 0x03, 0x00, (byte) 0x80, 0x00,
                        0x00, 0x19, 0x47, (byte) 0x8B, 0x17, 0x24,
                        0x00, 0x00, 0x00, 0x01,
                        0x68, (byte) 0xCE, 0x3C, (byte) 0x80,
                        0x00, 0x00, 0x00, 0x01,
                        0x65, (byte) 0x88, (byte) 0x84, 0x00, 0x0A, (byte) 0xF2, 0x62, (byte) 0x80
                })
                .codecName("h264")
                .keyFrame(true)
                .timestampMs(33L)
                .build();
    }

    private static String buildNativeTransportUrlForHost(String streamName, String host) {
        final String fallbackHost = shouldRunPublisherValidationScenario()
                ? EMULATOR_HOST_LOOPBACK
                : DEFAULT_DEVELOPMENT_RTMP_HOST;
        return "rtmp://"
                + (host == null ? fallbackHost : host)
                + ':'
                + RTMP_PORT
                + "/live/"
                + streamName;
    }

    private static StreamCorePublisher.RawAudioFrame buildValidationRawAudioFrame(long timestampMs) {
        final int sampleRate = 48000;
        final int channelCount = 2;
        final int bytesPerSample = 2;
        final int frameSamples = 1024;
        final byte[] pcmData = new byte[frameSamples * channelCount * bytesPerSample];
        return StreamCorePublisher.RawAudioFrame.newBuilder()
                .data(pcmData)
                .sampleRate(sampleRate)
                .channelCount(channelCount)
                .bitsPerSample(16)
                .timestampMs(timestampMs)
                .build();
    }

    private static boolean containsAnyIgnoreCase(String value, String... candidates) {
        if (value == null || value.isEmpty()) {
            return false;
        }
        final String normalized = value.toLowerCase(Locale.US);
        for (String candidate : candidates) {
            if (normalized.contains(candidate.toLowerCase(Locale.US))) {
                return true;
            }
        }
        return false;
    }

    private static void sleepQuietly(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException failure) {
            Thread.currentThread().interrupt();
        }
    }
}
