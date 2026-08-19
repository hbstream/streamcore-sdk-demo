/*******************************************************************************
 * StreamCoreDemoViewController.h
 * Copyright (c) 2026 HBRun.
 * SPDX-License-Identifier: Apache-2.0
 *
 * iOS UIKit demo view controller declaration. This public demo only calls the
 * StreamCore SDK Objective-C API shipped with the SDK package and does not
 * include internal SDK implementation files.
 ******************************************************************************/

#ifndef _STREAMCORE_DEMO_IOS_VIEW_CONTROLLER_H_
#define _STREAMCORE_DEMO_IOS_VIEW_CONTROLLER_H_

#import <TargetConditionals.h>

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface StreamCoreDemoViewController : UIViewController
@end

NS_ASSUME_NONNULL_END
#endif

#endif // _STREAMCORE_DEMO_IOS_VIEW_CONTROLLER_H_
