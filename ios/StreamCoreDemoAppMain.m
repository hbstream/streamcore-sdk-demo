/*******************************************************************************
 * StreamCoreDemoAppMain.m
 * Copyright (c) 2026 HBRun.
 * SPDX-License-Identifier: Apache-2.0
 *
 * iOS application entry point for the StreamCore SDK demo.
 ******************************************************************************/

#import <UIKit/UIKit.h>

#import "StreamCoreDemoAppDelegate.h"

int main(int argc, char* argv[])
{
    @autoreleasepool
    {
        return UIApplicationMain(
            argc,
            argv,
            nil,
            NSStringFromClass(StreamCoreDemoAppDelegate.class));
    }
}
