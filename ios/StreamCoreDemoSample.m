/*******************************************************************************
 * StreamCoreDemoSample.m
 * Copyright (c) 2026 HBRun.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Small Objective-C public API sample for the StreamCore SDK demo.
 ******************************************************************************/

#import <Foundation/Foundation.h>

#if __has_include(<StreamCoreSDK/StreamCoreSDK.h>)
#import <StreamCoreSDK/StreamCoreSDK.h>
#elif __has_include("StreamCoreSDK.h")
#import "StreamCoreSDK.h"
#else
#error "StreamCoreSDK public header is missing. Add StreamCoreSDK.xcframework or the released public header path to the target."
#endif

static void PrintStatus(NSString* title, HBRStreamCoreOperationStatus* status)
{
    NSLog(@"%@: code=%ld name=%@ summary=%@ detail=%@",
          title,
          (long)status.resultCode,
          status.statusName,
          status.summary,
          status.detail);
}

static void PrintPlayerRuntimeInfo(HBRStreamCorePlayerRuntimeInfo* info)
{
    NSLog(@"player runtime: state=%ld hasConfig=%d lastReady=%d lastLicense=%d source=%@ summary=%@",
          (long)info.sessionState,
          info.hasConfig,
          info.lastPreflightReady,
          info.lastLicenseValid,
          info.sourceIdentity,
          info.stateSummary);
}

static void PrintCaptureRuntimeInfo(HBRStreamCoreCaptureRuntimeInfo* info)
{
    NSLog(@"capture runtime: state=%ld hasConfig=%d lastReady=%d lastLicense=%d source=%@ summary=%@",
          (long)info.sessionState,
          info.hasConfig,
          info.lastPreflightReady,
          info.lastLicenseValid,
          info.sourceIdentity,
          info.stateSummary);
}

static void PrintPublisherRuntimeInfo(HBRStreamCorePublisherRuntimeInfo* info)
{
    NSLog(@"publisher runtime: state=%ld inputKind=%ld hasConfig=%d lastReady=%d lastLicense=%d transcodePolicy=%d rawUsed=%d encodedUsed=%d input=%@ publish=%@ summary=%@",
          (long)info.sessionState,
          (long)info.inputKind,
          info.hasConfig,
          info.lastPreflightReady,
          info.lastLicenseValid,
          info.lastTranscodePolicySatisfied,
          info.acceptedRawInput,
          info.acceptedEncodedInput,
          info.inputIdentity,
          info.publishIdentity,
          info.stateSummary);
}

