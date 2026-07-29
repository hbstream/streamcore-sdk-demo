// StreamCore SDK Demo WinForms main window.
//
// The demo is a product host for the public .NET SDK facade. It must not call
// private native P/Invoke entrypoints directly; all player, capture, publisher,
// GB28181, and runtime operations go through StreamCore.Sdk wrappers.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using StreamCore.Sdk;
using StreamCore.Sdk.Capture;
using StreamCore.Sdk.Gb28181;
using StreamCore.Sdk.Onvif;
using StreamCore.Sdk.Player;
using StreamCore.Sdk.Publisher;
using StreamCore.Sdk.Runtime;

namespace StreamCore.Demo.WinForms
{
    /// <summary>
    /// Main WinForms host for the StreamCore SDK demo pages.
    /// </summary>
    /// <remarks>
    /// The form owns UI controls, SDK session lifetimes, preview HWND binding,
    /// and autorun modes used by regression scripts. Native resources remain
    /// behind the public StreamCore.Sdk wrapper sessions.
    /// </remarks>
    public sealed class MainForm : Form
    {
        private enum DemoLanguageMode
        {
            Auto,
            English,
            Chinese
        }

        private const string LocalPublishUrl = "rtmp://127.0.0.1:1935/live/local_native";
        private const string PublisherBindingPrefix = "dotnet-winforms";
        private const int SidebarPreferredWidth = 430;
        private const int SidebarMinWidth = 340;
        private const int MediaStatusRowHeight = 250;
        private const int MediaStatusMinHeight = 200;
        private const int PreviewAspectWidth = 16;
        private const int PreviewAspectHeight = 9;
        private const int PreviewGroupChromeHeight = 44;
        private const int OnvifDiscoveryCapacity = 32;
        private const string DemoWatermarkText = "StreamCore Demo | hbrun.com";
        private readonly bool systemPrefersChinese;
        private DemoLanguageMode languageMode;
        private bool suppressLanguageSelectionChange;

        private readonly TextBox licenseTextBox = CreateMultilineEditor();
        private readonly TextBox licenseDetailTextBox = CreateMultilineEditor();
        private readonly Label licenseStateLabel = CreateStateLabel();
        private readonly Label productNameValueLabel = CreateValueLabel();
        private readonly Label productVersionValueLabel = CreateValueLabel();
        private readonly Label productStageValueLabel = CreateValueLabel();
        private readonly Label machineIdValueLabel = CreateValueLabel();
        private readonly Label licenseStatusValueLabel = CreateValueLabel();
        private readonly Label licenseLatestEventLabel = CreateStateLabel();

        private readonly ComboBox publisherSourceComboBox = CreateComboBox();
        private readonly ComboBox publisherVideoSourceComboBox = CreateComboBox();
        private readonly ComboBox publisherAudioComboBox = CreateComboBox();
        private readonly ComboBox publisherAudioSourceComboBox = CreateComboBox();
        private readonly ComboBox publisherResolutionComboBox = CreateComboBox();
        private readonly ComboBox publisherFileModeComboBox = CreateComboBox();
        private readonly ComboBox publisherVideoCodecComboBox = CreateComboBox();
        private readonly ComboBox publisherAudioCodecComboBox = CreateComboBox();
        private readonly ComboBox publisherAudioProfileComboBox = CreateComboBox();
        private readonly ComboBox publisherAudioSampleRateComboBox = CreateComboBox();
        private readonly ComboBox publisherAudioBitrateComboBox = CreateComboBox();
        private readonly ComboBox publisherRtmpHevcModeComboBox = CreateComboBox();
        private readonly ComboBox publisherProtocolComboBox = CreateComboBox();
        private readonly TextBox publisherMediaFileTextBox = CreateSingleLineEditor();
        private readonly TextBox publisherAudioFileTextBox = CreateSingleLineEditor();
        private readonly TextBox publisherPublishUrlTextBox = CreateSingleLineEditor();
        private readonly TextBox publisherWhipBearerTokenTextBox = CreateSingleLineEditor();
        private readonly NumericUpDown publisherVideoBitrateBox = CreateIntegerBox(128, 50000, 2000);
        private readonly NumericUpDown publisherFpsBox = CreateIntegerBox(1, 60, 25);
        private readonly NumericUpDown publisherGopBox = CreateIntegerBox(1, 300, 50);
        private readonly TrackBar publisherAudioVolumeTrackBar = CreateTrackBar(0, 100, 100);
        private readonly Label publisherAudioVolumeValueLabel = CreateInlineValueLabel();
        private readonly CheckBox publisherPreviewCheckBox = CreateCheckBox();
        private readonly Panel publisherPreviewPanel = CreatePreviewPanel();
        private readonly Label publisherPreviewOverlayLabel = CreatePreviewOverlayLabel();
        private readonly Label publisherStateLabel = CreateStateLabel();
        private readonly TextBox publisherStatusTextBox = CreateMultilineEditor();
        private readonly Button publisherRunButton = CreateButton(string.Empty, null);

        private readonly TextBox playerUrlTextBox = CreateSingleLineEditor();
        private readonly ComboBox playerDecodeModeComboBox = CreateComboBox();
        private readonly ComboBox playerRenderPathComboBox = CreateComboBox();
        private readonly NumericUpDown playerBufferMsBox = CreateIntegerBox(0, 10000, 300);
        private readonly TrackBar playerAudioVolumeTrackBar = CreateTrackBar(0, 100, 100);
        private readonly Label playerAudioVolumeValueLabel = CreateInlineValueLabel();
        private readonly NumericUpDown playerAudioQueueBox = CreateIntegerBox(0, 256, 12);
        private readonly NumericUpDown playerVideoQueueBox = CreateIntegerBox(0, 256, 6);
        private readonly CheckBox playerNoCacheCheckBox = CreateCheckBox();
        private readonly CheckBox playerRealtimeProfileCheckBox = CreateCheckBox();
        private readonly CheckBox playerOnvifEnabledCheckBox = CreateCheckBox();
        private readonly Button playerOnvifSearchButton = CreateButton(string.Empty, null);
        private readonly Label playerOnvifHintLabel = CreateHintLabel();
        private readonly ListBox playerOnvifListBox = CreateListBox();
        private readonly TextBox playerOnvifUsernameTextBox = CreateSingleLineEditor();
        private readonly TextBox playerOnvifPasswordTextBox = CreateSingleLineEditor();
        private readonly Panel playerRenderPanel = CreatePreviewPanel();
        private readonly Label playerOverlayLabel = CreatePreviewOverlayLabel();
        private readonly Label playerStateLabel = CreateStateLabel();
        private readonly TextBox playerStatusTextBox = CreateMultilineEditor();
        private readonly Button playerRunButton = CreateButton(string.Empty, null);

        private readonly TextBox gbLocalDeviceIdTextBox = CreateSingleLineEditor();
        private readonly TextBox gbDomainTextBox = CreateSingleLineEditor();
        private readonly TextBox gbLocalDisplayNameTextBox = CreateSingleLineEditor();
        private readonly TextBox gbLocalIpTextBox = CreateSingleLineEditor();
        private readonly TextBox gbUpperIdTextBox = CreateSingleLineEditor();
        private readonly TextBox gbUpperDomainTextBox = CreateSingleLineEditor();
        private readonly TextBox gbUpperPasswordTextBox = CreateSingleLineEditor();
        private readonly TextBox gbUpperDisplayNameTextBox = CreateSingleLineEditor();
        private readonly TextBox gbUpperIpTextBox = CreateSingleLineEditor();
        private readonly NumericUpDown gbUpperPortBox = CreateIntegerBox(1, 65535, 15060);
        private readonly NumericUpDown gbLocalPortBox = CreateIntegerBox(1, 65535, 5060);
        private readonly TextBox gbMediaIpTextBox = CreateSingleLineEditor();
        private readonly NumericUpDown gbMediaPortBox = CreateIntegerBox(1, 65535, 19000);
        private readonly ComboBox gbTransportComboBox = CreateComboBox();
        private readonly ComboBox gbSourceComboBox = CreateComboBox();
        private readonly ComboBox gbVideoSourceComboBox = CreateComboBox();
        private readonly ComboBox gbAudioComboBox = CreateComboBox();
        private readonly ComboBox gbAudioSourceComboBox = CreateComboBox();
        private readonly ComboBox gbResolutionComboBox = CreateComboBox();
        private readonly NumericUpDown gbFpsBox = CreateIntegerBox(1, 60, 25);
        private readonly NumericUpDown gbRegisterExpiresBox = CreateIntegerBox(60, 86400, 3600);
        private readonly NumericUpDown gbKeepaliveIntervalBox = CreateIntegerBox(5, 3600, 60);
        private readonly CheckBox gbEnableDigestAuthCheckBox = CreateCheckBox();
        private readonly CheckBox gbAutoAcceptInviteCheckBox = CreateCheckBox();
        private readonly TrackBar gbAudioVolumeTrackBar = CreateTrackBar(0, 100, 100);
        private readonly Label gbAudioVolumeValueLabel = CreateInlineValueLabel();
        private readonly Panel gbPreviewPanel = CreatePreviewPanel();
        private readonly Label gbPreviewOverlayLabel = CreatePreviewOverlayLabel();
        private readonly Label gbStateLabel = CreateStateLabel();
        private readonly TextBox gbStatusTextBox = CreateMultilineEditor();
        private readonly TextBox logsTextBox = CreateMultilineEditor();
        private readonly TabControl rootTabControl = new TabControl();
        private readonly ComboBox languageComboBox = CreateComboBox();
        private readonly Button gbRunButton = CreateButton(string.Empty, null);

        private readonly Label statusLabel = new Label();
        private readonly Timer autorunTimer = new Timer();
        private readonly Timer runtimeRefreshTimer = new Timer();
        private readonly Timer gbPollTimer = new Timer();
        private readonly List<SplitContainer> pageSplits = new List<SplitContainer>();
        private readonly List<StreamCoreOnvifDevice> playerOnvifDevices = new List<StreamCoreOnvifDevice>();
        private Control publisherVideoDeviceRow;
        private Control publisherAudioDeviceRow;
        private Control publisherMediaFileRow;
        private Control publisherAudioFileRow;
        private Control publisherVideoDetailRow;
        private Control publisherAudioDetailRow;
        private Control publisherFileModeRow;
        private Control publisherWhipBearerTokenRow;
        private Control playerOnvifSearchRow;
        private Control playerOnvifListRow;
        private Control playerOnvifCredentialRow;
        private Control gbVideoDeviceRow;
        private Control gbAudioDeviceRow;
        private bool onvifSearchInProgress;
        private bool suppressPublisherProtocolChange;

        private StreamCorePlayerSession playerSession;
        private StreamCorePublisherSession publisherSession;
        private StreamCoreCaptureSession publisherPreviewSession;
        private StreamCoreCaptureSession publisherMediaInputSession;
        private StreamCoreGb28181Session gbSession;

        private StreamCoreCaptureSourceInfo[] cameraSources = new StreamCoreCaptureSourceInfo[0];
        private StreamCoreCaptureSourceInfo[] desktopSources = new StreamCoreCaptureSourceInfo[0];
        private StreamCoreCaptureSourceInfo[] microphoneSources = new StreamCoreCaptureSourceInfo[0];
        private StreamCoreCaptureSourceInfo[] systemAudioSources = new StreamCoreCaptureSourceInfo[0];

        public MainForm()
        {
            systemPrefersChinese = string.Equals(
                CultureInfo.CurrentUICulture.TwoLetterISOLanguageName,
                "zh",
                StringComparison.OrdinalIgnoreCase);
            languageMode = StartupLanguageMode();

            Text = T("StreamCore SDK Demo", "StreamCore SDK Demo");
            Icon = LoadApplicationIcon();
            Width = 1240;
            Height = 860;
            MinimumSize = new Size(1040, 720);
            StartPosition = FormStartPosition.CenterScreen;
            AutoScaleMode = AutoScaleMode.Dpi;

            BuildLayout();
            BindEvents();
            ApplyDefaults();

            Load += OnLoad;
            Shown += OnShown;
            Resize += (sender, args) => ApplyPageSplitWidths();
            FormClosing += OnFormClosing;
        }

        private static Icon LoadApplicationIcon()
        {
            try
            {
                string executablePath = Application.ExecutablePath;
                if (string.IsNullOrWhiteSpace(executablePath) || !File.Exists(executablePath))
                {
                    return null;
                }

                return Icon.ExtractAssociatedIcon(executablePath);
            }
            catch
            {
                return null;
            }
        }

        private void BuildLayout()
        {
            pageSplits.Clear();
            rootTabControl.TabPages.Clear();

            TableLayoutPanel root = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 1,
                RowCount = 3,
                Padding = new Padding(12)
            };
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            Controls.Add(root);

            ConfigureLanguageComboBox();
            root.Controls.Add(BuildHeaderBar(), 0, 0);

            rootTabControl.Dock = DockStyle.Fill;
            root.Controls.Add(rootTabControl, 0, 1);

            TabPage publisherPage = new TabPage(T("Publisher", "推流"));
            TabPage playerPage = new TabPage(T("Player", "播放"));
            TabPage gbPage = new TabPage("GB28181");
            TabPage licensePage = new TabPage(T("License", "授权"));
            rootTabControl.TabPages.Add(publisherPage);
            rootTabControl.TabPages.Add(playerPage);
            rootTabControl.TabPages.Add(gbPage);
            rootTabControl.TabPages.Add(licensePage);

            BuildPublisherPage(publisherPage);
            BuildPlayerPage(playerPage);
            BuildGbPage(gbPage);
            BuildLicensePage(licensePage);

            statusLabel.Dock = DockStyle.Fill;
            statusLabel.AutoSize = false;
            statusLabel.Height = 30;
            statusLabel.Padding = new Padding(8, 0, 8, 0);
            statusLabel.TextAlign = ContentAlignment.MiddleLeft;
            statusLabel.BorderStyle = BorderStyle.FixedSingle;
            root.Controls.Add(statusLabel, 0, 2);
        }

