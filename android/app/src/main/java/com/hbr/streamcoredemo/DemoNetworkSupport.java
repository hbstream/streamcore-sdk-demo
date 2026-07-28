package com.hbr.streamcoredemo;

import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.Socket;
import java.net.SocketException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;

final class DemoNetworkSupport {
    static final String EMULATOR_HOST_LOOPBACK = "10.0.2.2";
    static final int RTMP_PORT = 1935;
    static final int RTSP_PORT = 8554;
    static final String DEFAULT_DEVELOPMENT_RTMP_HOST = "192.0.2.1";
    static final int RTMP_REACHABILITY_TIMEOUT_MS = 400;

    private DemoNetworkSupport() {
    }

    static String defaultDevelopmentRtmpUrl(String streamName) {
        return "rtmp://"
                + DEFAULT_DEVELOPMENT_RTMP_HOST
                + ':'
                + RTMP_PORT
                + "/live/"
                + streamName;
    }

    static String defaultDevelopmentRtspUrl(String streamName) {
        return "rtsp://"
                + DEFAULT_DEVELOPMENT_RTMP_HOST
                + ':'
                + RTSP_PORT
                + "/live/"
                + streamName;
    }

    static String resolveFirstNonLoopbackIpv4(String fallback) {
        try {
            final Enumeration<NetworkInterface> interfaces =
                    NetworkInterface.getNetworkInterfaces();
            while (interfaces != null && interfaces.hasMoreElements()) {
                final NetworkInterface networkInterface = interfaces.nextElement();
                if (!networkInterface.isUp() || networkInterface.isLoopback()) {
                    continue;
                }
                final Enumeration<InetAddress> addresses =
                        networkInterface.getInetAddresses();
                while (addresses.hasMoreElements()) {
                    final InetAddress address = addresses.nextElement();
                    if (address instanceof Inet4Address && !address.isLoopbackAddress()) {
                        return address.getHostAddress();
                    }
                }
            }
        } catch (SocketException ignored) {
            return fallback;
        }
        return fallback;
    }

    static boolean isRtmpUrlSyntaxValid(String url) {
        return isPublisherUrlSyntaxValid(url);
    }

    static boolean isPublisherUrlSyntaxValid(String url) {
        try {
            final URI uri = new URI(url);
            return isPublisherScheme(uri.getScheme())
                    && uri.getHost() != null
                    && !uri.getHost().isEmpty();
        } catch (URISyntaxException failure) {
            return false;
        }
    }

    static boolean isRtmpUrlReachable(String url) {
        return isPublisherUrlReachable(url);
    }

    static boolean isPublisherUrlReachable(String url) {
        try {
            final URI uri = new URI(url);
            if (!isPublisherScheme(uri.getScheme())
                    || uri.getHost() == null
                    || uri.getHost().isEmpty()) {
                return false;
            }
            final int defaultPort =
                    "rtsp".equalsIgnoreCase(uri.getScheme()) ? RTSP_PORT : RTMP_PORT;
            final int port = uri.getPort() > 0 ? uri.getPort() : defaultPort;
            return isTcpEndpointReachable(
                    uri.getHost(),
                    port,
                    RTMP_REACHABILITY_TIMEOUT_MS);
        } catch (URISyntaxException failure) {
            return false;
        }
    }

    private static boolean isPublisherScheme(String scheme) {
        return "rtmp".equalsIgnoreCase(scheme)
                || "rtsp".equalsIgnoreCase(scheme);
    }

    static boolean isTcpEndpointReachable(
            String host,
            int port,
            int timeoutMs) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(host, port), timeoutMs);
            return true;
        } catch (IOException failure) {
            return false;
        }
    }
}
