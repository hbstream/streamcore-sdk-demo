/*******************************************************************************
 * streamcore_demo_qt_macos_permissions.h
 * Copyright (c) 2026 HBRun.
 * SPDX-License-Identifier: Apache-2.0
 *
 * macOS permission helper declarations used by the Qt demo.
 ******************************************************************************/

#ifndef _STREAMCORE_DEMO_QT_MACOS_PERMISSIONS_H_
#define _STREAMCORE_DEMO_QT_MACOS_PERMISSIONS_H_

#include <stddef.h>

#include <streamcore/streamcore_sdk.h>

// Configures the process-wide runtime through the public Apple wrapper so the
// SDK collects the Bundle ID from the current .app. The Qt snapshot must reuse
// that runtime instead of overwriting it with identity-free native C config.
streamcore_result_t StreamCoreDemoQtConfigureMacRuntime(
    const char* configuredLicensePath,
    char* message,
    size_t messageCapacity);

// Requests macOS Screen Recording, Camera, and/or Microphone permission for the
// current app bundle.
bool StreamCoreDemoQtRequestMacCapturePermissions(
    bool needsScreenRecording,
    bool needsCamera,
    bool needsMicrophone,
    int timeoutMs,
    char* message,
    size_t messageCapacity);

#endif // _STREAMCORE_DEMO_QT_MACOS_PERMISSIONS_H_
