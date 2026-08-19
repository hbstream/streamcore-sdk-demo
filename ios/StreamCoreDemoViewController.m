/*******************************************************************************
 * StreamCoreDemoViewController.m
 * Copyright (c) 2026 HBRun. All rights reserved.
 *
 * iOS UIKit demo controller for StreamCore SDK playback, publishing, and GB28181 examples.
 ******************************************************************************/

#import "StreamCoreDemoViewController.h"

#if TARGET_OS_IOS

#if __has_include(<StreamCoreSDK/StreamCoreSDK.h>)
#import <StreamCoreSDK/StreamCoreSDK.h>
#elif __has_include("StreamCoreSDK.h")
#import "StreamCoreSDK.h"
#else
#error "StreamCoreSDK public header is missing. Add StreamCoreSDK.xcframework or the released public header path to the target."
#endif

static const unsigned long long HBRLogPackageMaxBytes = 20ULL * 1024ULL * 1024ULL;
static const unsigned long long HBRLogPackageSingleFileMaxBytes = 8ULL * 1024ULL * 1024ULL;
static const NSUInteger HBRLogPackageMaxFiles = 24;
static const NSTimeInterval HBRInitialSnapshotDelaySeconds = 2.0;
static const NSInteger HBRPreviewHostLabelTag = 1001;
static const NSInteger HBRPreviewHostOverlayLabelTag = 1002;
static NSString* const HBRDemoWatermarkText = @"StreamCore Demo | hbrun.com";

typedef NS_ENUM(NSInteger, HBRDemoLanguageMode)
{
    HBRDemoLanguageModeAuto = 0,
    HBRDemoLanguageModeEnglish = 1,
    HBRDemoLanguageModeChinese = 2,
};

static void HBRAppendZipLe16(NSMutableData* data, uint16_t value)
{
    uint8_t bytes[2] = {
        (uint8_t)(value & 0xFF),
        (uint8_t)((value >> 8) & 0xFF),
    };
    [data appendBytes:bytes length:sizeof(bytes)];
}

static void HBRAppendZipLe32(NSMutableData* data, uint32_t value)
{
    uint8_t bytes[4] = {
        (uint8_t)(value & 0xFF),
        (uint8_t)((value >> 8) & 0xFF),
        (uint8_t)((value >> 16) & 0xFF),
        (uint8_t)((value >> 24) & 0xFF),
    };
    [data appendBytes:bytes length:sizeof(bytes)];
}

