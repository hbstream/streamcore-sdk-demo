/*******************************************************************************
 * streamcore_demo_qt_macos_permissions.mm
 * Copyright (c) 2026 HBRun.
 * SPDX-License-Identifier: Apache-2.0
 *
 * macOS Camera, Microphone, and Screen Recording permission bridge for the Qt demo.
 ******************************************************************************/

#include "streamcore_demo_qt_macos_permissions.h"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <StreamCoreSDK.h>

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

// 从 Apple wrapper 的固定 detail 中提取机器可读 summary；不把注册码、文件路径或
// 完整诊断详情复制到 Demo 界面和自动化截图。
static std::string RuntimeLicenseSummary(HBRStreamCoreOperationStatus* status)
{
    NSString* detail = status != nil ? status.detail : nil;
    const char* detailText = detail != nil ? [detail UTF8String] : nullptr;
    if (detailText != nullptr)
    {
        const std::string text(detailText);
        const std::string marker("summary=");
        const size_t begin = text.find(marker);
        if (begin != std::string::npos)
        {
            const size_t valueBegin = begin + marker.size();
            const size_t valueEnd = text.find_first_of(",\r\n", valueBegin);
            const std::string summary = text.substr(
                valueBegin,
                valueEnd == std::string::npos ?
                    std::string::npos :
                    valueEnd - valueBegin);
            if (!summary.empty() && summary.size() <= 128)
            {
                return summary;
            }
        }
    }

    NSString* statusName = status != nil ? status.statusName : nil;
    const char* statusText = statusName != nil ? [statusName UTF8String] : nullptr;
    return statusText != nullptr && statusText[0] != '\0' ?
        std::string(statusText) :
        std::string("runtime_configure_failed");
}

streamcore_result_t StreamCoreDemoQtConfigureMacRuntime(
    const char* configuredLicensePath,
    char* message,
    size_t messageCapacity)
{
    if (configuredLicensePath == nullptr || configuredLicensePath[0] == '\0')
    {
        SetPermissionMessage(message, messageCapacity, "demo license path is required");
        return STREAMCORE_RESULT_INVALID_ARGUMENT;
    }

    @autoreleasepool
    {
        NSString* licensePath = [NSString stringWithUTF8String:configuredLicensePath];
        if (licensePath == nil || [licensePath length] == 0)
        {
            SetPermissionMessage(message, messageCapacity, "demo license path is invalid UTF-8");
            return STREAMCORE_RESULT_INVALID_ARGUMENT;
        }
        if (![licensePath isAbsolutePath])
        {
            NSString* executablePath = [[NSBundle mainBundle] executablePath];
            if (executablePath == nil || [executablePath length] == 0)
            {
                SetPermissionMessage(
                    message,
                    messageCapacity,
                    "macOS app executable path is unavailable");
                return STREAMCORE_RESULT_OPERATION_FAILED;
            }
            licensePath = [[executablePath stringByDeletingLastPathComponent]
                stringByAppendingPathComponent:licensePath];
        }

        BOOL isDirectory = NO;
        if (![[NSFileManager defaultManager] fileExistsAtPath:licensePath
                                                 isDirectory:&isDirectory] ||
            isDirectory)
        {
            SetPermissionMessage(message, messageCapacity, "demo license file is unavailable");
            return STREAMCORE_RESULT_OPERATION_FAILED;
        }

        HBRStreamCoreRuntimeConfig* config =
            [[HBRStreamCoreSDK runtime] defaultConfig];
        config.licensePath = licensePath;
        HBRStreamCoreOperationStatus* status =
            [[HBRStreamCoreSDK runtime] configure:config];
        if (status == nil)
        {
            SetPermissionMessage(message, messageCapacity, "Apple runtime returned no status");
            return STREAMCORE_RESULT_OPERATION_FAILED;
        }
        if (![status isOk])
        {
            SetPermissionMessage(
                message,
                messageCapacity,
                "license validation failed: " + RuntimeLicenseSummary(status));
            return static_cast<streamcore_result_t>(status.resultCode);
        }
    }

    SetPermissionMessage(message, messageCapacity, "Apple runtime configured");
    return STREAMCORE_RESULT_OK;
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