int main(int argc, char* argv[])
{
    @autoreleasepool
    {
        HBRStreamCoreProductInfo* productInfo = [HBRStreamCoreSDK productInfo];
        NSLog(@"%@ (%@) stage=%@",
              productInfo.productName,
              productInfo.productCode,
              productInfo.stage);

        HBRStreamCoreRuntime* runtime = [HBRStreamCoreSDK runtime];
        HBRStreamCoreRuntimeConfig* runtimeConfig = [runtime defaultConfig];
        NSString* bundleIdentifier = [NSBundle mainBundle].bundleIdentifier;
        if (bundleIdentifier.length == 0) {
            bundleIdentifier = @"com.hbr.streamcoredemo";
        }
        runtimeConfig.expectedProduct = @"streamcore_demo";
        runtimeConfig.applicationIdentifier = bundleIdentifier;
        runtimeConfig.packageIdentifier = bundleIdentifier;
        runtimeConfig.companyIdentifier = @"hbr";
        PrintStatus(@"runtime.configure", [runtime configure:runtimeConfig]);
        PrintStatus(@"log.configure",
                    [runtime configureLogWithDirectory:@""
                                              fileName:@"streamcore_demo.log"
                                          minimumLevel:HBRStreamCoreLogLevelInfo
                                     enablePlatformLog:YES]);
        HBRStreamCoreLogInfo* logInfo = [runtime logInfo];
        NSLog(@"log configured=%d level=%ld logFile=%@ summary=%@",
              logInfo.configured,
              (long)logInfo.minimumLogLevel,
              logInfo.logFileName,
              logInfo.stateSummary);

        HBRStreamCoreRuntimeLicenseInfo* licenseInfo = [runtime licenseInfo];
        NSLog(@"license configured=%d loaded=%d valid=%d status=%@ summary=%@",
              licenseInfo.configured,
              licenseInfo.licenseLoaded,
              licenseInfo.licenseValid,
              licenseInfo.statusName,
              licenseInfo.summary);

        HBRStreamCorePlayerSession* playerSession = [[HBRStreamCorePlayerSession alloc] init];
        HBRStreamCorePlayerConfig* playerConfig = [playerSession defaultConfig];
        playerConfig.sessionName = @"ios_demo_player";
        playerConfig.sourceKind = HBRStreamCorePlayerSourceKindURL;
        playerConfig.sourceURL = @"rtmp://demo.example/live/sample";
        PrintStatus(@"player.configureWithConfig", [playerSession configureWithConfig:playerConfig]);
        HBRStreamCorePlayerPreflight* playerPreflight = [playerSession preflight];
        PrintStatus(@"player.preflight", playerPreflight.status);
        PrintStatus(@"player.start", [playerSession start]);
        PrintPlayerRuntimeInfo([playerSession runtimeInfo]);
        [playerSession stop];
        PrintPlayerRuntimeInfo([playerSession runtimeInfo]);

        HBRStreamCoreCaptureSession* captureSession = [[HBRStreamCoreCaptureSession alloc] init];
        HBRStreamCoreCaptureConfig* captureConfig = [captureSession defaultConfig];
        captureConfig.sessionName = @"ios_demo_capture";
        captureConfig.sourceKind = HBRStreamCoreCaptureSourceKindCamera;
        captureConfig.sourceIdentifier = @"front_camera";
        captureConfig.enableAudio = NO;
        captureConfig.enableVideo = YES;
        PrintStatus(@"capture.configureWithConfig", [captureSession configureWithConfig:captureConfig]);
        HBRStreamCoreCapturePreflight* capturePreflight = [captureSession preflight];
        PrintStatus(@"capture.preflight", capturePreflight.status);
        PrintStatus(@"capture.start", [captureSession start]);
        PrintCaptureRuntimeInfo([captureSession runtimeInfo]);
        [captureSession stop];
        PrintCaptureRuntimeInfo([captureSession runtimeInfo]);

        HBRStreamCorePublisherSession* publisherSession = [[HBRStreamCorePublisherSession alloc] init];
        HBRStreamCorePublisherConfig* publisherConfig = [publisherSession defaultConfig];
        publisherConfig.sessionName = @"ios_demo_publish_encoded";
        publisherConfig.publishURL = @"rtmp://demo.example/live/encoded";
        publisherConfig.inputKind = HBRStreamCorePublisherInputKindAppEncodedFeed;
        publisherConfig.inputBindingIdentifier = @"encoded_feed";
        publisherConfig.sourceMediaProfile.containerName = @"flv";
        publisherConfig.sourceMediaProfile.audioCodecName = @"aac";
        publisherConfig.sourceMediaProfile.videoCodecName = @"h264";
        publisherConfig.sourceMediaProfile.hasAudio = YES;
        publisherConfig.sourceMediaProfile.hasVideo = YES;
        publisherConfig.transcodeOptions.audioMode = HBRStreamCorePublisherTranscodeModeForceTranscode;
        publisherConfig.transcodeOptions.videoMode = HBRStreamCorePublisherTranscodeModeForceTranscode;
        publisherConfig.transcodeRequirementHandler = ^(HBRStreamCorePublisherTranscodeReport* report) {
            NSLog(@"publisher transcode callback: audio=%d/%ld video=%d/%ld summary=%@",
                  report.audioNeedsTranscode,
                  (long)report.audioReason,
                  report.videoNeedsTranscode,
                  (long)report.videoReason,
                  report.summary);
        };
        PrintStatus(@"publisher.configureWithConfig", [publisherSession configureWithConfig:publisherConfig]);
        HBRStreamCorePublisherPreflight* publisherPreflight = [publisherSession preflight];
        PrintStatus(@"publisher.preflight", publisherPreflight.status);
        PrintStatus(@"publisher.start", [publisherSession start]);

        HBRStreamCorePublisherEncodedPacket* audioPacket = [HBRStreamCorePublisherEncodedPacket defaultPacket];
        audioPacket.data = [NSData dataWithBytes:"\x11\x12\x13" length:3];
        audioPacket.codecName = @"aac";
        audioPacket.keyFrame = YES;
        audioPacket.timestampMs = 1000;
        PrintStatus(@"publisher.pushEncodedAudioPacket", [publisherSession pushEncodedAudioPacket:audioPacket]);

        HBRStreamCorePublisherEncodedPacket* videoPacket = [HBRStreamCorePublisherEncodedPacket defaultPacket];
        videoPacket.data = [NSData dataWithBytes:"\x21\x22\x23\x24" length:4];
        videoPacket.codecName = @"h264";
        videoPacket.keyFrame = YES;
        videoPacket.timestampMs = 1033;
        PrintStatus(@"publisher.pushEncodedVideoPacket", [publisherSession pushEncodedVideoPacket:videoPacket]);
        PrintPublisherRuntimeInfo([publisherSession runtimeInfo]);
        [publisherSession stop];
        PrintPublisherRuntimeInfo([publisherSession runtimeInfo]);
    }

    return 0;
}
