/*******************************************************************************
 * StreamCoreDemoAppDelegate.m
 * Copyright (c) 2026 HBRun. All rights reserved.
 *
 * iOS application delegate for the StreamCore SDK demo.
 ******************************************************************************/

#import "StreamCoreDemoAppDelegate.h"

#import "StreamCoreDemoViewController.h"

@implementation StreamCoreDemoAppDelegate

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id>*)launchOptions
{
    (void)application;
    (void)launchOptions;

    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    StreamCoreDemoViewController* rootController =
        [[StreamCoreDemoViewController alloc] initWithNibName:nil bundle:nil];
    UINavigationController* navigationController =
        [[UINavigationController alloc] initWithRootViewController:rootController];
    navigationController.navigationBar.prefersLargeTitles = NO;
    self.window.rootViewController = navigationController;
    [self.window makeKeyAndVisible];
    return YES;
}

@end