        private Control BuildHeaderBar()
        {
            TableLayoutPanel header = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 2,
                AutoSize = true,
                Margin = new Padding(0, 0, 0, 8),
                Padding = new Padding(0)
            };
            header.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            header.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));

            Panel textHost = new Panel
            {
                Dock = DockStyle.Fill,
                Height = 44,
                Margin = new Padding(0)
            };

            Label titleLabel = new Label
            {
                Dock = DockStyle.Top,
                AutoSize = false,
                Height = 24,
                Font = new Font(Font, FontStyle.Bold),
                Text = "StreamCore SDK Demo",
                TextAlign = ContentAlignment.MiddleLeft
            };
            Label subtitleLabel = new Label
            {
                Dock = DockStyle.Top,
                AutoSize = false,
                Height = 18,
                ForeColor = Color.DimGray,
                Text = T(
                    "Publisher / Player / GB28181 / License",
                    "推流 / 播放 / GB28181 / 授权"),
                TextAlign = ContentAlignment.MiddleLeft
            };
            textHost.Controls.Add(subtitleLabel);
            textHost.Controls.Add(titleLabel);

            TableLayoutPanel languageHost = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 2,
                AutoSize = true,
                Anchor = AnchorStyles.Right | AnchorStyles.Top,
                Margin = new Padding(12, 0, 0, 0)
            };
            languageHost.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            languageHost.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120));

            Label languageLabel = new Label
            {
                AutoSize = true,
                Anchor = AnchorStyles.Left,
                Margin = new Padding(0, 8, 8, 0),
                Text = T("Language", "语言")
            };
            languageComboBox.Margin = new Padding(0);
            languageComboBox.Anchor = AnchorStyles.Right | AnchorStyles.Top;

            languageHost.Controls.Add(languageLabel, 0, 0);
            languageHost.Controls.Add(languageComboBox, 1, 0);

            header.Controls.Add(textHost, 0, 0);
            header.Controls.Add(languageHost, 1, 0);
            return header;
        }

        private void ConfigureLanguageComboBox()
        {
            suppressLanguageSelectionChange = true;
            languageComboBox.Items.Clear();
            languageComboBox.Items.Add(T("Auto", "自动"));
            languageComboBox.Items.Add("English");
            languageComboBox.Items.Add("中文");
            languageComboBox.SelectedIndex = (int)languageMode;
            suppressLanguageSelectionChange = false;
        }

        private static DemoLanguageMode StartupLanguageMode()
        {
            string language = Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_LANGUAGE");
            if (string.IsNullOrWhiteSpace(language))
            {
                return DemoLanguageMode.Auto;
            }

            language = language.Trim().ToLowerInvariant();
            if (language == "en" || language == "english")
            {
                return DemoLanguageMode.English;
            }
            if (language == "zh" || language == "cn" || language == "chinese")
            {
                return DemoLanguageMode.Chinese;
            }
            return DemoLanguageMode.Auto;
        }

        private void BuildPublisherPage(Control parent)
        {
            SplitContainer split = CreatePageSplit();
            pageSplits.Add(split);
            parent.Controls.Add(split);

            TableLayoutPanel left = CreateSidebarStack();
            Panel leftHost = CreateScrollHost(left);
            split.Panel1.Controls.Add(leftHost);

            publisherVideoDeviceRow = CreateFieldRow(T("Video device", "视频设备"), publisherVideoSourceComboBox);
            publisherAudioDeviceRow = CreateFieldRow(T("Audio device", "音频设备"), publisherAudioSourceComboBox);
            publisherMediaFileRow = CreateFieldRow(
                T("Video file", "视频文件"),
                CreatePathEditor(publisherMediaFileTextBox, BrowsePublisherMediaFile));
            publisherAudioFileRow = CreateFieldRow(
                T("Audio file", "音频文件"),
                CreatePathEditor(publisherAudioFileTextBox, BrowsePublisherAudioFile));
            publisherVideoDetailRow = CreateFieldRow(
                T("Video details", "视频设置"),
                CreateStackPanel(publisherVideoDeviceRow, publisherMediaFileRow));
            publisherAudioDetailRow = CreateFieldRow(
                T("Audio details", "音频设置"),
                CreateStackPanel(publisherAudioDeviceRow, publisherAudioFileRow));
            publisherFileModeRow = CreateFieldRow(
                T("File transcode", "文件转码"),
                publisherFileModeComboBox);
            publisherWhipBearerTokenTextBox.UseSystemPasswordChar = true;
            publisherWhipBearerTokenRow = CreateFieldRow(
                "Bearer Token",
                publisherWhipBearerTokenTextBox);
            publisherPreviewCheckBox.Text = T("Enable", "启用");

            AddSidebarSection(
                left,
                CreateSectionCard(
                    T("Source setup", "来源设置"),
                    CreateFieldRow(T("Video", "视频"), publisherSourceComboBox),
                    publisherVideoDetailRow,
                    CreateFieldRow(
                        T("Resolution", "分辨率"),
                        CreateInlineLabeledEditor(
                            publisherResolutionComboBox,
                            T("FPS", "帧率"),
                            publisherFpsBox)),
                    CreateFieldRow(T("Audio", "音频"), publisherAudioComboBox),
                    publisherAudioDetailRow));
            AddSidebarSection(
                left,
                CreateSectionCard(
                    T("Publish setup", "推流设置"),
                    CreateFieldRow(T("Protocol", "推流协议"), publisherProtocolComboBox),
                    CreateFieldRow(T("Video codec", "视频编码"), publisherVideoCodecComboBox),
                    CreateFieldRow(
                        T("Video bitrate", "视频码率"),
                        CreateInlineLabeledEditor(
                            publisherVideoBitrateBox,
                            "GOP",
                            publisherGopBox)),
                    CreateFieldRow(
                        T("Audio codec", "音频编码"),
                        CreateInlineLabeledEditor(
                            publisherAudioCodecComboBox,
                            T("Audio profile", "音频参数"),
                            publisherAudioProfileComboBox)),
                    CreateFieldRow(
                        T("Audio sample rate", "音频采样率"),
                        CreateInlineLabeledEditor(
                            publisherAudioSampleRateComboBox,
                            T("Audio bitrate", "音频码率"),
                            publisherAudioBitrateComboBox)),
                    CreateFieldRow(T("Publish URL", "推流地址"), publisherPublishUrlTextBox),
                    publisherWhipBearerTokenRow,
                    publisherFileModeRow,
                    CreateFieldRow(
                        "RTMP HEVC",
                        CreateInlineLabeledEditor(
                            publisherRtmpHevcModeComboBox,
                            T("Preview", "预览"),
                            publisherPreviewCheckBox))));
            AddSidebarSection(
                left,
                CreateActionPanel(
                    CreateRunVolumeRow(
                        publisherRunButton,
                        publisherAudioVolumeTrackBar,
                        publisherAudioVolumeValueLabel),
                    CreateActionBar(
                        CreateButton(T("Refresh sources", "刷新来源"), (sender, args) => RefreshCaptureSources()))));

            TableLayoutPanel right = CreateMediaRightPane();
            split.Panel2.Controls.Add(right);
            right.Controls.Add(
                CreatePreviewGroup(
                    T("Publisher Preview", "推流预览"),
                    publisherPreviewPanel,
                    publisherPreviewOverlayLabel),
                0,
                0);
            right.Controls.Add(
                CreateStatusGroup(
                    T("Publisher Runtime", "推流运行日志"),
                    publisherStateLabel,
                    publisherStatusTextBox),
                0,
                1);
        }

        private void BuildPlayerPage(Control parent)
        {
            SplitContainer split = CreatePageSplit();
            pageSplits.Add(split);
            parent.Controls.Add(split);

            TableLayoutPanel left = CreateSidebarStack();
            split.Panel1.Controls.Add(CreateScrollHost(left));

            playerOnvifListBox.IntegralHeight = false;
            playerOnvifListBox.Height = 76;
            playerOnvifSearchRow = CreateActionBar(playerOnvifSearchButton);
            playerOnvifListRow = CreateFieldRow(
                T("Device list", "设备列表"),
                playerOnvifListBox);
            playerOnvifCredentialRow = CreateFieldRow(
                T("Username", "用户名"),
                CreateInlineLabeledEditor(
                    playerOnvifUsernameTextBox,
                    T("Password", "密码"),
                    playerOnvifPasswordTextBox));
            AddSidebarSection(
                left,
                CreateSectionCard(
                    T("Player setup", "播放设置"),
                    CreateFieldRow(T("ONVIF", "ONVIF"), playerOnvifEnabledCheckBox),
                    playerOnvifSearchRow,
                    playerOnvifListRow,
                    playerOnvifCredentialRow,
                    CreateFieldRow(T("Player URL", "播放地址"), playerUrlTextBox),
                    CreateFieldRow(
                        T("Decode", "解码"),
                        CreateInlineLabeledEditor(
                            playerDecodeModeComboBox,
                            T("Render", "渲染"),
                            playerRenderPathComboBox)),
                    CreateFieldRow(
                        T("Network buffer ms", "网络缓冲 ms"),
                        CreateInlineLabeledEditor(
                            playerBufferMsBox,
                            T("Video queue", "视频队列"),
                            playerVideoQueueBox)),
                    CreateFieldRow(
                        T("Audio queue", "音频队列"),
                        playerAudioQueueBox)));
            AddSidebarSection(
                left,
                CreateActionPanel(
                    CreateRunVolumeRow(
                        playerRunButton,
                        playerAudioVolumeTrackBar,
                        playerAudioVolumeValueLabel),
                    CreateActionBar(
                        CreateButton(T("Preflight", "预检"), (sender, args) => RunPlayerPreflight()))));

            TableLayoutPanel right = CreateMediaRightPane();
            split.Panel2.Controls.Add(right);
            right.Controls.Add(
                CreatePreviewGroup(
                    T("Player Render Target", "播放渲染目标"),
                    playerRenderPanel,
                    playerOverlayLabel),
                0,
                0);
            right.Controls.Add(
                CreateStatusGroup(
                    T("Player Runtime", "播放运行日志"),
                    playerStateLabel,
                    playerStatusTextBox),
                0,
                1);
        }

        private void BuildGbPage(Control parent)
        {
            SplitContainer split = CreatePageSplit();
            pageSplits.Add(split);
            parent.Controls.Add(split);

            TableLayoutPanel left = CreateSidebarStack();
            split.Panel1.Controls.Add(CreateScrollHost(left));

            gbVideoDeviceRow = CreateFieldRow(T("Video device", "视频设备"), gbVideoSourceComboBox);
            gbAudioDeviceRow = CreateFieldRow(T("Audio device", "音频设备"), gbAudioSourceComboBox);
            Control gbLocalIdentityRow = CreateFieldRow(
                T("Device/channel ID", "设备/通道 ID"),
                gbLocalDeviceIdTextBox);
            Control gbLocalAuthRow = CreateFieldRow(
                T("SIP domain", "SIP 域"),
                gbDomainTextBox);
            Control gbUpperIdentityRow = CreateFieldRow(
                T("Upper ID", "上级 ID"),
                gbUpperIdTextBox);
            Control gbUpperDomainRow = CreateFieldRow(
                T("SIP domain", "SIP 域"),
                gbUpperDomainTextBox);
            Control gbUpperEndpointRow = CreateFieldRow(
                T("Upper SIP", "上级 SIP"),
                CreateInlineLabeledEditor(
                    gbUpperIpTextBox,
                    T("Port", "端口"),
                    gbUpperPortBox));
            Control gbSessionRow = CreateFieldRow(
                T("Transport", "传输方式"),
                CreateInlineLabeledEditor(
                    gbTransportComboBox,
                    T("Media port", "媒体端口"),
                    gbMediaPortBox));

            AddSidebarSection(
                left,
                CreateSectionCard(
                    T("Source setup", "来源设置"),
                    CreateFieldRow(T("Video", "视频"), gbSourceComboBox),
                    gbVideoDeviceRow,
                    CreateFieldRow(
                        T("Resolution", "分辨率"),
                        CreateInlineLabeledEditor(
                            gbResolutionComboBox,
                            T("FPS", "帧率"),
                            gbFpsBox)),
                    CreateFieldRow(T("Audio", "音频"), gbAudioComboBox),
                    gbAudioDeviceRow));
            AddSidebarSection(
                left,
                CreateSectionCard(
                    T("Local SIP", "本机 SIP"),
                    gbLocalIdentityRow,
                    gbLocalAuthRow,
                    CreateFieldRow(T("Local SIP port", "本机 SIP 端口"), gbLocalPortBox),
                    CreateFieldRow(T("Digest auth", "Digest 鉴权"), gbEnableDigestAuthCheckBox)));
            AddSidebarSection(
                left,
                CreateSectionCard(
                    T("Upper platform", "上级平台"),
                    gbUpperIdentityRow,
                    gbUpperDomainRow,
                    CreateFieldRow(T("SIP password", "SIP 密码"), gbUpperPasswordTextBox),
                    gbUpperEndpointRow,
                    gbSessionRow,
                    CreateFieldRow(T("INVITE policy", "INVITE 策略"), gbAutoAcceptInviteCheckBox)));

            TableLayoutPanel right = CreateMediaRightPane();
            split.Panel2.Controls.Add(right);
            TableLayoutPanel gbPreviewStack = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 1,
                RowCount = 2
            };
            gbPreviewStack.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            gbPreviewStack.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
            gbPreviewStack.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            gbPreviewStack.Controls.Add(
                CreatePreviewGroup(
                    T("GB28181 Preview", "GB28181 预览"),
                    gbPreviewPanel,
                    gbPreviewOverlayLabel),
                0,
                0);
            gbPreviewStack.Controls.Add(
                CreateActionPanel(
                    CreateRunVolumeRow(
                        gbRunButton,
                        gbAudioVolumeTrackBar,
                        gbAudioVolumeValueLabel),
                    CreateActionBar(
                        CreateButton(T("Refresh sources", "刷新来源"), (sender, args) => RefreshCaptureSources()))),
                0,
                1);
            right.Controls.Add(
                gbPreviewStack,
                0,
                0);
            right.Controls.Add(
                CreateStatusGroup(
                    T("GB28181 Runtime", "GB28181 运行日志"),
                    gbStateLabel,
                    gbStatusTextBox),
                0,
                1);
        }

        private void BuildLicensePage(Control parent)
        {
            SplitContainer split = CreatePageSplit();
            pageSplits.Add(split);
            parent.Controls.Add(split);

            TableLayoutPanel left = CreateFormGrid();
            split.Panel1.Controls.Add(CreateScrollHost(left));

            int row = 0;
            AddGridRow(left, ref row, T("Product", "产品"), productNameValueLabel);
            AddGridRow(left, ref row, T("Version", "版本"), productVersionValueLabel);
            AddGridRow(left, ref row, T("Stage", "阶段"), productStageValueLabel);
            AddGridRow(left, ref row, T("Machine ID", "机器码"), machineIdValueLabel);
            AddGridRow(left, ref row, T("License state", "授权状态"), licenseStatusValueLabel);
            AddGridRow(left, ref row, T("License text", "授权字符串"), licenseTextBox);
            AddGridRow(left, ref row, T("Latest event", "最近事件"), licenseLatestEventLabel);
            AddGridRow(
                left,
                ref row,
                T("Actions", "操作"),
                CreateActionBar(
                    CreateButton(T("Register license", "注册授权"), (sender, args) => RegisterLicense()),
                    CreateButton(T("Refresh runtime", "刷新运行时"), (sender, args) => RefreshLicensePanel(null)),
                    CreateButton(T("Copy report", "复制报告"), (sender, args) => CopyLogReport()),
                    CreateButton(T("Export report", "导出报告"), (sender, args) => ExportLogReport()),
                    CreateButton(T("Clear logs", "清空日志"), (sender, args) => ClearLogView()),
                    CreateButton(T("Clear runtime", "清空运行时"), (sender, args) => ClearRuntimeState())));

            TableLayoutPanel right = CreateRightPane();
            split.Panel2.Controls.Add(right);
            GroupBox detailGroup = CreateStatusGroup(
                T("License Detail", "授权详情"),
                licenseStateLabel,
                licenseDetailTextBox);
            right.Controls.Add(detailGroup, 0, 0);
            right.SetRowSpan(detailGroup, 2);
        }

        private void BindEvents()
        {
            languageComboBox.SelectedIndexChanged += (sender, args) =>
            {
                if (suppressLanguageSelectionChange)
                {
                    return;
                }

                DemoLanguageMode selectedMode =
                    languageComboBox.SelectedIndex == 2 ? DemoLanguageMode.Chinese :
                    languageComboBox.SelectedIndex == 1 ? DemoLanguageMode.English :
                    DemoLanguageMode.Auto;
                if (selectedMode == languageMode)
                {
                    return;
                }

                languageMode = selectedMode;
                RebuildLayoutForLanguageChange();
            };
            publisherSourceComboBox.SelectedIndexChanged += (sender, args) =>
            {
                RefreshPublisherVideoSources();
                RefreshPublisherResolutionOptions();
                UpdatePublisherUi();
            };
            publisherAudioComboBox.SelectedIndexChanged += (sender, args) => UpdatePublisherUi();
            publisherFileModeComboBox.SelectedIndexChanged += (sender, args) => UpdatePublisherUi();
            publisherVideoCodecComboBox.SelectedIndexChanged += (sender, args) => UpdatePublisherUi();
            publisherAudioCodecComboBox.SelectedIndexChanged += (sender, args) =>
            {
                RefreshPublisherAudioProfileOptions();
                UpdatePublisherUi();
            };
            publisherProtocolComboBox.SelectedIndexChanged += (sender, args) =>
            {
                if (!suppressPublisherProtocolChange)
                {
                    ApplyPublisherProtocolSelection();
                }
            };
            publisherPublishUrlTextBox.TextChanged += (sender, args) =>
            {
                SyncPublisherProtocolFromUrl();
                UpdatePublisherUi();
            };
            publisherWhipBearerTokenTextBox.TextChanged += (sender, args) => UpdatePublisherUi();
            publisherVideoSourceComboBox.SelectedIndexChanged += (sender, args) =>
            {
                RefreshPublisherResolutionOptions();
                UpdatePublisherUi();
            };
            publisherRunButton.Click += (sender, args) => TogglePublisher();
            publisherAudioVolumeTrackBar.Scroll += (sender, args) =>
            {
                publisherAudioVolumeValueLabel.Text = publisherAudioVolumeTrackBar.Value + "%";
            };
            publisherPreviewCheckBox.CheckedChanged += (sender, args) => ApplyPublisherPreviewToggle();
            publisherPreviewPanel.Resize += (sender, args) =>
            {
                if (publisherPreviewSession != null)
                {
                    publisherPreviewSession.SetWindowsPreviewRenderTarget(
                        publisherPreviewPanel.Handle,
                        publisherPreviewPanel.Width,
                        publisherPreviewPanel.Height);
                }
                if (publisherMediaInputSession != null && publisherPreviewCheckBox.Checked)
                {
                    publisherMediaInputSession.SetWindowsPreviewRenderTarget(
                        publisherPreviewPanel.Handle,
                        publisherPreviewPanel.Width,
                        publisherPreviewPanel.Height);
                }
            };

            playerDecodeModeComboBox.SelectedIndexChanged += (sender, args) => UpdatePlayerOverlay();
            playerRenderPathComboBox.SelectedIndexChanged += (sender, args) => UpdatePlayerOverlay();
            playerRunButton.Click += (sender, args) => TogglePlayer();
            playerOnvifEnabledCheckBox.CheckedChanged += (sender, args) => UpdatePlayerOnvifUi();
            playerOnvifSearchButton.Click += (sender, args) => SearchOnvifDevices();
            playerOnvifListBox.DoubleClick += (sender, args) => ApplySelectedOnvifDevice();
            playerAudioVolumeTrackBar.Scroll += (sender, args) =>
            {
                playerAudioVolumeValueLabel.Text = playerAudioVolumeTrackBar.Value + "%";
                if (playerSession != null)
                {
                    StreamCoreResult result = playerSession.SetAudioVolume(playerAudioVolumeTrackBar.Value);
                    if (result != StreamCoreResult.Ok)
                    {
                        LogPlayer("set_audio_volume failed: " + result);
                    }
                }
            };
            playerRenderPanel.Resize += (sender, args) =>
            {
                if (playerSession != null)
                {
                    playerSession.SetWindowsRenderTarget(
                        playerRenderPanel.Handle,
                        playerRenderPanel.Width,
                        playerRenderPanel.Height);
                }
            };

            gbSourceComboBox.SelectedIndexChanged += (sender, args) =>
            {
                RefreshGbVideoSources();
                RefreshGbResolutionOptions();
                UpdateGbUi();
            };
            gbAudioComboBox.SelectedIndexChanged += (sender, args) => UpdateGbUi();
            gbRunButton.Click += (sender, args) => ToggleGb();
            gbVideoSourceComboBox.SelectedIndexChanged += (sender, args) =>
            {
                RefreshGbResolutionOptions();
                UpdateGbUi();
            };
            gbAudioVolumeTrackBar.Scroll += (sender, args) =>
            {
                gbAudioVolumeValueLabel.Text = gbAudioVolumeTrackBar.Value + "%";
            };

            runtimeRefreshTimer.Interval = 1000;
            runtimeRefreshTimer.Tick += (sender, args) => RefreshRuntimeViews();

            gbPollTimer.Interval = 250;
            gbPollTimer.Tick += (sender, args) => PollGbRuntime();

            autorunTimer.Tick += (sender, args) =>
            {
                autorunTimer.Stop();
                try
                {
                    string autorunMode =
                        Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_AUTORUN");
                    if (string.Equals(autorunMode, "gb28181_lifecycle", StringComparison.OrdinalIgnoreCase)
                        || string.Equals(autorunMode, "gb_lifecycle", StringComparison.OrdinalIgnoreCase))
                    {
                        StopGb();
                    }
                }
                catch (Exception ex)
                {
                    SetStatus("autorun_tick failed: " + ex.Message);
                }
                WriteAutorunReport();
                Close();
            };
        }

        private void ApplyDefaults()
        {
            publisherSourceComboBox.Items.Clear();
            publisherAudioComboBox.Items.Clear();
            publisherFileModeComboBox.Items.Clear();
            publisherVideoCodecComboBox.Items.Clear();
            publisherAudioCodecComboBox.Items.Clear();
            publisherAudioProfileComboBox.Items.Clear();
            publisherAudioSampleRateComboBox.Items.Clear();
            publisherAudioBitrateComboBox.Items.Clear();
            publisherRtmpHevcModeComboBox.Items.Clear();
            publisherProtocolComboBox.Items.Clear();
            playerDecodeModeComboBox.Items.Clear();
            playerRenderPathComboBox.Items.Clear();
            gbTransportComboBox.Items.Clear();
            gbSourceComboBox.Items.Clear();
            gbAudioComboBox.Items.Clear();

            AddComboValue(publisherSourceComboBox, T("Camera", "摄像头"), PublisherSourceMode.Camera);
            AddComboValue(publisherSourceComboBox, T("Desktop capture", "桌面采集"), PublisherSourceMode.Desktop);
            AddComboValue(publisherSourceComboBox, T("Video file", "视频文件"), PublisherSourceMode.VideoFile);
            AddComboValue(publisherSourceComboBox, T("Still image", "静态图片"), PublisherSourceMode.StillImage);
            AddComboValue(publisherSourceComboBox, T("None", "无"), PublisherSourceMode.None);

            AddComboValue(publisherAudioComboBox, T("None", "无"), PublisherAudioMode.None);
            AddComboValue(publisherAudioComboBox, T("Microphone", "麦克风"), PublisherAudioMode.Microphone);
            AddComboValue(publisherAudioComboBox, T("System audio", "系统音频"), PublisherAudioMode.SystemAudio);
            AddComboValue(publisherAudioComboBox, T("Audio file", "音频文件"), PublisherAudioMode.AudioFile);

            AddComboValue(publisherFileModeComboBox, T("Auto passthrough", "自动透传"), StreamCorePublisherTranscodeMode.Auto);
            AddComboValue(publisherFileModeComboBox, T("Force transcode", "强制转码"), StreamCorePublisherTranscodeMode.ForceTranscode);
            AddComboValue(publisherVideoCodecComboBox, "H.264", "h264");
            AddComboValue(publisherVideoCodecComboBox, "H.265 / HEVC", "h265");
            AddComboValue(publisherAudioCodecComboBox, "AAC", "aac");
            AddComboValue(publisherAudioCodecComboBox, "Opus", "opus");
            RefreshPublisherAudioProfileOptions();
            AddComboValue(publisherAudioSampleRateComboBox, "32000 Hz", 32000);
            AddComboValue(publisherAudioSampleRateComboBox, "44100 Hz", 44100);
            AddComboValue(publisherAudioSampleRateComboBox, "48000 Hz", 48000);
            AddComboValue(publisherAudioBitrateComboBox, "64 kbps", 64);
            AddComboValue(publisherAudioBitrateComboBox, "96 kbps", 96);
            AddComboValue(publisherAudioBitrateComboBox, "128 kbps", 128);
            AddComboValue(publisherAudioBitrateComboBox, "192 kbps", 192);
            AddComboValue(
                publisherRtmpHevcModeComboBox,
                T("Compatibility default", "兼容默认"),
                StreamCorePublisherRtmpHevcMode.AutoCompatibility);
            AddComboValue(
                publisherRtmpHevcModeComboBox,
                "Legacy HEVC FLV",
                StreamCorePublisherRtmpHevcMode.LegacyFlvTag);
            AddComboValue(
                publisherRtmpHevcModeComboBox,
                "Enhanced RTMP",
                StreamCorePublisherRtmpHevcMode.EnhancedRtmp);
            AddComboValue(publisherProtocolComboBox, "RTMP", "rtmp");
            AddComboValue(publisherProtocolComboBox, "RTSP", "rtsp");
            AddComboValue(publisherProtocolComboBox, "SRT", "srt");
            AddComboValue(publisherProtocolComboBox, "WHIP", "whip");

            AddComboValue(playerDecodeModeComboBox, T("Software decode", "软件解码"), false);
            AddComboValue(playerDecodeModeComboBox, T("Hardware decode", "硬件解码"), true);
            AddComboValue(playerRenderPathComboBox, T("Software frame", "软件帧"), StreamCorePlayerVideoPresentPath.SoftwareFrame);
            AddComboValue(playerRenderPathComboBox, "GPU frame", StreamCorePlayerVideoPresentPath.GpuFrame);
            AddComboValue(playerRenderPathComboBox, "Direct surface", StreamCorePlayerVideoPresentPath.DirectSurface);
            AddComboValue(playerRenderPathComboBox, "SDK auto", StreamCorePlayerVideoPresentPath.Auto);
            playerOnvifEnabledCheckBox.Text = T("Enable", "启用");
            playerOnvifSearchButton.Text = T("Search ONVIF", "搜索 ONVIF");
            playerOnvifHintLabel.Text = T(
                "Double-click a discovered device below to resolve and use its stream.",
                "双击下方设备即可解析并使用流地址。");
            playerOnvifPasswordTextBox.UseSystemPasswordChar = true;

            AddComboValue(gbTransportComboBox, "TCP", StreamCoreGb28181TransportMode.Tcp);
            AddComboValue(gbTransportComboBox, "UDP", StreamCoreGb28181TransportMode.Udp);
            AddComboValue(gbSourceComboBox, T("Camera", "摄像头"), PublisherSourceMode.Camera);
            AddComboValue(gbSourceComboBox, T("Desktop capture", "桌面采集"), PublisherSourceMode.Desktop);
            AddComboValue(gbAudioComboBox, T("Microphone", "麦克风"), GbAudioMode.Microphone);
            AddComboValue(gbAudioComboBox, T("System audio", "系统音频"), GbAudioMode.SystemAudio);
            AddComboValue(gbAudioComboBox, T("None", "无"), GbAudioMode.None);

            publisherSourceComboBox.SelectedIndex = 0;
            publisherAudioComboBox.SelectedIndex = 1;
            publisherFileModeComboBox.SelectedIndex = 0;
            publisherVideoCodecComboBox.SelectedIndex = 0;
            publisherAudioCodecComboBox.SelectedIndex = 0;
            publisherAudioProfileComboBox.SelectedIndex = 0;
            publisherAudioSampleRateComboBox.SelectedIndex = 2;
            publisherAudioBitrateComboBox.SelectedIndex = 2;
            publisherRtmpHevcModeComboBox.SelectedIndex = 0;
            publisherPreviewCheckBox.Checked = !string.Equals(
                Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_PUBLISHER_PREVIEW"),
                "0",
                StringComparison.OrdinalIgnoreCase);

            playerDecodeModeComboBox.SelectedIndex = 0;
            playerRenderPathComboBox.SelectedIndex = 0;
            playerNoCacheCheckBox.Text = T("Disable audio/video cache", "禁用音视频缓存");
            playerNoCacheCheckBox.Checked = false;
            playerRealtimeProfileCheckBox.Text = T("Enable realtime queue profile", "启用实时队列配置");
            playerRealtimeProfileCheckBox.Checked = false;
            playerOnvifEnabledCheckBox.Checked = false;
            playerOnvifDevices.Clear();
            playerOnvifListBox.Height = 144;
            playerOnvifListBox.Items.Clear();
            playerOnvifListBox.Items.Add(T("No ONVIF devices discovered", "尚未搜索到 ONVIF 设备"));

            gbTransportComboBox.SelectedIndex = 0;
            gbSourceComboBox.SelectedIndex = 0;
            gbAudioComboBox.SelectedIndex = 0;
            gbEnableDigestAuthCheckBox.Text = T("Enable", "启用");
            gbEnableDigestAuthCheckBox.Checked = true;
            gbAutoAcceptInviteCheckBox.Text = T("Auto accept", "自动接受");
            gbAutoAcceptInviteCheckBox.Checked = true;

            publisherPublishUrlTextBox.Text = LocalPublishUrl;
            playerUrlTextBox.Text = LocalPublishUrl;
            publisherMediaFileTextBox.Text = FindWorkspaceRelativeFile(
                "testfile",
                "streamcore_probe_h264_aac_12s.mp4");

            gbLocalDeviceIdTextBox.Text = "34020000001320000001";
            gbDomainTextBox.Text = "3402000000";
            gbUpperPasswordTextBox.Text = "123456";
            gbLocalDisplayNameTextBox.Text = "StreamCore WinForms Device";
            gbLocalIpTextBox.Text = "0.0.0.0";
            gbUpperIdTextBox.Text = "34020000002000000001";
            gbUpperDomainTextBox.Text = gbDomainTextBox.Text;
            gbUpperDisplayNameTextBox.Text = "Upper SIP Platform";
            gbUpperIpTextBox.Text = "127.0.0.1";
            gbMediaIpTextBox.Text = "0.0.0.0";

            licenseTextBox.Height = 180;
            licenseDetailTextBox.ReadOnly = true;
            publisherStatusTextBox.ReadOnly = true;
            playerStatusTextBox.ReadOnly = true;
            gbStatusTextBox.ReadOnly = true;
            logsTextBox.ReadOnly = true;
            licenseLatestEventLabel.Height = 82;
            licenseLatestEventLabel.Text = T("No events yet.", "暂无事件。");

            publisherAudioVolumeValueLabel.Text = publisherAudioVolumeTrackBar.Value + "%";
            playerAudioVolumeValueLabel.Text = playerAudioVolumeTrackBar.Value + "%";
            gbAudioVolumeValueLabel.Text = gbAudioVolumeTrackBar.Value + "%";
            publisherRunButton.MinimumSize = new Size(132, 0);
            playerRunButton.MinimumSize = new Size(132, 0);
            gbRunButton.MinimumSize = new Size(132, 0);

            RefreshPublisherResolutionOptions();
            RefreshGbResolutionOptions();
            UpdatePlayerOnvifUi();
            UpdatePlayerOverlay();
            UpdatePublisherUi();
            UpdateGbUi();
        }

        private void OnLoad(object sender, EventArgs e)
        {
            StreamCoreProductInfo product = StreamCoreRuntime.GetProductInfo();
            SetStatus(product.ProductName + " " + product.Version);
            RefreshCaptureSources();
            ConfigureDemoLicenseIfPresent();
            RefreshLicensePanel(null);
            runtimeRefreshTimer.Start();
            ScheduleAutorunIfRequested();
        }

        private void OnFormClosing(object sender, FormClosingEventArgs e)
        {
            autorunTimer.Stop();
            runtimeRefreshTimer.Stop();
            gbPollTimer.Stop();

            StopPlayer();
            StopPublisher();
            StopGb();
        }

        private void OnShown(object sender, EventArgs e)
        {
            ApplyPageSplitWidths();
            ApplyStartupPageIfRequested();
        }

        private void RebuildLayoutForLanguageChange()
        {
            int selectedTabIndex = rootTabControl.SelectedIndex;

            StopPlayer();
            StopPublisher();
            StopGb();

            SuspendLayout();
            Controls.Clear();
            BuildLayout();
            ApplyDefaults();
            ResumeLayout(true);

            if (rootTabControl.TabPages.Count > 0)
            {
                rootTabControl.SelectedIndex = Math.Max(
                    0,
                    Math.Min(selectedTabIndex, rootTabControl.TabPages.Count - 1));
            }

            RefreshCaptureSources();
            RefreshLicensePanel(null);
            RefreshRuntimeViews();
            ApplyPageSplitWidths();
            SetStatus(T("language switched", "语言已切换"));
        }

        private void ApplyStartupPageIfRequested()
        {
            string page = EnvironmentValue("STREAMCORE_DEMO_DOTNET_START_PAGE");
            if (string.IsNullOrWhiteSpace(page))
            {
                return;
            }

            if (page == "publisher")
            {
                rootTabControl.SelectedIndex = 0;
            }
            else if (page == "player")
            {
                rootTabControl.SelectedIndex = 1;
            }
            else if (page == "gb" || page == "gb28181")
            {
                rootTabControl.SelectedIndex = 2;
            }
            else if (page == "license" || page == "runtime")
            {
                rootTabControl.SelectedIndex = 3;
            }
            else if (page == "logs" || page == "diagnostics")
            {
                rootTabControl.SelectedIndex = 3;
            }
        }

        private void RegisterLicense()
        {
            try
            {
                StreamCoreCallResult call = StreamCoreRuntime.RegisterLicenseText(licenseTextBox.Text.Trim());
                RefreshLicensePanel("register_license=" + call.Result + " " + call.ErrorText);
            }
            catch (Exception ex)
            {
                RefreshLicensePanel(ex.Message);
            }
        }

        private void ClearRuntimeState()
        {
            StreamCoreRuntime.ClearLicense();
            RefreshLicensePanel(T("runtime_cleared", "runtime_cleared"));
        }

        private string BuildLogReportText(string autorunMode)
        {
            StringBuilder builder = new StringBuilder();
            builder.AppendLine("status=" + statusLabel.Text);
            if (!string.IsNullOrWhiteSpace(autorunMode))
            {
                builder.AppendLine("autorun_mode=" + autorunMode);
            }

            builder.AppendLine("product=" + productNameValueLabel.Text);
            builder.AppendLine("version=" + productVersionValueLabel.Text);
            builder.AppendLine("machine_id=" + machineIdValueLabel.Text);
            builder.AppendLine("gb_transport=" + SelectedValue(
                gbTransportComboBox,
                StreamCoreGb28181TransportMode.Tcp));
            builder.AppendLine("gb_runtime_active=" + (gbSession != null));
            builder.AppendLine("[license_detail]");
            builder.AppendLine(licenseDetailTextBox.Text);
            builder.AppendLine("[global_log]");
            builder.AppendLine(logsTextBox.Text);
            builder.AppendLine("[publisher_state]");
            builder.AppendLine(publisherStateLabel.Text);
            builder.AppendLine("[publisher_log]");
            builder.AppendLine(publisherStatusTextBox.Text);
            builder.AppendLine("[player_state]");
            builder.AppendLine(playerStateLabel.Text);
            builder.AppendLine("[player_log]");
            builder.AppendLine(playerStatusTextBox.Text);
            builder.AppendLine("[gb_state]");
            builder.AppendLine(gbStateLabel.Text);
            builder.AppendLine("[gb_log]");
            builder.AppendLine(gbStatusTextBox.Text);
            return builder.ToString();
        }

        private void CopyLogReport()
        {
            try
            {
                Clipboard.SetText(BuildLogReportText(string.Empty));
                SetStatus("log_report copied");
            }
            catch (Exception ex)
            {
                SetStatus("copy_log_report failed: " + ex.Message);
            }
        }

        private void ExportLogReport()
        {
            try
            {
                using (SaveFileDialog dialog = new SaveFileDialog())
                {
                    dialog.Filter = "Text report (*.txt)|*.txt|All files (*.*)|*.*";
                    dialog.FileName = "streamcore_sdk_demo_logs_"
                        + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".txt";
                    if (dialog.ShowDialog(this) != DialogResult.OK)
                    {
                        return;
                    }

                    File.WriteAllText(
                        dialog.FileName,
                        BuildLogReportText(string.Empty),
                        Encoding.UTF8);
                    SetStatus("export_log_report=" + dialog.FileName);
                }
            }
            catch (Exception ex)
            {
                SetStatus("export_log_report failed: " + ex.Message);
            }
        }

        private void ClearLogView()
        {
            logsTextBox.Clear();
            publisherStatusTextBox.Clear();
            playerStatusTextBox.Clear();
            gbStatusTextBox.Clear();
            licenseLatestEventLabel.Text = T("Log view cleared.", "日志视图已清空。");
            SetStatus("log_view cleared");
        }

        private void RefreshPublisherAudioProfileOptions()
        {
            string selectedCodec = SelectedTextValue(
                publisherAudioCodecComboBox,
                "aac");
            string previousProfile = SelectedTextValue(
                publisherAudioProfileComboBox,
                string.Empty);
            publisherAudioProfileComboBox.Items.Clear();
            if (string.Equals(selectedCodec, "opus", StringComparison.OrdinalIgnoreCase))
            {
                AddComboValue(publisherAudioProfileComboBox, "Opus", "opus");
                return;
            }

            AddComboValue(publisherAudioProfileComboBox, "AAC-LC", "aac");
            AddComboValue(publisherAudioProfileComboBox, "HE-AAC", "heaac");
            AddComboValue(publisherAudioProfileComboBox, "AAC-ELD", "aac-eld");
            if (!string.IsNullOrWhiteSpace(previousProfile))
            {
                SelectComboByValue(publisherAudioProfileComboBox, previousProfile);
            }
        }

        private bool IsWhipPublisherTarget()
        {
            string url = publisherPublishUrlTextBox.Text.Trim();
            return url.StartsWith("https://", StringComparison.OrdinalIgnoreCase)
                || url.StartsWith("http://", StringComparison.OrdinalIgnoreCase);
        }

        private void SyncPublisherProtocolFromUrl()
        {
            string url = publisherPublishUrlTextBox.Text.Trim();
            string protocol = string.Empty;
            if (IsWhipPublisherTarget())
            {
                protocol = "whip";
            }
            else
            {
                int separator = url.IndexOf("://", StringComparison.Ordinal);
                if (separator > 0)
                {
                    protocol = url.Substring(0, separator).ToLowerInvariant();
                }
            }

            string current = SelectedTextValue(publisherProtocolComboBox, "rtmp");
            if (string.IsNullOrWhiteSpace(protocol)
                || string.Equals(current, protocol, StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            suppressPublisherProtocolChange = true;
            try
            {
                SelectComboByValue(publisherProtocolComboBox, protocol);
            }
            finally
            {
                suppressPublisherProtocolChange = false;
            }
            if (string.Equals(protocol, "whip", StringComparison.OrdinalIgnoreCase))
            {
                SelectComboByValue(publisherVideoCodecComboBox, "h264");
                SelectComboByValue(publisherAudioCodecComboBox, "opus");
            }
        }

        private void ApplyPublisherProtocolSelection()
        {
            string protocol = SelectedTextValue(publisherProtocolComboBox, "rtmp");
            string currentUrl = publisherPublishUrlTextBox.Text.Trim();
            bool matches = string.Equals(protocol, "whip", StringComparison.OrdinalIgnoreCase)
                ? IsWhipPublisherTarget()
                : currentUrl.StartsWith(
                    protocol + "://",
                    StringComparison.OrdinalIgnoreCase);
            if (!matches)
            {
                if (string.Equals(protocol, "whip", StringComparison.OrdinalIgnoreCase))
                {
                    publisherPublishUrlTextBox.Text = "https://localhost:8443/whip";
                }
                else if (string.Equals(protocol, "srt", StringComparison.OrdinalIgnoreCase))
                {
                    publisherPublishUrlTextBox.Text = "srt://127.0.0.1:9000?mode=caller";
                }
                else if (string.Equals(protocol, "rtsp", StringComparison.OrdinalIgnoreCase))
                {
                    publisherPublishUrlTextBox.Text = "rtsp://127.0.0.1:8554/live/demo";
                }
                else
                {
                    publisherPublishUrlTextBox.Text = LocalPublishUrl;
                }
            }
            if (string.Equals(protocol, "whip", StringComparison.OrdinalIgnoreCase))
            {
                SelectComboByValue(publisherVideoCodecComboBox, "h264");
                SelectComboByValue(publisherAudioCodecComboBox, "opus");
            }
            UpdatePublisherUi();
        }

        private void StartPublisher()
        {
            try
            {
                StopPublisher();

                PublisherSourceMode sourceMode = SelectedValue(publisherSourceComboBox, PublisherSourceMode.Camera);
                PublisherAudioMode audioMode = SelectedValue(publisherAudioComboBox, PublisherAudioMode.None);
                if (sourceMode == PublisherSourceMode.None && audioMode == PublisherAudioMode.None)
                {
                    LogPublisher(T(
                        "Publisher needs a video source, an audio source, or both.",
                        "推流至少需要一个视频来源、一个音频来源，或两者同时存在。"));
                    return;
                }

                publisherSession = new StreamCorePublisherSession();
                StreamCorePublisherConfig config = BuildPublisherConfig(sourceMode, audioMode);
                StreamCoreResult configResult = publisherSession.SetConfig(config);
                StreamCorePublisherPreflightResult preflight = publisherSession.Preflight();
                StreamCoreCallResult start = publisherSession.Start();

                LogPublisher(
                    "set_config=" + FormatResult(configResult)
                    + " preflight=" + FormatResult(preflight.Call.Result) + "/" + SafePreflight(preflight.Preflight)
                    + " preflight_error=" + preflight.Call.ErrorText
                    + " start=" + FormatResult(start.Result) + " " + start.ErrorText);

                if (start.Result == StreamCoreResult.Ok)
                {
                    if (UsesPublisherMediaCapture(sourceMode, audioMode))
                    {
                        if (!StartPublisherMediaInput(sourceMode))
                        {
                            StopPublisher();
                        }
                    }
                    else
                    {
                        StartPublisherPreviewIfNeeded(sourceMode);
                    }
                }
                else
                {
                    StopPublisher();
                }

                RefreshRuntimeViews();
                UpdatePublisherUi();
            }
            catch (Exception ex)
            {
                LogPublisher(ex.Message);
                StopPublisher();
                UpdatePublisherUi();
            }
        }

        private void TogglePublisher()
        {
            if (publisherSession == null)
            {
                StartPublisher();
            }
            else
            {
                StopPublisher();
            }
        }

        private StreamCorePublisherConfig BuildPublisherConfig(
            PublisherSourceMode sourceMode,
            PublisherAudioMode audioMode)
        {
            StreamCorePublisherConfig config = StreamCorePublisherConfig.CreateDefault();
            ResolutionPreset resolution = SelectedResolution(publisherResolutionComboBox, 1280, 720);
            bool enableVideo = sourceMode != PublisherSourceMode.None;
            bool enableAudio = audioMode != PublisherAudioMode.None;
            int audioSampleRate = SelectedValue(publisherAudioSampleRateComboBox, 48000);
            int audioBitrate = SelectedValue(publisherAudioBitrateComboBox, 128);
            string audioCodecName = SelectedTextValue(
                publisherAudioProfileComboBox,
                SelectedTextValue(publisherAudioCodecComboBox, "aac"));
            if (string.Equals(
                    Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_AUTORUN"),
                    "publisher",
                    StringComparison.OrdinalIgnoreCase))
            {
                string scriptedAudioCodec =
                    Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_PUBLISHER_AUDIO_CODEC");
                if (!string.IsNullOrWhiteSpace(scriptedAudioCodec))
                {
                    audioCodecName = scriptedAudioCodec.Trim().ToLowerInvariant();
                }
            }

            config.SessionName = "dotnet-winforms-publisher";
            config.PublishUrl = publisherPublishUrlTextBox.Text.Trim();
            config.WhipBearerToken = publisherWhipBearerTokenTextBox.Text.Trim();
            config.EnableVideo = enableVideo;
            config.EnableAudio = enableAudio;
            config.AllowReconnect = true;
            config.RtmpHevcMode = SelectedValue(
                publisherRtmpHevcModeComboBox,
                StreamCorePublisherRtmpHevcMode.AutoCompatibility);
            config.SourceMediaProfile.ContainerName = "raw";
            config.SourceMediaProfile.HasVideo = enableVideo;
            config.SourceMediaProfile.HasAudio = enableAudio;
            config.SourceMediaProfile.VideoCodecName = enableVideo ? "nv12" : string.Empty;
            config.SourceMediaProfile.AudioCodecName = enableAudio ? "pcm" : string.Empty;
            config.SourceMediaProfile.Width = enableVideo ? resolution.Width : 0;
            config.SourceMediaProfile.Height = enableVideo ? resolution.Height : 0;
            config.SourceMediaProfile.Fps = enableVideo ? (int)publisherFpsBox.Value : 0;
            config.SourceMediaProfile.SampleRate = enableAudio ? audioSampleRate : 0;
            config.SourceMediaProfile.ChannelCount = enableAudio ? 2 : 0;
            config.Transcode.AudioMode = StreamCorePublisherTranscodeMode.ForceTranscode;
            config.Transcode.VideoMode = StreamCorePublisherTranscodeMode.ForceTranscode;
            config.Transcode.AudioCodecName = audioCodecName;
            config.Transcode.VideoCodecName = SelectedTextValue(publisherVideoCodecComboBox, "h264");
            config.Transcode.TargetAudioBitrateKbps = enableAudio ? audioBitrate : 0;
            config.Transcode.TargetVideoBitrateKbps = (int)publisherVideoBitrateBox.Value;
            config.Transcode.TargetSampleRate = enableAudio ? audioSampleRate : 0;
            config.Transcode.TargetChannelCount = enableAudio ? 2 : 0;
            config.Transcode.TargetWidth = enableVideo ? resolution.Width : 0;
            config.Transcode.TargetHeight = enableVideo ? resolution.Height : 0;
            config.Transcode.TargetFps = enableVideo ? (int)publisherFpsBox.Value : 0;
            config.Transcode.TargetGopFrames = enableVideo ? (int)publisherGopBox.Value : 0;

            if (sourceMode == PublisherSourceMode.Camera
                || sourceMode == PublisherSourceMode.Desktop
                || (sourceMode == PublisherSourceMode.None
                    && (audioMode == PublisherAudioMode.Microphone || audioMode == PublisherAudioMode.SystemAudio)))
            {
                config.InputKind = StreamCorePublisherInputKind.AppRawFeed;
                config.InputBindingId = "capture:" + BuildPublisherBinding(sourceMode, audioMode);
                return config;
            }

            if (sourceMode == PublisherSourceMode.VideoFile)
            {
                config.InputKind = StreamCorePublisherInputKind.AppRawFeed;
                config.InputBindingId = "capture:media-file";
                config.EnableAudio = true;
                config.EnableVideo = true;
                config.SourceMediaProfile.HasAudio = true;
                config.SourceMediaProfile.AudioCodecName = "pcm";
                config.SourceMediaProfile.SampleRate = audioSampleRate;
                config.SourceMediaProfile.ChannelCount = 2;
                config.Transcode.AudioMode = SelectedValue(
                    publisherFileModeComboBox,
                    StreamCorePublisherTranscodeMode.Auto);
                config.Transcode.VideoMode = SelectedValue(
                    publisherFileModeComboBox,
                    StreamCorePublisherTranscodeMode.Auto);
                return config;
            }

            if (sourceMode == PublisherSourceMode.StillImage)
            {
                config.InputKind = StreamCorePublisherInputKind.AppRawFeed;
                config.InputBindingId = "capture:still-image";
                config.EnableAudio = false;
                config.EnableVideo = true;
                return config;
            }

            config.InputKind = StreamCorePublisherInputKind.AppRawFeed;
            config.InputBindingId = "capture:audio-file";
            config.EnableAudio = true;
            config.EnableVideo = false;
            return config;
        }

        private static bool UsesPublisherMediaCapture(
            PublisherSourceMode sourceMode,
            PublisherAudioMode audioMode)
        {
            return sourceMode != PublisherSourceMode.None
                || audioMode != PublisherAudioMode.None;
        }

        /// <summary>
        /// Opens a file-backed capture session and routes its decoded raw frames into
        /// the already-started publisher. Direct file input is intentionally not part
        /// of the publisher ABI; capture owns file decoding and optional local preview.
        /// </summary>
        private bool StartPublisherMediaInput(PublisherSourceMode sourceMode)
        {
            if (publisherSession == null)
            {
                return false;
            }

            StopPublisherMediaInputSession();
            publisherMediaInputSession = new StreamCoreCaptureSession();

            StreamCoreCaptureConfig config = StreamCoreCaptureConfig.CreateDefault();
            ResolutionPreset resolution = SelectedResolution(publisherResolutionComboBox, 1280, 720);
            bool isAudioOnly = sourceMode == PublisherSourceMode.None;
            PublisherAudioMode audioMode = SelectedValue(
                publisherAudioComboBox,
                PublisherAudioMode.None);
            if (audioMode == PublisherAudioMode.AudioFile && !isAudioOnly)
            {
                LogPublisher(T(
                    "A separate audio file cannot be mixed into this video source by one Capture session.",
                    "单个 Capture 会话暂不把独立音频文件混入当前视频来源。"));
                StopPublisherMediaInputSession();
                return false;
            }
            StreamCoreCaptureSourceInfo videoSource =
                SelectedCaptureSource(publisherVideoSourceComboBox);
            StreamCoreCaptureSourceInfo audioSource =
                SelectedCaptureSource(publisherAudioSourceComboBox);

            config.SessionName = "dotnet-winforms-publisher-media-input";
            if (sourceMode == PublisherSourceMode.Camera)
            {
                config.SourceKind = StreamCoreCaptureSourceKind.Camera;
                config.SourceId = videoSource == null ? string.Empty : videoSource.SourceId;
                config.DisplayId = videoSource == null ? string.Empty : videoSource.DisplayId;
            }
            else if (sourceMode == PublisherSourceMode.Desktop)
            {
                config.SourceKind = StreamCoreCaptureSourceKind.Desktop;
                config.SourceId = videoSource == null ? string.Empty : videoSource.SourceId;
                config.DisplayId = videoSource == null ? string.Empty : videoSource.DisplayId;
            }
            else
            {
                config.SourceKind = StreamCoreCaptureSourceKind.MediaFile;
                config.SourceId = isAudioOnly
                    ? publisherAudioFileTextBox.Text.Trim()
                    : publisherMediaFileTextBox.Text.Trim();
                config.DisplayId = string.Empty;
            }
            config.EnableAudio = sourceMode == PublisherSourceMode.VideoFile
                || audioMode != PublisherAudioMode.None;
            config.EnableVideo = !isAudioOnly;
            if (audioMode == PublisherAudioMode.Microphone)
            {
                config.AudioSourceKind = StreamCoreCaptureSourceKind.Microphone;
                config.AudioSourceId =
                    audioSource == null ? string.Empty : audioSource.SourceId;
            }
            else if (audioMode == PublisherAudioMode.SystemAudio)
            {
                config.AudioSourceKind = StreamCoreCaptureSourceKind.SystemAudio;
                config.AudioSourceId =
                    audioSource == null ? string.Empty : audioSource.SourceId;
            }
            config.AudioVolumePercent = publisherAudioVolumeTrackBar.Value;
            config.PreferredWidth = config.EnableVideo ? resolution.Width : 0;
            config.PreferredHeight = config.EnableVideo ? resolution.Height : 0;
            config.PreferredFps = config.EnableVideo ? (int)publisherFpsBox.Value : 0;
            config.SampleRate = config.EnableAudio
                ? SelectedValue(publisherAudioSampleRateComboBox, 48000)
                : 0;
            config.ChannelCount = config.EnableAudio ? 2 : 0;
            config.PreviewMode = sourceMode == PublisherSourceMode.VideoFile
                ? StreamCoreCapturePreviewMode.Auto
                : StreamCoreCapturePreviewMode.Disabled;
            config.PreferredBackend = StreamCoreCaptureBackend.Auto;

            StreamCoreResult setConfig = publisherMediaInputSession.SetConfig(config);
            StreamCoreResult connectPublisher =
                publisherMediaInputSession.ConnectPublisher(publisherSession);
            StreamCoreResult setPreview = StreamCoreResult.Ok;
            if (config.EnableVideo && publisherPreviewCheckBox.Checked)
            {
                setPreview = publisherMediaInputSession.SetWindowsPreviewRenderTarget(
                    publisherPreviewPanel.Handle,
                    publisherPreviewPanel.Width,
                    publisherPreviewPanel.Height);
            }
            StreamCoreCapturePreflightResult preflight = publisherMediaInputSession.Preflight();
            StreamCoreCallResult start = publisherMediaInputSession.Start();

            LogPublisher(
                "media_input set_config=" + setConfig
                + " connect_publisher=" + connectPublisher
                + " preview_target=" + setPreview
                + " preflight=" + preflight.Call.Result + "/" + SafePreflight(preflight.Preflight)
                + " start=" + start.Result + " " + start.ErrorText);

            if (setConfig == StreamCoreResult.Ok
                && connectPublisher == StreamCoreResult.Ok
                && preflight.Call.Result == StreamCoreResult.Ok
                && start.Result == StreamCoreResult.Ok)
            {
                return true;
            }

            StopPublisherMediaInputSession();
            return false;
        }

        private string BuildPublisherBinding(PublisherSourceMode sourceMode, PublisherAudioMode audioMode)
        {
            List<string> tokens = new List<string>();
            StreamCoreCaptureSourceInfo videoSource = SelectedCaptureSource(publisherVideoSourceComboBox);
            StreamCoreCaptureSourceInfo audioSource = SelectedCaptureSource(publisherAudioSourceComboBox);

            if (sourceMode == PublisherSourceMode.Camera)
            {
                string sourceId = videoSource == null ? string.Empty : videoSource.SourceId;
                tokens.Add("camera:" + sourceId);
            }
            else if (sourceMode == PublisherSourceMode.Desktop)
            {
                string displayId = videoSource != null && !string.IsNullOrWhiteSpace(videoSource.DisplayId)
                    ? videoSource.DisplayId
                    : string.Empty;
                tokens.Add(string.IsNullOrWhiteSpace(displayId) ? "desktop" : "desktop:" + displayId);
            }

            if (audioMode == PublisherAudioMode.Microphone)
            {
                tokens.Add(audioSource == null || string.IsNullOrWhiteSpace(audioSource.SourceId)
                    ? "microphone"
                    : "microphone:" + audioSource.SourceId);
            }
            else if (audioMode == PublisherAudioMode.SystemAudio)
            {
                tokens.Add(audioSource == null || string.IsNullOrWhiteSpace(audioSource.SourceId)
                    ? "system_audio"
                    : "system_audio:" + audioSource.SourceId);
            }

            if (tokens.Count == 0)
            {
                return string.Empty;
            }

            return string.Join("+", tokens);
        }

        private bool PublisherPreviewAvailableForSelection(PublisherSourceMode sourceMode)
        {
            if (sourceMode == PublisherSourceMode.Camera ||
                sourceMode == PublisherSourceMode.Desktop)
            {
                return true;
            }

            return sourceMode == PublisherSourceMode.VideoFile &&
                SelectedValue(
                    publisherFileModeComboBox,
                    StreamCorePublisherTranscodeMode.Auto) ==
                    StreamCorePublisherTranscodeMode.ForceTranscode;
        }

        private void StopPublisherPreviewSession()
        {
            if (publisherPreviewSession != null)
            {
                publisherPreviewSession.Dispose();
                publisherPreviewSession = null;
            }
        }

        private void StopPublisherMediaInputSession()
        {
            if (publisherMediaInputSession != null)
            {
                publisherMediaInputSession.Dispose();
                publisherMediaInputSession = null;
            }
        }

        private void ApplyPublisherPreviewToggle()
        {
            PublisherSourceMode sourceMode = SelectedValue(
                publisherSourceComboBox,
                PublisherSourceMode.Camera);
            if (publisherMediaInputSession != null)
            {
                if (publisherPreviewCheckBox.Checked)
                {
                    publisherMediaInputSession.SetWindowsPreviewRenderTarget(
                        publisherPreviewPanel.Handle,
                        publisherPreviewPanel.Width,
                        publisherPreviewPanel.Height);
                }
                else
                {
                    publisherMediaInputSession.ClearPreviewRenderTarget();
                }
                UpdatePublisherUi();
                return;
            }

            if (!PublisherPreviewAvailableForSelection(sourceMode))
            {
                StopPublisherPreviewSession();
                UpdatePublisherUi();
                return;
            }

            if (!publisherPreviewCheckBox.Checked)
            {
                StopPublisherPreviewSession();
                UpdatePublisherUi();
                return;
            }

            if (publisherSession == null)
            {
                UpdatePublisherUi();
                return;
            }

            if (publisherPreviewSession == null)
            {
                StartPublisherPreviewIfNeeded(sourceMode);
            }
            else
            {
                publisherPreviewSession.SetWindowsPreviewRenderTarget(
                    publisherPreviewPanel.Handle,
                    publisherPreviewPanel.Width,
                    publisherPreviewPanel.Height);
            }
            UpdatePublisherUi();
        }

        private void StartPublisherPreviewIfNeeded(PublisherSourceMode sourceMode)
        {
            if (!publisherPreviewCheckBox.Checked ||
                !PublisherPreviewAvailableForSelection(sourceMode))
            {
                UpdatePublisherUi();
                return;
            }

            if (sourceMode == PublisherSourceMode.VideoFile)
            {
                if (publisherMediaInputSession != null)
                {
                    publisherMediaInputSession.SetWindowsPreviewRenderTarget(
                        publisherPreviewPanel.Handle,
                        publisherPreviewPanel.Width,
                        publisherPreviewPanel.Height);
                }
                UpdatePublisherUi();
                return;
            }

            try
            {
                StreamCoreCaptureSourceInfo previewSource = SelectedCaptureSource(publisherVideoSourceComboBox);
                if ((sourceMode == PublisherSourceMode.Camera ||
                    sourceMode == PublisherSourceMode.Desktop) &&
                    previewSource == null)
                {
                    return;
                }

                StopPublisherPreviewSession();
                publisherPreviewSession = new StreamCoreCaptureSession();
                StreamCoreCaptureConfig config = StreamCoreCaptureConfig.CreateDefault();
                ResolutionPreset resolution = SelectedResolution(publisherResolutionComboBox, 1280, 720);

                config.SessionName = "dotnet-winforms-publisher-preview";
                if (sourceMode == PublisherSourceMode.Camera)
                {
                    config.SourceKind = StreamCoreCaptureSourceKind.Camera;
                    config.SourceId = previewSource.SourceId;
                    config.DisplayId = previewSource.DisplayId;
                }
                else if (sourceMode == PublisherSourceMode.Desktop)
                {
                    config.SourceKind = StreamCoreCaptureSourceKind.Desktop;
                    config.SourceId = previewSource.SourceId;
                    config.DisplayId = previewSource.DisplayId;
                }
                else
                {
                    return;
                }
                config.EnableAudio = false;
                config.EnableVideo = true;
                config.AudioVolumePercent = publisherAudioVolumeTrackBar.Value;
                config.PreferredWidth = resolution.Width;
                config.PreferredHeight = resolution.Height;
                config.PreferredFps = (int)publisherFpsBox.Value;
                config.PreviewMode = StreamCoreCapturePreviewMode.Auto;
                config.PreferredBackend = StreamCoreCaptureBackend.Auto;

                StreamCoreResult setConfig = publisherPreviewSession.SetConfig(config);
                StreamCoreResult setPreview = publisherPreviewSession.SetWindowsPreviewRenderTarget(
                    publisherPreviewPanel.Handle,
                    publisherPreviewPanel.Width,
                    publisherPreviewPanel.Height);
                StreamCoreCapturePreflightResult preflight = publisherPreviewSession.Preflight();
                StreamCoreCallResult start = publisherPreviewSession.Start();

                LogPublisher(
                    "preview set_config=" + setConfig
                    + " preview_target=" + setPreview
                    + " preflight=" + preflight.Call.Result + "/" + SafePreflight(preflight.Preflight)
                    + " start=" + start.Result + " " + start.ErrorText);

                if (start.Result != StreamCoreResult.Ok)
                {
                    publisherPreviewSession.Dispose();
                    publisherPreviewSession = null;
                }
            }
            catch (Exception ex)
            {
                LogPublisher("preview " + ex.Message);
                if (publisherPreviewSession != null)
                {
                    publisherPreviewSession.Dispose();
                    publisherPreviewSession = null;
                }
            }
        }

        private void StopPublisher()
        {
            StopPublisherMediaInputSession();
            StopPublisherPreviewSession();

            if (publisherSession != null)
            {
                publisherSession.Dispose();
                publisherSession = null;
            }

            RefreshRuntimeViews();
            UpdatePublisherUi();
        }

        private void RunPlayerPreflight()
        {
            try
            {
                using (StreamCorePlayerSession session = new StreamCorePlayerSession())
                {
                    StreamCorePlayerConfig config = BuildPlayerConfig();
                    StreamCoreResult setConfig = session.SetConfig(config);
                    StreamCoreResult setRender = session.SetWindowsRenderTarget(
                        playerRenderPanel.Handle,
                        playerRenderPanel.Width,
                        playerRenderPanel.Height);
                    StreamCorePlayerPreflightResult preflight = session.Preflight();
                    LogPlayer(
                        "set_config=" + setConfig
                        + " render_target=" + setRender
                        + " preflight=" + preflight.Call.Result + "/" + SafePreflight(preflight.Preflight)
                        + " " + preflight.Call.ErrorText);
                }
            }
            catch (Exception ex)
            {
                LogPlayer(ex.Message);
            }
        }

        private void StartPlayer()
        {
            try
            {
                StopPlayer();
                playerSession = new StreamCorePlayerSession();
                StreamCorePlayerConfig config = BuildPlayerConfig();
                StreamCoreResult setConfig = playerSession.SetConfig(config);
                StreamCoreResult setRender = playerSession.SetWindowsRenderTarget(
                    playerRenderPanel.Handle,
                    playerRenderPanel.Width,
                    playerRenderPanel.Height);
                StreamCoreResult setVolume = playerSession.SetAudioVolume(playerAudioVolumeTrackBar.Value);
                StreamCorePlayerPreflightResult preflight = playerSession.Preflight();
                StreamCoreCallResult start = playerSession.Start();

                LogPlayer(
                    "set_config=" + setConfig
                    + " render_target=" + setRender
                    + " audio_volume=" + setVolume
                    + " preflight=" + preflight.Call.Result + "/" + SafePreflight(preflight.Preflight)
                    + " start=" + start.Result + " " + start.ErrorText);

                if (start.Result != StreamCoreResult.Ok)
                {
                    StopPlayer();
                }

                RefreshRuntimeViews();
                UpdatePlayerOverlay();
            }
            catch (Exception ex)
            {
                LogPlayer(ex.Message);
                StopPlayer();
            }
        }

        private void TogglePlayer()
        {
            if (playerSession == null)
            {
                StartPlayer();
            }
            else
            {
                StopPlayer();
            }
        }

        private StreamCorePlayerConfig BuildPlayerConfig()
        {
            StreamCorePlayerConfig config = StreamCorePlayerConfig.Url(playerUrlTextBox.Text.Trim());
            int bufferMs = (int)playerBufferMsBox.Value;
            config.SessionName = "dotnet-winforms-player";
            config.RenderTargetId = "winforms-player-panel";
            config.UseHardwareDecode = SelectedValue(playerDecodeModeComboBox, false);
            config.VideoPresentPath = SelectedValue(
                playerRenderPathComboBox,
                StreamCorePlayerVideoPresentPath.SoftwareFrame);
            config.PreferSoftwareRenderBackend =
                config.VideoPresentPath == StreamCorePlayerVideoPresentPath.SoftwareFrame;
            config.AudioCacheMilliseconds = bufferMs;
            config.VideoCacheMilliseconds = bufferMs;
            config.VideoNoCache = bufferMs <= 0;
            config.AudioNoCache = bufferMs <= 0;
            config.VideoMaxQueueSize = (int)playerVideoQueueBox.Value;
            config.AudioMaxQueueSize = (int)playerAudioQueueBox.Value;
            config.EnableRealtimeProfile = false;
            config.RealtimeAudioMaxQueueSize = (int)playerAudioQueueBox.Value;
            config.RealtimeVideoMaxQueueSize = (int)playerVideoQueueBox.Value;
            config.AudioVolumePercent = playerAudioVolumeTrackBar.Value;
            return config;
        }

        private void StopPlayer()
        {
            if (playerSession != null)
            {
                playerSession.Dispose();
                playerSession = null;
            }

            RefreshRuntimeViews();
            UpdatePlayerOverlay();
        }

        private void StartGbRuntimeAndRegister()
        {
            StartGbRuntime();
            if (gbSession != null)
            {
                RegisterGb();
            }
        }

        private void ToggleGb()
        {
            if (gbSession == null)
            {
                StartGbRuntimeAndRegister();
            }
            else
            {
                StopGb();
            }
        }

        private void StartGbRuntime()
        {
            try
            {
                StopGb();
                gbSession = new StreamCoreGb28181Session();
                HookGbCallbacks(gbSession);

                StreamCoreGb28181Config config = BuildGbConfig();
                StreamCoreResult setConfig = gbSession.SetConfig(config);
                StreamCoreResult deviceInfo = gbSession.SetDeviceInfo(new StreamCoreGb28181DeviceInfo
                {
                    DeviceName = "StreamCore .NET Demo",
                    Manufacturer = "HBR",
                    Model = "WinForms",
                    Firmware = "1.0.0"
                });
                StreamCoreResult deviceStatus = gbSession.SetDeviceStatus(new StreamCoreGb28181DeviceStatus
                {
                    StatusText = "OK",
                    IsOnline = true,
                    IsRecording = false
                });
                StreamCoreResult catalog = gbSession.SetCatalogItems(new[]
                {
                    new StreamCoreGb28181CatalogItem
                    {
                        ChannelId = CurrentGbDeviceChannelId(),
                        ParentId = CurrentGbDeviceChannelId(),
                        Name = "StreamCore SDK Demo Channel",
                        Manufacturer = "HBR",
                        Model = "WinForms",
                        Owner = "streamcore_demo",
                        CivilCode = gbDomainTextBox.Text.Trim(),
                        Address = "Windows Host",
                        Parental = false,
                        IsOnline = true,
                        StatusText = "ON"
                    }
                });
                StreamCoreCallResult bindings = gbSession.SetSourceBindings(new[] { BuildGbSourceBinding() });
                StreamCoreCallResult start = gbSession.Start();

                LogGb(
                    "set_config=" + setConfig
                    + " device_info=" + deviceInfo
                    + " device_status=" + deviceStatus
                    + " catalog=" + catalog
                    + " bindings=" + bindings.Result + " " + bindings.ErrorText
                    + " start=" + start.Result + " " + start.ErrorText);

                if (start.Result == StreamCoreResult.Ok)
                {
                    gbPollTimer.Start();
                }
                else
                {
                    StopGb();
                }

                RefreshRuntimeViews();
                UpdateGbUi();
            }
            catch (Exception ex)
            {
                LogGb(ex.Message);
                StopGb();
                UpdateGbUi();
            }
        }

        private StreamCoreGb28181Config BuildGbConfig()
        {
            StreamCoreGb28181Config config = StreamCoreGb28181Config.CreateDefault();
            string deviceChannelId = CurrentGbDeviceChannelId();
            string sipPassword = gbUpperPasswordTextBox.Text.Trim();
            config.SessionName = "dotnet-winforms-gb28181";
            config.LocalIdentity.Id = deviceChannelId;
            config.LocalIdentity.Domain = gbDomainTextBox.Text.Trim();
            config.LocalIdentity.Password = sipPassword;
            config.LocalIdentity.DisplayName = gbLocalDisplayNameTextBox.Text.Trim();
            config.UpperPlatformIdentity.Id = gbUpperIdTextBox.Text.Trim();
            config.UpperPlatformIdentity.Domain = gbUpperDomainTextBox.Text.Trim();
            config.UpperPlatformIdentity.Password = sipPassword;
            config.UpperPlatformIdentity.DisplayName = gbUpperDisplayNameTextBox.Text.Trim();
            config.LocalEndpoint.Ip = gbLocalIpTextBox.Text.Trim();
            config.LocalEndpoint.Port = (int)gbLocalPortBox.Value;
            config.LocalEndpoint.Transport = SelectedValue(
                gbTransportComboBox,
                StreamCoreGb28181TransportMode.Tcp);
            config.UpperPlatformEndpoint.Ip = gbUpperIpTextBox.Text.Trim();
            config.UpperPlatformEndpoint.Port = (int)gbUpperPortBox.Value;
            config.UpperPlatformEndpoint.Transport = SelectedValue(
                gbTransportComboBox,
                StreamCoreGb28181TransportMode.Tcp);
            config.RegisterExpiresSeconds = (int)gbRegisterExpiresBox.Value;
            config.KeepaliveIntervalSeconds = (int)gbKeepaliveIntervalBox.Value;
            config.EnableDigestAuth = gbEnableDigestAuthCheckBox.Checked;
            config.DefaultAnswer.MediaEndpoint.Ip = gbMediaIpTextBox.Text.Trim();
            config.DefaultAnswer.MediaEndpoint.Port = (int)gbMediaPortBox.Value;
            config.DefaultAnswer.MediaEndpoint.Transport = SelectedValue(
                gbTransportComboBox,
                StreamCoreGb28181TransportMode.Tcp);
            return config;
        }

        private StreamCoreGb28181SourceBinding BuildGbSourceBinding()
        {
            PublisherSourceMode sourceMode = SelectedValue(gbSourceComboBox, PublisherSourceMode.Camera);
            GbAudioMode audioMode = SelectedValue(gbAudioComboBox, GbAudioMode.Microphone);
            ResolutionPreset resolution = SelectedResolution(gbResolutionComboBox, 1280, 720);
            StreamCoreCaptureSourceInfo videoSource = SelectedCaptureSource(gbVideoSourceComboBox);
            StreamCoreCaptureSourceInfo audioSource = SelectedCaptureSource(gbAudioSourceComboBox);

            StreamCoreGb28181SourceBinding binding = new StreamCoreGb28181SourceBinding
            {
                Target = new StreamCoreGb28181StreamTarget
                {
                    DeviceId = CurrentGbDeviceChannelId(),
                    ChannelId = CurrentGbDeviceChannelId(),
                    StreamKind = StreamCoreGb28181StreamKind.Live
                },
                Width = resolution.Width,
                Height = resolution.Height,
                Fps = (int)gbFpsBox.Value,
                AudioSampleRate = 48000,
                AudioChannelCount = 2,
                FirstUseTcp = SelectedValue(
                    gbTransportComboBox,
                    StreamCoreGb28181TransportMode.Tcp) == StreamCoreGb28181TransportMode.Tcp,
                AllowRawFramePacket = true,
                AutoReconnect = true,
                RepeatFile = false
            };

            binding.SourceKind = StreamCoreGb28181MediaSourceKind.LocalDevice;
            binding.EnableVideo = true;
            binding.EnableAudio = audioMode != GbAudioMode.None;
            binding.CaptureSourceKind = sourceMode == PublisherSourceMode.Camera
                ? StreamCoreCaptureSourceKind.Camera
                : StreamCoreCaptureSourceKind.Desktop;
            binding.SourceId = videoSource == null ? string.Empty : videoSource.SourceId;
            if (audioMode == GbAudioMode.Microphone)
            {
                binding.AudioCaptureSourceKind = StreamCoreCaptureSourceKind.Microphone;
                binding.AudioSourceId = audioSource == null ? string.Empty : audioSource.SourceId;
            }
            else if (audioMode == GbAudioMode.SystemAudio)
            {
                binding.AudioCaptureSourceKind = StreamCoreCaptureSourceKind.SystemAudio;
                binding.AudioSourceId = audioSource == null ? string.Empty : audioSource.SourceId;
            }
            else if (audioMode == GbAudioMode.None)
            {
                binding.EnableAudio = false;
            }
            return binding;
        }

        private string CurrentGbDeviceChannelId()
        {
            string deviceChannelId = gbLocalDeviceIdTextBox.Text.Trim();
            return string.IsNullOrWhiteSpace(deviceChannelId)
                ? "34020000001320000001"
                : deviceChannelId;
        }

        private void HookGbCallbacks(StreamCoreGb28181Session session)
        {
            session.RegisterResultReceived += result => PostUi(() =>
                LogGb("register_result=" + result.Success + " " + result.Reason));
            session.InviteReceived += invite => PostUi(() =>
            {
                LogGb("invite stream=" + invite.Target.StreamKind + " codec=" + invite.VideoCodecName + "/" + invite.AudioCodecName);
                if (gbAutoAcceptInviteCheckBox.Checked && gbSession != null)
                {
                    StreamCoreCallResult reply = gbSession.ReplyInviteAccepted(invite);
                    LogGb("reply_invite=" + reply.Result + " " + reply.ErrorText);
                }
            });
            session.SessionUpdated += info => PostUi(() =>
                LogGb("session_updated=" + info.State + " " + info.SessionKey + " " + info.RoutePath));
            session.SessionClosed += info => PostUi(() =>
                LogGb("session_closed=" + info.State + " " + info.SessionKey));
            session.CommandReceived += info => PostUi(() =>
                LogGb("command=" + info.Command + " " + info.EventName + " " + info.Payload));
            session.MediaRequestReceived += info => PostUi(() =>
                LogGb(
                    "media_request=" + info.StreamKind
                    + " " + info.MediaDirectionText
                    + " " + info.VideoCodecName + "/" + info.AudioCodecName
                    + " endpoint=" + info.MediaEndpoint.Ip + ":" + info.MediaEndpoint.Port + "/" + info.MediaEndpoint.Transport
                    + " pt=" + info.VideoPayloadType + "/" + info.AudioPayloadType));
            session.MediaPacketReceived += info => PostUi(() =>
                LogGb("media_packet=" + info.Packet.MediaType + " bytes=" + info.Packet.Data.Length + " key=" + info.Packet.IsKeyFrame));
        }

        private void RegisterGb()
        {
            if (gbSession == null)
            {
                LogGb(T("Start the GB28181 runtime first.", "请先启动 GB28181 运行时。"));
                return;
            }

            StreamCoreCallResult call = gbSession.RegisterToUpperPlatform();
            LogGb("register=" + call.Result + " " + call.ErrorText);
            RefreshRuntimeViews();
        }

        private void SendGbKeepalive()
        {
            if (gbSession == null)
            {
                LogGb(T("Start the GB28181 runtime first.", "请先启动 GB28181 运行时。"));
                return;
            }

            StreamCoreCallResult call = gbSession.SendKeepalive();
            LogGb("keepalive=" + call.Result + " " + call.ErrorText);
        }

        private void UnregisterGb()
        {
            if (gbSession == null)
            {
                LogGb(T("Start the GB28181 runtime first.", "请先启动 GB28181 运行时。"));
                return;
            }

            StreamCoreCallResult call = gbSession.UnregisterFromUpperPlatform();
            LogGb("unregister=" + call.Result + " " + call.ErrorText);
            RefreshRuntimeViews();
        }

        private void StopGb()
        {
            gbPollTimer.Stop();
            if (gbSession != null)
            {
                gbSession.Dispose();
                gbSession = null;
            }

            RefreshRuntimeViews();
            UpdateGbUi();
        }

        private void PollGbRuntime()
        {
            if (gbSession == null)
            {
                return;
            }

            try
            {
                StreamCoreResult result = gbSession.Poll();
                if (result != StreamCoreResult.Ok)
                {
                    gbStateLabel.Text = "poll=" + result;
                }
                RefreshGbRuntimeView();
            }
            catch (Exception ex)
            {
                LogGb("poll " + ex.Message);
            }
        }

        private void RefreshCaptureSources()
        {
            cameraSources = LoadCaptureSources(StreamCoreCaptureSourceKind.Camera, "camera");
            desktopSources = LoadCaptureSources(StreamCoreCaptureSourceKind.Desktop, "desktop");
            microphoneSources = LoadCaptureSources(StreamCoreCaptureSourceKind.Microphone, "microphone");
            systemAudioSources = LoadCaptureSources(StreamCoreCaptureSourceKind.SystemAudio, "system_audio");

            RefreshPublisherVideoSources();
            RefreshGbVideoSources();
            UpdatePublisherUi();
            UpdateGbUi();
        }

        private StreamCoreCaptureSourceInfo[] LoadCaptureSources(StreamCoreCaptureSourceKind kind, string role)
        {
            try
            {
                StreamCoreCaptureSourceListResult result = StreamCoreCaptureSession.CopySourceList(kind);
                if (result.Call.Result != StreamCoreResult.Ok)
                {
                    SetStatus(role + " source_list=" + result.Call.Result + " " + result.Call.ErrorText);
                    return new StreamCoreCaptureSourceInfo[0];
                }
                return result.Sources ?? new StreamCoreCaptureSourceInfo[0];
            }
            catch (Exception ex)
            {
                SetStatus(role + " source_list exception: " + ex.Message);
                return new StreamCoreCaptureSourceInfo[0];
            }
        }

        private void RefreshPublisherVideoSources()
        {
            object previousVideo = SelectedObjectValue(publisherVideoSourceComboBox);
            object previousAudio = SelectedObjectValue(publisherAudioSourceComboBox);
            PublisherSourceMode sourceMode = SelectedValue(publisherSourceComboBox, PublisherSourceMode.Camera);
            PublisherAudioMode audioMode = SelectedValue(publisherAudioComboBox, PublisherAudioMode.None);

            PopulateCaptureSourceCombo(
                publisherVideoSourceComboBox,
                sourceMode == PublisherSourceMode.Desktop ? desktopSources : cameraSources,
                previousVideo);

            if (audioMode == PublisherAudioMode.SystemAudio)
            {
                PopulateCaptureSourceCombo(publisherAudioSourceComboBox, systemAudioSources, previousAudio);
            }
            else
            {
                PopulateCaptureSourceCombo(publisherAudioSourceComboBox, microphoneSources, previousAudio);
            }
        }

        private void RefreshGbVideoSources()
        {
            object previousVideo = SelectedObjectValue(gbVideoSourceComboBox);
            object previousAudio = SelectedObjectValue(gbAudioSourceComboBox);
            PublisherSourceMode sourceMode = SelectedValue(gbSourceComboBox, PublisherSourceMode.Camera);
            GbAudioMode audioMode = SelectedValue(gbAudioComboBox, GbAudioMode.Microphone);

            PopulateCaptureSourceCombo(
                gbVideoSourceComboBox,
                sourceMode == PublisherSourceMode.Desktop ? desktopSources : cameraSources,
                previousVideo);

            if (audioMode == GbAudioMode.SystemAudio)
            {
                PopulateCaptureSourceCombo(gbAudioSourceComboBox, systemAudioSources, previousAudio);
            }
            else
            {
                PopulateCaptureSourceCombo(gbAudioSourceComboBox, microphoneSources, previousAudio);
            }
        }

        private void PopulateCaptureSourceCombo(
            ComboBox comboBox,
            IEnumerable<StreamCoreCaptureSourceInfo> sources,
            object preferredValue)
        {
            comboBox.BeginUpdate();
            comboBox.Items.Clear();

            foreach (StreamCoreCaptureSourceInfo source in sources ?? Enumerable.Empty<StreamCoreCaptureSourceInfo>())
            {
                comboBox.Items.Add(new ComboValue(DescribeCaptureSource(source), source));
            }

            comboBox.EndUpdate();

            if (comboBox.Items.Count == 0)
            {
                comboBox.Items.Add(new ComboValue(T("No source found", "未发现来源"), null));
                comboBox.SelectedIndex = 0;
                return;
            }

            SelectComboByValue(comboBox, preferredValue);
        }

        private void RefreshPublisherResolutionOptions()
        {
            PopulateResolutionCombo(
                publisherResolutionComboBox,
                SelectedCaptureSource(publisherVideoSourceComboBox));
        }

        private void RefreshGbResolutionOptions()
        {
            PopulateResolutionCombo(
                gbResolutionComboBox,
                SelectedCaptureSource(gbVideoSourceComboBox));
        }

        private void PopulateResolutionCombo(
            ComboBox comboBox,
            StreamCoreCaptureSourceInfo preferredSource)
        {
            string previous = SelectedResolutionKey(comboBox);
            List<ResolutionPreset> items = new List<ResolutionPreset>();

            if (preferredSource != null && preferredSource.DisplayWidth > 0 && preferredSource.DisplayHeight > 0)
            {
                items.Add(new ResolutionPreset(preferredSource.DisplayWidth, preferredSource.DisplayHeight));
            }

            items.Add(new ResolutionPreset(320, 180));
            items.Add(new ResolutionPreset(640, 360));
            items.Add(new ResolutionPreset(640, 480));
            items.Add(new ResolutionPreset(1280, 720));
            items.Add(new ResolutionPreset(1920, 1080));
            items.Add(new ResolutionPreset(2560, 1440));

            comboBox.BeginUpdate();
            comboBox.Items.Clear();
            foreach (ResolutionPreset preset in items.GroupBy(value => value.Key).Select(group => group.First()))
            {
                comboBox.Items.Add(new ComboValue(preset.Text, preset));
            }
            comboBox.EndUpdate();

            if (!string.IsNullOrWhiteSpace(previous))
            {
                SelectComboByValue(comboBox, previous, value => value is ResolutionPreset preset && preset.Key == previous);
            }
            if (comboBox.SelectedIndex < 0 && comboBox.Items.Count > 0)
            {
                SelectComboByValue(comboBox, "1280x720", value => value is ResolutionPreset preset && preset.Key == "1280x720");
                if (comboBox.SelectedIndex < 0)
                {
                    comboBox.SelectedIndex = 0;
                }
            }
        }

        private void UpdatePublisherUi()
        {
            PublisherSourceMode sourceMode = SelectedValue(publisherSourceComboBox, PublisherSourceMode.Camera);
            PublisherAudioMode audioMode = SelectedValue(publisherAudioComboBox, PublisherAudioMode.None);

            if ((sourceMode == PublisherSourceMode.VideoFile || sourceMode == PublisherSourceMode.StillImage)
                && audioMode != PublisherAudioMode.None)
            {
                SelectComboByValue(publisherAudioComboBox, PublisherAudioMode.None);
                audioMode = PublisherAudioMode.None;
            }
            if (sourceMode != PublisherSourceMode.None && audioMode == PublisherAudioMode.AudioFile)
            {
                SelectComboByValue(publisherAudioComboBox, PublisherAudioMode.None);
                audioMode = PublisherAudioMode.None;
            }

            bool usesVideoDevice = sourceMode == PublisherSourceMode.Camera || sourceMode == PublisherSourceMode.Desktop;
            bool usesMediaFile = sourceMode == PublisherSourceMode.VideoFile || sourceMode == PublisherSourceMode.StillImage;
            bool usesAudioFile = sourceMode == PublisherSourceMode.None && audioMode == PublisherAudioMode.AudioFile;
            bool usesAudioDevice = audioMode == PublisherAudioMode.Microphone || audioMode == PublisherAudioMode.SystemAudio;

            if (publisherVideoDeviceRow != null)
            {
                publisherVideoDeviceRow.Visible = usesVideoDevice;
            }
            if (publisherMediaFileRow != null)
            {
                publisherMediaFileRow.Visible = usesMediaFile;
            }
            if (publisherAudioFileRow != null)
            {
                publisherAudioFileRow.Visible = usesAudioFile;
            }
            if (publisherAudioDeviceRow != null)
            {
                publisherAudioDeviceRow.Visible = usesAudioDevice;
            }
            if (publisherFileModeRow != null)
            {
                publisherFileModeRow.Visible = sourceMode == PublisherSourceMode.VideoFile;
            }
            bool isWhip = IsWhipPublisherTarget();
            if (publisherWhipBearerTokenRow != null)
            {
                publisherWhipBearerTokenRow.Visible = isWhip;
            }
            if (publisherVideoDetailRow != null)
            {
                publisherVideoDetailRow.Visible = usesVideoDevice || usesMediaFile;
            }
            if (publisherAudioDetailRow != null)
            {
                publisherAudioDetailRow.Visible = usesAudioDevice || usesAudioFile;
            }
            publisherVideoSourceComboBox.Enabled = usesVideoDevice;
            publisherMediaFileTextBox.Enabled = usesMediaFile;
            publisherAudioFileTextBox.Enabled = usesAudioFile;
            publisherAudioSourceComboBox.Enabled = usesAudioDevice;
            publisherFileModeComboBox.Enabled = sourceMode == PublisherSourceMode.VideoFile;
            publisherVideoCodecComboBox.Enabled =
                !isWhip && sourceMode != PublisherSourceMode.VideoFile;
            publisherAudioCodecComboBox.Enabled =
                !isWhip && sourceMode != PublisherSourceMode.VideoFile;
            publisherAudioProfileComboBox.Enabled =
                !isWhip && audioMode != PublisherAudioMode.None;
            publisherAudioSampleRateComboBox.Enabled = audioMode != PublisherAudioMode.None;
            publisherAudioBitrateComboBox.Enabled = audioMode != PublisherAudioMode.None;
            publisherRtmpHevcModeComboBox.Enabled =
                !isWhip &&
                publisherPublishUrlTextBox.Text.Trim().StartsWith(
                    "rtmp://",
                    StringComparison.OrdinalIgnoreCase);
            publisherRunButton.Text = publisherSession == null ?
                T("Start publish", "启动推流") :
                T("Stop publish", "停止推流");

            if (publisherPreviewSession != null
                || (publisherMediaInputSession != null && publisherPreviewCheckBox.Checked))
            {
                ShowPreviewOverlay(publisherPreviewOverlayLabel, string.Empty, false);
            }
            else if (sourceMode == PublisherSourceMode.VideoFile)
            {
                ShowPreviewOverlay(
                    publisherPreviewOverlayLabel,
                    T("PASSTHROUGH READY\r\nNO LOCAL PREVIEW", "编码包透传就绪\r\n当前无本地预览"),
                    true);
            }
            else if (sourceMode == PublisherSourceMode.StillImage)
            {
                ShowPreviewOverlay(
                    publisherPreviewOverlayLabel,
                    T("STILL IMAGE READY\r\nNO LOCAL PREVIEW", "静态图片就绪\r\n当前无本地预览"),
                    true);
            }
            else if (sourceMode == PublisherSourceMode.None && audioMode != PublisherAudioMode.None)
            {
                ShowPreviewOverlay(
                    publisherPreviewOverlayLabel,
                    T("AUDIO ONLY\r\nNO VIDEO PREVIEW", "仅音频\r\n当前无视频预览"),
                    true);
            }
            else
            {
                ShowPreviewOverlay(
                    publisherPreviewOverlayLabel,
                    T("LOCAL PREVIEW\r\nSELECT OR START A SOURCE", "本地预览\r\n请选择或启动来源"),
                    true);
            }
        }

        private void UpdatePlayerOverlay()
        {
            bool running = playerSession != null;
            playerRunButton.Text = running ?
                T("Stop playback", "停止播放") :
                T("Start playback", "启动播放");
            ShowPreviewOverlay(
                playerOverlayLabel,
                T("PLAYER RENDER TARGET\r\nSTART PLAYBACK", "播放渲染目标\r\n请启动播放"),
                !running);
        }

        private void UpdatePlayerOnvifUi()
        {
            bool enabled = playerOnvifEnabledCheckBox.Checked;
            if (playerOnvifSearchRow != null)
            {
                playerOnvifSearchRow.Visible = enabled;
            }
            if (playerOnvifListRow != null)
            {
                playerOnvifListRow.Visible = enabled;
            }
            if (playerOnvifCredentialRow != null)
            {
                playerOnvifCredentialRow.Visible = enabled;
            }

            playerOnvifSearchButton.Enabled = enabled && !onvifSearchInProgress;
            playerOnvifListBox.Enabled = enabled && !onvifSearchInProgress;
            playerOnvifUsernameTextBox.Enabled = enabled;
            playerOnvifPasswordTextBox.Enabled = enabled;

            if (!enabled)
            {
                playerOnvifHintLabel.Text = T(
                    "Enable ONVIF to discover devices and resolve RTSP stream URIs.",
                    "启用 ONVIF 后即可搜索设备并解析 RTSP 流地址。");
            }
        }

        private void SearchOnvifDevices()
        {
            if (!playerOnvifEnabledCheckBox.Checked || onvifSearchInProgress)
            {
                return;
            }

            onvifSearchInProgress = true;
            playerOnvifHintLabel.Text = T("Searching ONVIF devices...", "正在搜索 ONVIF 设备...");
            UpdatePlayerOnvifUi();

            Task.Run(() =>
            {
                try
                {
                    StreamCoreOnvifDiscoveryConfig config = StreamCoreOnvif.CreateDefaultDiscoveryConfig();
                    config.MaximumDeviceCount = OnvifDiscoveryCapacity;
                    StreamCoreOnvifDiscoveryResult result = StreamCoreOnvif.DiscoverDevices(config);
                    PostUi(() =>
                    {
                        onvifSearchInProgress = false;
                        playerOnvifDevices.Clear();
                        playerOnvifListBox.Items.Clear();
                        if (result.Devices != null)
                        {
                            playerOnvifDevices.AddRange(result.Devices);
                        }

                        if (playerOnvifDevices.Count == 0)
                        {
                            playerOnvifListBox.Items.Add(T("No ONVIF devices discovered", "尚未搜索到 ONVIF 设备"));
                        }
                        else
                        {
                            foreach (StreamCoreOnvifDevice device in playerOnvifDevices)
                            {
                                playerOnvifListBox.Items.Add(OnvifDeviceLabel(device));
                            }
                            playerOnvifListBox.SelectedIndex = 0;
                        }

                        playerOnvifHintLabel.Text = result.Call.Result == StreamCoreResult.Ok ?
                            T(
                                "Double-click a device below to show its discovered ONVIF service URL.",
                                "双击下方设备即可查看已发现的 ONVIF 服务地址。") :
                            T(
                                "ONVIF search failed. See the runtime log for details.",
                                "ONVIF 搜索失败，详情见运行日志。");
                        LogPlayer(
                            "onvif_search=" + result.Call.Result
                            + " devices=" + playerOnvifDevices.Count
                            + " " + result.Call.ErrorText);
                        UpdatePlayerOnvifUi();
                    });
                }
                catch (Exception ex)
                {
                    PostUi(() =>
                    {
                        onvifSearchInProgress = false;
                        playerOnvifHintLabel.Text = T(
                            "ONVIF search failed. See the runtime log for details.",
                            "ONVIF 搜索失败，详情见运行日志。");
                        LogPlayer("onvif_search exception: " + ex.Message);
                        UpdatePlayerOnvifUi();
                    });
                }
            });
        }

        private void ApplySelectedOnvifDevice()
        {
            if (!playerOnvifEnabledCheckBox.Checked ||
                playerOnvifListBox.SelectedIndex < 0 ||
                playerOnvifListBox.SelectedIndex >= playerOnvifDevices.Count)
            {
                return;
            }

            StreamCoreOnvifDevice selected = playerOnvifDevices[playerOnvifListBox.SelectedIndex];
            if (!string.IsNullOrWhiteSpace(selected.ServiceUrl))
            {
                ApplyOnvifServiceToPlayerHint(selected);
                return;
            }

            playerOnvifHintLabel.Text = T(
                "The selected device did not report an ONVIF service URL. Enter a known RTSP URL manually.",
                "选中的设备没有返回 ONVIF 服务地址。请手动输入已知 RTSP 地址。");
            LogPlayer("onvif_apply=no_service_url");
            UpdatePlayerOnvifUi();
        }

        private void ApplyOnvifServiceToPlayerHint(StreamCoreOnvifDevice device)
        {
            playerOnvifHintLabel.Text = T(
                "Discovered ONVIF service URL: " + device.ServiceUrl + ". Enter the device RTSP URL after it is known.",
                "已发现 ONVIF 服务地址：" + device.ServiceUrl + "。获取设备 RTSP 地址后请手动填入播放地址。");
            LogPlayer("onvif_service=" + device.ServiceUrl);
            UpdatePlayerOnvifUi();
        }

        private string OnvifDeviceLabel(StreamCoreOnvifDevice device)
        {
            if (device == null)
            {
                return T("Unknown ONVIF device", "未知 ONVIF 设备");
            }

            string name = string.IsNullOrWhiteSpace(device.DeviceUrn) ?
                T("ONVIF device", "ONVIF 设备") :
                device.DeviceUrn;
            if (device.ProfileTAdvertised)
            {
                name += " | Profile T";
            }
            else if (device.ProfileSAdvertised)
            {
                name += " | Profile S";
            }
            string endpoint = !string.IsNullOrWhiteSpace(device.ResponseEndpoint) ?
                device.ResponseEndpoint :
                device.ServiceUrl;
            return string.IsNullOrWhiteSpace(endpoint) ? name : name + " | " + endpoint;
        }

        private void UpdateGbUi()
        {
            PublisherSourceMode sourceMode = SelectedValue(gbSourceComboBox, PublisherSourceMode.Camera);
            GbAudioMode audioMode = SelectedValue(gbAudioComboBox, GbAudioMode.Microphone);
            bool usesVideoDevice = sourceMode == PublisherSourceMode.Camera || sourceMode == PublisherSourceMode.Desktop;
            bool usesAudioDevice = audioMode == GbAudioMode.Microphone || audioMode == GbAudioMode.SystemAudio;
            if (gbVideoDeviceRow != null)
            {
                gbVideoDeviceRow.Visible = usesVideoDevice;
            }
            if (gbAudioDeviceRow != null)
            {
                gbAudioDeviceRow.Visible = usesAudioDevice;
            }
            gbVideoSourceComboBox.Enabled = usesVideoDevice;
            gbAudioSourceComboBox.Enabled = usesAudioDevice;
            gbRunButton.Text = gbSession == null ?
                T("Start + REGISTER", "启动并 REGISTER") :
                T("Stop GB28181", "停止 GB28181");
            ShowPreviewOverlay(
                gbPreviewOverlayLabel,
                T("GB28181 SESSION FOLLOWS SOURCE BINDING\r\nSTATUS ONLY IN THIS DEMO", "GB28181 会话跟随来源绑定\r\n此示例当前只展示状态"),
                true);
        }

        private void RefreshRuntimeViews()
        {
            RefreshPublisherRuntimeView();
            RefreshPlayerRuntimeView();
            RefreshGbRuntimeView();
            RefreshLicensePanel(null);
        }

        private void RefreshPublisherRuntimeView()
        {
            if (publisherSession == null)
            {
                publisherStateLabel.Text = T("Idle", "空闲");
                return;
            }

            try
            {
                StreamCorePublisherRuntimeInfoResult runtime = publisherSession.GetRuntimeInfo();
                StreamCorePublisherRuntimeInfo info = runtime.RuntimeInfo;
                if (info == null)
                {
                    publisherStateLabel.Text = "runtime=" + runtime.Result;
                    return;
                }

                publisherStateLabel.Text =
                    T("State: ", "状态：") + info.State
                    + " | " + T("Input: ", "输入：") + info.InputKind
                    + " | " + T("Audio/Video: ", "音视频：") + info.EnableAudio + "/" + info.EnableVideo
                    + Environment.NewLine
                    + T("Publish: ", "发布：") + info.PublishIdentity
                    + Environment.NewLine
                    + info.StateSummary;
            }
            catch (Exception ex)
            {
                publisherStateLabel.Text = ex.Message;
            }
        }

        private void RefreshPlayerRuntimeView()
        {
            if (playerSession == null)
            {
                playerStateLabel.Text = T("Idle", "空闲");
                return;
            }

            try
            {
                StreamCorePlayerRuntimeInfoResult runtime = playerSession.GetRuntimeInfo();
                StreamCorePlayerRuntimeInfo info = runtime.RuntimeInfo;
                if (info == null)
                {
                    playerStateLabel.Text = "runtime=" + runtime.Result;
                    return;
                }

                playerStateLabel.Text =
                    T("State: ", "状态：") + info.State
                    + " | " + T("Audio/Video: ", "音视频：") + info.EnableAudio + "/" + info.EnableVideo
                    + Environment.NewLine
                    + T("Source: ", "来源：") + info.SourceIdentity
                    + Environment.NewLine
                    + T("Video: ", "画面：") + info.VideoWidth + "x" + info.VideoHeight + " | " + info.StateSummary;
            }
            catch (Exception ex)
            {
                playerStateLabel.Text = ex.Message;
            }
        }

        private void RefreshGbRuntimeView()
        {
            if (gbSession == null)
            {
                gbStateLabel.Text = T("Idle", "空闲");
                return;
            }

            try
            {
                StreamCoreGb28181RuntimeInfoResult runtime = gbSession.GetRuntimeInfo();
                StreamCoreGb28181RuntimeInfo info = runtime.RuntimeInfo;
                if (info == null)
                {
                    gbStateLabel.Text = "runtime=" + runtime.Result;
                    return;
                }

                gbStateLabel.Text =
                    T("Started/Registered: ", "启动/注册：") + info.IsStarted + "/" + info.IsRegistered
                    + " | " + T("Sessions: ", "会话：") + info.ActiveSessionCount
                    + " | " + T("Media: ", "媒体：") + info.ActiveMediaRuntimeCount
                    + Environment.NewLine
                    + T("Packets A/V/R: ", "包数 A/V/R：")
                    + info.SentAudioPacketCount + "/" + info.SentVideoPacketCount + "/" + info.ReceivedPacketCount
                    + Environment.NewLine
                    + info.StateSummary
                    + (string.IsNullOrWhiteSpace(info.MediaStateSummary) ? string.Empty : Environment.NewLine + info.MediaStateSummary)
                    + (string.IsNullOrWhiteSpace(info.LastError) ? string.Empty : Environment.NewLine + info.LastError);
            }
            catch (Exception ex)
            {
                gbStateLabel.Text = ex.Message;
            }
        }

        private void RefreshLicensePanel(string contextMessage)
        {
            try
            {
                StreamCoreProductInfo product = StreamCoreRuntime.GetProductInfo();
                StreamCoreLicenseInfoResult license = StreamCoreRuntime.GetLicenseInfo();
                StreamCoreLicenseInfo info = license.Info;

                productNameValueLabel.Text = product.ProductName;
                productVersionValueLabel.Text = product.Version;
                productStageValueLabel.Text = product.Stage;
                machineIdValueLabel.Text = info == null ? string.Empty : info.MachineId;
                licenseStatusValueLabel.Text = info == null ? string.Empty : info.StatusName;

                if (info == null)
                {
                    licenseStateLabel.Text = "license=" + license.Call.Result;
                    return;
                }

                licenseStateLabel.Text =
                    T("Configured/Loaded/Valid: ", "已配置/已加载/有效：")
                    + info.IsConfigured + "/" + info.IsLicenseLoaded + "/" + info.IsLicenseValid
                    + " | " + T("Watermark: ", "水印：") + info.NeedWatermark
                    + Environment.NewLine
                    + T("Status: ", "状态：") + info.StatusName
                    + Environment.NewLine
                    + info.Summary;

                if (!string.IsNullOrWhiteSpace(contextMessage))
                {
                    SetStatus("license  " + contextMessage);
                }

                string message = string.IsNullOrWhiteSpace(contextMessage)
                    ? string.Empty
                    : DateTime.Now.ToString("HH:mm:ss") + "  " + contextMessage + Environment.NewLine + Environment.NewLine;
                licenseDetailTextBox.Text = message
                    + "product=" + product.ProductCode + " / " + product.PrimaryTargetName + Environment.NewLine
                    + "machine_id=" + info.MachineId + Environment.NewLine
                    + "runtime_call=" + license.Call.Result + " " + license.Call.ErrorText + Environment.NewLine
                    + "status=" + info.StatusName + Environment.NewLine
                    + "summary=" + info.Summary + Environment.NewLine
                    + "detail=" + info.Detail;
            }
            catch (Exception ex)
            {
                licenseStateLabel.Text = ex.Message;
                licenseDetailTextBox.Text = ex.ToString();
            }
        }

        private void ConfigureDemoLicenseIfPresent()
        {
            string licenseDirectory = ResolveDemoLicenseDirectory();
            string licensePath = Path.Combine(licenseDirectory, "streamcore_demo_winforms.lic");
            string publicKeyPath = Path.Combine(licenseDirectory, "streamcore_demo_public.pem");
            if (!File.Exists(licensePath) || !File.Exists(publicKeyPath))
            {
                return;
            }

            StreamCoreRuntimeConfig config = new StreamCoreRuntimeConfig
            {
                ExpectedProduct = "streamcore_demo",
                LicensePath = licensePath,
                PublicKeyPemPath = publicKeyPath
            };
            StreamCoreCallResult call = StreamCoreRuntime.Configure(config);
            RefreshLicensePanel("demo_license=" + call.Result + " " + call.ErrorText);
        }

        private static string ResolveDemoLicenseDirectory()
        {
            string baseDirectory = AppDomain.CurrentDomain.BaseDirectory;
            string packagedDirectory = Path.Combine(baseDirectory, "license", "demo");
            if (Directory.Exists(packagedDirectory))
            {
                return packagedDirectory;
            }

            // 开发态构建目录保留源码仓库中的授权文件，公开绿色包优先使用运行目录内文件。
            return Path.GetFullPath(
                Path.Combine(baseDirectory, "..", "..", "..", "..", "license", "demo"));
        }

        private void ScheduleAutorunIfRequested()
        {
            string mode = Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_AUTORUN");
            if (!string.Equals(mode, "player", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(mode, "publisher", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(mode, "gb28181", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(mode, "gb", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(mode, "gb28181_lifecycle", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(mode, "gb_lifecycle", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(mode, "runtime", StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            string licenseText = Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_LICENSE_TEXT");
            if (!string.IsNullOrWhiteSpace(licenseText))
            {
                licenseTextBox.Text = licenseText.Trim();
                RegisterLicense();
            }

            string playerUrl = Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_PLAYER_URL");
            if (!string.IsNullOrWhiteSpace(playerUrl))
            {
                playerUrlTextBox.Text = playerUrl.Trim();
            }
            ApplyAutorunPlayerOptions();
            ApplyAutorunPublisherOptions();
            ApplyAutorunGbOptions();

            if (string.Equals(mode, "player", StringComparison.OrdinalIgnoreCase))
            {
                BeginInvoke(new Action(StartPlayer));
            }
            else if (string.Equals(mode, "publisher", StringComparison.OrdinalIgnoreCase))
            {
                BeginInvoke(new Action(StartPublisher));
            }
            else if (string.Equals(mode, "gb28181", StringComparison.OrdinalIgnoreCase)
                || string.Equals(mode, "gb", StringComparison.OrdinalIgnoreCase))
            {
                BeginInvoke(new Action(StartGbRuntimeAndRegister));
            }
            else if (string.Equals(mode, "gb28181_lifecycle", StringComparison.OrdinalIgnoreCase)
                || string.Equals(mode, "gb_lifecycle", StringComparison.OrdinalIgnoreCase))
            {
                BeginInvoke(new Action(StartGbRuntime));
            }

            int quitMs;
            if (!int.TryParse(
                    Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_QUIT_MS"),
                    out quitMs))
            {
                quitMs = 5000;
            }

            autorunTimer.Interval = Math.Max(1000, quitMs);
            autorunTimer.Start();
        }

        /// <summary>
        /// Applies script-supplied publisher options so release-gate media tests
        /// use the same public WinForms path as an interactive customer run.
        /// </summary>
        private void ApplyAutorunPublisherOptions()
        {
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_PUBLISHER_URL", publisherPublishUrlTextBox);
            ApplyTextEnvironment(
                "STREAMCORE_DEMO_DOTNET_PUBLISHER_WHIP_BEARER_TOKEN",
                publisherWhipBearerTokenTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_PUBLISHER_MEDIA_FILE", publisherMediaFileTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_PUBLISHER_AUDIO_FILE", publisherAudioFileTextBox);
            ApplyNumericEnvironment("STREAMCORE_DEMO_DOTNET_PUBLISHER_BITRATE_KBPS", publisherVideoBitrateBox);
            ApplyNumericEnvironment("STREAMCORE_DEMO_DOTNET_PUBLISHER_FPS", publisherFpsBox);
            ApplyNumericEnvironment("STREAMCORE_DEMO_DOTNET_PUBLISHER_GOP", publisherGopBox);

            string source = EnvironmentValue("STREAMCORE_DEMO_DOTNET_PUBLISHER_SOURCE");
            if (source == "desktop")
            {
                SelectComboByValue(publisherSourceComboBox, PublisherSourceMode.Desktop);
            }
            else if (source == "video_file" || source == "video-file" || source == "file" || source == "media")
            {
                SelectComboByValue(publisherSourceComboBox, PublisherSourceMode.VideoFile);
            }
            else if (source == "still" || source == "still_image" || source == "image")
            {
                SelectComboByValue(publisherSourceComboBox, PublisherSourceMode.StillImage);
            }
            else if (source == "none" || source == "audio")
            {
                SelectComboByValue(publisherSourceComboBox, PublisherSourceMode.None);
            }
            else if (source == "camera")
            {
                SelectComboByValue(publisherSourceComboBox, PublisherSourceMode.Camera);
            }

            string audio = EnvironmentValue("STREAMCORE_DEMO_DOTNET_PUBLISHER_AUDIO");
            if (audio == "none" || audio == "off")
            {
                SelectComboByValue(publisherAudioComboBox, PublisherAudioMode.None);
            }
            else if (audio == "system" || audio == "system_audio")
            {
                SelectComboByValue(publisherAudioComboBox, PublisherAudioMode.SystemAudio);
            }
            else if (audio == "file" || audio == "audio_file")
            {
                SelectComboByValue(publisherAudioComboBox, PublisherAudioMode.AudioFile);
            }
            else if (audio == "microphone" || audio == "mic")
            {
                SelectComboByValue(publisherAudioComboBox, PublisherAudioMode.Microphone);
            }

            string fileMode = EnvironmentValue("STREAMCORE_DEMO_DOTNET_PUBLISHER_FILE_MODE");
            if (fileMode == "force" || fileMode == "force_transcode")
            {
                SelectComboByValue(publisherFileModeComboBox, StreamCorePublisherTranscodeMode.ForceTranscode);
            }
            else if (fileMode == "auto" || fileMode == "passthrough")
            {
                SelectComboByValue(publisherFileModeComboBox, StreamCorePublisherTranscodeMode.Auto);
            }

            string videoCodec = EnvironmentValue("STREAMCORE_DEMO_DOTNET_PUBLISHER_VIDEO_CODEC");
            if (videoCodec == "h265" || videoCodec == "hevc")
            {
                SelectComboByValue(publisherVideoCodecComboBox, "h265");
            }
            else if (videoCodec == "h264" || videoCodec == "avc")
            {
                SelectComboByValue(publisherVideoCodecComboBox, "h264");
            }

            string audioCodec = EnvironmentValue("STREAMCORE_DEMO_DOTNET_PUBLISHER_AUDIO_CODEC");
            if (audioCodec == "aac")
            {
                SelectComboByValue(publisherAudioCodecComboBox, "aac");
            }
            else if (audioCodec == "opus")
            {
                SelectComboByValue(publisherAudioCodecComboBox, "opus");
            }

            string hevcMode = EnvironmentValue("STREAMCORE_DEMO_DOTNET_PUBLISHER_RTMP_HEVC_MODE");
            if (hevcMode == "enhanced" || hevcMode == "enhanced_rtmp")
            {
                SelectComboByValue(publisherRtmpHevcModeComboBox, StreamCorePublisherRtmpHevcMode.EnhancedRtmp);
            }
            else if (hevcMode == "legacy" || hevcMode == "legacy_flv")
            {
                SelectComboByValue(publisherRtmpHevcModeComboBox, StreamCorePublisherRtmpHevcMode.LegacyFlvTag);
            }
            else if (hevcMode == "auto" || hevcMode == "compat")
            {
                SelectComboByValue(publisherRtmpHevcModeComboBox, StreamCorePublisherRtmpHevcMode.AutoCompatibility);
            }

            UpdatePublisherUi();
        }

        private void ApplyAutorunPlayerOptions()
        {
            string hardwareDecode =
                Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_HARDWARE_DECODE");
            bool hardware;
            if (bool.TryParse(hardwareDecode, out hardware))
            {
                SelectComboByValue(playerDecodeModeComboBox, hardware);
            }

            string renderPath =
                Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_RENDER_PATH");
            if (string.IsNullOrWhiteSpace(renderPath))
            {
                return;
            }

            renderPath = renderPath.Trim().ToLowerInvariant();
            if (renderPath == "gpu")
            {
                SelectComboByValue(playerRenderPathComboBox, StreamCorePlayerVideoPresentPath.GpuFrame);
            }
            else if (renderPath == "direct")
            {
                SelectComboByValue(playerRenderPathComboBox, StreamCorePlayerVideoPresentPath.DirectSurface);
            }
            else if (renderPath == "auto")
            {
                SelectComboByValue(playerRenderPathComboBox, StreamCorePlayerVideoPresentPath.Auto);
            }
            else
            {
                SelectComboByValue(playerRenderPathComboBox, StreamCorePlayerVideoPresentPath.SoftwareFrame);
            }
        }

        /// <summary>
        /// Applies GB28181 autorun options before the public Start + REGISTER
        /// action is invoked by the release-gate script.
        /// </summary>
        private void ApplyAutorunGbOptions()
        {
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_LOCAL_ID", gbLocalDeviceIdTextBox);
            string legacyChannelId = Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_GB_CHANNEL_ID");
            if (!string.IsNullOrWhiteSpace(legacyChannelId))
            {
                gbLocalDeviceIdTextBox.Text = legacyChannelId.Trim();
            }
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_DOMAIN", gbDomainTextBox);
            string legacySipPassword = Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_GB_PASSWORD");
            if (!string.IsNullOrWhiteSpace(legacySipPassword))
            {
                gbUpperPasswordTextBox.Text = legacySipPassword.Trim();
            }
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_LOCAL_DISPLAY_NAME", gbLocalDisplayNameTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_LOCAL_IP", gbLocalIpTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_UPPER_ID", gbUpperIdTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_UPPER_DOMAIN", gbUpperDomainTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_UPPER_PASSWORD", gbUpperPasswordTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_UPPER_DISPLAY_NAME", gbUpperDisplayNameTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_UPPER_IP", gbUpperIpTextBox);
            ApplyTextEnvironment("STREAMCORE_DEMO_DOTNET_GB_MEDIA_IP", gbMediaIpTextBox);
            ApplyNumericEnvironment("STREAMCORE_DEMO_DOTNET_GB_UPPER_PORT", gbUpperPortBox);
            ApplyNumericEnvironment("STREAMCORE_DEMO_DOTNET_GB_LOCAL_PORT", gbLocalPortBox);
            ApplyNumericEnvironment("STREAMCORE_DEMO_DOTNET_GB_MEDIA_PORT", gbMediaPortBox);
            ApplyNumericEnvironment("STREAMCORE_DEMO_DOTNET_GB_REGISTER_EXPIRES", gbRegisterExpiresBox);
            ApplyNumericEnvironment("STREAMCORE_DEMO_DOTNET_GB_KEEPALIVE_SECONDS", gbKeepaliveIntervalBox);

            string transport = EnvironmentValue("STREAMCORE_DEMO_DOTNET_GB_TRANSPORT");
            if (transport == "udp")
            {
                SelectComboByValue(gbTransportComboBox, StreamCoreGb28181TransportMode.Udp);
            }
            else if (transport == "tcp")
            {
                SelectComboByValue(gbTransportComboBox, StreamCoreGb28181TransportMode.Tcp);
            }

            string source = EnvironmentValue("STREAMCORE_DEMO_DOTNET_GB_SOURCE");
            if (source == "camera")
            {
                SelectComboByValue(gbSourceComboBox, PublisherSourceMode.Camera);
            }
            else if (source == "desktop")
            {
                SelectComboByValue(gbSourceComboBox, PublisherSourceMode.Desktop);
            }

            string audio = EnvironmentValue("STREAMCORE_DEMO_DOTNET_GB_AUDIO");
            if (audio == "none" || audio == "off")
            {
                SelectComboByValue(gbAudioComboBox, GbAudioMode.None);
            }
            else if (audio == "microphone" || audio == "mic")
            {
                SelectComboByValue(gbAudioComboBox, GbAudioMode.Microphone);
            }
            else if (audio == "system" || audio == "system_audio")
            {
                SelectComboByValue(gbAudioComboBox, GbAudioMode.SystemAudio);
            }
            else if (audio == "default" || audio == "embedded")
            {
                SelectComboByValue(gbAudioComboBox, GbAudioMode.Microphone);
            }

            UpdateGbUi();
        }

        /// <summary>
        /// Writes autorun UI status into a script-selected file, giving headless
        /// test runs durable evidence in addition to external ffprobe output.
        /// </summary>
        private void WriteAutorunReport()
        {
            string autorunMode =
                Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_AUTORUN");
            string reportPath = Environment.GetEnvironmentVariable("STREAMCORE_DEMO_DOTNET_AUTORUN_REPORT");
            if (string.IsNullOrWhiteSpace(reportPath))
            {
                return;
            }

            try
            {
                string directory = Path.GetDirectoryName(reportPath);
                if (!string.IsNullOrWhiteSpace(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                File.WriteAllText(
                    reportPath,
                    BuildLogReportText(autorunMode),
                    Encoding.UTF8);
            }
            catch (Exception ex)
            {
                SetStatus("autorun_report failed: " + ex.Message);
            }
        }

        private static string EnvironmentValue(string name)
        {
            string value = Environment.GetEnvironmentVariable(name);
            return string.IsNullOrWhiteSpace(value) ? string.Empty : value.Trim().ToLowerInvariant();
        }

        private static void ApplyTextEnvironment(string name, TextBox textBox)
        {
            string value = Environment.GetEnvironmentVariable(name);
            if (!string.IsNullOrWhiteSpace(value))
            {
                textBox.Text = value.Trim();
            }
        }

        private static void ApplyNumericEnvironment(string name, NumericUpDown control)
        {
            int value;
            if (int.TryParse(Environment.GetEnvironmentVariable(name), out value)
                && value >= control.Minimum
                && value <= control.Maximum)
            {
                control.Value = value;
            }
        }

        private void BrowsePublisherMediaFile()
        {
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                dialog.Filter = T(
                    "Media files|*.mp4;*.flv;*.mkv;*.ts;*.mov;*.png;*.jpg;*.jpeg;*.bmp|All files|*.*",
                    "媒体文件|*.mp4;*.flv;*.mkv;*.ts;*.mov;*.png;*.jpg;*.jpeg;*.bmp|所有文件|*.*");
                if (dialog.ShowDialog(this) == DialogResult.OK)
                {
                    publisherMediaFileTextBox.Text = dialog.FileName;
                }
            }
        }

        private void BrowsePublisherAudioFile()
        {
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                dialog.Filter = T(
                    "Audio files|*.aac;*.m4a;*.mp3;*.wav;*.flac|All files|*.*",
                    "音频文件|*.aac;*.m4a;*.mp3;*.wav;*.flac|所有文件|*.*");
                if (dialog.ShowDialog(this) == DialogResult.OK)
                {
                    publisherAudioFileTextBox.Text = dialog.FileName;
                }
            }
        }

        private void LogPublisher(string text)
        {
            AppendLog(publisherStatusTextBox, text);
            SetStatus("publisher  " + text);
        }

        private void LogPlayer(string text)
        {
            AppendLog(playerStatusTextBox, text);
            SetStatus("player  " + text);
        }

        private void LogGb(string text)
        {
            AppendLog(gbStatusTextBox, text);
            SetStatus("gb28181  " + text);
        }

        private void SetStatus(string text)
        {
            statusLabel.Text = DateTime.Now.ToString("HH:mm:ss") + "  " + text;
            licenseLatestEventLabel.Text = statusLabel.Text;
            AppendLog(logsTextBox, text);
            Console.WriteLine(statusLabel.Text);
        }

        private void AppendLog(TextBox textBox, string text)
        {
            if (textBox.TextLength > 40000)
            {
                textBox.Text = textBox.Text.Substring(textBox.TextLength / 2);
            }

            textBox.AppendText(DateTime.Now.ToString("HH:mm:ss") + "  " + text + Environment.NewLine);
        }

        private void ShowPreviewOverlay(Label label, string text, bool visible)
        {
            label.Text = text;
            label.Visible = visible;
            if (visible)
            {
                label.BringToFront();
            }
        }

        private void PostUi(Action action)
        {
            if (IsDisposed)
            {
                return;
            }
            if (InvokeRequired)
            {
                BeginInvoke(action);
                return;
            }
            action();
        }

        private bool IsChineseUi()
        {
            return languageMode == DemoLanguageMode.Chinese
                || (languageMode == DemoLanguageMode.Auto && systemPrefersChinese);
        }

        private string T(string english, string chinese)
        {
            return IsChineseUi() ? chinese : english;
        }

        private static string FindWorkspaceRelativeFile(params string[] pathSegments)
        {
            DirectoryInfo current = new DirectoryInfo(AppDomain.CurrentDomain.BaseDirectory);
            while (current != null)
            {
                string[] effectiveSegments = pathSegments ?? new string[0];
                string candidate = Path.Combine(new[] { current.FullName }.Concat(effectiveSegments).ToArray());
                if (File.Exists(candidate))
                {
                    return candidate;
                }
                current = current.Parent;
            }
            return string.Empty;
        }

        private static string GuessExistingPath(params string[] candidates)
        {
            foreach (string candidate in candidates ?? new string[0])
            {
                if (!string.IsNullOrWhiteSpace(candidate) && File.Exists(candidate))
                {
                    return candidate;
                }
            }
            return string.Empty;
        }

        private static string SafePreflight(StreamCorePlayerPreflight preflight)
        {
            return preflight == null ? "null" : preflight.IsReadyToStart.ToString();
        }

        private static string SafePreflight(StreamCoreCapturePreflight preflight)
        {
            return preflight == null ? "null" : preflight.IsReadyToStart.ToString();
        }

        private static string SafePreflight(StreamCorePublisherPreflight preflight)
        {
            return preflight == null ? "null" : preflight.IsReadyToStart.ToString();
        }

        private static string FormatResult(StreamCoreResult result)
        {
            return ((int)result).ToString(CultureInfo.InvariantCulture)
                + "/" + result.Name();
        }

        private static SplitContainer CreatePageSplit()
        {
            return new SplitContainer
            {
                Dock = DockStyle.Fill,
                Orientation = Orientation.Vertical,
                FixedPanel = FixedPanel.Panel1,
                Panel1MinSize = SidebarMinWidth
            };
        }

        private void ApplyPageSplitWidths()
        {
            foreach (SplitContainer split in pageSplits)
            {
                if (split.Width <= 0)
                {
                    continue;
                }

                int preferred = SidebarPreferredWidth;
                int maximum = Math.Max(split.Panel1MinSize, split.Width - 600);
                int distance = Math.Min(preferred, maximum);
                if (distance >= split.Panel1MinSize)
                {
                    split.SplitterDistance = distance;
                }
            }
        }

        private static TableLayoutPanel CreateFormGrid()
        {
            TableLayoutPanel grid = new TableLayoutPanel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = 2,
                Padding = new Padding(6)
            };
            grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 140));
            grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            return grid;
        }

        private static TableLayoutPanel CreateSidebarStack()
        {
            TableLayoutPanel stack = new TableLayoutPanel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = 1,
                Padding = new Padding(8)
            };
            stack.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            return stack;
        }

        private static void AddSidebarSection(TableLayoutPanel sidebar, Control section)
        {
            if (sidebar == null || section == null)
            {
                return;
            }

            sidebar.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            section.Dock = DockStyle.Top;
            section.Margin = new Padding(0, 0, 0, 8);
            sidebar.Controls.Add(section, 0, sidebar.RowCount);
            sidebar.RowCount++;
        }

        private static GroupBox CreateSectionCard(string title, params Control[] rows)
        {
            GroupBox group = new GroupBox
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                Text = title,
                Padding = new Padding(8)
            };
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = 1
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

            foreach (Control row in rows ?? new Control[0])
            {
                if (row == null)
                {
                    continue;
                }

                row.Dock = DockStyle.Top;
                row.Margin = new Padding(0, 0, 0, 6);
                layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
                layout.Controls.Add(row, 0, layout.RowCount);
                layout.RowCount++;
            }

            group.Controls.Add(layout);
            return group;
        }

        private static Control CreateActionPanel(params Control[] rows)
        {
            Panel panel = new Panel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                Padding = new Padding(0, 2, 0, 0)
            };
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = 1
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            foreach (Control row in rows ?? new Control[0])
            {
                if (row == null)
                {
                    continue;
                }

                row.Dock = DockStyle.Top;
                row.Margin = new Padding(0, 0, 0, 5);
                layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
                layout.Controls.Add(row, 0, layout.RowCount);
                layout.RowCount++;
            }

            panel.Controls.Add(layout);
            return panel;
        }

        private static Control CreateFieldRow(string labelText, Control control)
        {
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = 2
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

            Label label = new Label
            {
                Text = labelText,
                AutoSize = true,
                MinimumSize = new Size(0, 28),
                Margin = new Padding(0, 0, 6, 0),
                TextAlign = ContentAlignment.MiddleLeft,
                Anchor = AnchorStyles.Left
            };
            control.Dock = DockStyle.Fill;
            control.Margin = new Padding(0, 1, 0, 1);
            layout.Controls.Add(label, 0, 0);
            layout.Controls.Add(control, 1, 0);
            return layout;
        }

        private static Control CreateInlineLabeledEditor(
            Control primaryControl,
            string secondaryLabelText,
            Control secondaryControl)
        {
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = 3,
                RowCount = 1
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 54f));
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 46f));

            Label secondaryLabel = new Label
            {
                AutoSize = true,
                Anchor = AnchorStyles.Left,
                Margin = new Padding(4, 0, 4, 0),
                TextAlign = ContentAlignment.MiddleLeft,
                Text = secondaryLabelText
            };

            primaryControl.Dock = DockStyle.Fill;
            primaryControl.Margin = new Padding(0);
            secondaryControl.Dock = DockStyle.Fill;
            secondaryControl.Margin = new Padding(0);

            layout.Controls.Add(primaryControl, 0, 0);
            layout.Controls.Add(secondaryLabel, 1, 0);
            layout.Controls.Add(secondaryControl, 2, 0);
            return layout;
        }

        private static Control CreateStackPanel(params Control[] controls)
        {
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = 1
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            foreach (Control control in controls ?? new Control[0])
            {
                if (control == null)
                {
                    continue;
                }

                control.Dock = DockStyle.Top;
                control.Margin = new Padding(0, 0, 0, 4);
                layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
                layout.Controls.Add(control, 0, layout.RowCount);
                layout.RowCount++;
            }
            return layout;
        }

        private static Control CreateInlineEditors(params Control[] controls)
        {
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = Math.Max(1, controls?.Length ?? 0),
                RowCount = 1
            };

            Control[] items = controls ?? new Control[0];
            if (items.Length == 0)
            {
                return layout;
            }

            float width = 100f / items.Length;
            for (int i = 0; i < items.Length; i++)
            {
                layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, width));
                items[i].Dock = DockStyle.Fill;
                items[i].Margin = i == 0 ? new Padding(0) : new Padding(8, 0, 0, 0);
                layout.Controls.Add(items[i], i, 0);
            }
            return layout;
        }

        private static TableLayoutPanel CreateRightPane()
        {
            TableLayoutPanel right = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 1,
                RowCount = 2,
                Padding = new Padding(8)
            };
            right.RowStyles.Add(new RowStyle(SizeType.Percent, 66));
            right.RowStyles.Add(new RowStyle(SizeType.Percent, 34));
            return right;
        }

        private TableLayoutPanel CreateMediaRightPane()
        {
            TableLayoutPanel right = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 1,
                RowCount = 2,
                Padding = new Padding(8)
            };
            right.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
            right.RowStyles.Add(new RowStyle(SizeType.Absolute, MediaStatusRowHeight));
            right.Resize += (sender, args) => ApplyMediaRightPaneLayout(right);
            ApplyMediaRightPaneLayout(right);
            return right;
        }

        private void ApplyMediaRightPaneLayout(TableLayoutPanel right)
        {
            if (right == null || right.RowStyles.Count < 2)
            {
                return;
            }

            int contentWidth = Math.Max(1, right.DisplayRectangle.Width);
            int contentHeight = Math.Max(1, right.DisplayRectangle.Height);
            int minPreviewHeight = 236 + PreviewGroupChromeHeight;
            int desiredPreviewHeight = Math.Max(
                minPreviewHeight,
                (int)Math.Round(contentWidth * (PreviewAspectHeight / (double)PreviewAspectWidth))
                + PreviewGroupChromeHeight);
            int maxPreviewHeight = Math.Max(
                minPreviewHeight,
                contentHeight - MediaStatusMinHeight);
            int previewHeight = Math.Min(desiredPreviewHeight, maxPreviewHeight);

            right.RowStyles[0].SizeType = SizeType.Absolute;
            right.RowStyles[0].Height = previewHeight;
            right.RowStyles[1].SizeType = SizeType.Percent;
            right.RowStyles[1].Height = 100f;
        }

        private static Panel CreateScrollHost(Control child)
        {
            Panel panel = new Panel
            {
                Dock = DockStyle.Fill,
                AutoScroll = true
            };
            panel.Controls.Add(child);
            return panel;
        }

        private static GroupBox CreatePreviewGroup(string title, Panel previewPanel, Label overlay)
        {
            GroupBox group = new GroupBox
            {
                Dock = DockStyle.Fill,
                Text = title,
                Padding = new Padding(10)
            };
            previewPanel.Controls.Add(overlay);
            Label watermark = CreateDemoWatermarkLabel();
            previewPanel.Controls.Add(watermark);
            previewPanel.Resize += (sender, args) => LayoutDemoWatermark(previewPanel, watermark);
            LayoutDemoWatermark(previewPanel, watermark);
            group.Controls.Add(CreatePreviewHost(previewPanel));
            return group;
        }

        private static Panel CreatePreviewHost(Panel previewPanel)
        {
            Panel host = new Panel
            {
                Dock = DockStyle.Fill,
                BackColor = SystemColors.Control
            };
            if (previewPanel == null)
            {
                return host;
            }

            host.Controls.Add(previewPanel);
            host.Resize += (sender, args) => LayoutPreviewPanel(host, previewPanel);
            LayoutPreviewPanel(host, previewPanel);
            return host;
        }

        private static void LayoutPreviewPanel(Panel host, Panel previewPanel)
        {
            if (host == null || previewPanel == null)
            {
                return;
            }

            int availableWidth = Math.Max(1, host.ClientSize.Width);
            int availableHeight = Math.Max(1, host.ClientSize.Height);
            int targetWidth = availableWidth;
            int targetHeight = (int)Math.Round(
                targetWidth * (PreviewAspectHeight / (double)PreviewAspectWidth));
            if (targetHeight > availableHeight)
            {
                targetHeight = availableHeight;
                targetWidth = (int)Math.Round(
                    targetHeight * (PreviewAspectWidth / (double)PreviewAspectHeight));
            }

            targetWidth = Math.Max(1, Math.Min(targetWidth, availableWidth));
            targetHeight = Math.Max(1, Math.Min(targetHeight, availableHeight));
            int left = Math.Max(0, (availableWidth - targetWidth) / 2);
            int top = Math.Max(0, (availableHeight - targetHeight) / 2);
            previewPanel.Bounds = new Rectangle(left, top, targetWidth, targetHeight);
        }

        private static GroupBox CreateStatusGroup(string title, Label stateLabel, TextBox logTextBox)
        {
            GroupBox group = new GroupBox
            {
                Dock = DockStyle.Fill,
                Text = title,
                Padding = new Padding(10)
            };
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 1,
                RowCount = 2
            };
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
            layout.Controls.Add(stateLabel, 0, 0);
            layout.Controls.Add(logTextBox, 0, 1);
            group.Controls.Add(layout);
            return group;
        }

        private static GroupBox CreateSummaryGroup(string title, Label stateLabel)
        {
            GroupBox group = new GroupBox
            {
                Dock = DockStyle.Fill,
                Text = title,
                Padding = new Padding(10)
            };
            stateLabel.Dock = DockStyle.Fill;
            group.Controls.Add(stateLabel);
            return group;
        }

        private static void AddGridRow(TableLayoutPanel grid, ref int row, string labelText, Control control)
        {
            grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            Label label = new Label
            {
                Text = labelText,
                AutoSize = true,
                Anchor = AnchorStyles.Left,
                Margin = new Padding(0, 8, 8, 8)
            };
            control.Margin = new Padding(0, 4, 0, 4);
            control.Dock = DockStyle.Fill;
            grid.Controls.Add(label, 0, row);
            grid.Controls.Add(control, 1, row);
            row++;
        }

        private static Control CreatePathEditor(TextBox textBox, Action browseAction)
        {
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 2,
                RowCount = 1
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            Button browse = CreateButton("...", (sender, args) => browseAction());
            browse.Width = 38;
            layout.Controls.Add(textBox, 0, 0);
            layout.Controls.Add(browse, 1, 0);
            return layout;
        }

        private static Control CreateTrackEditor(TrackBar trackBar, Label valueLabel)
        {
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 2,
                RowCount = 1
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            trackBar.Margin = new Padding(0);
            valueLabel.Margin = new Padding(8, 0, 0, 0);
            layout.Controls.Add(trackBar, 0, 0);
            layout.Controls.Add(valueLabel, 1, 0);
            return layout;
        }

        private static Control CreateRunVolumeRow(Button runButton, TrackBar trackBar, Label valueLabel)
        {
            TableLayoutPanel layout = new TableLayoutPanel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                ColumnCount = 3,
                RowCount = 1
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            runButton.Dock = DockStyle.Left;
            runButton.Margin = new Padding(0, 0, 10, 0);
            trackBar.Dock = DockStyle.Fill;
            trackBar.Margin = new Padding(0);
            valueLabel.Margin = new Padding(8, 0, 0, 0);
            layout.Controls.Add(runButton, 0, 0);
            layout.Controls.Add(trackBar, 1, 0);
            layout.Controls.Add(valueLabel, 2, 0);
            return layout;
        }

        private static FlowLayoutPanel CreateActionBar(params Control[] controls)
        {
            FlowLayoutPanel panel = new FlowLayoutPanel
            {
                Dock = DockStyle.Top,
                AutoSize = true,
                WrapContents = true,
                FlowDirection = FlowDirection.LeftToRight
            };
            foreach (Control control in controls)
            {
                panel.Controls.Add(control);
            }
            return panel;
        }

        private static Button CreateButton(string text, EventHandler onClick)
        {
            Size textSize = TextRenderer.MeasureText(text ?? string.Empty, SystemFonts.MessageBoxFont);
            int width = string.IsNullOrEmpty(text) ?
                124 :
                string.Equals(text, "...", StringComparison.Ordinal) ?
                38 :
                Math.Min(156, Math.Max(94, textSize.Width + 22));
            Button button = new Button
            {
                Text = text,
                AutoSize = false,
                Height = 30,
                Width = width,
                MinimumSize = new Size(Math.Min(width, 94), 30)
            };
            button.Click += onClick;
            return button;
        }

        private static ComboBox CreateComboBox()
        {
            return new ComboBox
            {
                Dock = DockStyle.Fill,
                DropDownStyle = ComboBoxStyle.DropDownList
            };
        }

        private static ListBox CreateListBox()
        {
            return new ListBox
            {
                Dock = DockStyle.Fill,
                IntegralHeight = false
            };
        }

        private static Label CreateHintLabel()
        {
            return new Label
            {
                Dock = DockStyle.Fill,
                AutoSize = false,
                ForeColor = Color.DimGray,
                TextAlign = ContentAlignment.MiddleLeft,
                Height = 22
            };
        }

        private static Label CreateHintLabel(string text)
        {
            Label label = CreateHintLabel();
            label.Text = text;
            label.AutoSize = true;
            return label;
        }

        private static TextBox CreateSingleLineEditor()
        {
            return new TextBox
            {
                Dock = DockStyle.Fill
            };
        }

        private static TextBox CreateMultilineEditor()
        {
            return new TextBox
            {
                Dock = DockStyle.Fill,
                Multiline = true,
                ReadOnly = false,
                ScrollBars = ScrollBars.Vertical
            };
        }

        private static NumericUpDown CreateIntegerBox(int minimum, int maximum, int value)
        {
            return new NumericUpDown
            {
                Dock = DockStyle.Left,
                Width = 120,
                Minimum = minimum,
                Maximum = maximum,
                Value = Math.Min(maximum, Math.Max(minimum, value))
            };
        }

        private static TrackBar CreateTrackBar(int minimum, int maximum, int value)
        {
            return new TrackBar
            {
                Dock = DockStyle.Fill,
                Minimum = minimum,
                Maximum = maximum,
                TickFrequency = 10,
                Value = Math.Min(maximum, Math.Max(minimum, value))
            };
        }

        private static CheckBox CreateCheckBox()
        {
            return new CheckBox
            {
                AutoSize = true,
                Anchor = AnchorStyles.Left
            };
        }

        private static Panel CreatePreviewPanel()
        {
            return new Panel
            {
                Dock = DockStyle.None,
                MinimumSize = new Size(420, 236),
                BackColor = Color.Black,
                BorderStyle = BorderStyle.FixedSingle
            };
        }

        private static Label CreatePreviewOverlayLabel()
        {
            return new Label
            {
                Dock = DockStyle.Fill,
                BackColor = Color.FromArgb(17, 24, 39),
                ForeColor = Color.FromArgb(221, 230, 239),
                TextAlign = ContentAlignment.MiddleCenter,
                Padding = new Padding(24),
                Font = new Font("Segoe UI", 12f, FontStyle.Bold),
                Visible = true
            };
        }

        private static Label CreateDemoWatermarkLabel()
        {
            return new Label
            {
                AutoSize = true,
                BackColor = Color.FromArgb(170, 17, 24, 39),
                ForeColor = Color.White,
                Text = DemoWatermarkText,
                TextAlign = ContentAlignment.MiddleCenter,
                Padding = new Padding(8, 4, 8, 4),
                Font = new Font("Segoe UI", 9f, FontStyle.Bold),
                Visible = true
            };
        }

        private static void LayoutDemoWatermark(Panel previewPanel, Label watermark)
        {
            if (previewPanel == null || watermark == null)
            {
                return;
            }

            watermark.BringToFront();
            const int margin = 8;
            Size preferredSize = watermark.GetPreferredSize(Size.Empty);
            watermark.Size = preferredSize;
            watermark.Location = new Point(
                Math.Max(margin, previewPanel.ClientSize.Width - preferredSize.Width - margin),
                Math.Max(margin, previewPanel.ClientSize.Height - preferredSize.Height - margin));
        }

        private static Label CreateStateLabel()
        {
            return new Label
            {
                Dock = DockStyle.Fill,
                AutoSize = false,
                Height = 68,
                BorderStyle = BorderStyle.FixedSingle,
                Padding = new Padding(8),
                TextAlign = ContentAlignment.TopLeft
            };
        }

        private static Label CreateValueLabel()
        {
            return new Label
            {
                Dock = DockStyle.Fill,
                AutoSize = false,
                BorderStyle = BorderStyle.FixedSingle,
                Padding = new Padding(8, 6, 8, 6),
                TextAlign = ContentAlignment.MiddleLeft
            };
        }

        private static Label CreateInlineValueLabel()
        {
            return new Label
            {
                AutoSize = true,
                Width = 56,
                TextAlign = ContentAlignment.MiddleLeft
            };
        }

        private static void AddComboValue(ComboBox comboBox, string text, object value)
        {
            comboBox.Items.Add(new ComboValue(text, value));
        }

        private static void SelectComboByValue(ComboBox comboBox, object value)
        {
            SelectComboByValue(comboBox, value, itemValue => Equals(itemValue, value));
        }

        private static void SelectComboByValue(
            ComboBox comboBox,
            object value,
            Func<object, bool> predicate)
        {
            for (int i = 0; i < comboBox.Items.Count; i++)
            {
                ComboValue item = comboBox.Items[i] as ComboValue;
                if (item != null && predicate(item.Value))
                {
                    comboBox.SelectedIndex = i;
                    return;
                }
            }
            if (comboBox.Items.Count > 0)
            {
                comboBox.SelectedIndex = 0;
            }
        }

        private static T SelectedValue<T>(ComboBox comboBox, T fallback)
        {
            ComboValue item = comboBox.SelectedItem as ComboValue;
            if (item != null && item.Value is T)
            {
                return (T)item.Value;
            }
            return fallback;
        }

        private static object SelectedObjectValue(ComboBox comboBox)
        {
            ComboValue item = comboBox.SelectedItem as ComboValue;
            return item == null ? null : item.Value;
        }

        private static string SelectedTextValue(ComboBox comboBox, string fallback)
        {
            ComboValue item = comboBox.SelectedItem as ComboValue;
            return item == null || item.Value == null ? fallback : item.Value.ToString();
        }

        private static StreamCoreCaptureSourceInfo SelectedCaptureSource(ComboBox comboBox)
        {
            ComboValue item = comboBox.SelectedItem as ComboValue;
            return item == null ? null : item.Value as StreamCoreCaptureSourceInfo;
        }

        private static string DescribeCaptureSource(StreamCoreCaptureSourceInfo source)
        {
            if (source == null)
            {
                return string.Empty;
            }

            string primary = !string.IsNullOrWhiteSpace(source.SourceName)
                ? source.SourceName
                : source.SourceId;
            if (source.DisplayWidth > 0 && source.DisplayHeight > 0)
            {
                return primary + " (" + source.DisplayWidth + "x" + source.DisplayHeight + ")";
            }
            return primary;
        }

        private static ResolutionPreset SelectedResolution(ComboBox comboBox, int fallbackWidth, int fallbackHeight)
        {
            ComboValue item = comboBox.SelectedItem as ComboValue;
            ResolutionPreset preset = item == null ? null : item.Value as ResolutionPreset;
            return preset ?? new ResolutionPreset(fallbackWidth, fallbackHeight);
        }

        private static string SelectedResolutionKey(ComboBox comboBox)
        {
            ComboValue item = comboBox.SelectedItem as ComboValue;
            ResolutionPreset preset = item == null ? null : item.Value as ResolutionPreset;
            return preset == null ? string.Empty : preset.Key;
        }

        /// <summary>
        /// ComboBox display item with a stable payload value.
        /// </summary>
        private sealed class ComboValue
        {
            public ComboValue(string text, object value)
            {
                Text = text;
                Value = value;
            }

            public string Text { get; private set; }

            public object Value { get; private set; }

            public override string ToString()
            {
                return Text;
            }
        }

        /// <summary>
        /// Named video-size option shared by publisher and GB28181 pages.
        /// </summary>
        private sealed class ResolutionPreset
        {
            public ResolutionPreset(int width, int height)
            {
                Width = width;
                Height = height;
                Key = width + "x" + height;
                Text = Key;
            }

            public int Width { get; private set; }

            public int Height { get; private set; }

            public string Key { get; private set; }

            public string Text { get; private set; }
        }

        /// <summary>
        /// High-level publisher source mode selected by the demo UI.
        /// </summary>
        private enum PublisherSourceMode
        {
            Camera,
            Desktop,
            VideoFile,
            StillImage,
            None
        }

        /// <summary>
        /// Audio input mode paired with the selected publisher source.
        /// </summary>
        private enum PublisherAudioMode
        {
            None,
            Microphone,
            SystemAudio,
            AudioFile
        }

        /// <summary>
        /// GB28181 local-device audio choice for automatic source binding.
        /// </summary>
        private enum GbAudioMode
        {
            Default,
            Microphone,
            SystemAudio,
            None
        }
    }
}