static uint32_t HBRCrc32(NSData* data)
{
    static uint32_t table[256];
    static BOOL tableReady = NO;
    if (!tableReady)
    {
        for (uint32_t index = 0; index < 256; ++index)
        {
            uint32_t value = index;
            for (int bit = 0; bit < 8; ++bit)
            {
                value = (value & 1U) ? (0xEDB88320U ^ (value >> 1U)) : (value >> 1U);
            }
            table[index] = value;
        }
        tableReady = YES;
    }
    uint32_t crc = 0xFFFFFFFFU;
    const uint8_t* bytes = data.bytes;
    for (NSUInteger index = 0; index < data.length; ++index)
    {
        crc = table[(crc ^ bytes[index]) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}

@interface StreamCoreDemoViewController ()
@property(nonatomic, strong) UIView* initialPlaceholderView;
@property(nonatomic, strong) UIScrollView* contentScrollView;
@property(nonatomic, strong) UIStackView* stackView;
@property(nonatomic, strong) UIStackView* rootStackView;
@property(nonatomic, strong) UIStackView* publisherPanelStackView;
@property(nonatomic, strong) UIStackView* playerPanelStackView;
@property(nonatomic, strong) UIStackView* gb28181PanelStackView;
@property(nonatomic, strong) UIStackView* licensePanelStackView;
@property(nonatomic, strong) UISegmentedControl* tabControl;
@property(nonatomic, strong) NSArray<UIButton*>* tabButtons;
@property(nonatomic, strong) UIButton* languageButton;
@property(nonatomic, strong) UISegmentedControl* displayModeControl;
@property(nonatomic, strong) UISegmentedControl* playerSourceControl;
@property(nonatomic, strong) UISegmentedControl* publisherDisplayModeControl;
@property(nonatomic, strong) UISegmentedControl* captureSourceControl;
@property(nonatomic, strong) UISegmentedControl* publisherInputControl;
@property(nonatomic, strong) UISegmentedControl* publisherAudioControl;
@property(nonatomic, strong) UISegmentedControl* publisherResolutionControl;
@property(nonatomic, strong) UISegmentedControl* publisherVideoCodecControl;
@property(nonatomic, strong) UISegmentedControl* publisherAudioCodecControl;
@property(nonatomic, strong) UISlider* publisherAudioVolumeSlider;
@property(nonatomic, strong) UILabel* publisherAudioVolumeLabel;
@property(nonatomic, strong) UIView* playerPreviewHostView;
@property(nonatomic, strong) UIView* publisherPreviewHostView;
@property(nonatomic, strong) UIView* gbPreviewHostView;
@property(nonatomic, strong) UITextField* playerURLEdit;
@property(nonatomic, strong) UIStackView* playerWhepOptionsStack;
@property(nonatomic, strong) UITextField* playerWhepBearerEdit;
@property(nonatomic, strong) UITextField* playerWhepLocalBindEdit;
@property(nonatomic, strong) UISwitch* playerWhepAllowHTTPControl;
@property(nonatomic, strong) UITextField* publisherURLEdit;
@property(nonatomic, strong) UITextField* gbLocalIdEdit;
@property(nonatomic, strong) UITextField* gbLocalDomainEdit;
@property(nonatomic, strong) UITextField* gbLocalIpEdit;
@property(nonatomic, strong) UITextField* gbLocalPortEdit;
@property(nonatomic, strong) UITextField* gbUpperIdEdit;
@property(nonatomic, strong) UITextField* gbUpperDomainEdit;
@property(nonatomic, strong) UITextField* gbUpperPasswordEdit;
@property(nonatomic, strong) UITextField* gbUpperIpEdit;
@property(nonatomic, strong) UITextField* gbUpperPortEdit;
@property(nonatomic, strong) UISegmentedControl* gbTransportControl;
@property(nonatomic, strong) UISegmentedControl* gbSourceControl;
@property(nonatomic, strong) UITextField* gbMediaSourceEdit;
@property(nonatomic, strong) UISegmentedControl* gbResolutionControl;
@property(nonatomic, strong) UISegmentedControl* gbPreviewDisplayModeControl;
@property(nonatomic, strong) UITextField* gbMediaIpEdit;
@property(nonatomic, strong) UITextField* gbMediaPortEdit;
@property(nonatomic, strong) UITextField* gbRegisterExpiresEdit;
@property(nonatomic, strong) UITextField* gbKeepaliveEdit;
@property(nonatomic, strong) UILabel* statusLabel;
@property(nonatomic, strong) UILabel* productLabel;
@property(nonatomic, strong) UILabel* displayModeLabel;
@property(nonatomic, strong) UILabel* licenseLabel;
@property(nonatomic, strong) UILabel* playerLabel;
@property(nonatomic, strong) UILabel* captureLabel;
@property(nonatomic, strong) UILabel* publisherLabel;
@property(nonatomic, strong) UILabel* gb28181Label;
@property(nonatomic, strong) UILabel* gbMediaSourceLabel;
@property(nonatomic, strong) UIButton* playerStartStopButton;
@property(nonatomic, strong) UIButton* publisherStartStopButton;
@property(nonatomic, strong) UIButton* gb28181StartStopButton;
@property(nonatomic, copy) NSString* logDirectory;
@property(nonatomic, copy) NSString* logFileName;
@property(nonatomic, copy) NSString* productCrashDirectory;
@property(nonatomic, copy) NSString* latestAction;
@property(nonatomic, copy) NSString* latestStatusCode;
@property(nonatomic, copy) NSString* latestStatusName;
@property(nonatomic, copy) NSString* latestStatusSummary;
@property(nonatomic, copy) NSString* latestLogZipPath;
@property(nonatomic, assign) NSInteger selectedTabIndex;
@property(nonatomic, assign) BOOL didBuildInterface;
@property(nonatomic, assign) BOOL didScheduleInitialRefresh;
@property(nonatomic, assign) HBRDemoLanguageMode languageMode;
@property(nonatomic, strong) HBRStreamCorePlayerSession* playerSession;
@property(nonatomic, strong) HBRStreamCoreCaptureSession* captureSession;
@property(nonatomic, strong) HBRStreamCoreCaptureSession* publisherCaptureSession;
@property(nonatomic, strong) HBRStreamCorePublisherSession* publisherSession;
@property(nonatomic, strong) HBRStreamCoreGB28181Session* gb28181Session;
@property(nonatomic, strong) NSTimer* gb28181PollTimer;
- (BOOL)prefersSystemChineseLanguage;
- (BOOL)prefersChineseLanguage;
- (NSString*)uiTextEnglish:(NSString*)english chinese:(NSString*)chinese;
- (HBRDemoLanguageMode)startupLanguageModeFromEnvironment;
- (NSString*)languageButtonTitle;
- (void)updateLanguageButtonTitle;
- (void)showLanguageSelectionSheet;
- (void)applyLanguageMode:(HBRDemoLanguageMode)languageMode;
- (void)rebuildInterfaceForLanguageChange;
- (UILabel*)previewLabelForHost:(UIView*)hostView;
- (UILabel*)previewOverlayLabelForHost:(UIView*)hostView;
- (BOOL)shouldShowPublisherPassthroughHint;
- (UISegmentedControl*)newCaptureSourceControl;
- (UIView*)buildBottomTabBar;
- (UIButton*)newTabButtonWithTitle:(NSString*)title index:(NSInteger)index;
- (void)tabBarButtonTapped:(UIButton*)sender;
- (void)updateTabButtonStates;
- (void)syncHiddenCaptureSourceSelection;
- (void)applyDisplayModeIndex:(NSInteger)selectedSegmentIndex
                toPreviewHost:(UIView*)previewHost;
- (void)applyPublisherDisplayModeToPreviewHost;
- (void)applyGB28181DisplayModeToPreviewHost;
- (void)syncGB28181SourceControls;
- (void)updatePublisherPreviewPresentation;
- (void)refreshPrimaryActionButtons;
// 处理用户显式切换媒体 URL / WHEP，并释放旧会话中的创建期配置。
- (void)playerSourceChanged:(UISegmentedControl*)sender;
// 仅在 WHEP 模式展示协议专属选项。
- (void)updatePlayerSourceUI;
// 返回当前是否显式选择 WHEP，不根据 endpoint 文本猜测协议。
- (BOOL)isPlayerWhepSelected;
// 返回移除 userinfo、query 与 fragment 后的安全展示地址。
- (NSString*)playerDisplaySource;
- (void)refreshPreviewOverlays;
@end

@implementation StreamCoreDemoViewController

- (BOOL)prefersSystemChineseLanguage
{
    NSString* preferredLanguage = NSLocale.preferredLanguages.firstObject;
    return [preferredLanguage hasPrefix:@"zh"];
}

- (BOOL)prefersChineseLanguage
{
    if (self.languageMode == HBRDemoLanguageModeChinese)
    {
        return YES;
    }
    if (self.languageMode == HBRDemoLanguageModeEnglish)
    {
        return NO;
    }
    return [self prefersSystemChineseLanguage];
}

- (NSString*)uiTextEnglish:(NSString*)english chinese:(NSString*)chinese
{
    return [self prefersChineseLanguage] ? chinese : english;
}

- (HBRDemoLanguageMode)startupLanguageModeFromEnvironment
{
    NSString* value =
        [NSProcessInfo.processInfo.environment[@"STREAMCORE_DEMO_IOS_LANGUAGE"] lowercaseString];
    if ([value isEqualToString:@"en"] || [value isEqualToString:@"english"])
    {
        return HBRDemoLanguageModeEnglish;
    }
    if ([value isEqualToString:@"zh"] ||
        [value isEqualToString:@"cn"] ||
        [value isEqualToString:@"chinese"])
    {
        return HBRDemoLanguageModeChinese;
    }
    return HBRDemoLanguageModeAuto;
}

- (NSString*)languageButtonTitle
{
    switch (self.languageMode)
    {
    case HBRDemoLanguageModeChinese:
        return @"中文";
    case HBRDemoLanguageModeEnglish:
        return @"English";
    case HBRDemoLanguageModeAuto:
    default:
        return [self uiTextEnglish:@"Auto" chinese:@"自动"];
    }
}

- (void)updateLanguageButtonTitle
{
    [self.languageButton setTitle:[self languageButtonTitle] forState:UIControlStateNormal];
}

- (void)showLanguageSelectionSheet
{
    UIAlertController* sheet = [UIAlertController
        alertControllerWithTitle:[self uiTextEnglish:@"Language" chinese:@"语言"]
                         message:nil
                  preferredStyle:UIAlertControllerStyleActionSheet];
    __weak StreamCoreDemoViewController* weakSelf = self;
    NSArray<NSString*>* titles = @[
        [self uiTextEnglish:@"Auto" chinese:@"自动"],
        @"English",
        @"中文"
    ];
    NSArray<NSNumber*>* modes = @[
        @(HBRDemoLanguageModeAuto),
        @(HBRDemoLanguageModeEnglish),
        @(HBRDemoLanguageModeChinese)
    ];
    for (NSUInteger index = 0; index < titles.count; ++index)
    {
        [sheet addAction:[UIAlertAction actionWithTitle:titles[index]
                                                  style:UIAlertActionStyleDefault
                                                handler:^(__unused UIAlertAction* action) {
                                                    [weakSelf applyLanguageMode:(HBRDemoLanguageMode)modes[index].integerValue];
                                                }]];
    }
    [sheet addAction:[UIAlertAction actionWithTitle:[self uiTextEnglish:@"Cancel" chinese:@"取消"]
                                              style:UIAlertActionStyleCancel
                                            handler:nil]];

    UIPopoverPresentationController* popover = sheet.popoverPresentationController;
    if (popover != nil && self.languageButton != nil)
    {
        popover.sourceView = self.languageButton;
        popover.sourceRect = self.languageButton.bounds;
    }
    [self presentViewController:sheet animated:YES completion:nil];
}

- (void)applyLanguageMode:(HBRDemoLanguageMode)languageMode
{
    if (self.languageMode == languageMode)
    {
        return;
    }
    self.languageMode = languageMode;
    [self rebuildInterfaceForLanguageChange];
}

- (void)rebuildInterfaceForLanguageChange
{
    NSInteger selectedTabIndex = self.selectedTabIndex;

    [self stopGB28181PollTimer];
    [self stopPlayer];
    [self stopCapture];
    [self stopPublisher];
    [self stopGB28181];

    for (UIView* subview in self.view.subviews.copy)
    {
        [subview removeFromSuperview];
    }

    self.contentScrollView = nil;
    self.rootStackView = nil;
    self.stackView = nil;
    self.publisherPanelStackView = nil;
    self.playerPanelStackView = nil;
    self.gb28181PanelStackView = nil;
    self.licensePanelStackView = nil;
    self.tabControl = nil;
    self.tabButtons = nil;
    self.languageButton = nil;

    [self buildInterface];
    self.selectedTabIndex = MAX(0, MIN(selectedTabIndex, 3));
    self.tabControl.selectedSegmentIndex = self.selectedTabIndex;
    [self showSelectedTab];
    [self reloadDemoSnapshot];
    [self setOperationAction:@"ui.language"
                        code:@"0"
                      status:@"ok"
                     summary:[self uiTextEnglish:@"Language switched." chinese:@"语言已切换。"]];
}

- (UILabel*)previewLabelForHost:(UIView*)hostView
{
    if (hostView == nil)
    {
        return nil;
    }
    UIView* view = [hostView viewWithTag:HBRPreviewHostLabelTag];
    return [view isKindOfClass:UILabel.class] ? (UILabel*)view : nil;
}

- (UILabel*)previewOverlayLabelForHost:(UIView*)hostView
{
    if (hostView == nil)
    {
        return nil;
    }
    UIView* view = [hostView viewWithTag:HBRPreviewHostOverlayLabelTag];
    return [view isKindOfClass:UILabel.class] ? (UILabel*)view : nil;
}

- (BOOL)shouldShowPublisherPassthroughHint
{
    return NO;
}

- (void)updatePublisherPreviewPresentation
{
    [self applyPublisherDisplayModeToPreviewHost];
    UILabel* previewLabel = [self previewLabelForHost:self.publisherPreviewHostView];
    if (previewLabel == nil || self.publisherPreviewHostView == nil)
    {
        return;
    }

    previewLabel.numberOfLines = 0;
    previewLabel.textAlignment = NSTextAlignmentCenter;
    if ([self shouldShowPublisherPassthroughHint])
    {
        self.publisherPreviewHostView.backgroundColor =
            [UIColor colorWithRed:0.49 green:0.18 blue:0.07 alpha:1.0];
        self.publisherPreviewHostView.layer.borderColor =
            [UIColor colorWithRed:0.99 green:0.73 blue:0.45 alpha:1.0].CGColor;
        self.publisherPreviewHostView.layer.borderWidth = 2.0;
        previewLabel.textColor =
            [UIColor colorWithRed:1.0 green:0.97 blue:0.93 alpha:1.0];
        previewLabel.font = [UIFont systemFontOfSize:18.0 weight:UIFontWeightSemibold];
        previewLabel.text = [self uiTextEnglish:
            @"PASSTHROUGH READY\nNO LOCAL PREVIEW\n\nThis file will be published from encoded packets. Verify the output from a player or from runtime logs."
            chinese:
            @"编码包透传就绪\n当前无本地预览\n\n当前文件会以编码包直接推流。请通过播放器或运行日志验证输出。"];
        return;
    }

    self.publisherPreviewHostView.backgroundColor = UIColor.blackColor;
    self.publisherPreviewHostView.layer.borderColor = UIColor.clearColor.CGColor;
    self.publisherPreviewHostView.layer.borderWidth = 0.0;
    previewLabel.textColor = UIColor.whiteColor;
    previewLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    previewLabel.text = [self uiTextEnglish:@"Publisher Preview" chinese:@"推流预览"];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.languageMode = [self startupLanguageModeFromEnvironment];
    self.title = @"StreamCore SDK Demo";
    self.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    [self buildInitialPlaceholder];
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (BOOL)shouldAutorotate
{
    return NO;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    if (!self.didBuildInterface)
    {
        self.didBuildInterface = YES;
        [self buildInterface];
    }
    if (self.didScheduleInitialRefresh)
    {
        return;
    }
    self.didScheduleInitialRefresh = YES;
    // Keep the first visible scene free of SDK runtime queries and autorun work.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                       (int64_t)(HBRInitialSnapshotDelaySeconds * NSEC_PER_SEC)),
        dispatch_get_main_queue(),
        ^{
            [self reloadDemoSnapshot];
            [self scheduleAutorunIfRequested];
        });
}

- (void)buildInitialPlaceholder
{
    UIView* placeholderView = [[UIView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    placeholderView.translatesAutoresizingMaskIntoConstraints = NO;
    UIActivityIndicatorView* loadingIndicator =
        [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleMedium];
    loadingIndicator.translatesAutoresizingMaskIntoConstraints = NO;
    [loadingIndicator startAnimating];

    [self.view addSubview:placeholderView];
    [placeholderView addSubview:loadingIndicator];
    [NSLayoutConstraint activateConstraints:@[
        [placeholderView.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor],
        [placeholderView.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor],
        [placeholderView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
        [placeholderView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
        [loadingIndicator.centerXAnchor constraintEqualToAnchor:placeholderView.centerXAnchor],
        [loadingIndicator.centerYAnchor constraintEqualToAnchor:placeholderView.centerYAnchor],
    ]];
    self.initialPlaceholderView = placeholderView;
}

- (void)buildInterface
{
    [self.initialPlaceholderView removeFromSuperview];
    self.initialPlaceholderView = nil;
    UIScrollView* scrollView = [[UIScrollView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    self.contentScrollView = scrollView;
    self.rootStackView = [self newVerticalStackWithSpacing:12.0];
    self.stackView = self.rootStackView;
    [self.view addSubview:scrollView];
    [scrollView addSubview:self.rootStackView];
    self.languageButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.languageButton.titleLabel.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold];
    [self.languageButton addTarget:self
                            action:@selector(showLanguageSelectionSheet)
                  forControlEvents:UIControlEventTouchUpInside];
    [self updateLanguageButtonTitle];
    self.navigationItem.rightBarButtonItem =
        [[UIBarButtonItem alloc] initWithCustomView:self.languageButton];
    self.tabControl = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"Publisher" chinese:@"推流"],
        [self uiTextEnglish:@"Player" chinese:@"播放"],
        @"GB28181",
        [self uiTextEnglish:@"License" chinese:@"授权"]
    ]];
    self.tabControl.selectedSegmentIndex = [self initialTabIndexFromEnvironment];
    self.selectedTabIndex = self.tabControl.selectedSegmentIndex;
    [self.tabControl addTarget:self
                        action:@selector(tabChanged:)
              forControlEvents:UIControlEventValueChanged];
    self.captureSourceControl = [self newCaptureSourceControl];
    UIView* tabBarView = [self buildBottomTabBar];
    [self.view addSubview:tabBarView];
    self.publisherPanelStackView = [self newPanelStackView];
    self.playerPanelStackView = [self newPanelStackView];
    self.gb28181PanelStackView = [self newPanelStackView];
    self.licensePanelStackView = [self newPanelStackView];
    [self.rootStackView addArrangedSubview:self.publisherPanelStackView];
    [self.rootStackView addArrangedSubview:self.playerPanelStackView];
    [self.rootStackView addArrangedSubview:self.gb28181PanelStackView];
    [self.rootStackView addArrangedSubview:self.licensePanelStackView];
    self.stackView = self.publisherPanelStackView;
    [self addPublisherOperationSection];
    self.stackView = self.playerPanelStackView;
    [self addPlayerOperationSection];
    self.stackView = self.gb28181PanelStackView;
    [self addGB28181OperationSection];
    self.stackView = self.licensePanelStackView;
    [self addLogStatusSection];
    self.licenseLabel = [self addSectionWithTitle:[self uiTextEnglish:@"License / Feature / Log" chinese:@"授权 / 能力 / 日志"]];
    self.productLabel = [self addSectionWithTitle:[self uiTextEnglish:@"Product" chinese:@"产品"]];
    [self addRefreshButtonToStack:self.licensePanelStackView];
    self.stackView = self.rootStackView;
    [self showSelectedTab];
    [self refreshPreviewOverlays];
    [NSLayoutConstraint activateConstraints:@[
        [scrollView.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor],
        [scrollView.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor],
        [scrollView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
        [scrollView.bottomAnchor constraintEqualToAnchor:tabBarView.topAnchor],
        [self.rootStackView.leadingAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.leadingAnchor constant:16.0],
        [self.rootStackView.trailingAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.trailingAnchor constant:-16.0],
        [self.rootStackView.topAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.topAnchor constant:12.0],
        [self.rootStackView.bottomAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.bottomAnchor constant:-16.0],
        [self.rootStackView.widthAnchor constraintEqualToAnchor:scrollView.frameLayoutGuide.widthAnchor constant:-32.0],
        [tabBarView.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor],
        [tabBarView.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor],
        [tabBarView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
    ]];
}
- (UIStackView*)newVerticalStackWithSpacing:(CGFloat)spacing
{
    UIStackView* stack = [[UIStackView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = spacing;
    return stack;
}

- (UIStackView*)newPanelStackView
{
    UIStackView* stack = [self newVerticalStackWithSpacing:10.0];
    stack.layoutMargins = UIEdgeInsetsMake(4.0, 0.0, 0.0, 0.0);
    stack.layoutMarginsRelativeArrangement = YES;
    return stack;
}

- (void)addRefreshButtonToStack:(UIStackView*)stack
{
    UIButton* reloadButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [reloadButton setTitle:[self uiTextEnglish:@"Refresh Status" chinese:@"刷新状态"]
                  forState:UIControlStateNormal];
    [reloadButton addTarget:self
                     action:@selector(reloadDemoSnapshot)
           forControlEvents:UIControlEventTouchUpInside];
    [stack addArrangedSubview:reloadButton];
}
- (NSInteger)initialTabIndexFromEnvironment
{
    NSString* requestedTab =
        [NSProcessInfo.processInfo.environment[@"STREAMCORE_DEMO_IOS_INITIAL_TAB"] lowercaseString];
    if ([requestedTab isEqualToString:@"player"])
    {
        return 1;
    }
    if ([requestedTab isEqualToString:@"gb28181"])
    {
        return 2;
    }
    if ([requestedTab isEqualToString:@"license"])
    {
        return 3;
    }
    return 0;
}

- (void)tabChanged:(UISegmentedControl*)sender
{
    self.selectedTabIndex = sender.selectedSegmentIndex;
    [self showSelectedTab];
}

- (void)showSelectedTab
{
    self.publisherPanelStackView.hidden = self.selectedTabIndex != 0;
    self.playerPanelStackView.hidden = self.selectedTabIndex != 1;
    self.gb28181PanelStackView.hidden = self.selectedTabIndex != 2;
    self.licensePanelStackView.hidden = self.selectedTabIndex != 3;
    [self updateTabButtonStates];
}

- (UIView*)buildBottomTabBar
{
    UIView* container = [[UIView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    container.translatesAutoresizingMaskIntoConstraints = NO;
    container.backgroundColor = UIColor.systemBackgroundColor;
    container.layer.borderColor = [UIColor colorWithWhite:0.86 alpha:1.0].CGColor;
    container.layer.borderWidth = 0.5;

    UIStackView* stack = [self newActionRow];
    NSArray<NSString*>* titles = @[
        [self uiTextEnglish:@"Publisher" chinese:@"推流"],
        [self uiTextEnglish:@"Player" chinese:@"播放"],
        @"GB28181",
        [self uiTextEnglish:@"License" chinese:@"授权"]
    ];
    NSMutableArray<UIButton*>* buttons = [NSMutableArray arrayWithCapacity:titles.count];
    for (NSInteger index = 0; index < (NSInteger)titles.count; ++index)
    {
        UIButton* button = [self newTabButtonWithTitle:titles[(NSUInteger)index] index:index];
        [stack addArrangedSubview:button];
        [buttons addObject:button];
    }
    self.tabButtons = buttons.copy;

    [container addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:container.leadingAnchor constant:12.0],
        [stack.trailingAnchor constraintEqualToAnchor:container.trailingAnchor constant:-12.0],
        [stack.topAnchor constraintEqualToAnchor:container.topAnchor constant:8.0],
        [stack.bottomAnchor constraintEqualToAnchor:container.bottomAnchor constant:-8.0],
        [container.heightAnchor constraintGreaterThanOrEqualToConstant:60.0],
    ]];
    return container;
}

- (UIButton*)newTabButtonWithTitle:(NSString*)title index:(NSInteger)index
{
    UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
    button.tag = index;
    button.titleLabel.font = [UIFont systemFontOfSize:12.0 weight:UIFontWeightRegular];
    [button setTitle:title forState:UIControlStateNormal];
    [button addTarget:self
               action:@selector(tabBarButtonTapped:)
     forControlEvents:UIControlEventTouchUpInside];
    return button;
}

- (void)tabBarButtonTapped:(UIButton*)sender
{
    self.selectedTabIndex = sender.tag;
    self.tabControl.selectedSegmentIndex = self.selectedTabIndex;
    [self.contentScrollView setContentOffset:(CGPoint){0, 0} animated:NO];
    [self showSelectedTab];
}

- (void)updateTabButtonStates
{
    [self.tabButtons enumerateObjectsUsingBlock:^(UIButton* button, NSUInteger index, BOOL* stop) {
        (void)stop;
        const BOOL selected = (NSInteger)index == self.selectedTabIndex;
        button.selected = selected;
        button.titleLabel.font = [UIFont systemFontOfSize:12.0
                                                   weight:selected ? UIFontWeightSemibold : UIFontWeightRegular];
        [button setTitleColor:selected ? UIColor.systemBlueColor : UIColor.secondaryLabelColor
                     forState:UIControlStateNormal];
    }];
}

- (void)addLogStatusSection
{
    UIStackView* languageRow = [[UIStackView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    languageRow.translatesAutoresizingMaskIntoConstraints = NO;
    languageRow.axis = UILayoutConstraintAxisHorizontal;
    languageRow.alignment = UIStackViewAlignmentCenter;
    languageRow.spacing = 8.0;
    UILabel* languageLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    languageLabel.text = [self uiTextEnglish:@"Language" chinese:@"语言"];
    languageLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    languageLabel.textColor = UIColor.secondaryLabelColor;
    [languageRow addArrangedSubview:languageLabel];
    UIView* languageSpacer = [[UIView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    [languageRow addArrangedSubview:languageSpacer];
    UIButton* languageModeButton = [UIButton buttonWithType:UIButtonTypeSystem];
    languageModeButton.titleLabel.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold];
    [languageModeButton addTarget:self
                            action:@selector(showLanguageSelectionSheet)
                  forControlEvents:UIControlEventTouchUpInside];
    [self updateLanguageButtonTitle];
    [languageModeButton setTitle:[self languageButtonTitle] forState:UIControlStateNormal];
    [languageRow addArrangedSubview:languageModeButton];

    UIStackView* actionRow = [[UIStackView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    actionRow.axis = UILayoutConstraintAxisHorizontal;
    actionRow.spacing = 8.0;
    actionRow.distribution = UIStackViewDistributionFillEqually;
    UIButton* shareButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [shareButton setTitle:[self uiTextEnglish:@"Share Logs" chinese:@"分享日志"]
                 forState:UIControlStateNormal];
    [shareButton addTarget:self
                    action:@selector(shareLogs)
          forControlEvents:UIControlEventTouchUpInside];
    UIButton* uploadButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [uploadButton setTitle:[self uiTextEnglish:@"Upload Reserved" chinese:@"上传预留"]
                  forState:UIControlStateNormal];
    [uploadButton addTarget:self
                     action:@selector(showUploadReserved)
           forControlEvents:UIControlEventTouchUpInside];
    self.statusLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    self.statusLabel.numberOfLines = 4;
    self.statusLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    self.statusLabel.textColor = UIColor.secondaryLabelColor;
    [actionRow addArrangedSubview:shareButton];
    [actionRow addArrangedSubview:uploadButton];
    [self.stackView addArrangedSubview:actionRow];
    [self.stackView addArrangedSubview:self.statusLabel];
    [self setOperationAction:@"startup"
                        code:@"-"
                      status:@"pending"
                     summary:[self uiTextEnglish:@"Runtime is loading." chinese:@"运行时正在加载。"]];
}
- (UITextField*)addTextFieldWithText:(NSString*)text placeholder:(NSString*)placeholder
{
    UITextField* textField = [[UITextField alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    textField.borderStyle = UITextBorderStyleRoundedRect;
    textField.clearButtonMode = UITextFieldViewModeWhileEditing;
    textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    textField.autocorrectionType = UITextAutocorrectionTypeNo;
    textField.text = text;
    textField.placeholder = placeholder;
    return textField;
}

- (UILabel*)newFieldLabelWithText:(NSString*)text
{
    UILabel* label = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    label.text = text;
    label.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    label.textColor = UIColor.secondaryLabelColor;
    return label;
}

- (void)addLabeledControl:(UIView*)control
                    label:(NSString*)labelText
                  toStack:(UIStackView*)stack
{
    [stack addArrangedSubview:[self newFieldLabelWithText:labelText]];
    [stack addArrangedSubview:control];
}

- (UIStackView*)newActionRow
{
    UIStackView* row = [[UIStackView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    row.translatesAutoresizingMaskIntoConstraints = NO;
    row.axis = UILayoutConstraintAxisHorizontal;
    row.spacing = 8.0;
    row.distribution = UIStackViewDistributionFillEqually;
    return row;
}

- (UIButton*)newActionButtonWithTitle:(NSString*)title action:(SEL)action
{
    UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    [button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
    return button;
}

- (UIView*)newPreviewHostWithText:(NSString*)text
{
    UIView* hostView = [[UIView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    hostView.translatesAutoresizingMaskIntoConstraints = NO;
    hostView.backgroundColor = UIColor.blackColor;
    hostView.clipsToBounds = YES;
    hostView.layer.cornerRadius = 4.0;

    UILabel* label = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    label.translatesAutoresizingMaskIntoConstraints = NO;
    label.tag = HBRPreviewHostLabelTag;
    label.text = text;
    label.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    label.textColor = UIColor.whiteColor;
    label.numberOfLines = 0;
    label.textAlignment = NSTextAlignmentCenter;
    [hostView addSubview:label];
    UILabel* overlayLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    overlayLabel.translatesAutoresizingMaskIntoConstraints = NO;
    overlayLabel.tag = HBRPreviewHostOverlayLabelTag;
    overlayLabel.numberOfLines = 3;
    overlayLabel.font = [UIFont systemFontOfSize:11.0 weight:UIFontWeightSemibold];
    overlayLabel.textColor = UIColor.whiteColor;
    overlayLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.55];
    overlayLabel.layer.cornerRadius = 4.0;
    overlayLabel.layer.masksToBounds = YES;
    overlayLabel.textAlignment = NSTextAlignmentLeft;
    [hostView addSubview:overlayLabel];
    UILabel* watermarkLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    watermarkLabel.translatesAutoresizingMaskIntoConstraints = NO;
    watermarkLabel.text = HBRDemoWatermarkText;
    watermarkLabel.font = [UIFont systemFontOfSize:11.0 weight:UIFontWeightBold];
    watermarkLabel.textColor = UIColor.whiteColor;
    watermarkLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.62];
    watermarkLabel.layer.cornerRadius = 6.0;
    watermarkLabel.layer.masksToBounds = YES;
    watermarkLabel.textAlignment = NSTextAlignmentCenter;
    [hostView addSubview:watermarkLabel];
    NSLayoutConstraint* aspectConstraint =
        [hostView.heightAnchor constraintEqualToAnchor:hostView.widthAnchor multiplier:0.75];
    [NSLayoutConstraint activateConstraints:@[
        aspectConstraint,
        [label.centerXAnchor constraintEqualToAnchor:hostView.centerXAnchor],
        [label.centerYAnchor constraintEqualToAnchor:hostView.centerYAnchor],
        [overlayLabel.leadingAnchor constraintEqualToAnchor:hostView.leadingAnchor constant:8.0],
        [overlayLabel.bottomAnchor constraintEqualToAnchor:hostView.bottomAnchor constant:-8.0],
        [overlayLabel.trailingAnchor constraintLessThanOrEqualToAnchor:hostView.trailingAnchor constant:-8.0],
        [watermarkLabel.trailingAnchor constraintEqualToAnchor:hostView.trailingAnchor constant:-8.0],
        [watermarkLabel.bottomAnchor constraintEqualToAnchor:hostView.bottomAnchor constant:-8.0],
        [watermarkLabel.heightAnchor constraintGreaterThanOrEqualToConstant:24.0],
        [watermarkLabel.widthAnchor constraintGreaterThanOrEqualToConstant:176.0],
    ]];
    return hostView;
}

- (void)addPlayerOperationSection
{
    self.playerSourceControl = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"Media URL" chinese:@"媒体 URL"],
        @"WHEP"
    ]];
    self.playerSourceControl.selectedSegmentIndex = 0;
    [self.playerSourceControl addTarget:self
                                 action:@selector(playerSourceChanged:)
                       forControlEvents:UIControlEventValueChanged];
    self.playerURLEdit = [self addTextFieldWithText:@"rtmp://demo.example/live/sample"
                                        placeholder:[self uiTextEnglish:@"Media URL or WHEP endpoint"
                                                                     chinese:@"媒体 URL 或 WHEP endpoint"]];
    self.playerWhepBearerEdit = [self addTextFieldWithText:@""
                                                placeholder:[self uiTextEnglish:@"Optional Bearer token"
                                                                             chinese:@"可选 Bearer Token"]];
    self.playerWhepBearerEdit.secureTextEntry = YES;
    self.playerWhepLocalBindEdit = [self addTextFieldWithText:@""
                                                   placeholder:[self uiTextEnglish:@"Optional numeric local bind IP"
                                                                                chinese:@"可选 numeric 本地绑定 IP"]];
    self.playerWhepAllowHTTPControl = [[UISwitch alloc] initWithFrame:CGRectZero];
    self.playerWhepOptionsStack = [[UIStackView alloc] initWithFrame:CGRectZero];
    self.playerWhepOptionsStack.axis = UILayoutConstraintAxisVertical;
    self.playerWhepOptionsStack.spacing = 6.0;
    [self addLabeledControl:self.playerWhepBearerEdit
                      label:[self uiTextEnglish:@"Bearer token" chinese:@"Bearer Token"]
                    toStack:self.playerWhepOptionsStack];
    [self addLabeledControl:self.playerWhepLocalBindEdit
                      label:[self uiTextEnglish:@"Local numeric bind" chinese:@"本地 numeric 绑定"]
                    toStack:self.playerWhepOptionsStack];
    [self addLabeledControl:self.playerWhepAllowHTTPControl
                      label:[self uiTextEnglish:@"Allow HTTP for isolated tests"
                                           chinese:@"仅隔离测试允许 HTTP"]
                    toStack:self.playerWhepOptionsStack];
    self.playerLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    self.playerLabel.numberOfLines = 0;
    self.playerLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    self.playerLabel.textColor = UIColor.secondaryLabelColor;
    self.displayModeControl = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"Stretch" chinese:@"拉伸"],
        [self uiTextEnglish:@"Fit" chinese:@"适应"],
        [self uiTextEnglish:@"Crop" chinese:@"裁剪"]
    ]];
    self.displayModeControl.selectedSegmentIndex = 1;
    [self.displayModeControl addTarget:self
                                action:@selector(displayModeChanged:)
                      forControlEvents:UIControlEventValueChanged];
    self.displayModeLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    self.displayModeLabel.numberOfLines = 0;
    self.displayModeLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    self.displayModeLabel.textColor = UIColor.secondaryLabelColor;
    self.playerPreviewHostView =
        [self newPreviewHostWithText:[self uiTextEnglish:@"Player Preview" chinese:@"播放器预览"]];
    UIStackView* actionRow = [self newActionRow];
    self.playerStartStopButton =
        [self newActionButtonWithTitle:[self uiTextEnglish:@"Play" chinese:@"播放"]
                                action:@selector(togglePlayerPrimaryAction)];
    [actionRow addArrangedSubview:self.playerStartStopButton];
    [self.stackView addArrangedSubview:self.playerPreviewHostView];
    [self addLabeledControl:self.playerSourceControl
                      label:[self uiTextEnglish:@"Source type" chinese:@"来源类型"]
                    toStack:self.stackView];
    [self.stackView addArrangedSubview:self.playerURLEdit];
    [self.stackView addArrangedSubview:self.playerWhepOptionsStack];
    [self.stackView addArrangedSubview:actionRow];
    [self addLabeledControl:self.displayModeControl
                      label:[self uiTextEnglish:@"Display Mode" chinese:@"显示模式"]
                    toStack:self.stackView];
    [self applyDisplayModeToPreviewHost];
    [self updatePlayerSourceUI];
    [self refreshPrimaryActionButtons];
    [self refreshPreviewOverlays];
}

- (void)playerSourceChanged:(UISegmentedControl*)sender
{
    (void)sender;
    // 来源类型和 WHEP 参数属于创建期合同；切换时释放旧会话及其深拷贝凭据。
    if (self.playerSession != nil)
    {
        [self.playerSession stop];
        self.playerSession = nil;
    }
    [self updatePlayerSourceUI];
    [self reloadDemoSnapshot];
}

- (void)updatePlayerSourceUI
{
    self.playerWhepOptionsStack.hidden = ![self isPlayerWhepSelected];
}

- (BOOL)isPlayerWhepSelected
{
    return self.playerSourceControl.selectedSegmentIndex == 1;
}
- (UISegmentedControl*)newCaptureSourceControl
{
    UISegmentedControl* control = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"Camera" chinese:@"相机"],
        [self uiTextEnglish:@"Microphone" chinese:@"麦克风"],
        [self uiTextEnglish:@"Screen" chinese:@"屏幕"]
    ]];
    control.selectedSegmentIndex = 0;
    [control addTarget:self
                action:@selector(captureSourceChanged:)
      forControlEvents:UIControlEventValueChanged];
    return control;
}
- (void)addCaptureOperationSection
{
    UILabel* titleLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    titleLabel.text = @"采集验证";
    titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];

    self.captureSourceControl = [[UISegmentedControl alloc] initWithItems:@[
        @"摄像头",
        @"麦克风",
        @"屏幕计划"
    ]];
    self.captureSourceControl.selectedSegmentIndex = 0;
    [self.captureSourceControl addTarget:self
                                  action:@selector(captureSourceChanged:)
                        forControlEvents:UIControlEventValueChanged];

    self.captureLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    self.captureLabel.numberOfLines = 0;
    self.captureLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    self.captureLabel.textColor = UIColor.secondaryLabelColor;

    UIStackView* actionRow = [[UIStackView alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    actionRow.axis = UILayoutConstraintAxisHorizontal;
    actionRow.spacing = 8.0;
    actionRow.distribution = UIStackViewDistributionFillEqually;

    UIButton* preflightButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [preflightButton setTitle:[self uiTextEnglish:@"Preflight" chinese:@"预检"]
                     forState:UIControlStateNormal];
    [preflightButton addTarget:self
                        action:@selector(preflightCapture)
              forControlEvents:UIControlEventTouchUpInside];

    UIButton* startButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [startButton setTitle:@"开始" forState:UIControlStateNormal];
    [startButton addTarget:self
                    action:@selector(startCapture)
          forControlEvents:UIControlEventTouchUpInside];

    UIButton* stopButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [stopButton setTitle:@"停止" forState:UIControlStateNormal];
    [stopButton addTarget:self
                   action:@selector(stopCapture)
         forControlEvents:UIControlEventTouchUpInside];

    [actionRow addArrangedSubview:preflightButton];
    [actionRow addArrangedSubview:startButton];
    [actionRow addArrangedSubview:stopButton];
    [self.stackView addArrangedSubview:titleLabel];
    [self.stackView addArrangedSubview:self.captureSourceControl];
    [self.stackView addArrangedSubview:actionRow];
    [self.stackView addArrangedSubview:self.captureLabel];
}

- (void)addPublisherOperationSection
{
    self.publisherInputControl = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"Camera" chinese:@"相机"],
        [self uiTextEnglish:@"Desktop" chinese:@"桌面"],
        [self uiTextEnglish:@"Encoded feed" chinese:@"编码样包"]
    ]];
    self.publisherInputControl.selectedSegmentIndex = 0;
    [self.publisherInputControl addTarget:self
                                   action:@selector(publisherInputChanged:)
                         forControlEvents:UIControlEventValueChanged];
    self.publisherAudioControl = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"None" chinese:@"无"],
        [self uiTextEnglish:@"Mic" chinese:@"麦克风"],
        [self uiTextEnglish:@"System" chinese:@"系统音频"],
        [self uiTextEnglish:@"File" chinese:@"文件"]
    ]];
    self.publisherAudioControl.selectedSegmentIndex = 1;
    [self.publisherAudioControl addTarget:self
                                   action:@selector(publisherInputChanged:)
                         forControlEvents:UIControlEventValueChanged];
    self.publisherResolutionControl = [[UISegmentedControl alloc] initWithItems:@[@"1280x720", @"1920x1080", @"640x480"]];
    self.publisherResolutionControl.selectedSegmentIndex = 0;
    [self.publisherResolutionControl addTarget:self
                                        action:@selector(publisherInputChanged:)
                              forControlEvents:UIControlEventValueChanged];
    self.publisherVideoCodecControl = [[UISegmentedControl alloc] initWithItems:@[@"H.264", @"H.265"]];
    self.publisherVideoCodecControl.selectedSegmentIndex = 0;
    [self.publisherVideoCodecControl addTarget:self
                                        action:@selector(publisherInputChanged:)
                              forControlEvents:UIControlEventValueChanged];
    self.publisherAudioCodecControl = [[UISegmentedControl alloc] initWithItems:@[@"AAC"]];
    self.publisherAudioCodecControl.selectedSegmentIndex = 0;
    self.publisherAudioVolumeSlider = [[UISlider alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    self.publisherAudioVolumeSlider.minimumValue = 0.0f;
    self.publisherAudioVolumeSlider.maximumValue = 100.0f;
    self.publisherAudioVolumeSlider.value = 80.0f;
    [self.publisherAudioVolumeSlider addTarget:self
                                        action:@selector(publisherInputChanged:)
                              forControlEvents:UIControlEventValueChanged];
    self.publisherAudioVolumeLabel =
        [self newFieldLabelWithText:[self uiTextEnglish:@"Capture Volume: 80%" chinese:@"采集音量：80%"]];
    self.publisherURLEdit = [self addTextFieldWithText:@"rtmp://192.0.2.1:1935/live/ios_demo"
                                           placeholder:[self uiTextEnglish:@"Publish URL" chinese:@"推流地址"]];
    self.publisherLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    self.publisherLabel.numberOfLines = 0;
    self.publisherLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    self.publisherLabel.textColor = UIColor.secondaryLabelColor;
    self.publisherPreviewHostView =
        [self newPreviewHostWithText:[self uiTextEnglish:@"Publisher Preview" chinese:@"推流预览"]];
    UIStackView* actionRow = [self newActionRow];
    self.publisherStartStopButton =
        [self newActionButtonWithTitle:[self uiTextEnglish:@"Publish" chinese:@"推流"]
                                action:@selector(togglePublisherPrimaryAction)];
    self.publisherDisplayModeControl = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"Stretch" chinese:@"拉伸"],
        [self uiTextEnglish:@"Fit" chinese:@"适应"],
        [self uiTextEnglish:@"Crop" chinese:@"裁剪"]
    ]];
    self.publisherDisplayModeControl.selectedSegmentIndex = 1;
    [self.publisherDisplayModeControl addTarget:self
                                         action:@selector(publisherDisplayModeChanged:)
                               forControlEvents:UIControlEventValueChanged];
    [actionRow addArrangedSubview:self.publisherStartStopButton];
    [actionRow addArrangedSubview:self.publisherDisplayModeControl];
    [self.stackView addArrangedSubview:self.publisherPreviewHostView];
    [self addLabeledControl:self.publisherURLEdit
                      label:[self uiTextEnglish:@"Publish URL" chinese:@"推流地址"]
                    toStack:self.stackView];
    [self.stackView addArrangedSubview:actionRow];
    [self addLabeledControl:self.publisherInputControl
                      label:[self uiTextEnglish:@"Source" chinese:@"来源"]
                    toStack:self.stackView];
    [self addLabeledControl:self.publisherAudioControl
                      label:[self uiTextEnglish:@"Audio" chinese:@"音频"]
                    toStack:self.stackView];
    [self addLabeledControl:self.publisherResolutionControl
                      label:[self uiTextEnglish:@"Resolution" chinese:@"分辨率"]
                    toStack:self.stackView];
    [self addLabeledControl:self.publisherVideoCodecControl
                      label:[self uiTextEnglish:@"Video Codec" chinese:@"视频编码"]
                    toStack:self.stackView];
    [self addLabeledControl:self.publisherAudioCodecControl
                      label:[self uiTextEnglish:@"Audio Codec" chinese:@"音频编码"]
                    toStack:self.stackView];
    [self.stackView addArrangedSubview:self.publisherAudioVolumeLabel];
    [self.stackView addArrangedSubview:self.publisherAudioVolumeSlider];
    [self syncHiddenCaptureSourceSelection];
    [self applyPublisherDisplayModeToPreviewHost];
    [self updatePublisherPreviewPresentation];
    [self refreshPrimaryActionButtons];
    [self refreshPreviewOverlays];
}
- (void)addGB28181OperationSection
{
    self.gbLocalIdEdit = [self addTextFieldWithText:@"34020000001320000001"
                                        placeholder:[self uiTextEnglish:@"Local Device/Channel ID" chinese:@"本地设备/通道 ID"]];
    self.gbLocalDomainEdit = [self addTextFieldWithText:@"3402000000"
                                             placeholder:[self uiTextEnglish:@"Local SIP Domain" chinese:@"本地 SIP 域"]];
    self.gbLocalIpEdit = [self addTextFieldWithText:@"0.0.0.0"
                                        placeholder:[self uiTextEnglish:@"Local SIP IP" chinese:@"本地 SIP IP"]];
    self.gbLocalPortEdit = [self addTextFieldWithText:@"5060"
                                          placeholder:[self uiTextEnglish:@"Local SIP Port" chinese:@"本地 SIP 端口"]];
    self.gbLocalPortEdit.keyboardType = UIKeyboardTypeNumberPad;
    self.gbUpperIdEdit = [self addTextFieldWithText:@"34020000002000000001"
                                        placeholder:[self uiTextEnglish:@"Upper Platform ID" chinese:@"上级平台 ID"]];
    self.gbUpperDomainEdit = [self addTextFieldWithText:@"3402000000"
                                             placeholder:[self uiTextEnglish:@"Upper SIP Domain" chinese:@"上级 SIP 域"]];
    self.gbUpperPasswordEdit = [self addTextFieldWithText:@"123456"
                                               placeholder:[self uiTextEnglish:@"SIP Password" chinese:@"SIP 密码"]];
    self.gbUpperPasswordEdit.secureTextEntry = YES;
    self.gbUpperIpEdit = [self addTextFieldWithText:@"192.0.2.1"
                                        placeholder:[self uiTextEnglish:@"Upper Platform IP" chinese:@"上级平台 IP"]];
    self.gbUpperPortEdit = [self addTextFieldWithText:@"5060"
                                          placeholder:[self uiTextEnglish:@"Upper Port" chinese:@"上级端口"]];
    self.gbUpperPortEdit.keyboardType = UIKeyboardTypeNumberPad;
    self.gbTransportControl = [[UISegmentedControl alloc] initWithItems:@[
        @"TCP",
        @"UDP"
    ]];
    self.gbTransportControl.selectedSegmentIndex = 0;
    self.gbSourceControl = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"Camera + Mic" chinese:@"相机 + 麦克风"],
        [self uiTextEnglish:@"Video File" chinese:@"视频文件"]
    ]];
    self.gbSourceControl.selectedSegmentIndex = 0;
    [self.gbSourceControl addTarget:self
                             action:@selector(gb28181SourceChanged:)
                   forControlEvents:UIControlEventValueChanged];
    self.gbMediaSourceEdit = [self addTextFieldWithText:@""
                                            placeholder:[self uiTextEnglish:@"Selected video path" chinese:@"已选视频路径"]];
    self.gbMediaSourceEdit.keyboardType = UIKeyboardTypeURL;
    [self.gbMediaSourceEdit addTarget:self
                               action:@selector(gb28181FieldChanged:)
                     forControlEvents:UIControlEventEditingChanged];
    self.gbResolutionControl = [[UISegmentedControl alloc] initWithItems:@[
        @"1280x720",
        @"1920x1080",
        @"640x480"
    ]];
    self.gbResolutionControl.selectedSegmentIndex = 0;
    [self.gbResolutionControl addTarget:self
                                 action:@selector(gb28181SourceChanged:)
                       forControlEvents:UIControlEventValueChanged];
    self.gbPreviewDisplayModeControl = [[UISegmentedControl alloc] initWithItems:@[
        [self uiTextEnglish:@"Stretch" chinese:@"拉伸"],
        [self uiTextEnglish:@"Fit" chinese:@"适应"],
        [self uiTextEnglish:@"Crop" chinese:@"裁剪"]
    ]];
    self.gbPreviewDisplayModeControl.selectedSegmentIndex = 1;
    [self.gbPreviewDisplayModeControl addTarget:self
                                         action:@selector(gb28181DisplayModeChanged:)
                               forControlEvents:UIControlEventValueChanged];
    self.gbMediaIpEdit = [self addTextFieldWithText:@"0.0.0.0"
                                        placeholder:[self uiTextEnglish:@"Media RTP IP" chinese:@"媒体 RTP IP"]];
    self.gbMediaPortEdit = [self addTextFieldWithText:@"15060"
                                          placeholder:[self uiTextEnglish:@"Media Port" chinese:@"媒体端口"]];
    self.gbMediaPortEdit.keyboardType = UIKeyboardTypeNumberPad;
    self.gbRegisterExpiresEdit = [self addTextFieldWithText:@"3600"
                                                 placeholder:[self uiTextEnglish:@"Register Expires" chinese:@"注册有效期"]];
    self.gbRegisterExpiresEdit.keyboardType = UIKeyboardTypeNumberPad;
    self.gbKeepaliveEdit = [self addTextFieldWithText:@"60"
                                          placeholder:[self uiTextEnglish:@"Keepalive Interval" chinese:@"心跳间隔"]];
    self.gbKeepaliveEdit.keyboardType = UIKeyboardTypeNumberPad;
    self.gbPreviewHostView =
        [self newPreviewHostWithText:[self uiTextEnglish:@"GB28181 Preview" chinese:@"GB28181 预览"]];
    UIStackView* previewActionRow = [self newActionRow];
    self.gb28181StartStopButton =
        [self newActionButtonWithTitle:[self uiTextEnglish:@"Start" chinese:@"启动"]
                                action:@selector(toggleGB28181PrimaryAction)];
    [previewActionRow addArrangedSubview:self.gb28181StartStopButton];
    [previewActionRow addArrangedSubview:self.gbPreviewDisplayModeControl];
    self.gb28181Label = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    self.gb28181Label.numberOfLines = 0;
    self.gb28181Label.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    self.gb28181Label.textColor = UIColor.secondaryLabelColor;
    [self.stackView addArrangedSubview:self.gbPreviewHostView];
    [self.stackView addArrangedSubview:previewActionRow];
    [self addLabeledControl:self.gbSourceControl
                      label:[self uiTextEnglish:@"Source" chinese:@"来源"]
                    toStack:self.stackView];
    self.gbMediaSourceLabel =
        [self newFieldLabelWithText:[self uiTextEnglish:@"Video File" chinese:@"视频文件"]];
    [self.stackView addArrangedSubview:self.gbMediaSourceLabel];
    [self.stackView addArrangedSubview:self.gbMediaSourceEdit];
    [self addLabeledControl:self.gbResolutionControl
                      label:[self uiTextEnglish:@"Resolution" chinese:@"分辨率"]
                    toStack:self.stackView];
    [self.stackView addArrangedSubview:[self newFieldLabelWithText:[self uiTextEnglish:@"Local SIP" chinese:@"本机 SIP"]]];
    [self addLabeledControl:self.gbLocalIdEdit
                      label:[self uiTextEnglish:@"Device/channel ID" chinese:@"设备/通道 ID"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbLocalDomainEdit
                      label:[self uiTextEnglish:@"SIP Domain" chinese:@"SIP 域"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbLocalIpEdit
                      label:[self uiTextEnglish:@"Local SIP IP" chinese:@"本机 SIP IP"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbLocalPortEdit
                      label:[self uiTextEnglish:@"Local SIP Port" chinese:@"本机 SIP 端口"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbMediaIpEdit
                      label:[self uiTextEnglish:@"Media RTP IP" chinese:@"媒体 RTP IP"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbMediaPortEdit
                      label:[self uiTextEnglish:@"Media Port" chinese:@"媒体端口"]
                    toStack:self.stackView];
    [self.stackView addArrangedSubview:[self newFieldLabelWithText:[self uiTextEnglish:@"Upper Platform" chinese:@"上级平台"]]];
    [self addLabeledControl:self.gbUpperIdEdit
                      label:[self uiTextEnglish:@"Upper ID" chinese:@"上级 ID"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbUpperDomainEdit
                      label:[self uiTextEnglish:@"SIP Domain" chinese:@"SIP 域"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbUpperPasswordEdit
                      label:[self uiTextEnglish:@"SIP Password" chinese:@"SIP 密码"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbUpperIpEdit
                      label:[self uiTextEnglish:@"Upper IP" chinese:@"上级 IP"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbUpperPortEdit
                      label:[self uiTextEnglish:@"Upper Port" chinese:@"上级端口"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbTransportControl
                      label:[self uiTextEnglish:@"Transport" chinese:@"传输方式"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbRegisterExpiresEdit
                      label:[self uiTextEnglish:@"Register Expires (s)" chinese:@"注册有效期（秒）"]
                    toStack:self.stackView];
    [self addLabeledControl:self.gbKeepaliveEdit
                      label:[self uiTextEnglish:@"Keepalive (s)" chinese:@"心跳间隔（秒）"]
                    toStack:self.stackView];
    [self syncGB28181SourceControls];
    [self applyGB28181DisplayModeToPreviewHost];
    [self refreshPrimaryActionButtons];
    [self refreshPreviewOverlays];
}

- (void)togglePlayerPrimaryAction
{
    HBRStreamCorePlayerRuntimeInfo* runtimeInfo =
        self.playerSession != nil ? [self.playerSession runtimeInfo] : nil;
    if (runtimeInfo != nil && runtimeInfo.sessionState == HBRStreamCoreSessionStateRunning)
    {
        [self stopPlayer];
        return;
    }
    [self startPlayer];
}

- (void)togglePublisherPrimaryAction
{
    HBRStreamCorePublisherRuntimeInfo* runtimeInfo =
        self.publisherSession != nil ? [self.publisherSession runtimeInfo] : nil;
    if (runtimeInfo != nil && runtimeInfo.sessionState == HBRStreamCoreSessionStateRunning)
    {
        [self stopPublisher];
        return;
    }
    [self startPublisher];
}

- (void)toggleGB28181PrimaryAction
{
    HBRStreamCoreGB28181RuntimeInfo* runtimeInfo =
        self.gb28181Session != nil ? [self.gb28181Session runtimeInfo] : nil;
    if (runtimeInfo != nil && (runtimeInfo.started || runtimeInfo.registeredToUpperPlatform))
    {
        [self stopGB28181];
        return;
    }
    [self startGB28181];
}

- (void)refreshPrimaryActionButtons
{
    HBRStreamCorePlayerRuntimeInfo* playerRuntime =
        self.playerSession != nil ? [self.playerSession runtimeInfo] : nil;
    if (self.playerStartStopButton != nil)
    {
        NSString* title = (playerRuntime != nil && playerRuntime.sessionState == HBRStreamCoreSessionStateRunning) ?
            [self uiTextEnglish:@"Stop" chinese:@"停止"] :
            [self uiTextEnglish:@"Play" chinese:@"播放"];
        [self.playerStartStopButton setTitle:title forState:UIControlStateNormal];
    }

    HBRStreamCorePublisherRuntimeInfo* publisherRuntime =
        self.publisherSession != nil ? [self.publisherSession runtimeInfo] : nil;
    if (self.publisherStartStopButton != nil)
    {
        NSString* title = (publisherRuntime != nil && publisherRuntime.sessionState == HBRStreamCoreSessionStateRunning) ?
            [self uiTextEnglish:@"Stop" chinese:@"停止"] :
            [self uiTextEnglish:@"Publish" chinese:@"推流"];
        [self.publisherStartStopButton setTitle:title forState:UIControlStateNormal];
    }

    HBRStreamCoreGB28181RuntimeInfo* gbRuntime =
        self.gb28181Session != nil ? [self.gb28181Session runtimeInfo] : nil;
    if (self.gb28181StartStopButton != nil)
    {
        NSString* title = (gbRuntime != nil && (gbRuntime.started || gbRuntime.registeredToUpperPlatform)) ?
            [self uiTextEnglish:@"Stop" chinese:@"停止"] :
            [self uiTextEnglish:@"Start" chinese:@"启动"];
        [self.gb28181StartStopButton setTitle:title forState:UIControlStateNormal];
    }
}
- (UILabel*)addSectionWithTitle:(NSString*)title
{
    UILabel* titleLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    titleLabel.text = title;
    titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];

    UILabel* bodyLabel = [[UILabel alloc] initWithFrame:(CGRect){0, 0, 0, 0}];
    bodyLabel.numberOfLines = 0;
    bodyLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    bodyLabel.textColor = UIColor.secondaryLabelColor;

    [self.stackView addArrangedSubview:titleLabel];
    [self.stackView addArrangedSubview:bodyLabel];
    return bodyLabel;
}

- (void)reloadDemoSnapshot
{
    [self applyDisplayModeToPreviewHost];
    [self applyPublisherDisplayModeToPreviewHost];
    [self applyGB28181DisplayModeToPreviewHost];
    [self syncGB28181SourceControls];
    self.displayModeLabel.text = [self displayModeText];

    HBRStreamCoreProductInfo* productInfo = [HBRStreamCoreSDK productInfo];
    self.productLabel.text = [NSString stringWithFormat:
        @"%@ %@\nTarget: %@ | Package: %@",
        productInfo.productName,
        productInfo.version,
        productInfo.primaryTargetName,
        [self compactText:[HBRStreamCoreSDK applePackagingStatus] maxLength:44]];

    HBRStreamCoreRuntime* runtime = [HBRStreamCoreSDK runtime];
    HBRStreamCoreRuntimeConfig* runtimeConfig = [runtime defaultConfig];
    // 1.6.0 起产品标识、验签材料和 Bundle 身份由正式 framework 自动提供并采集；
    // Demo 只提交客户可配置的授权文件入口，避免重建已从公开合同移除的兼容字段。
    runtimeConfig.licensePath = [self bundledPathForResource:@"streamcore_demo" type:@"lic"];
    HBRStreamCoreOperationStatus* configureStatus = [runtime configure:runtimeConfig];
    HBRStreamCoreOperationStatus* logStatus =
        [runtime configureLogWithDirectory:@""
                                  fileName:@"streamcore_demo.log"
                              minimumLevel:HBRStreamCoreLogLevelInfo
                         enablePlatformLog:YES];
    HBRStreamCoreLogInfo* logInfo = [runtime logInfo];
    self.logDirectory = logInfo.logDirectory;
    self.logFileName = logInfo.logFileName;
    self.productCrashDirectory = @"";
    [self setOperationAction:@"log.configure"
                        code:[NSString stringWithFormat:@"%ld", (long)logStatus.resultCode]
                      status:logStatus.statusName
                     summary:logInfo.stateSummary];
    HBRStreamCoreRuntimeLicenseInfo* licenseInfo = [runtime licenseInfo];
    NSLog(@"StreamCoreDemo runtime.configure status=%@ result=%ld platformIdentity=auto",
          configureStatus.statusName,
          (long)configureStatus.resultCode);
    NSLog(@"StreamCoreDemo license configured=%d loaded=%d valid=%d status=%@ summary=%@",
          licenseInfo.configured,
          licenseInfo.licenseLoaded,
          licenseInfo.licenseValid,
          licenseInfo.statusName,
          licenseInfo.summary);
    self.licenseLabel.text = [self runtimeTextWithStatus:configureStatus
                                             licenseInfo:licenseInfo
                                              logStatus:logStatus
                                                logInfo:logInfo
                                                runtime:runtime];
    self.playerLabel.text = [self playerText];
    self.captureLabel.text = [self captureText];
    self.publisherLabel.text = [self publisherText];
    self.gb28181Label.text = [NSString stringWithFormat:@"%@\n%@",
        [self gb28181ConfigText],
        [self gb28181Text]];
    [self refreshPreviewOverlays];
}

- (NSString*)runtimeTextWithStatus:(HBRStreamCoreOperationStatus*)status
                       licenseInfo:(HBRStreamCoreRuntimeLicenseInfo*)licenseInfo
                          logStatus:(HBRStreamCoreOperationStatus*)logStatus
                            logInfo:(HBRStreamCoreLogInfo*)logInfo
                            runtime:(HBRStreamCoreRuntime*)runtime
{
    HBRStreamCoreFeatureResult* feature = [runtime featureResultForName:@"streamcore_demo"
                                                            defaultValue:NO];
    HBRStreamCoreLimitResult* limit = [runtime limitResultForName:@"max_input_channels"
                                                     defaultValue:0];
    return [NSString stringWithFormat:
        @"%@: %@ / %ld\n%@: %@ valid=%d watermark=%d\n%@: %@=%d | %@=%lld\n%@: %@ / %ld | %@",
        [self uiTextEnglish:@"Runtime" chinese:@"运行时"],
        status.statusName,
        (long)status.resultCode,
        [self uiTextEnglish:@"License" chinese:@"授权"],
        licenseInfo.statusName,
        licenseInfo.licenseValid,
        licenseInfo.needWatermark,
        [self uiTextEnglish:@"Feature" chinese:@"能力"],
        feature.featureName,
        feature.enabled,
        limit.limitName,
        (long long)limit.limitValue,
        [self uiTextEnglish:@"Log" chinese:@"日志"],
        logStatus.statusName,
        (long)logStatus.resultCode,
        logInfo.logFileName];
}

- (NSString*)playerText
{
    HBRStreamCorePlayerRuntimeInfo* runtimeInfo =
        self.playerSession != nil ? [self.playerSession runtimeInfo] : nil;
    return [NSString stringWithFormat:
        @"%@: %@ · %@\n%@: %@\n%@: %@",
        [self uiTextEnglish:@"Source" chinese:@"来源"],
        [self isPlayerWhepSelected] ? @"WHEP" : [self uiTextEnglish:@"Media URL" chinese:@"媒体 URL"],
        [self compactText:[self playerDisplaySource] maxLength:56],
        [self uiTextEnglish:@"Display" chinese:@"显示"],
        [self selectedDisplayModeText],
        [self uiTextEnglish:@"State" chinese:@"状态"],
        runtimeInfo != nil ?
            [self compactText:runtimeInfo.stateSummary maxLength:56] :
            [self uiTextEnglish:@"Waiting for preflight or start." chinese:@"等待预检或启动。"]];
}

- (NSString*)captureText
{
    const HBRStreamCoreCaptureSourceKind sourceKind = [self selectedCaptureSourceKind];
    HBRStreamCoreApplePermissionRequirements* permission =
        [HBRStreamCoreApplePermissions requirementsForCaptureSourceKind:sourceKind
                                                            enableAudio:[self selectedCaptureEnableAudio]
                                                            enableVideo:[self selectedCaptureEnableVideo]];
    HBRStreamCoreAppleScreenCaptureRequirements* screen =
        [HBRStreamCoreApplePermissions
            screenCaptureRequirementsForSourceKind:HBRStreamCoreCaptureSourceKindDesktop];
    HBRStreamCoreCaptureRuntimeInfo* runtimeInfo =
        self.captureSession != nil ? [self.captureSession runtimeInfo] : nil;
    return [NSString stringWithFormat:
        @"%@: %@\n%@: %@\n%@: %@\n%@: %@\n%@: %@",
        [self uiTextEnglish:@"Source" chinese:@"来源"],
        [self selectedCaptureSourceText],
        [self uiTextEnglish:@"Display" chinese:@"显示"],
        [self selectedDisplayModeText],
        [self uiTextEnglish:@"Permission Keys" chinese:@"权限键"],
        [permission.usageDescriptionKeys componentsJoinedByString:@", "],
        [self uiTextEnglish:@"Screen Capture" chinese:@"屏幕采集"],
        screen.summary,
        [self uiTextEnglish:@"State" chinese:@"状态"],
        runtimeInfo != nil ? runtimeInfo.stateSummary : [self uiTextEnglish:@"Waiting for preflight or start." chinese:@"等待预检或启动。"]];
}

- (NSString*)publisherText
{
    HBRStreamCorePublisherRuntimeInfo* runtimeInfo =
        self.publisherSession != nil ? [self.publisherSession runtimeInfo] : nil;
    return [NSString stringWithFormat:
        @"%@: %@\n%@: %@ %.0f%%\n%@: %@ / %@ / %@\n%@: %@",
        [self uiTextEnglish:@"Source" chinese:@"来源"],
        [self selectedPublisherSourceText],
        [self uiTextEnglish:@"Audio" chinese:@"音频"],
        [self selectedPublisherAudioText],
        [self selectedPublisherAudioVolumePercent],
        [self uiTextEnglish:@"Video" chinese:@"视频"],
        [self selectedPublisherVideoCodecName],
        [self selectedPublisherAudioCodecName],
        [self selectedPublisherResolutionText],
        [self uiTextEnglish:@"State" chinese:@"状态"],
        runtimeInfo != nil ?
            [self compactText:runtimeInfo.stateSummary maxLength:56] :
            [self uiTextEnglish:@"Waiting for preflight or start." chinese:@"等待预检或启动。"]];
}

- (NSString*)playerPreviewOverlayText
{
    HBRStreamCorePlayerRuntimeInfo* runtimeInfo =
        self.playerSession != nil ? [self.playerSession runtimeInfo] : nil;
    NSString* stateText = runtimeInfo != nil ?
        [self compactText:runtimeInfo.stateSummary maxLength:40] :
        [self uiTextEnglish:@"Idle" chinese:@"Idle"];
    return [NSString stringWithFormat:@"%@ | %@\n%@",
        [self uiTextEnglish:@"Player" chinese:@"播放"],
        [self selectedDisplayModeText],
        stateText];
}

- (NSString*)publisherPreviewOverlayText
{
    HBRStreamCorePublisherRuntimeInfo* runtimeInfo =
        self.publisherSession != nil ? [self.publisherSession runtimeInfo] : nil;
    NSString* stateText = runtimeInfo != nil ?
        [self compactText:runtimeInfo.stateSummary maxLength:40] :
        [self uiTextEnglish:@"Idle" chinese:@"Idle"];
    return [NSString stringWithFormat:@"%@ | %@\n%@",
        [self selectedPublisherSourceText],
        [self selectedPublisherResolutionText],
        stateText];
}

- (NSString*)gb28181PreviewOverlayText
{
    HBRStreamCoreGB28181RuntimeInfo* runtimeInfo =
        self.gb28181Session != nil ? [self.gb28181Session runtimeInfo] : nil;
    NSString* stateText = runtimeInfo != nil ?
        [self compactText:runtimeInfo.stateSummary maxLength:40] :
        [self uiTextEnglish:@"Idle" chinese:@"Idle"];
    return [NSString stringWithFormat:@"%@ | %@\n%@:%@ %@\n%@ | RTP %@:%@",
        [self selectedGB28181SourceText],
        [self selectedGB28181DisplayModeText],
        self.gbUpperIpEdit.text.length > 0 ? self.gbUpperIpEdit.text : @"-",
        self.gbUpperPortEdit.text.length > 0 ? self.gbUpperPortEdit.text : @"-",
        [self gb28181TransportText],
        stateText,
        self.gbMediaIpEdit.text.length > 0 ? self.gbMediaIpEdit.text : @"-",
        self.gbMediaPortEdit.text.length > 0 ? self.gbMediaPortEdit.text : @"-"];
}

- (void)refreshPreviewOverlays
{
    UILabel* playerOverlay = [self previewOverlayLabelForHost:self.playerPreviewHostView];
    if (playerOverlay != nil)
    {
        playerOverlay.text = [self playerPreviewOverlayText];
    }
    UILabel* publisherOverlay = [self previewOverlayLabelForHost:self.publisherPreviewHostView];
    if (publisherOverlay != nil)
    {
        publisherOverlay.hidden = [self shouldShowPublisherPassthroughHint];
        publisherOverlay.text = [self publisherPreviewOverlayText];
    }
    UILabel* gbOverlay = [self previewOverlayLabelForHost:self.gbPreviewHostView];
    if (gbOverlay != nil)
    {
        gbOverlay.text = [self gb28181PreviewOverlayText];
    }
    [self refreshPrimaryActionButtons];
}

- (NSString*)playerURLText
{
    NSString* url = self.playerURLEdit.text.length > 0 ?
        self.playerURLEdit.text :
        @"rtmp://demo.example/live/sample";
    return url;
}

- (NSString*)playerDisplaySource
{
    NSString* source = [self playerURLText];
    if (![self isPlayerWhepSelected])
    {
        return source;
    }
    NSURLComponents* components = [NSURLComponents componentsWithString:source];
    if (components.scheme.length == 0 || components.host.length == 0)
    {
        return @"<invalid WHEP endpoint>";
    }
    components.user = nil;
    components.password = nil;
    components.query = nil;
    components.fragment = nil;
    return components.string.length > 0 ? components.string : @"<invalid WHEP endpoint>";
}

- (NSString*)publisherURLText
{
    NSString* url = self.publisherURLEdit.text.length > 0 ?
        self.publisherURLEdit.text :
        [self defaultPublisherURLText];
    return url;
}

- (NSString*)defaultPublisherURLText
{
    return @"rtmp://192.0.2.1:1935/live/ios_demo";
}

- (HBRStreamCoreCaptureSourceKind)selectedCaptureSourceKind
{
    if (self.captureSourceControl.selectedSegmentIndex == 1)
    {
        return HBRStreamCoreCaptureSourceKindMicrophone;
    }
    if (self.captureSourceControl.selectedSegmentIndex == 2)
    {
        return HBRStreamCoreCaptureSourceKindDesktop;
    }
    return HBRStreamCoreCaptureSourceKindCamera;
}

- (BOOL)selectedCaptureEnableAudio
{
    return self.captureSourceControl.selectedSegmentIndex == 1;
}

- (BOOL)selectedCaptureEnableVideo
{
    return self.captureSourceControl.selectedSegmentIndex != 1;
}

- (NSString*)selectedCaptureSourceText
{
    if (self.captureSourceControl.selectedSegmentIndex == 1)
    {
        return [self uiTextEnglish:@"Microphone" chinese:@"麦克风"];
    }
    if (self.captureSourceControl.selectedSegmentIndex == 2)
    {
        return [self uiTextEnglish:@"Screen" chinese:@"屏幕"];
    }
    return [self uiTextEnglish:@"Camera" chinese:@"相机"];
}

- (HBRStreamCorePublisherInputKind)selectedPublisherInputKind
{
    if (self.publisherInputControl.selectedSegmentIndex == 2)
    {
        return HBRStreamCorePublisherInputKindAppEncodedFeed;
    }
    if (self.publisherInputControl.selectedSegmentIndex == 0 ||
        self.publisherInputControl.selectedSegmentIndex == 1)
    {
        return HBRStreamCorePublisherInputKindAppRawFeed;
    }
    return HBRStreamCorePublisherInputKindAppEncodedFeed;
}

- (NSString*)selectedPublisherInputText
{
    return [NSString stringWithFormat:@"%@ + %@",
        [self selectedPublisherSourceText],
        [self selectedPublisherAudioText]];
}

- (NSString*)selectedPublisherSourceText
{
    switch (self.publisherInputControl.selectedSegmentIndex)
    {
    case 0:
        return [self uiTextEnglish:@"Camera" chinese:@"相机"];
    case 1:
        return [self uiTextEnglish:@"Desktop" chinese:@"桌面"];
    case 2:
        return [self uiTextEnglish:@"Encoded feed" chinese:@"编码样包"];
    default:
        return [self uiTextEnglish:@"Camera" chinese:@"相机"];
    }
}

- (NSString*)selectedPublisherAudioText
{
    switch (self.publisherAudioControl.selectedSegmentIndex)
    {
    case 1:
        return [self uiTextEnglish:@"Mic" chinese:@"麦克风"];
    case 2:
        return [self uiTextEnglish:@"System audio" chinese:@"系统音频"];
    case 3:
        return [self uiTextEnglish:@"File audio" chinese:@"文件音轨"];
    case 0:
    default:
        return [self uiTextEnglish:@"No audio" chinese:@"无音频"];
    }
}

- (BOOL)selectedPublisherAudioEnabled
{
    return self.publisherAudioControl.selectedSegmentIndex != 0;
}

- (BOOL)selectedPublisherVideoEnabled
{
    return YES;
}

- (CGFloat)selectedPublisherAudioVolumePercent
{
    return self.publisherAudioVolumeSlider != nil ?
        self.publisherAudioVolumeSlider.value :
        80.0f;
}

- (CGSize)selectedPublisherTargetSize
{
    if (self.publisherResolutionControl.selectedSegmentIndex == 1)
    {
        return CGSizeMake(1920.0, 1080.0);
    }
    if (self.publisherResolutionControl.selectedSegmentIndex == 2)
    {
        return CGSizeMake(640.0, 480.0);
    }
    return CGSizeMake(1280.0, 720.0);
}

- (NSString*)selectedPublisherResolutionText
{
    CGSize size = [self selectedPublisherTargetSize];
    if (size.width <= 0.0 || size.height <= 0.0)
    {
        return [self uiTextEnglish:@"No video" chinese:@"无视频"];
    }
    return [NSString stringWithFormat:@"%ldx%ld",
        (long)size.width,
        (long)size.height];
}

- (NSString*)selectedPublisherVideoCodecName
{
    return self.publisherVideoCodecControl.selectedSegmentIndex == 1 ? @"h265" : @"h264";
}

- (NSString*)selectedPublisherAudioCodecName
{
    return @"aac";
}

- (NSString*)statusText:(HBRStreamCoreOperationStatus*)status
{
    if (status == nil)
    {
        return @"status=nil";
    }
    NSString* summary = status.summary.length > 0 ? status.summary : status.detail;
    return [NSString stringWithFormat:@"%@ / %ld\n%@",
        status.statusName,
        (long)status.resultCode,
        summary.length > 0 ? summary : @""];
}

- (HBRStreamCoreOperationStatus*)configurePlayerSession
{
    if (self.playerSession == nil)
    {
        self.playerSession = [[HBRStreamCorePlayerSession alloc] init];
    }
    if ([self isPlayerWhepSelected])
    {
        HBRStreamCorePlayerWhepOptions* options =
            [HBRStreamCorePlayerWhepOptions defaultOptions];
        options.bearerToken = self.playerWhepBearerEdit.text ?: @"";
        options.localBindIPAddress = self.playerWhepLocalBindEdit.text ?: @"";
        options.allowInsecureHTTP = self.playerWhepAllowHTTPControl.isOn;
        HBRStreamCoreOperationStatus* optionsStatus =
            [self.playerSession configureWhepOptions:options];
        if (optionsStatus.resultCode != 0)
        {
            return optionsStatus;
        }
    }
    HBRStreamCorePlayerConfig* config = [self.playerSession defaultConfig];
    config.sessionName = [self isPlayerWhepSelected] ?
        @"ios_demo_whep_player" :
        @"ios_demo_url_player";
    config.sourceKind = [self isPlayerWhepSelected] ?
        HBRStreamCorePlayerSourceKindWhep :
        HBRStreamCorePlayerSourceKindURL;
    config.sourceURL = [self playerURLText];
    config.renderMode = HBRStreamCorePlayerRenderModeNativeWindow;
    config.nativeRenderView = self.playerPreviewHostView;
    config.enableAudio = YES;
    return [self.playerSession configureWithConfig:config];
}

- (HBRStreamCoreOperationStatus*)configureCaptureSession
{
    if (self.captureSession == nil)
    {
        self.captureSession = [[HBRStreamCoreCaptureSession alloc] init];
    }
    HBRStreamCoreCaptureConfig* config = [self.captureSession defaultConfig];
    config.sessionName = @"ios_demo_capture";
    config.sourceKind = [self selectedCaptureSourceKind];
    config.sourceIdentifier = config.sourceKind == HBRStreamCoreCaptureSourceKindCamera ?
        @"front_camera" :
        @"";
    config.enableAudio = [self selectedCaptureEnableAudio];
    config.enableVideo = [self selectedCaptureEnableVideo];
    return [self.captureSession configureWithConfig:config];
}

- (HBRStreamCoreOperationStatus*)configurePublisherSession
{
    if (self.publisherSession == nil)
    {
        self.publisherSession = [[HBRStreamCorePublisherSession alloc] init];
    }
    HBRStreamCorePublisherConfig* config = [self.publisherSession defaultConfig];
    CGSize targetSize = [self selectedPublisherTargetSize];
    NSString* targetAudioCodecName = [self selectedPublisherAudioCodecName];
    NSString* targetVideoCodecName = [self selectedPublisherVideoCodecName];
    const BOOL enableAudio = [self selectedPublisherAudioEnabled];
    const BOOL enableVideo = [self selectedPublisherVideoEnabled];
    config.sessionName = @"ios_demo_publisher";
    config.publishURL = [self publisherURLText];
    config.inputKind = [self selectedPublisherInputKind];
    config.transcodeOptions.targetAudioCodecName = targetAudioCodecName;
    config.transcodeOptions.targetVideoCodecName = targetVideoCodecName;
    config.transcodeOptions.targetAudioBitrateKbps = 96;
    config.transcodeOptions.targetVideoBitrateKbps = 1200;
    config.transcodeOptions.targetSampleRate = 48000;
    config.transcodeOptions.targetChannelCount = 2;
    config.transcodeOptions.targetWidth = enableVideo ? (NSInteger)targetSize.width : 0;
    config.transcodeOptions.targetHeight = enableVideo ? (NSInteger)targetSize.height : 0;
    config.transcodeOptions.targetFps = 25;
    config.transcodeOptions.targetGopFrames = 50;
    if (config.inputKind == HBRStreamCorePublisherInputKindAppRawFeed)
    {
        config.inputBindingIdentifier =
            self.publisherInputControl.selectedSegmentIndex == 1 ?
                @"screen:broadcast" :
                @"camera:front";
        if (enableAudio)
        {
            config.inputBindingIdentifier =
                [config.inputBindingIdentifier stringByAppendingString:@"+microphone"];
        }
        config.sourceMediaProfile.containerName = @"raw";
        config.sourceMediaProfile.audioCodecName = enableAudio ? @"pcm" : @"";
        config.sourceMediaProfile.videoCodecName = enableVideo ? @"nv12" : @"";
        config.sourceMediaProfile.hasAudio = enableAudio;
        config.sourceMediaProfile.hasVideo = enableVideo;
        config.sourceMediaProfile.width = enableVideo ? (NSInteger)targetSize.width : 0;
        config.sourceMediaProfile.height = enableVideo ? (NSInteger)targetSize.height : 0;
        config.sourceMediaProfile.fps = 25;
        config.sourceMediaProfile.sampleRate = 48000;
        config.sourceMediaProfile.channelCount = 2;
        config.transcodeOptions.audioMode = HBRStreamCorePublisherTranscodeModeAuto;
        config.transcodeOptions.videoMode = HBRStreamCorePublisherTranscodeModeAuto;
    }
    else
    {
        config.inputBindingIdentifier = enableVideo ? @"encoded_feed" : @"audio_raw_feed";
        config.sourceMediaProfile.containerName =
            config.inputKind == HBRStreamCorePublisherInputKindAppEncodedFeed ?
                @"annexb" :
                @"raw";
        config.sourceMediaProfile.audioCodecName = enableAudio ?
            (config.inputKind == HBRStreamCorePublisherInputKindAppRawFeed ? @"pcm" : targetAudioCodecName) :
            @"";
        config.sourceMediaProfile.videoCodecName = enableVideo ? targetVideoCodecName : @"";
        config.sourceMediaProfile.hasAudio = enableAudio;
        config.sourceMediaProfile.hasVideo = enableVideo;
        config.sourceMediaProfile.width = enableVideo ? (NSInteger)targetSize.width : 0;
        config.sourceMediaProfile.height = enableVideo ? (NSInteger)targetSize.height : 0;
        config.sourceMediaProfile.fps = 25;
        config.sourceMediaProfile.sampleRate = 48000;
        config.sourceMediaProfile.channelCount = 2;
        config.transcodeOptions.audioMode = HBRStreamCorePublisherTranscodeModeAuto;
        config.transcodeOptions.videoMode = HBRStreamCorePublisherTranscodeModeAuto;
    }
    return [self.publisherSession configureWithConfig:config];
}

- (HBRStreamCoreOperationStatus*)configurePublisherCaptureSession
{
    if (self.publisherCaptureSession == nil)
    {
        self.publisherCaptureSession = [[HBRStreamCoreCaptureSession alloc] init];
    }

    HBRStreamCoreCaptureConfig* config = [self.publisherCaptureSession defaultConfig];
    config.sessionName = @"ios_demo_publisher_capture";
    config.sourceKind =
        self.publisherInputControl.selectedSegmentIndex == 1 ?
            HBRStreamCoreCaptureSourceKindDesktop :
            HBRStreamCoreCaptureSourceKindCamera;
    config.sourceIdentifier =
        config.sourceKind == HBRStreamCoreCaptureSourceKindCamera ?
            @"front_camera" :
            @"";
    config.audioSourceKind = HBRStreamCoreCaptureSourceKindMicrophone;
    config.enableAudio = [self selectedPublisherAudioEnabled];
    config.audioVolumePercent =
        (NSInteger)[self selectedPublisherAudioVolumePercent];
    config.enableVideo = [self selectedPublisherVideoEnabled];
    config.targetFrameRate = 25;
    return [self.publisherCaptureSession configureWithConfig:config];
}

- (void)preflightPlayer
{
    HBRStreamCoreOperationStatus* configureStatus = [self configurePlayerSession];
    if (configureStatus.resultCode != 0)
    {
        self.playerLabel.text = [NSString stringWithFormat:
            @"%@: %@\nconfigure: %@",
            [self uiTextEnglish:@"Source" chinese:@"来源"],
            [self playerDisplaySource],
            [self statusText:configureStatus]];
        [self setOperationAction:@"player.preflight"
                            code:[NSString stringWithFormat:@"%ld", (long)configureStatus.resultCode]
                          status:configureStatus.statusName
                         summary:configureStatus.summary];
        return;
    }
    HBRStreamCorePlayerPreflight* preflight = [self.playerSession preflight];
    HBRStreamCorePlayerRuntimeInfo* runtimeInfo = [self.playerSession runtimeInfo];
    self.playerLabel.text = [NSString stringWithFormat:
        @"Source: %@ · %@\nconfigure: %@\npreflight: ready=%d state=%ld\n%@\nruntime: %@",
        [self isPlayerWhepSelected] ? @"WHEP" : @"URL",
        [self playerDisplaySource],
        [self statusText:configureStatus],
        preflight.readyToStart,
        (long)preflight.sessionState,
        preflight.summary,
        runtimeInfo.stateSummary];
    [self setOperationAction:@"player.preflight"
                        code:[NSString stringWithFormat:@"%ld", (long)preflight.status.resultCode]
                      status:preflight.status.statusName
                     summary:preflight.summary];
}

- (void)startPlayer
{
    HBRStreamCoreOperationStatus* configureStatus = [self configurePlayerSession];
    if (configureStatus.resultCode != 0)
    {
        self.playerLabel.text = [NSString stringWithFormat:
            @"%@: %@\nconfigure: %@",
            [self uiTextEnglish:@"Source" chinese:@"来源"],
            [self playerDisplaySource],
            [self statusText:configureStatus]];
        [self setOperationAction:@"player.start"
                            code:[NSString stringWithFormat:@"%ld", (long)configureStatus.resultCode]
                          status:configureStatus.statusName
                         summary:configureStatus.summary];
        return;
    }
    HBRStreamCorePlayerPreflight* preflight = [self.playerSession preflight];
    HBRStreamCoreOperationStatus* startStatus =
        preflight.readyToStart ? [self.playerSession start] : preflight.status;
    HBRStreamCorePlayerRuntimeInfo* runtimeInfo = [self.playerSession runtimeInfo];
    NSString* playerOutcome = [self uiTextEnglish:@"Playback started." chinese:@"播放已启动。"];
    if (startStatus.resultCode != 0)
    {
        playerOutcome = [self uiTextEnglish:@"Playback failed to start." chinese:@"播放启动失败。"];
    }
    self.playerLabel.text = [NSString stringWithFormat:
        @"%@\nSource: %@ · %@\nconfigure: %@\nstart: %@\nruntime: %@",
        playerOutcome,
        [self isPlayerWhepSelected] ? @"WHEP" : @"URL",
        [self playerDisplaySource],
        [self statusText:configureStatus],
        [self statusText:startStatus],
        runtimeInfo.stateSummary];
    [self setOperationAction:@"player.start"
                        code:[NSString stringWithFormat:@"%ld", (long)startStatus.resultCode]
                      status:startStatus.statusName
                     summary:startStatus.summary.length > 0 ? startStatus.summary : runtimeInfo.stateSummary];
}

- (void)stopPlayer
{
    [self.playerSession stop];
    HBRStreamCorePlayerRuntimeInfo* runtimeInfo = [self.playerSession runtimeInfo];
    // 释放 session，确保 WHEP Bearer 等创建期参数不跨下一次播放继续保留。
    self.playerSession = nil;
    self.playerLabel.text = [NSString stringWithFormat:@"%@\nruntime: %@",
        [self uiTextEnglish:@"Playback stopped." chinese:@"播放已停止。"],
        runtimeInfo != nil ? runtimeInfo.stateSummary : [self uiTextEnglish:@"No active session." chinese:@"没有活动会话。"]];
    [self setOperationAction:@"player.stop" code:@"0" status:@"ok" summary:[self uiTextEnglish:@"Playback session stopped." chinese:@"播放会话已停止。"]];
}

- (void)preflightCapture
{
    HBRStreamCoreOperationStatus* configureStatus = [self configureCaptureSession];
    HBRStreamCoreCapturePreflight* preflight = [self.captureSession preflight];
    HBRStreamCoreCaptureRuntimeInfo* runtimeInfo = [self.captureSession runtimeInfo];
    self.captureLabel.text = [NSString stringWithFormat:
        @"source: %@\nconfigure: %@\npreflight: ready=%d permission=%d state=%ld\n%@\nruntime: %@",
        [self selectedCaptureSourceText],
        [self statusText:configureStatus],
        preflight.readyToStart,
        preflight.requiresPermission,
        (long)preflight.sessionState,
        preflight.summary,
        runtimeInfo.stateSummary];
    [self setOperationAction:@"capture.preflight"
                        code:[NSString stringWithFormat:@"%ld", (long)preflight.status.resultCode]
                      status:preflight.status.statusName
                     summary:preflight.summary];
}

- (void)startCapture
{
    HBRStreamCoreOperationStatus* configureStatus = [self configureCaptureSession];
    HBRStreamCoreCapturePreflight* preflight = [self.captureSession preflight];
    HBRStreamCoreOperationStatus* startStatus =
        preflight.readyToStart ? [self.captureSession start] : preflight.status;
    HBRStreamCoreCaptureRuntimeInfo* runtimeInfo = [self.captureSession runtimeInfo];
    self.captureLabel.text = [NSString stringWithFormat:
        @"source: %@\nconfigure: %@\nstart: %@\nruntime: %@",
        [self selectedCaptureSourceText],
        [self statusText:configureStatus],
        [self statusText:startStatus],
        runtimeInfo.stateSummary];
    [self setOperationAction:@"capture.start"
                        code:[NSString stringWithFormat:@"%ld", (long)startStatus.resultCode]
                      status:startStatus.statusName
                     summary:startStatus.summary.length > 0 ? startStatus.summary : runtimeInfo.stateSummary];
}

- (void)stopCapture
{
    [self.captureSession stop];
    HBRStreamCoreCaptureRuntimeInfo* runtimeInfo = [self.captureSession runtimeInfo];
    self.captureLabel.text = [NSString stringWithFormat:@"%@\nruntime: %@",
        [self uiTextEnglish:@"Capture stopped." chinese:@"采集已停止。"],
        runtimeInfo != nil ? runtimeInfo.stateSummary : [self uiTextEnglish:@"No active session." chinese:@"没有活动会话。"]];
    [self setOperationAction:@"capture.stop" code:@"0" status:@"ok" summary:[self uiTextEnglish:@"Capture session stopped." chinese:@"采集会话已停止。"]];
}

- (void)preflightPublisher
{
    HBRStreamCoreOperationStatus* configureStatus = [self configurePublisherSession];
    HBRStreamCorePublisherPreflight* preflight = [self.publisherSession preflight];
    HBRStreamCorePublisherRuntimeInfo* runtimeInfo = [self.publisherSession runtimeInfo];
    self.publisherLabel.text = [NSString stringWithFormat:
        @"URL: %@\ninput: %@\nconfigure: %@\npreflight: ready=%d policy=%d state=%ld\n%@\nruntime: %@",
        [self publisherURLText],
        [NSString stringWithFormat:@"%@ %@/%@ %@",
            [self selectedPublisherInputText],
            [self selectedPublisherVideoCodecName],
            [self selectedPublisherAudioCodecName],
            [self selectedPublisherResolutionText]],
        [self statusText:configureStatus],
        preflight.readyToStart,
        preflight.transcodePolicySatisfied,
        (long)preflight.sessionState,
        preflight.summary,
        runtimeInfo.stateSummary];
    [self setOperationAction:@"publisher.preflight"
                        code:[NSString stringWithFormat:@"%ld", (long)preflight.status.resultCode]
                      status:preflight.status.statusName
                     summary:preflight.summary];
}

- (void)startPublisher
{
    HBRStreamCoreOperationStatus* configureStatus = [self configurePublisherSession];
    const BOOL usesCapture =
        [self selectedPublisherInputKind] ==
        HBRStreamCorePublisherInputKindAppRawFeed;
    HBRStreamCoreOperationStatus* captureConfigureStatus = nil;
    HBRStreamCoreOperationStatus* connectStatus = nil;
    HBRStreamCoreCapturePreflight* capturePreflight = nil;
    if (configureStatus.resultCode == 0 && usesCapture)
    {
        captureConfigureStatus = [self configurePublisherCaptureSession];
        if (captureConfigureStatus.resultCode == 0)
        {
            connectStatus =
                [self.publisherCaptureSession connectPublisher:self.publisherSession];
            if (connectStatus.resultCode == 0)
            {
                capturePreflight = [self.publisherCaptureSession preflight];
            }
        }
    }
    HBRStreamCorePublisherPreflight* preflight = [self.publisherSession preflight];
    HBRStreamCoreOperationStatus* startStatus =
        preflight.readyToStart &&
                (!usesCapture ||
                 (capturePreflight != nil && capturePreflight.readyToStart)) ?
            [self.publisherSession start] :
            (usesCapture && capturePreflight != nil &&
                    !capturePreflight.readyToStart ?
                capturePreflight.status :
                (usesCapture && connectStatus != nil &&
                        connectStatus.resultCode != 0 ?
                    connectStatus :
                    (usesCapture && captureConfigureStatus != nil &&
                            captureConfigureStatus.resultCode != 0 ?
                        captureConfigureStatus :
                        preflight.status)));
    HBRStreamCoreOperationStatus* captureStartStatus = nil;
    if (startStatus.resultCode == 0 && usesCapture)
    {
        captureStartStatus = [self.publisherCaptureSession start];
        if (captureStartStatus.resultCode != 0)
        {
            [self.publisherCaptureSession stop];
            [self.publisherCaptureSession connectPublisher:nil];
            [self.publisherSession stop];
            startStatus = captureStartStatus;
        }
    }
    HBRStreamCorePublisherRuntimeInfo* runtimeInfo = [self.publisherSession runtimeInfo];
    NSString* publisherOutcome = [self uiTextEnglish:@"Publish started." chinese:@"推流已启动。"];
    if (startStatus.resultCode != 0)
    {
        publisherOutcome = [self uiTextEnglish:@"Publish failed to start." chinese:@"推流启动失败。"];
    }
    self.publisherLabel.text = [NSString stringWithFormat:
        @"%@\nURL: %@\ninput: %@\nconfigure: %@\nstart: %@%@\nruntime: %@",
        publisherOutcome,
        [self publisherURLText],
        [NSString stringWithFormat:@"%@ %@/%@ %@",
            [self selectedPublisherInputText],
            [self selectedPublisherVideoCodecName],
            [self selectedPublisherAudioCodecName],
            [self selectedPublisherResolutionText]],
        [self statusText:configureStatus],
        [self statusText:startStatus],
        usesCapture ?
            [NSString stringWithFormat:@"\ncapture: %@",
                [self statusText:captureStartStatus != nil ?
                    captureStartStatus :
                    (connectStatus != nil ? connectStatus : captureConfigureStatus)]] :
            @"",
        runtimeInfo.stateSummary];
    [self setOperationAction:@"publisher.start"
                        code:[NSString stringWithFormat:@"%ld", (long)startStatus.resultCode]
                      status:startStatus.statusName
                     summary:startStatus.summary.length > 0 ? startStatus.summary : runtimeInfo.stateSummary];
}

- (void)pushPublisherSample
{
    if (self.publisherSession == nil ||
        [self selectedPublisherInputKind] != HBRStreamCorePublisherInputKindAppEncodedFeed)
    {
        [self setOperationAction:@"publisher.push"
                            code:@"-1"
                          status:@"invalid_argument"
                         summary:[self uiTextEnglish:@"Encoded sample push works only after the encoded-feed publisher path starts." chinese:@"只有编码喂流路径启动后，才能推送样包。"]];
        return;
    }

    HBRStreamCorePublisherEncodedPacket* audioPacket =
        [HBRStreamCorePublisherEncodedPacket defaultPacket];
    static const uint8_t kAudioSampleBytes[] = { 0x11, 0x12, 0x13 };
    audioPacket.data = [NSData dataWithBytes:kAudioSampleBytes length:sizeof(kAudioSampleBytes)];
    audioPacket.codecName = [self selectedPublisherAudioCodecName];
    audioPacket.timestampMs = 1000;
    HBRStreamCoreOperationStatus* audioStatus =
        [self.publisherSession pushEncodedAudioPacket:audioPacket];

    HBRStreamCorePublisherEncodedPacket* videoPacket =
        [HBRStreamCorePublisherEncodedPacket defaultPacket];
    static const uint8_t kVideoSampleBytes[] = { 0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84 };
    videoPacket.data = [NSData dataWithBytes:kVideoSampleBytes length:sizeof(kVideoSampleBytes)];
    videoPacket.codecName = [self selectedPublisherVideoCodecName];
    videoPacket.keyFrame = YES;
    videoPacket.timestampMs = 1033;
    HBRStreamCoreOperationStatus* videoStatus =
        [self.publisherSession pushEncodedVideoPacket:videoPacket];
    HBRStreamCorePublisherRuntimeInfo* runtimeInfo = [self.publisherSession runtimeInfo];
    self.publisherLabel.text = [NSString stringWithFormat:
        @"sample packets: audio %@ / video %@\nruntime: %@",
        [self statusText:audioStatus],
        [self statusText:videoStatus],
        runtimeInfo.stateSummary];
    [self setOperationAction:@"publisher.push"
                        code:[NSString stringWithFormat:@"%ld", (long)videoStatus.resultCode]
                      status:videoStatus.statusName
                     summary:videoStatus.summary.length > 0 ? videoStatus.summary : runtimeInfo.stateSummary];
}

- (void)stopPublisher
{
    [self.publisherCaptureSession stop];
    [self.publisherCaptureSession connectPublisher:nil];
    [self.publisherSession stop];
    HBRStreamCorePublisherRuntimeInfo* runtimeInfo = [self.publisherSession runtimeInfo];
    self.publisherLabel.text = [NSString stringWithFormat:@"%@\nruntime: %@",
        [self uiTextEnglish:@"Publish stopped." chinese:@"推流已停止。"],
        runtimeInfo != nil ? runtimeInfo.stateSummary : [self uiTextEnglish:@"No active session." chinese:@"没有活动会话。"]];
    [self setOperationAction:@"publisher.stop" code:@"0" status:@"ok" summary:[self uiTextEnglish:@"Publisher session stopped." chinese:@"推流会话已停止。"]];
}

- (NSString*)gb28181Text
{
    HBRStreamCoreGB28181RuntimeInfo* runtimeInfo =
        self.gb28181Session != nil ? [self.gb28181Session runtimeInfo] : nil;
    return [NSString stringWithFormat:
        @"%@: %@\n%@: %@",
        [self uiTextEnglish:@"State" chinese:@"状态"],
        runtimeInfo != nil ?
            [self compactText:runtimeInfo.stateSummary maxLength:60] :
            [self uiTextEnglish:@"Waiting for preflight or register." chinese:@"等待预检或注册。"],
        [self uiTextEnglish:@"Packaging" chinese:@"打包状态"],
        [HBRStreamCoreSDK applePackagingStatus]];
}

- (NSString*)gb28181ConfigText
{
    return [NSString stringWithFormat:
        @"%@: %@ %@ %@:%@ | %@ %@:%@\n%@: %@ %@ %@:%@ %@ | %@ %@/%@s\n%@: %@ %@ | %@: %@",
        [self uiTextEnglish:@"Local" chinese:@"本地"],
        self.gbLocalIdEdit.text.length > 0 ? self.gbLocalIdEdit.text : @"-",
        self.gbLocalDomainEdit.text.length > 0 ? self.gbLocalDomainEdit.text : @"-",
        self.gbLocalIpEdit.text.length > 0 ? self.gbLocalIpEdit.text : @"-",
        self.gbLocalPortEdit.text.length > 0 ? self.gbLocalPortEdit.text : @"-",
        [self uiTextEnglish:@"Media" chinese:@"媒体"],
        self.gbMediaIpEdit.text.length > 0 ? self.gbMediaIpEdit.text : @"-",
        self.gbMediaPortEdit.text.length > 0 ? self.gbMediaPortEdit.text : @"-",
        [self uiTextEnglish:@"Upper" chinese:@"上级"],
        self.gbUpperIdEdit.text.length > 0 ? self.gbUpperIdEdit.text : @"-",
        self.gbUpperDomainEdit.text.length > 0 ? self.gbUpperDomainEdit.text : @"-",
        self.gbUpperIpEdit.text.length > 0 ? self.gbUpperIpEdit.text : @"-",
        self.gbUpperPortEdit.text.length > 0 ? self.gbUpperPortEdit.text : @"-",
        [self gb28181TransportText],
        [self uiTextEnglish:@"register/keepalive" chinese:@"注册/心跳"],
        self.gbRegisterExpiresEdit.text.length > 0 ? self.gbRegisterExpiresEdit.text : @"-",
        self.gbKeepaliveEdit.text.length > 0 ? self.gbKeepaliveEdit.text : @"-",
        [self uiTextEnglish:@"Source" chinese:@"来源"],
        [self selectedGB28181SourceText],
        [self selectedGB28181ResolutionText],
        [self uiTextEnglish:@"Display" chinese:@"显示"],
        [self selectedGB28181DisplayModeText]];
}

- (NSString*)trimmedGB28181Text:(UITextField*)field defaultText:(NSString*)defaultText
{
    NSString* text = [field.text stringByTrimmingCharactersInSet:
        NSCharacterSet.whitespaceAndNewlineCharacterSet];
    return text.length > 0 ? text : defaultText;
}

- (NSInteger)gb28181PortText:(UITextField*)field defaultPort:(NSInteger)defaultPort
{
    NSString* text = [self trimmedGB28181Text:field defaultText:@""];
    NSInteger port = text.integerValue;
    return port > 0 && port <= 65535 ? port : defaultPort;
}

- (NSInteger)gb28181IntegerText:(UITextField*)field
                   defaultValue:(NSInteger)defaultValue
                       minValue:(NSInteger)minValue
                       maxValue:(NSInteger)maxValue
{
    NSString* text = [self trimmedGB28181Text:field defaultText:@""];
    NSInteger value = text.integerValue;
    if (value < minValue || value > maxValue)
    {
        return defaultValue;
    }
    return value;
}

- (HBRStreamCoreGB28181TransportMode)selectedGB28181TransportMode
{
    return self.gbTransportControl.selectedSegmentIndex == 1 ?
        HBRStreamCoreGB28181TransportModeUDP :
        HBRStreamCoreGB28181TransportModeTCP;
}

- (NSString*)gb28181TransportText
{
    return [self selectedGB28181TransportMode] == HBRStreamCoreGB28181TransportModeUDP ?
        @"UDP" :
        @"TCP";
}

- (BOOL)selectedGB28181UsesMediaFile
{
    return self.gbSourceControl != nil && self.gbSourceControl.selectedSegmentIndex == 1;
}

- (HBRStreamCoreGB28181MediaSourceKind)selectedGB28181SourceKind
{
    return [self selectedGB28181UsesMediaFile] ?
        HBRStreamCoreGB28181MediaSourceKindMediaFile :
        HBRStreamCoreGB28181MediaSourceKindLocalDevice;
}

- (NSString*)selectedGB28181SourceText
{
    return [self selectedGB28181UsesMediaFile] ?
        [self uiTextEnglish:@"Video file" chinese:@"视频文件"] :
        [self uiTextEnglish:@"Camera + microphone" chinese:@"相机 + 麦克风"];
}

- (NSString*)selectedGB28181MediaSourceText
{
    return [self trimmedGB28181Text:self.gbMediaSourceEdit defaultText:@""];
}

- (CGSize)selectedGB28181TargetSize
{
    if (self.gbResolutionControl != nil && self.gbResolutionControl.selectedSegmentIndex == 1)
    {
        return CGSizeMake(1920.0, 1080.0);
    }
    if (self.gbResolutionControl != nil && self.gbResolutionControl.selectedSegmentIndex == 2)
    {
        return CGSizeMake(640.0, 480.0);
    }
    return CGSizeMake(1280.0, 720.0);
}

- (NSString*)selectedGB28181ResolutionText
{
    CGSize size = [self selectedGB28181TargetSize];
    return [NSString stringWithFormat:@"%ldx%ld",
        (long)size.width,
        (long)size.height];
}

- (NSString*)selectedGB28181DisplayModeText
{
    if (self.gbPreviewDisplayModeControl != nil &&
        self.gbPreviewDisplayModeControl.selectedSegmentIndex == 0)
    {
        return [self uiTextEnglish:@"Stretch fill" chinese:@"拉伸填满"];
    }
    if (self.gbPreviewDisplayModeControl != nil &&
        self.gbPreviewDisplayModeControl.selectedSegmentIndex == 2)
    {
        return [self uiTextEnglish:@"Crop fill" chinese:@"裁剪填满"];
    }
    return [self uiTextEnglish:@"Keep aspect" chinese:@"保持比例"];
}

- (BOOL)gb28181Request:(HBRStreamCoreGB28181MediaRequest*)request
    supportsSourceKind:(HBRStreamCoreGB28181MediaSourceKind)sourceKind
{
    if (request == nil || !request.requiresLocalSource)
    {
        return YES;
    }
    HBRStreamCoreGB28181SupportedSourceMask mask = request.supportedSourceMask;
    if (sourceKind == HBRStreamCoreGB28181MediaSourceKindMediaFile)
    {
        return (mask & HBRStreamCoreGB28181SupportedSourceMaskMediaFile) != 0;
    }
    if (sourceKind == HBRStreamCoreGB28181MediaSourceKindNetworkURL)
    {
        return (mask & HBRStreamCoreGB28181SupportedSourceMaskNetworkURL) != 0;
    }
    return (mask & HBRStreamCoreGB28181SupportedSourceMaskLocalDevice) != 0;
}

- (HBRStreamCoreOperationStatus*)gb28181InvalidStatusWithName:(NSString*)statusName
                                                       summary:(NSString*)summary
{
    return [HBRStreamCoreOperationStatus
        statusWithResultCode:HBRStreamCoreResultCodeInvalidArgument
                  statusName:statusName
                     summary:summary
                      detail:@"GB28181 requires a reachable upper-platform SIP endpoint before start/register can be tested."];
}

- (HBRStreamCoreGB28181StreamTarget*)gb28181StreamTarget
{
    HBRStreamCoreGB28181StreamTarget* target =
        [[HBRStreamCoreGB28181StreamTarget alloc] init];
    NSString* localId =
        [self trimmedGB28181Text:self.gbLocalIdEdit defaultText:@"34020000001320000001"];
    target.deviceId = localId;
    target.channelId = localId;
    target.streamKind = HBRStreamCoreGB28181StreamKindLive;
    return target;
}

- (HBRStreamCoreGB28181StreamTarget*)gb28181StreamTargetForRequest:
    (HBRStreamCoreGB28181MediaRequest*)request
{
    if (request == nil)
    {
        return [self gb28181StreamTarget];
    }
    HBRStreamCoreGB28181StreamTarget* target =
        [[HBRStreamCoreGB28181StreamTarget alloc] init];
    target.deviceId = request.deviceId.length > 0 ?
        request.deviceId :
        [self trimmedGB28181Text:self.gbLocalIdEdit defaultText:@"34020000001320000001"];
    target.channelId = request.channelId.length > 0 ?
        request.channelId :
        target.deviceId;
    target.streamKind = request.streamKind;
    return target;
}

- (HBRStreamCoreGB28181Config*)gb28181ConfigForCurrentFields
{
    HBRStreamCoreGB28181Config* config = [self.gb28181Session defaultConfig];
    NSString* localId =
        [self trimmedGB28181Text:self.gbLocalIdEdit defaultText:@"34020000001320000001"];
    NSString* localDomain =
        [self trimmedGB28181Text:self.gbLocalDomainEdit defaultText:@"3402000000"];
    NSString* localIp =
        [self trimmedGB28181Text:self.gbLocalIpEdit defaultText:@"0.0.0.0"];
    NSInteger localPort = [self gb28181PortText:self.gbLocalPortEdit defaultPort:5060];
    NSString* upperId =
        [self trimmedGB28181Text:self.gbUpperIdEdit defaultText:@"34020000002000000001"];
    NSString* upperDomain =
        [self trimmedGB28181Text:self.gbUpperDomainEdit defaultText:localDomain];
    NSString* sipPassword =
        [self trimmedGB28181Text:self.gbUpperPasswordEdit defaultText:@"123456"];
    NSString* upperIp =
        [self trimmedGB28181Text:self.gbUpperIpEdit defaultText:@""];
    NSInteger upperPort = [self gb28181PortText:self.gbUpperPortEdit defaultPort:5060];
    HBRStreamCoreGB28181TransportMode transportMode = [self selectedGB28181TransportMode];
    NSString* mediaIp =
        [self trimmedGB28181Text:self.gbMediaIpEdit defaultText:@"0.0.0.0"];
    NSInteger mediaPort = [self gb28181PortText:self.gbMediaPortEdit defaultPort:15060];
    NSInteger registerExpires = [self gb28181IntegerText:self.gbRegisterExpiresEdit
                                            defaultValue:3600
                                                minValue:60
                                                maxValue:86400];
    NSInteger keepalive = [self gb28181IntegerText:self.gbKeepaliveEdit
                                      defaultValue:60
                                          minValue:5
                                          maxValue:3600];

    config.sessionName = @"ios_demo_gb28181_device";
    config.localIdentity.identifier = localId;
    config.localIdentity.domain = localDomain;
    config.localIdentity.password = sipPassword;
    config.localIdentity.displayName = @"StreamCore SDK Demo";
    config.upperPlatformIdentity.identifier = upperId;
    config.upperPlatformIdentity.domain = upperDomain;
    config.upperPlatformIdentity.password = sipPassword;
    config.upperPlatformIdentity.displayName = @"Demo Upper Platform";
    config.localEndpoint.ip = localIp;
    config.localEndpoint.port = localPort;
    config.localEndpoint.transportMode = transportMode;
    config.upperPlatformEndpoint.ip = upperIp;
    config.upperPlatformEndpoint.port = upperPort;
    config.upperPlatformEndpoint.transportMode = transportMode;
    config.registerExpiresSeconds = registerExpires;
    config.keepaliveIntervalSeconds = keepalive;
    config.enableDigestAuth = YES;
    config.autoReplyCatalog = YES;
    config.autoReplyDeviceInfo = YES;
    config.autoReplyDeviceStatus = YES;
    config.defaultAnswer.sessionName = @"StreamCore iOS camera";
    config.defaultAnswer.mediaEndpoint.ip = mediaIp;
    config.defaultAnswer.mediaEndpoint.port = mediaPort;
    config.defaultAnswer.mediaEndpoint.transportMode = transportMode;
    config.defaultAnswer.protocolVersion = HBRStreamCoreGB28181ProtocolVersion2016;
    config.defaultAnswer.mediaDirection = @"sendonly";
    config.defaultAnswer.videoCodecName = @"H264";
    config.defaultAnswer.videoPayloadType = 96;
    config.defaultAnswer.audioCodecName = @"G711A";
    config.defaultAnswer.audioPayloadType = 8;
    config.defaultAnswer.audioClockRate = 8000;
    config.defaultAnswer.audioChannelCount = 1;
    return config;
}

- (HBRStreamCoreGB28181SourceBinding*)gb28181SourceBindingForRequest:
    (HBRStreamCoreGB28181MediaRequest*)request
{
    HBRStreamCoreGB28181SourceBinding* binding =
        [[HBRStreamCoreGB28181SourceBinding alloc] init];
    BOOL requestHasAudio = request == nil || request.audioCodecName.length > 0;
    BOOL requestHasVideo = request == nil || request.videoCodecName.length > 0 || !requestHasAudio;
    CGSize targetSize = [self selectedGB28181TargetSize];
    binding.target = [self gb28181StreamTargetForRequest:request];
    binding.sourceKind = [self selectedGB28181SourceKind];
    if (binding.sourceKind == HBRStreamCoreGB28181MediaSourceKindMediaFile)
    {
        NSString* mediaSource = [self selectedGB28181MediaSourceText];
        binding.filePath = mediaSource;
        binding.sourceIdentifier = mediaSource;
        binding.audioSourceIdentifier = @"file_audio";
        binding.repeatFile = YES;
        binding.readAsFastAsPossible = NO;
    }
    else
    {
        binding.captureSourceKind = HBRStreamCoreCaptureSourceKindCamera;
        binding.audioCaptureSourceKind = HBRStreamCoreCaptureSourceKindMicrophone;
        binding.sourceIdentifier = @"front_camera";
        binding.audioSourceIdentifier = @"microphone";
    }
    binding.enableAudio = requestHasAudio;
    binding.enableVideo = requestHasVideo;
    binding.allowRawFramePacket = YES;
    binding.width = (NSInteger)targetSize.width;
    binding.height = (NSInteger)targetSize.height;
    binding.fps = 25;
    binding.audioSampleRate = 48000;
    binding.audioChannelCount = 2;
    binding.firstUseTcp = YES;
    binding.autoReconnect = YES;
    return binding;
}

- (NSArray<HBRStreamCoreGB28181CatalogItem*>*)gb28181CatalogItems
{
    HBRStreamCoreGB28181CatalogItem* item =
        [[HBRStreamCoreGB28181CatalogItem alloc] init];
    NSString* localId =
        [self trimmedGB28181Text:self.gbLocalIdEdit defaultText:@"34020000001320000001"];
    item.channelId = localId;
    item.name = @"iOS Demo Camera";
    item.parentId = @"";
    item.manufacturer = @"HBR";
    item.model = @"iOSDemo";
    item.owner = @"StreamCore SDK Demo";
    item.civilCode = @"340200";
    item.address = @"iOS demo";
    item.parental = NO;
    item.online = YES;
    item.statusText = @"ON";
    return @[ item ];
}

- (void)postGB28181CallbackAction:(NSString*)action
                            status:(HBRStreamCoreOperationStatus*)status
                            detail:(NSString*)detail
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HBRStreamCoreGB28181RuntimeInfo* runtimeInfo = [self.gb28181Session runtimeInfo];
        NSString* runtimeText = runtimeInfo != nil ? runtimeInfo.stateSummary : @"无活动 GB28181 会话";
        self.gb28181Label.text = [NSString stringWithFormat:
            @"%@\n%@\n%@\nruntime：%@\nmedia：audio=%llu video=%llu sessions=%ld bindings=%ld",
            detail.length > 0 ? detail : @"GB28181 callback",
            [self gb28181ConfigText],
            [self statusText:status],
            runtimeText,
            runtimeInfo != nil ? runtimeInfo.sentAudioPacketCount : 0,
            runtimeInfo != nil ? runtimeInfo.sentVideoPacketCount : 0,
            runtimeInfo != nil ? (long)runtimeInfo.activeSessionCount : 0,
            runtimeInfo != nil ? (long)runtimeInfo.activeSourceBindingCount : 0];
        [self setOperationAction:action
                            code:[NSString stringWithFormat:@"%ld", (long)status.resultCode]
                          status:status.statusName
                         summary:status.summary.length > 0 ? status.summary : runtimeText];
    });
}

- (void)installGB28181Callbacks
{
    __weak StreamCoreDemoViewController* weakSelf = self;
    self.gb28181Session.registerResultHandler = ^(BOOL success, NSString* reason) {
        StreamCoreDemoViewController* strongSelf = weakSelf;
        if (strongSelf == nil)
        {
            return;
        }
        HBRStreamCoreOperationStatus* status =
            [HBRStreamCoreOperationStatus
                statusWithResultCode:(success ?
                    HBRStreamCoreResultCodeOk :
                    HBRStreamCoreResultCodeOperationFailed)
                          statusName:(success ?
                              @"gb28181_register_result_ok" :
                              @"gb28181_register_result_failed")
                             summary:(success ?
                                 @"GB28181 register result succeeded." :
                                 @"GB28181 register result failed.")
                               detail:reason.length > 0 ? reason : @""];
        [strongSelf postGB28181CallbackAction:@"gb28181.register_result"
                                       status:status
                                       detail:[NSString stringWithFormat:@"REGISTER result=%d", success]];
    };
    self.gb28181Session.inviteReceivedHandler = ^(HBRStreamCoreGB28181Invite* invite) {
        StreamCoreDemoViewController* strongSelf = weakSelf;
        if (strongSelf == nil)
        {
            return;
        }
        HBRStreamCoreOperationStatus* status =
            [strongSelf.gb28181Session replyInviteAccepted:invite];
        [strongSelf postGB28181CallbackAction:@"gb28181.invite"
                                       status:status
                                       detail:[NSString stringWithFormat:
                                           @"INVITE accepted: %@/%@ audio=%@ video=%@",
                                           invite.target.deviceId,
                                           invite.target.channelId,
                                           invite.audioCodecName,
                                           invite.videoCodecName]];
    };
    self.gb28181Session.mediaRequestHandler =
        ^(HBRStreamCoreGB28181MediaRequest* request) {
            StreamCoreDemoViewController* strongSelf = weakSelf;
            if (strongSelf == nil)
            {
                return;
            }
            HBRStreamCoreGB28181SourceBinding* binding =
                [strongSelf gb28181SourceBindingForRequest:request];
            if (![strongSelf gb28181Request:request supportsSourceKind:binding.sourceKind])
            {
                HBRStreamCoreOperationStatus* unsupported =
                    [HBRStreamCoreOperationStatus
                        statusWithResultCode:HBRStreamCoreResultCodeOperationFailed
                                  statusName:@"gb28181_source_unsupported"
                                     summary:@"Current GB28181 request does not accept the selected source binding."
                                      detail:@"Choose an upper profile that accepts the selected iOS GB28181 source, or switch the demo source control."];
                [strongSelf postGB28181CallbackAction:@"gb28181.media_request"
                                               status:unsupported
                                               detail:[NSString stringWithFormat:
                                                   @"media request cannot use %@ binding",
                                                   [strongSelf selectedGB28181SourceText]]];
                return;
            }
            HBRStreamCoreOperationStatus* status =
                request.requiresLocalSource ?
                    [strongSelf.gb28181Session setSourceBindings:@[ binding ]] :
                    [strongSelf.gb28181Session clearSourceBindings];
            [strongSelf postGB28181CallbackAction:@"gb28181.media_request"
                                           status:status
                                           detail:[NSString stringWithFormat:
                                               @"mediaRequest %@/%@ direction=%@ audio=%@ video=%@",
                                               request.deviceId,
                                               request.channelId,
                                               request.mediaDirectionText,
                                               request.audioCodecName,
                                               request.videoCodecName]];
        };
    self.gb28181Session.sessionUpdatedHandler =
        ^(HBRStreamCoreGB28181SessionInfo* sessionInfo) {
            StreamCoreDemoViewController* strongSelf = weakSelf;
            if (strongSelf == nil)
            {
                return;
            }
            HBRStreamCoreOperationStatus* status =
                [HBRStreamCoreOperationStatus
                    statusWithResultCode:HBRStreamCoreResultCodeOk
                              statusName:@"gb28181_session_updated"
                                 summary:@"GB28181 session updated."
                                  detail:sessionInfo.sessionKey];
            [strongSelf postGB28181CallbackAction:@"gb28181.session_updated"
                                           status:status
                                           detail:[NSString stringWithFormat:
                                               @"session %@ state=%ld",
                                               sessionInfo.target.channelId,
                                               (long)sessionInfo.sessionState]];
        };
    self.gb28181Session.sessionClosedHandler =
        ^(HBRStreamCoreGB28181SessionInfo* sessionInfo) {
            StreamCoreDemoViewController* strongSelf = weakSelf;
            if (strongSelf == nil)
            {
                return;
            }
            HBRStreamCoreOperationStatus* status =
                [HBRStreamCoreOperationStatus
                    statusWithResultCode:HBRStreamCoreResultCodeOk
                              statusName:@"gb28181_session_closed"
                                 summary:@"GB28181 session closed."
                                  detail:sessionInfo.sessionKey];
            [strongSelf postGB28181CallbackAction:@"gb28181.session_closed"
                                           status:status
                                           detail:[NSString stringWithFormat:
                                               @"session %@ closed",
                                               sessionInfo.target.channelId]];
        };
}

- (HBRStreamCoreOperationStatus*)configureGB28181Session
{
    NSString* upperIp =
        [self trimmedGB28181Text:self.gbUpperIpEdit defaultText:@""];
    if (upperIp.length == 0)
    {
        return [self gb28181InvalidStatusWithName:@"gb28181_upper_ip_missing"
                                           summary:@"GB28181 upper-platform SIP IP is missing."];
    }
    if (self.gb28181Session == nil)
    {
        self.gb28181Session = [[HBRStreamCoreGB28181Session alloc] init];
    }
    [self installGB28181Callbacks];
    HBRStreamCoreOperationStatus* configureStatus =
        [self.gb28181Session configureWithConfig:[self gb28181ConfigForCurrentFields]];
    if (![configureStatus isOk])
    {
        return configureStatus;
    }

    HBRStreamCoreGB28181DeviceInfo* deviceInfo =
        [HBRStreamCoreGB28181DeviceInfo defaultInfo];
    deviceInfo.deviceName = @"StreamCore SDK Demo";
    deviceInfo.manufacturer = @"HBR";
    deviceInfo.model = @"iOSDemo";
    deviceInfo.firmware = @"1.0.0";
    HBRStreamCoreOperationStatus* infoStatus =
        [self.gb28181Session setDeviceInfo:deviceInfo];
    if (![infoStatus isOk])
    {
        return infoStatus;
    }

    HBRStreamCoreGB28181DeviceStatus* deviceStatus =
        [HBRStreamCoreGB28181DeviceStatus defaultStatus];
    deviceStatus.statusText = @"OK";
    deviceStatus.online = YES;
    deviceStatus.recording = NO;
    HBRStreamCoreOperationStatus* statusStatus =
        [self.gb28181Session setDeviceStatus:deviceStatus];
    if (![statusStatus isOk])
    {
        return statusStatus;
    }

    HBRStreamCoreOperationStatus* catalogStatus =
        [self.gb28181Session setCatalogItems:[self gb28181CatalogItems]];
    if (![catalogStatus isOk])
    {
        return catalogStatus;
    }

    HBRStreamCoreGB28181SourceBinding* binding =
        [self gb28181SourceBindingForRequest:nil];
    return [self.gb28181Session setSourceBindings:@[ binding ]];
}

- (void)preflightGB28181
{
    HBRStreamCoreOperationStatus* configureStatus = [self configureGB28181Session];
    HBRStreamCoreGB28181RuntimeInfo* runtimeInfo =
        [self.gb28181Session runtimeInfo];
    NSString* summary = [NSString stringWithFormat:
        @"iOS GB28181 SDK preflight.\n%@\nconfigure/source: %@\nruntime: %@\n%@",
        [self gb28181ConfigText],
        [self statusText:configureStatus],
        runtimeInfo != nil ? runtimeInfo.stateSummary : [self uiTextEnglish:@"No active GB28181 session." chinese:@"没有活动的 GB28181 会话。"],
        [self gb28181Text]];
    self.gb28181Label.text = summary;
    [self setOperationAction:@"gb28181.preflight"
                        code:[NSString stringWithFormat:@"%ld", (long)configureStatus.resultCode]
                      status:configureStatus.statusName
                     summary:configureStatus.summary.length > 0 ?
                         configureStatus.summary :
                         [self uiTextEnglish:@"GB28181 iOS SDK preflight finished." chinese:@"GB28181 iOS SDK 预检完成。"]];
}

- (void)startGB28181
{
    HBRStreamCoreOperationStatus* configureStatus = [self configureGB28181Session];
    HBRStreamCoreOperationStatus* startStatus =
        [configureStatus isOk] ? [self.gb28181Session start] : configureStatus;
    HBRStreamCoreOperationStatus* registerStatus =
        [startStatus isOk] ? [self.gb28181Session registerToUpperPlatform] : startStatus;
    HBRStreamCoreOperationStatus* pollStatus =
        [startStatus isOk] ? [self.gb28181Session poll] : startStatus;
    HBRStreamCoreGB28181RuntimeInfo* runtimeInfo =
        [self.gb28181Session runtimeInfo];
    if ([startStatus isOk])
    {
        [self startGB28181PollTimer];
    }
    NSString* summary = [NSString stringWithFormat:
        @"iOS GB28181 SDK start/register.\n%@\nconfigure: %@\nstart: %@\nregister: %@\npoll: %@\nruntime: started=%d registered=%d sessions=%ld bindings=%ld audio=%llu video=%llu\n%@",
        [self gb28181ConfigText],
        [self statusText:configureStatus],
        [self statusText:startStatus],
        [self statusText:registerStatus],
        [self statusText:pollStatus],
        runtimeInfo != nil ? runtimeInfo.started : NO,
        runtimeInfo != nil ? runtimeInfo.registeredToUpperPlatform : NO,
        runtimeInfo != nil ? (long)runtimeInfo.activeSessionCount : 0,
        runtimeInfo != nil ? (long)runtimeInfo.activeSourceBindingCount : 0,
        runtimeInfo != nil ? runtimeInfo.sentAudioPacketCount : 0,
        runtimeInfo != nil ? runtimeInfo.sentVideoPacketCount : 0,
        runtimeInfo != nil ? runtimeInfo.stateSummary : [self uiTextEnglish:@"No active GB28181 session." chinese:@"没有活动的 GB28181 会话。"]];
    self.gb28181Label.text = summary;
    [self setOperationAction:@"gb28181.start"
                        code:[NSString stringWithFormat:@"%ld", (long)registerStatus.resultCode]
                      status:registerStatus.statusName
                     summary:registerStatus.summary.length > 0 ?
                         registerStatus.summary :
                         runtimeInfo.stateSummary];
}

- (void)stopGB28181
{
    [self stopGB28181PollTimer];
    HBRStreamCoreOperationStatus* unregisterStatus =
        self.gb28181Session != nil ?
            [self.gb28181Session unregisterFromUpperPlatform] :
            [HBRStreamCoreOperationStatus
                statusWithResultCode:HBRStreamCoreResultCodeOk
                          statusName:@"gb28181_already_stopped"
                             summary:@"GB28181 session is not running."
                              detail:@""];
    [self.gb28181Session stop];
    self.gb28181Session.registerResultHandler = nil;
    self.gb28181Session.inviteReceivedHandler = nil;
    self.gb28181Session.sessionUpdatedHandler = nil;
    self.gb28181Session.sessionClosedHandler = nil;
    self.gb28181Session.commandHandler = nil;
    self.gb28181Session.mediaRequestHandler = nil;
    self.gb28181Session.mediaPacketHandler = nil;
    HBRStreamCoreGB28181RuntimeInfo* runtimeInfo =
        [self.gb28181Session runtimeInfo];
    self.gb28181Label.text = [NSString stringWithFormat:
        @"%@\n%@\nunregister: %@\nruntime: %@",
        [self uiTextEnglish:@"GB28181 stopped." chinese:@"GB28181 已停止。"],
        [self gb28181ConfigText],
        [self statusText:unregisterStatus],
        runtimeInfo != nil ? runtimeInfo.stateSummary : [self uiTextEnglish:@"No active GB28181 session." chinese:@"没有活动的 GB28181 会话。"]];
    self.gb28181Session = nil;
    [self setOperationAction:@"gb28181.stop"
                        code:[NSString stringWithFormat:@"%ld", (long)unregisterStatus.resultCode]
                      status:unregisterStatus.statusName
                     summary:unregisterStatus.summary.length > 0 ?
                         unregisterStatus.summary :
                         [self uiTextEnglish:@"GB28181 session stopped." chinese:@"GB28181 会话已停止。"]];
}

- (void)startGB28181PollTimer
{
    [self stopGB28181PollTimer];
    self.gb28181PollTimer =
        [NSTimer scheduledTimerWithTimeInterval:1.0
                                         target:self
                                       selector:@selector(pollGB28181Session)
                                       userInfo:nil
                                        repeats:YES];
}

- (void)stopGB28181PollTimer
{
    [self.gb28181PollTimer invalidate];
    self.gb28181PollTimer = nil;
}

- (void)pollGB28181Session
{
    if (self.gb28181Session == nil)
    {
        [self stopGB28181PollTimer];
        return;
    }
    HBRStreamCoreOperationStatus* pollStatus = [self.gb28181Session poll];
    HBRStreamCoreGB28181RuntimeInfo* runtimeInfo =
        [self.gb28181Session runtimeInfo];
    self.gb28181Label.text = [NSString stringWithFormat:
        @"GB28181 running.\n%@\npoll: %@\nruntime: registered=%d sessions=%ld bindings=%ld audio=%llu video=%llu\n%@",
        [self gb28181ConfigText],
        [self statusText:pollStatus],
        runtimeInfo != nil ? runtimeInfo.registeredToUpperPlatform : NO,
        runtimeInfo != nil ? (long)runtimeInfo.activeSessionCount : 0,
        runtimeInfo != nil ? (long)runtimeInfo.activeSourceBindingCount : 0,
        runtimeInfo != nil ? runtimeInfo.sentAudioPacketCount : 0,
        runtimeInfo != nil ? runtimeInfo.sentVideoPacketCount : 0,
        runtimeInfo != nil ? runtimeInfo.stateSummary : [self uiTextEnglish:@"No active GB28181 session." chinese:@"没有活动的 GB28181 会话。"]];
    [self setOperationAction:@"gb28181.poll"
                        code:[NSString stringWithFormat:@"%ld", (long)pollStatus.resultCode]
                      status:pollStatus.statusName
                     summary:pollStatus.summary.length > 0 ?
                         pollStatus.summary :
                         runtimeInfo.stateSummary];
}

- (void)scheduleAutorunIfRequested
{
    NSString* autorun =
        NSProcessInfo.processInfo.environment[@"STREAMCORE_DEMO_IOS_AUTORUN"];
    if (![autorun isEqualToString:@"preflight"])
    {
        return;
    }

    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)),
        dispatch_get_main_queue(),
        ^{
#if TARGET_OS_SIMULATOR
            // Keep simulator autorun away from real camera and screen-capture
            // paths. Screen capture can wait for host/UI state under SSH; the
            // microphone path still exercises capture configuration and
            // preflight without requiring a camera frame.
            self.captureSourceControl.selectedSegmentIndex = 1;
            [self captureSourceChanged:self.captureSourceControl];
#endif
            [self preflightPlayer];
            [self preflightCapture];
            [self preflightPublisher];
            [self preflightGB28181];
            [self setOperationAction:@"ios.autorun"
                                code:@"0"
                              status:@"ok"
                             summary:@"iOS simulator preflight autorun finished for player/capture/publisher/GB28181."];
        });
}

// 用户切换显示模式后，重新生成当前可见的 SDK 调用快照。
- (void)setOperationAction:(NSString*)action
                      code:(NSString*)code
                    status:(NSString*)status
                   summary:(NSString*)summary
{
    self.latestAction = action.length > 0 ? action : @"demo";
    self.latestStatusCode = code.length > 0 ? code : @"-";
    self.latestStatusName = status.length > 0 ? status : @"unknown";
    self.latestStatusSummary = summary.length > 0 ? summary : @"No details.";
    [self appendOperationLogLine];
    [self updateStatusLabel];
    [self refreshPreviewOverlays];
}

- (void)updateStatusLabel
{
    self.statusLabel.text = [NSString stringWithFormat:
        @"%@: %@ %@/%@\n%@\n%@: %@ | Zip: %@",
        [self uiTextEnglish:@"Status" chinese:@"状态"],
        self.latestAction ?: @"demo",
        self.latestStatusCode ?: @"-",
        self.latestStatusName ?: @"unknown",
        [self compactText:self.latestStatusSummary maxLength:72],
        [self uiTextEnglish:@"Log" chinese:@"日志"],
        [self compactPath:[self currentLogFilePath]],
        self.latestLogZipPath.length > 0 ? [self compactPath:self.latestLogZipPath] : [self uiTextEnglish:@"not exported" chinese:@"未导出"]];
}

- (NSString*)currentLogFilePath
{
    NSString* fileName = self.logFileName.length > 0 ?
        self.logFileName :
        @"streamcore_demo.log";
    if ([fileName hasPrefix:@"/"])
    {
        return fileName;
    }
    return [[self currentLogDirectory] stringByAppendingPathComponent:fileName];
}

// 优先使用 SDK log 生效目录，避免 Apple demo 与 SDK 默认路径不一致。
- (NSString*)currentLogDirectory
{
    if (self.logDirectory.length > 0)
    {
        return self.logDirectory;
    }
    NSArray<NSString*>* paths =
        NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString* rootDirectory = paths.count > 0 ? paths[0] : NSTemporaryDirectory();
    return [[rootDirectory stringByAppendingPathComponent:@"streamcore"]
        stringByAppendingPathComponent:@"logs"];
}

- (NSString*)currentProductCrashDirectory
{
    if (self.productCrashDirectory.length > 0)
    {
        return self.productCrashDirectory;
    }
    NSArray<NSString*>* paths =
        NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString* rootDirectory = paths.count > 0 ? paths[0] : NSTemporaryDirectory();
    return [[rootDirectory stringByAppendingPathComponent:@"streamcore"]
        stringByAppendingPathComponent:@"crash"];
}

- (NSString*)compactText:(NSString*)text maxLength:(NSUInteger)maxLength
{
    if (text.length <= maxLength)
    {
        return text ?: @"";
    }
    return [[text substringToIndex:maxLength] stringByAppendingString:@"..."];
}

- (NSString*)compactPath:(NSString*)path
{
    if (path.length <= 64)
    {
        return path ?: @"";
    }
    NSArray<NSString*>* parts = [path pathComponents];
    if (parts.count >= 3)
    {
        NSString* tail = [[parts subarrayWithRange:NSMakeRange(parts.count - 3, 3)]
            componentsJoinedByString:@"/"];
        return [@".../" stringByAppendingString:tail];
    }
    return [self compactText:path maxLength:64];
}

// 将可见状态追加到日志文件，保证 demo 没有界面日志块时仍有可分享记录。
- (void)appendOperationLogLine
{
    NSString* logPath = [self currentLogFilePath];
    NSString* directory = [logPath stringByDeletingLastPathComponent];
    [NSFileManager.defaultManager createDirectoryAtPath:directory
                            withIntermediateDirectories:YES
                                             attributes:nil
                                                  error:nil];
    NSString* line = [NSString stringWithFormat:
        @"%@ action=%@ code=%@ status=%@ summary=%@\n",
        [NSDate date],
        self.latestAction ?: @"demo",
        self.latestStatusCode ?: @"-",
        self.latestStatusName ?: @"unknown",
        self.latestStatusSummary ?: @""];
    NSFileHandle* handle = [NSFileHandle fileHandleForWritingAtPath:logPath];
    NSData* data = [line dataUsingEncoding:NSUTF8StringEncoding];
    if (handle == nil)
    {
        [data writeToFile:logPath atomically:YES];
        return;
    }
    @try
    {
        [handle seekToEndOfFile];
        [handle writeData:data];
        [handle closeFile];
    }
    @catch (__unused NSException* exception)
    {
    }
}

- (NSData*)zipDataWithEntries:(NSArray<NSDictionary<NSString*, NSData*>*>*)entries
{
    NSMutableData* archive = [NSMutableData data];
    NSMutableArray<NSDictionary*>* central = [NSMutableArray array];
    for (NSDictionary<NSString*, NSData*>* entry in entries)
    {
        NSString* name = entry.allKeys.firstObject;
        NSData* payload = entry[name] ?: [NSData data];
        NSData* nameData = [name dataUsingEncoding:NSUTF8StringEncoding];
        uint32_t crc = HBRCrc32(payload);
        uint32_t size = (uint32_t)payload.length;
        uint32_t offset = (uint32_t)archive.length;
        HBRAppendZipLe32(archive, 0x04034B50U);
        HBRAppendZipLe16(archive, 20);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe32(archive, crc);
        HBRAppendZipLe32(archive, size);
        HBRAppendZipLe32(archive, size);
        HBRAppendZipLe16(archive, (uint16_t)nameData.length);
        HBRAppendZipLe16(archive, 0);
        [archive appendData:nameData];
        [archive appendData:payload];
        [central addObject:@{@"name": nameData, @"crc": @(crc), @"size": @(size), @"offset": @(offset)}];
    }
    uint32_t centralOffset = (uint32_t)archive.length;
    for (NSDictionary* entry in central)
    {
        NSData* nameData = entry[@"name"];
        HBRAppendZipLe32(archive, 0x02014B50U);
        HBRAppendZipLe16(archive, 20);
        HBRAppendZipLe16(archive, 20);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe32(archive, [entry[@"crc"] unsignedIntValue]);
        HBRAppendZipLe32(archive, [entry[@"size"] unsignedIntValue]);
        HBRAppendZipLe32(archive, [entry[@"size"] unsignedIntValue]);
        HBRAppendZipLe16(archive, (uint16_t)nameData.length);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe16(archive, 0);
        HBRAppendZipLe32(archive, 0);
        HBRAppendZipLe32(archive, [entry[@"offset"] unsignedIntValue]);
        [archive appendData:nameData];
    }
    uint32_t centralSize = (uint32_t)archive.length - centralOffset;
    HBRAppendZipLe32(archive, 0x06054B50U);
    HBRAppendZipLe16(archive, 0);
    HBRAppendZipLe16(archive, 0);
    HBRAppendZipLe16(archive, (uint16_t)central.count);
    HBRAppendZipLe16(archive, (uint16_t)central.count);
    HBRAppendZipLe32(archive, centralSize);
    HBRAppendZipLe32(archive, centralOffset);
    HBRAppendZipLe16(archive, 0);
    return archive;
}

// 生成 zip entry 前做最小路径清理，避免相对路径穿透。
- (NSString*)safeZipEntryName:(NSString*)entryName
{
    NSString* safe = [entryName stringByReplacingOccurrencesOfString:@"\\"
                                                           withString:@"/"];
    safe = [safe stringByReplacingOccurrencesOfString:@"../" withString:@""];
    safe = [safe stringByReplacingOccurrencesOfString:@".." withString:@""];
    return safe;
}

// 单个文件进入日志包前统一执行去重、数量上限、总量上限和单文件上限。
- (BOOL)addCappedFileAtPath:(NSString*)path
                  entryName:(NSString*)entryName
                    entries:(NSMutableArray<NSDictionary<NSString*, NSData*>*>*)entries
                  seenPaths:(NSMutableSet<NSString*>*)seenPaths
                 totalBytes:(unsigned long long*)totalBytes
                  fileCount:(NSUInteger*)fileCount
                    skipped:(NSMutableString*)skipped
{
    if (path.length == 0 || entryName.length == 0)
    {
        return NO;
    }
    NSString* canonicalPath = [path stringByResolvingSymlinksInPath];
    if (canonicalPath.length == 0)
    {
        canonicalPath = path;
    }
    if ([seenPaths containsObject:canonicalPath])
    {
        return NO;
    }
    [seenPaths addObject:canonicalPath];

    NSDictionary<NSFileAttributeKey, id>* attributes =
        [NSFileManager.defaultManager attributesOfItemAtPath:path error:nil];
    if (attributes == nil || [attributes[NSFileType] isEqualToString:NSFileTypeDirectory])
    {
        return NO;
    }
    unsigned long long fileSize = [attributes[NSFileSize] unsignedLongLongValue];
    NSString* safeName = [self safeZipEntryName:entryName];
    if (*fileCount >= HBRLogPackageMaxFiles)
    {
        [skipped appendFormat:@"%@ | %llu bytes | skipped by file-count cap\n",
            safeName,
            fileSize];
        return NO;
    }
    if (fileSize > HBRLogPackageSingleFileMaxBytes)
    {
        [skipped appendFormat:@"%@ | %llu bytes | skipped by single-file cap\n",
            safeName,
            fileSize];
        return NO;
    }
    if (*totalBytes + fileSize > HBRLogPackageMaxBytes)
    {
        [skipped appendFormat:@"%@ | %llu bytes | skipped by total-size cap\n",
            safeName,
            fileSize];
        return NO;
    }

    NSData* data = [NSData dataWithContentsOfFile:path];
    if (data == nil)
    {
        [skipped appendFormat:@"%@ | open failed\n", safeName];
        return NO;
    }
    [entries addObject:@{safeName: data}];
    *totalBytes += fileSize;
    *fileCount += 1;
    return YES;
}

// 从日志或 crash 目录递归收集最近文件；真实 crash 捕获由宿主 crash 系统负责写入目录。
- (NSUInteger)addCappedFilesFromDirectory:(NSString*)directory
                                   prefix:(NSString*)prefix
                                  entries:(NSMutableArray<NSDictionary<NSString*, NSData*>*>*)entries
                                seenPaths:(NSMutableSet<NSString*>*)seenPaths
                               totalBytes:(unsigned long long*)totalBytes
                                fileCount:(NSUInteger*)fileCount
                                  skipped:(NSMutableString*)skipped
{
    NSURL* rootURL = [NSURL fileURLWithPath:directory ?: @"" isDirectory:YES];
    NSFileManager* fileManager = NSFileManager.defaultManager;
    NSNumber* isDirectory = nil;
    if (![rootURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil] ||
        !isDirectory.boolValue)
    {
        return 0;
    }

    NSArray<NSURLResourceKey>* keys = @[
        NSURLIsDirectoryKey,
        NSURLContentModificationDateKey,
        NSURLFileSizeKey
    ];
    NSDirectoryEnumerator<NSURL*>* enumerator =
        [fileManager enumeratorAtURL:rootURL
          includingPropertiesForKeys:keys
                             options:0
                        errorHandler:nil];
    NSMutableArray<NSURL*>* fileURLs = [NSMutableArray array];
    for (NSURL* fileURL in enumerator)
    {
        NSNumber* childIsDirectory = nil;
        if ([fileURL getResourceValue:&childIsDirectory forKey:NSURLIsDirectoryKey error:nil] &&
            !childIsDirectory.boolValue)
        {
            [fileURLs addObject:fileURL];
        }
    }
    [fileURLs sortUsingComparator:^NSComparisonResult(NSURL* left, NSURL* right) {
        NSDate* leftDate = nil;
        NSDate* rightDate = nil;
        [left getResourceValue:&leftDate forKey:NSURLContentModificationDateKey error:nil];
        [right getResourceValue:&rightDate forKey:NSURLContentModificationDateKey error:nil];
        return [(rightDate ?: [NSDate distantPast]) compare:(leftDate ?: [NSDate distantPast])];
    }];

    NSUInteger added = 0;
    NSString* rootPath = rootURL.path;
    if (![rootPath hasSuffix:@"/"])
    {
        rootPath = [rootPath stringByAppendingString:@"/"];
    }
    for (NSURL* fileURL in fileURLs)
    {
        NSString* filePath = fileURL.path;
        NSString* relativePath = [filePath hasPrefix:rootPath] ?
            [filePath substringFromIndex:rootPath.length] :
            fileURL.lastPathComponent;
        NSString* entryName = [prefix stringByAppendingString:relativePath];
        if ([self addCappedFileAtPath:filePath
                            entryName:entryName
                              entries:entries
                            seenPaths:seenPaths
                           totalBytes:totalBytes
                            fileCount:fileCount
                              skipped:skipped])
        {
            ++added;
        }
    }
    return added;
}

- (NSURL*)createLogPackage
{
    NSString* statusText = [NSString stringWithFormat:
        @"StreamCore SDK Demo status\naction=%@\ncode=%@\nstatus=%@\nsummary=%@\n"
         "log=%@\nlog_dir=%@\ncrash_dir=%@\npackage_limit_files=%lu\n"
         "package_limit_bytes=%llu\n"
         "crash_capture_note=StreamCore SDK does not install an Apple crash handler or expose a crash directory. "
         "Demo/server/desktop products should integrate Crashlytics, PLCrashReporter, Sentry, "
         "Crashpad, or another mature crash solution and write artifacts into this product directory.\n"
         "zip=%@\n",
        self.latestAction ?: @"demo",
        self.latestStatusCode ?: @"-",
        self.latestStatusName ?: @"unknown",
        self.latestStatusSummary ?: @"",
        [self currentLogFilePath],
        [self currentLogDirectory],
        [self currentProductCrashDirectory],
        (unsigned long)HBRLogPackageMaxFiles,
        HBRLogPackageMaxBytes,
        self.latestLogZipPath.length > 0 ? self.latestLogZipPath : @"not exported"];
    NSMutableArray<NSDictionary<NSString*, NSData*>*>* entries = [NSMutableArray arrayWithObject:
        @{@"demo_status.txt": [statusText dataUsingEncoding:NSUTF8StringEncoding]}];
    NSMutableSet<NSString*>* seenPaths = [NSMutableSet set];
    NSMutableString* skipped = [NSMutableString string];
    unsigned long long totalBytes = 0;
    NSUInteger fileCount = 0;
    NSUInteger addedFiles = 0;
    addedFiles += [self addCappedFilesFromDirectory:[self currentLogDirectory]
                                            prefix:@"logs/"
                                           entries:entries
                                         seenPaths:seenPaths
                                        totalBytes:&totalBytes
                                         fileCount:&fileCount
                                           skipped:skipped];
    addedFiles += [self addCappedFilesFromDirectory:[self currentProductCrashDirectory]
                                            prefix:@"crash/"
                                           entries:entries
                                         seenPaths:seenPaths
                                        totalBytes:&totalBytes
                                         fileCount:&fileCount
                                           skipped:skipped];
    if ([self addCappedFileAtPath:[self currentLogFilePath]
                        entryName:[@"logs/" stringByAppendingString:[self currentLogFilePath].lastPathComponent]
                          entries:entries
                        seenPaths:seenPaths
                       totalBytes:&totalBytes
                        fileCount:&fileCount
                          skipped:skipped])
    {
        ++addedFiles;
    }
    if (addedFiles == 0)
    {
        [entries addObject:@{@"logs/README.txt": [@"No SDK or demo log file has been created yet.\n" dataUsingEncoding:NSUTF8StringEncoding]}];
    }
    if (skipped.length > 0)
    {
        NSString* skippedText = [NSString stringWithFormat:
            @"The log package is capped to %lu files and %llu bytes.\n%@",
            (unsigned long)HBRLogPackageMaxFiles,
            HBRLogPackageMaxBytes,
            skipped];
        [entries addObject:@{@"skipped_files.txt": [skippedText dataUsingEncoding:NSUTF8StringEncoding]}];
    }
    NSURL* directory = [NSFileManager.defaultManager.temporaryDirectory URLByAppendingPathComponent:@"streamcore-log-share"
                                                                                       isDirectory:YES];
    [NSFileManager.defaultManager createDirectoryAtURL:directory
                           withIntermediateDirectories:YES
                                            attributes:nil
                                                 error:nil];
    NSString* fileName = [NSString stringWithFormat:@"streamcore_demo_logs_%.0f.zip", NSDate.date.timeIntervalSince1970];
    NSURL* zipURL = [directory URLByAppendingPathComponent:fileName];
    NSData* zipData = [self zipDataWithEntries:entries];
    return [zipData writeToURL:zipURL atomically:YES] ? zipURL : nil;
}

- (void)shareLogs
{
    NSURL* zipURL = [self createLogPackage];
    if (zipURL == nil)
    {
        [self setOperationAction:@"logs.share"
                            code:@"-1"
                          status:@"failed"
                         summary:[self uiTextEnglish:@"Failed to create the log package." chinese:@"日志包生成失败。"]];
        return;
    }
    self.latestLogZipPath = zipURL.path;
    [self setOperationAction:@"logs.share"
                        code:@"0"
                      status:@"ready"
                     summary:[self uiTextEnglish:@"The log package is ready." chinese:@"日志包已生成。"]];
    UIActivityViewController* controller =
        [[UIActivityViewController alloc] initWithActivityItems:@[zipURL]
                                          applicationActivities:nil];
    [self presentViewController:controller animated:YES completion:nil];
}

- (void)showUploadReserved
{
    NSString* message = [self uiTextEnglish:
        @"The upload entry is reserved because no private upload server is configured on this host. Export the zip first through Share Logs."
        chinese:@"当前宿主没有配置私有上传服务，上传入口仅保留占位。请先通过“分享日志”导出 zip。"];
    [self setOperationAction:@"logs.upload" code:@"-" status:@"reserved" summary:message];
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:[self uiTextEnglish:@"Upload Reserved" chinese:@"上传预留"]
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK"
                                              style:UIAlertActionStyleDefault
                                            handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)displayModeChanged:(UISegmentedControl*)sender
{
    (void)sender;
    [self reloadDemoSnapshot];
}

- (void)publisherDisplayModeChanged:(UISegmentedControl*)sender
{
    (void)sender;
    [self applyPublisherDisplayModeToPreviewHost];
}

- (void)captureSourceChanged:(UISegmentedControl*)sender
{
    (void)sender;
    self.captureLabel.text = [self captureText];
}

- (void)publisherInputChanged:(UISegmentedControl*)sender
{
    (void)sender;
    [self syncHiddenCaptureSourceSelection];
    const BOOL hasVideo = YES;
    self.publisherResolutionControl.enabled = hasVideo;
    self.publisherVideoCodecControl.enabled = hasVideo;
    self.publisherAudioVolumeSlider.enabled = [self selectedPublisherAudioEnabled];
    self.publisherAudioCodecControl.enabled = [self selectedPublisherAudioEnabled];
    self.publisherAudioVolumeLabel.text = [NSString stringWithFormat:
        @"采集音量：%.0f%%",
        [self selectedPublisherAudioVolumePercent]];
    self.publisherLabel.text = [self publisherText];
    [self updatePublisherPreviewPresentation];
    [self refreshPrimaryActionButtons];
    [self refreshPreviewOverlays];
}

- (void)gb28181SourceChanged:(UISegmentedControl*)sender
{
    (void)sender;
    [self syncGB28181SourceControls];
    self.gb28181Label.text = [NSString stringWithFormat:@"%@\n%@",
        [self gb28181ConfigText],
        [self gb28181Text]];
    [self refreshPrimaryActionButtons];
    [self refreshPreviewOverlays];
}

- (void)gb28181FieldChanged:(UITextField*)sender
{
    (void)sender;
    [self refreshPreviewOverlays];
}

- (void)gb28181DisplayModeChanged:(UISegmentedControl*)sender
{
    (void)sender;
    [self applyGB28181DisplayModeToPreviewHost];
    [self refreshPreviewOverlays];
}

// 返回当前 iOS 宿主显示策略的客户可读文本。
- (void)syncHiddenCaptureSourceSelection
{
    if (self.captureSourceControl == nil || self.publisherInputControl == nil)
    {
        return;
    }

    NSInteger targetIndex = self.captureSourceControl.selectedSegmentIndex;
    if (self.publisherInputControl.selectedSegmentIndex == 0)
    {
        targetIndex = 0;
    }
    else if (self.publisherInputControl.selectedSegmentIndex == 1)
    {
        targetIndex = 2;
    }

    if (self.captureSourceControl.selectedSegmentIndex != targetIndex)
    {
        self.captureSourceControl.selectedSegmentIndex = targetIndex;
    }
}

- (void)syncGB28181SourceControls
{
    if (self.gbMediaSourceEdit == nil)
    {
        return;
    }
    BOOL mediaFile = [self selectedGB28181UsesMediaFile];
    self.gbMediaSourceEdit.enabled = mediaFile;
    self.gbMediaSourceEdit.hidden = !mediaFile;
    self.gbMediaSourceLabel.hidden = !mediaFile;
    self.gbMediaSourceEdit.alpha = 1.0;
}

- (NSString*)selectedDisplayModeText
{
    if (self.displayModeControl.selectedSegmentIndex == 0)
    {
        return [self uiTextEnglish:@"Stretch fill" chinese:@"拉伸填满"];
    }
    if (self.displayModeControl.selectedSegmentIndex == 2)
    {
        return [self uiTextEnglish:@"Crop fill" chinese:@"裁剪填满"];
    }
    return [self uiTextEnglish:@"Keep aspect" chinese:@"保持比例"];
}

- (void)applyDisplayModeToPreviewHost
{
    [self applyDisplayModeIndex:self.displayModeControl.selectedSegmentIndex
                 toPreviewHost:self.playerPreviewHostView];
}

- (void)applyPublisherDisplayModeToPreviewHost
{
    if (self.publisherDisplayModeControl == nil)
    {
        return;
    }
    [self applyDisplayModeIndex:self.publisherDisplayModeControl.selectedSegmentIndex
                 toPreviewHost:self.publisherPreviewHostView];
}

- (void)applyGB28181DisplayModeToPreviewHost
{
    if (self.gbPreviewDisplayModeControl == nil)
    {
        return;
    }
    [self applyDisplayModeIndex:self.gbPreviewDisplayModeControl.selectedSegmentIndex
                 toPreviewHost:self.gbPreviewHostView];
}

- (void)applyDisplayModeIndex:(NSInteger)selectedSegmentIndex
                toPreviewHost:(UIView*)previewHost
{
    if (previewHost == nil)
    {
        return;
    }
    if (selectedSegmentIndex == 0)
    {
        previewHost.contentMode = UIViewContentModeScaleToFill;
    }
    else if (selectedSegmentIndex == 2)
    {
        previewHost.contentMode = UIViewContentModeScaleAspectFill;
    }
    else
    {
        previewHost.contentMode = UIViewContentModeScaleAspectFit;
    }
}

// 说明 iOS 显示模式属于宿主视图几何策略，不是 SDK C/ObjC 配置字段。
- (NSString*)displayModeText
{
    return [NSString stringWithFormat:
        @"%@: %@",
        [self uiTextEnglish:@"Active" chinese:@"当前"],
        [self selectedDisplayModeText]];
}

- (NSString*)bundledPathForResource:(NSString*)name type:(NSString*)type
{
    NSString* path = [NSBundle.mainBundle pathForResource:name ofType:type];
    return path != nil ? path : @"";
}

@end

#endif
