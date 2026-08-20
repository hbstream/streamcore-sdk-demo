/*******************************************************************************
 * StreamCoreDemoAppMain.m
 * Copyright (c) 2026 HBRun. All rights reserved.
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
