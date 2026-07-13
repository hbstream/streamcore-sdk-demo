/*******************************************************************************
 * streamcore_demo_qt_macos_permissions.mm
 * Copyright (c) 2026 HBRun. All rights reserved.
 *
 * macOS Camera, Microphone, and Screen Recording permission bridge for the Qt demo.
 ******************************************************************************/

#include "streamcore_demo_qt_macos_permissions.h"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <dlfcn.h>
#include <string>
#include <string.h>

static void SetPermissionMessage(
    char* message,
    size_t messageCapacity,
    const std::string& value)
{
    if (message == nullptr || messageCapacity == 0)
    {
        return;
    }

    const size_t copySize =
        value.size() < messageCapacity - 1 ? value.size() : messageCapacity - 1;
    if (copySize > 0)
    {
        memcpy(message, value.data(), copySize);
    }
    message[copySize] = '\0';
}

static bool HasUsageDescription(NSString* key)
{
    NSDictionary* info = [[NSBundle mainBundle] infoDictionary];
    NSString* value = [info objectForKey:key];
    return value != nil && [value length] > 0;
}

static bool RequestPermissionForMediaType(
    AVMediaType mediaType,
    NSString* usageKey,
    const char* label,
    int timeoutMs,
    std::string* message)
{
    if (!HasUsageDescription(usageKey))
    {
        if (message != nullptr)
        {
            *message = std::string(label) +
                " permission requires Info.plist usage description";
        }
        return false;
    }

    AVAuthorizationStatus status =
        [AVCaptureDevice authorizationStatusForMediaType:mediaType];
    if (status == AVAuthorizationStatusAuthorized)
    {
        return true;
    }
    if (status == AVAuthorizationStatusDenied ||
        status == AVAuthorizationStatusRestricted)
    {
        if (message != nullptr)
        {
            *message = std::string(label) + " permission denied or restricted";
        }
        return false;
    }

    __block BOOL granted = NO;
    __block BOOL completed = NO;
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    [AVCaptureDevice requestAccessForMediaType:mediaType
                             completionHandler:^(BOOL isGranted) {
        granted = isGranted;
        completed = YES;
        dispatch_semaphore_signal(semaphore);
    }];

    const int effectiveTimeoutMs = timeoutMs > 0 ? timeoutMs : 60000;
    if ([NSThread isMainThread])
    {
        NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:
            static_cast<NSTimeInterval>(effectiveTimeoutMs) / 1000.0];
        while (!completed && [deadline timeIntervalSinceNow] > 0.0)
        {
            @autoreleasepool
            {
                [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode
                                      beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
            }
        }
    }
    else
    {
        dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW,
            (int64_t)effectiveTimeoutMs * NSEC_PER_MSEC);
        if (dispatch_semaphore_wait(semaphore, timeout) == 0)
        {
            completed = YES;
        }
    }

    if (!completed)
    {
        if (message != nullptr)
        {
            *message = std::string(label) + " permission request timed out";
        }
        return false;
    }
    if (!granted)
    {
        if (message != nullptr)
        {
            *message = std::string(label) + " permission was not granted";
        }
        return false;
    }
    return true;
}

static bool CallCoreGraphicsBoolFunction(
    const char* symbolName,
    bool* outValue)
{
    if (symbolName == nullptr || outValue == nullptr)
    {
        return false;
    }

    typedef bool (*CoreGraphicsBoolFunction)();
    void* symbol = dlsym(RTLD_DEFAULT, symbolName);
    if (symbol == nullptr)
    {
        return false;
    }

    CoreGraphicsBoolFunction function =
        reinterpret_cast<CoreGraphicsBoolFunction>(symbol);
    *outValue = function();
    return true;
}

static bool RequestScreenRecordingPermission(std::string* message)
{
    bool hasScreenAccess = false;
    const bool hasPreflight = CallCoreGraphicsBoolFunction(
        "CGPreflightScreenCaptureAccess",
        &hasScreenAccess);
    if (hasPreflight && hasScreenAccess)
    {
        return true;
    }

    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    const bool isAppBundle = bundlePath != nil &&
        [[bundlePath pathExtension] caseInsensitiveCompare:@"app"] ==
            NSOrderedSame;
    if (!isAppBundle)
    {
        if (message != nullptr)
        {
            *message =
                "macOS screen recording permission requires launching the demo "
                "from the .app bundle";
        }
        return false;
    }

    bool granted = false;
    const bool hasRequest = CallCoreGraphicsBoolFunction(
        "CGRequestScreenCaptureAccess",
        &granted);
    if (hasRequest && granted)
    {
        return true;
    }
    if (!hasPreflight || !hasRequest)
    {
        return true;
    }

    if (message != nullptr)
    {
        *message =
            "macOS screen recording permission was not granted; enable "
            "streamcore_demo_qt in System Preferences > Security & Privacy > "
            "Privacy > Screen Recording, then relaunch the app";
    }
    return false;
}

bool StreamCoreDemoQtRequestMacCapturePermissions(
    bool needsScreenRecording,
    bool needsCamera,
    bool needsMicrophone,
    int timeoutMs,
    char* message,
    size_t messageCapacity)
{
    std::string statusMessage;
    if (needsScreenRecording &&
        !RequestScreenRecordingPermission(&statusMessage))
    {
        SetPermissionMessage(message, messageCapacity, statusMessage);
        return false;
    }
    if (needsCamera &&
        !RequestPermissionForMediaType(
            AVMediaTypeVideo,
            @"NSCameraUsageDescription",
            "macOS camera",
            timeoutMs,
            &statusMessage))
    {
        SetPermissionMessage(message, messageCapacity, statusMessage);
        return false;
    }
    if (needsMicrophone &&
        !RequestPermissionForMediaType(
            AVMediaTypeAudio,
            @"NSMicrophoneUsageDescription",
            "macOS microphone",
            timeoutMs,
            &statusMessage))
    {
        SetPermissionMessage(message, messageCapacity, statusMessage);
        return false;
    }

    SetPermissionMessage(
        message,
        messageCapacity,
        "macOS capture permissions are ready");
    return true;
}
