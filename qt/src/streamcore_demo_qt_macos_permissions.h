/*******************************************************************************
 * streamcore_demo_qt_macos_permissions.h
 * Copyright (c) 2026 HBRun. All rights reserved.
 *
 * macOS permission helper declarations used by the Qt demo.
 ******************************************************************************/

#ifndef _STREAMCORE_DEMO_QT_MACOS_PERMISSIONS_H_
#define _STREAMCORE_DEMO_QT_MACOS_PERMISSIONS_H_

#include <stddef.h>

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
