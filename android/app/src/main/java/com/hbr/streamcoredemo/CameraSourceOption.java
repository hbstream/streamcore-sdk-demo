package com.hbr.streamcoredemo;

final class CameraSourceOption {
    final String label;
    final String sourceId;
    final boolean frontFacing;
    final java.util.List<PublisherResolutionOption> resolutions;

    CameraSourceOption(
            String label,
            String sourceId,
            boolean frontFacing,
            java.util.List<PublisherResolutionOption> resolutions) {
        this.label = label;
        this.sourceId = sourceId;
        this.frontFacing = frontFacing;
        this.resolutions = resolutions == null
                ? java.util.Collections.emptyList()
                : java.util.Collections.unmodifiableList(resolutions);
    }
}
