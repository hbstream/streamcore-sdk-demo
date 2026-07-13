package com.hbr.streamcoredemo;

import static com.hbr.streamcoredemo.DemoNetworkSupport.DEFAULT_DEVELOPMENT_RTMP_HOST;
import static com.hbr.streamcoredemo.DemoNetworkSupport.EMULATOR_HOST_LOOPBACK;
import static com.hbr.streamcoredemo.DemoNetworkSupport.RTMP_PORT;
import static com.hbr.streamcoredemo.DemoNetworkSupport.isPublisherUrlReachable;
import static com.hbr.streamcoredemo.DemoNetworkSupport.isPublisherUrlSyntaxValid;
import static com.hbr.streamcoredemo.DemoNetworkSupport.resolveFirstNonLoopbackIpv4;
import static com.hbr.streamcoredemo.DemoOperationSupport.buildStatus;
import static com.hbr.streamcoredemo.DemoOperationSupport.deferredBootstrapStatus;
import static com.hbr.streamcoredemo.DemoOperationSupport.notApplicableStatus;
import static com.hbr.streamcoredemo.DemoOperationSupport.safeMessage;
import static com.hbr.streamcoredemo.DemoScenarioFactory.buildEncodedFeedForcedScenario;
import static com.hbr.streamcoredemo.DemoScenarioFactory.buildLocalCapturePublisherScenario;
import static com.hbr.streamcoredemo.DemoScenarioFactory.buildMediaFilePassthroughScenario;
import static com.hbr.streamcoredemo.DemoScenarioFactory.buildRawFeedPublisherScenario;
import static com.hbr.streamcoredemo.DemoScenarioFactory.buildStillImageScenario;
import static com.hbr.streamcoredemo.PublisherValidationSupport.waitForPublisherLocalCaptureTransportDrain;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.SurfaceTexture;
import android.graphics.Typeface;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.projection.MediaProjectionManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.util.Size;
import android.view.Gravity;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.VideoView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.widget.PopupMenu;
import androidx.core.content.FileProvider;
import androidx.core.os.LocaleListCompat;

import com.hbr.streamcore.StreamCoreCapture;
import com.hbr.streamcore.StreamCoreCapabilityDescriptor;
import com.hbr.streamcore.StreamCoreFeatureResult;
import com.hbr.streamcore.StreamCoreLogInfo;
import com.hbr.streamcore.StreamCoreLogLevel;
import com.hbr.streamcore.StreamCoreMediaPayload;
import com.hbr.streamcore.StreamCoreOnvif;
import com.hbr.streamcore.StreamCoreLimitResult;
import com.hbr.streamcore.StreamCoreOperationStatus;
import com.hbr.streamcore.StreamCorePlatformPackagePlan;
import com.hbr.streamcore.StreamCorePlayer;
import com.hbr.streamcore.StreamCoreProductInfo;
import com.hbr.streamcore.StreamCorePublisher;
import com.hbr.streamcore.StreamCoreResultCode;
import com.hbr.streamcore.StreamCoreRuntime;
import com.hbr.streamcore.StreamCoreRuntimeConfig;
import com.hbr.streamcore.StreamCoreRuntimeLicenseInfo;
import com.hbr.streamcore.StreamCoreSdk;
import com.hbr.streamcore.StreamCoreGB28181;
import com.hbr.streamcore.StreamCoreSessionScope;
import com.hbr.streamcore.StreamCoreSurfaceTarget;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.net.URI;
import java.net.URISyntaxException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public final class MainActivity extends AppCompatActivity {
    private static final float PARAMETER_LABEL_TEXT_SIZE_SP = 13.0f;
    private static final float PARAMETER_CONTROL_TEXT_SIZE_SP = 14.0f;
    private static final float PARAMETER_LOG_TEXT_SIZE_SP = 13.0f;
    private static final int SPINNER_ROW_HORIZONTAL_PADDING_DP = 12;
    private static final int SPINNER_ROW_VERTICAL_PADDING_DP = 8;
    private static final int DEFAULT_PREVIEW_SOURCE_WIDTH = 1280;
    private static final int DEFAULT_PREVIEW_SOURCE_HEIGHT = 720;
    private static final float MOBILE_PREVIEW_HEIGHT_RATIO = 0.75f;
    private static final int TAB_COLOR_SELECTED = Color.rgb(29, 111, 184);
    private static final int TAB_COLOR_IDLE = Color.rgb(230, 235, 240);
    private static final int TAB_TEXT_SELECTED = Color.WHITE;
    private static final int TAB_TEXT_IDLE = Color.rgb(34, 49, 63);
    private static final String LOG_SHARE_DIR_NAME = "streamcore-log-share";
    private static final String LOG_TAG = "StreamCoreDemo";
    private static final String LOG_ZIP_MIME_TYPE = "application/zip";
    private static final long LOG_PACKAGE_MAX_BYTES = 20L * 1024L * 1024L;
    private static final long LOG_PACKAGE_SINGLE_FILE_MAX_BYTES = 8L * 1024L * 1024L;
    private static final int LOG_PACKAGE_MAX_FILES = 24;
    private static final int LOG_SHARE_MAX_ZIP_FILES = 5;
    private static final int ANDROID_EXIT_INFO_MAX_ITEMS = 5;
    private static final String EXTRA_AUTORUN = "streamcore_demo_autorun";
    private static final String EXTRA_LANGUAGE = "streamcore_demo_language";
    private static final String EXTRA_PLAYER_URL = "streamcore_demo_player_url";
    private static final String EXTRA_PUBLISH_URL = "streamcore_demo_publish_url";
    private static final String EXTRA_GB_UPPER_IP = "streamcore_demo_gb_upper_ip";
    private static final String EXTRA_GB_UPPER_PORT = "streamcore_demo_gb_upper_port";
    private static final String EXTRA_QUIT_MS = "streamcore_demo_quit_ms";
    private static final String EXTRA_PLAYER_DECODE_INDEX = "streamcore_demo_player_decode_index";
    private static final String EXTRA_PLAYER_RENDER_INDEX = "streamcore_demo_player_render_index";
    private static final String EXTRA_DISPLAY_MODE_INDEX = "streamcore_demo_display_mode_index";
    private static final String EXTRA_PUBLISHER_SOURCE_INDEX = "streamcore_demo_publisher_source_index";
    private static final String EXTRA_GB_SOURCE_INDEX = "streamcore_demo_gb_source_index";
    private static final String AUTORUN_PLAYER = "player";
    private static final String AUTORUN_PUBLISHER_MATRIX = "publisher_matrix";
    private static final String AUTORUN_PUBLISHER_LOCAL_CAPTURE = "publisher_local_capture";
    private static final String AUTORUN_PUBLISHER_PREVIEW = "publisher_preview";
    private static final String AUTORUN_CAMERA_PREVIEW = "camera_preview";
    private static final String AUTORUN_GB28181 = "gb28181";
    private static final String AUTORUN_LOGS = "logs";
    private static final String AUTORUN_UPLOAD = "upload";
    private static final int AUTORUN_SURFACE_RETRY_LIMIT = 40;
    private static final int AUTORUN_SURFACE_RETRY_DELAY_MS = 250;

    private enum PreviewDisplayMode {
        STRETCH,
        FIT,
        CROP
    }

    private static final class LogPackageFile {
        final File file;
        final String entryName;
        final long lastModified;
        final long sizeBytes;

        LogPackageFile(File file, String entryName) {
            this.file = file;
            this.entryName = entryName;
            this.lastModified = file.lastModified();
            this.sizeBytes = file.length();
        }
    }

    private static final class LogPackageSelection {
        int addedFileCount;
        long addedBytes;
        final StringBuilder skippedFiles = new StringBuilder();

        void skip(LogPackageFile file, String reason) {
            skippedFiles
                    .append(file.entryName)
                    .append(" | ")
                    .append(file.sizeBytes)
                    .append(" bytes | ")
                    .append(reason)
                    .append('\n');
        }
    }

    private ScrollView tabContentScroll;
    private Button licenseTabButton;
    private Button playerTabButton;
    private Button publisherTabButton;
    private Button gb28181TabButton;
    private View licensePanel;
    private View playerPanel;
    private View publisherPanel;
    private View gb28181Panel;
    private View playerOnvifControls;
    private View gbVideoFileRow;
    private View publisherCameraSourceRow;
    private View publisherMediaFileRow;
    private TextView demoStatusText;
    private TextView licenseSummaryText;
    private TextView playerSummaryText;
    private TextView captureSummaryText;
    private TextView desktopCaptureSummaryText;
    private TextView publisherActionStatusText;
    private TextView publisherSummaryText;
    private TextView gb28181SummaryText;
    private String playerOnvifStatus = "";
    private TextureView playerPreviewSurface;
    private TextureView cameraPreviewSurface;
    private TextureView publisherPreviewSurface;
    private TextureView gbCameraPreviewSurface;
    private VideoView gbVideoPreviewView;
    private TextView gbPreviewHintText;
    private TextView playerMetricsOverlayText;
    private TextView publisherPreviewHintText;
    private TextView publisherMetricsOverlayText;
    private TextView gbMetricsOverlayText;
    private View playerPreviewFrame;
    private View publisherPreviewFrame;
    private View gbPreviewFrame;
    private View publisherUrlRow;
    private LinearLayout playerActionRow;
    private LinearLayout publisherActionRow;
    private LinearLayout gbPreviewActionRow;
    private LinearLayout gbStartActionRow;
    private Button startPlayerPreviewButton;
    private Button stopPlayerPreviewButton;
    private Button startCameraPreviewButton;
    private Button stopCameraPreviewButton;
    private Button startPublisherPreviewButton;
    private Button stopPublisherPreviewButton;
    private Button shareLogsButton;
    private Button uploadLogsButton;
    private Button startGbPreviewButton;
    private Button stopGbPreviewButton;
    private Button toggleOnvifPanelButton;
    private Button searchOnvifButton;
    private Button applyOnvifStreamButton;
    private Button evaluatePublisherSourceButton;
    private Button runPublisherLocalCaptureButton;
    private Button stopPublisherLocalCaptureButton;
    private Button choosePublisherMediaFileButton;
    private Button requestDesktopCaptureButton;
    private Button stopDesktopCaptureButton;
    private Button chooseGbVideoFileButton;
    private Button startGb28181Button;
    private Button stopGb28181Button;
    private Spinner playerDecodeModeSpinner;
    private Spinner playerRenderBackendSpinner;
    private Spinner cameraFacingSpinner;
    private Spinner publisherSourceSpinner;
    private Spinner publisherAudioSpinner;
    private Spinner gbInviteSourceSpinner;
    private Spinner playerOnvifDeviceSpinner;
    private final List<Spinner> previewDisplayModeSpinners = new ArrayList<>();
    private Button languageButton;
    private EditText playerSourceUrlInput;
    private EditText playerBufferMsInput;
    private EditText playerAudioQueueInput;
    private EditText playerVideoQueueInput;
    private EditText playerOnvifUsernameInput;
    private EditText playerOnvifPasswordInput;
    private EditText cameraSourceIdInput;
    private EditText cameraFpsInput;
    private EditText desktopWidthInput;
    private EditText desktopHeightInput;
    private EditText desktopBitrateInput;
    private EditText desktopKeyIntervalInput;
    private EditText publisherPublishUrlInput;
    private EditText publisherMediaPathInput;
    private EditText publisherImagePathInput;
    private Spinner publisherVideoCodecSpinner;
    private Spinner publisherAudioCodecSpinner;
    private Spinner publisherResolutionSpinner;
    private EditText publisherVideoBitrateInput;
    private EditText publisherFpsInput;
    private EditText publisherGopInput;
    private EditText gbLocalIdInput;
    private EditText gbLocalDomainInput;
    private EditText gbUpperIdInput;
    private EditText gbUpperDomainInput;
    private EditText gbUpperPasswordInput;
    private EditText gbLocalIpInput;
    private EditText gbLocalPortInput;
    private EditText gbUpperIpInput;
    private EditText gbUpperPortInput;
    private Spinner gbTransportSpinner;
    private EditText gbMediaIpInput;
    private EditText gbMediaPortInput;
    private EditText gbRegisterExpiresInput;
    private EditText gbKeepaliveInput;
    private EditText gbMediaSourceInput;
    private ActivityResultLauncher<Intent> desktopCaptureGrantLauncher;
    private ActivityResultLauncher<String[]> cameraPermissionLauncher;
    private ActivityResultLauncher<String[]> publisherPermissionLauncher;
    private ActivityResultLauncher<String[]> publisherMediaFilePickerLauncher;
    private ActivityResultLauncher<String[]> gbPermissionLauncher;
    private ActivityResultLauncher<String[]> gbVideoFilePickerLauncher;

    private StreamCoreProductInfo productInfo;
    private StreamCoreRuntimeConfig runtimeConfig;
    private StreamCoreOperationStatus configureStatus;
    private StreamCoreOperationStatus logConfigureStatus;
    private StreamCoreRuntimeLicenseInfo licenseInfo;
    private StreamCoreLogInfo logInfo;
    private StreamCoreFeatureResult demoFeature;
    private StreamCoreLimitResult inputChannelLimit;
    private PlayerScenarioResult playerScenario;
    private CaptureScenarioResult cameraCaptureScenario;
    private List<PublisherScenarioResult> publisherScenarios = new ArrayList<>();
    private List<StreamCoreOnvif.Device> onvifDevices = new ArrayList<>();
    private File previewMediaFile;
    private File demoAudioFile;
    private File demoStillImageFile;
    private List<PublisherAudioChoice> publisherAudioChoices = new ArrayList<>();
    private List<CameraSourceOption> cameraSourceOptions = new ArrayList<>();
    private List<PublisherResolutionOption> publisherResolutionOptions = new ArrayList<>();
    private boolean updatingPublisherAudioChoices;

    private boolean demoInitialized;
    private boolean desktopCaptureStartInProgress;
    private boolean playerPreviewStartInProgress;
    private boolean cameraPreviewStartInProgress;
    private boolean publisherPreviewStartInProgress;
    private boolean publisherLocalCaptureRunInProgress;
    private boolean publisherPermissionRequestInProgress;
    private boolean publisherPermissionResumeDirectTransport;
    private boolean gb28181StartInProgress;
    private boolean gbPreviewStartInProgress;
    private boolean onvifSearchInProgress;
    private boolean onvifResolveInProgress;
    private boolean gb28181PermissionRequestInProgress;
    private boolean autorunDispatched;
    private String initializationFailureMessage = "";
    private String latestStatusAction = "startup";
    private String latestStatusCode = "-";
    private String latestStatusName = "pending";
    private String latestStatusSummary = "Runtime is loading.";
    private String latestLogZipPath = "";
    private int selectedPreviewDisplayModeIndex = PreviewDisplayMode.FIT.ordinal();
    private int selectedTabId = R.id.tab_publisher;
    private Runnable pendingCameraPermissionAction;
    private String pendingCameraPermissionScenario = "";
    private StreamCorePlayer.Session activePlayerPreviewSession;
    private StreamCoreCapture.Session activeCameraPreviewSession;
    private StreamCoreCapture.Session activePublisherPreviewSession;
    private StreamCorePublisher.Session activePublisherLocalCaptureSession;
    private StreamCoreCapture.Session activeDesktopCaptureSession;
    private StreamCoreCapture.Session activeGbPreviewSession;
    private StreamCoreGB28181.DeviceSession activeGb28181Session;
    private ScheduledExecutorService gb28181PollExecutor;
    private ScheduledFuture<?> gb28181PollFuture;
    private Uri selectedGbVideoUri;
    private int playerPreviewVideoWidth = DEFAULT_PREVIEW_SOURCE_WIDTH;
    private int playerPreviewVideoHeight = DEFAULT_PREVIEW_SOURCE_HEIGHT;
    private int gbVideoPreviewWidth = DEFAULT_PREVIEW_SOURCE_WIDTH;
    private int gbVideoPreviewHeight = DEFAULT_PREVIEW_SOURCE_HEIGHT;
    private String gb28181StatusText = "GB28181 idle.";
    private PlayerPreviewScenarioResult playerPreviewScenario =
            PlayerPreviewScenarioResult.idle();
    private CapturePreviewScenarioResult cameraPreviewScenario =
            CapturePreviewScenarioResult.idle("camera_preview");
    private CapturePreviewScenarioResult publisherPreviewScenario =
            CapturePreviewScenarioResult.idle("publisher_preview");
    private CapturePreviewScenarioResult gbPreviewScenario =
            CapturePreviewScenarioResult.idle("gb28181_preview");
    private DesktopCaptureScenarioResult desktopCaptureScenario =
            DesktopCaptureScenarioResult.idle();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        applyInitialLanguageFromIntent(getIntent());
        setContentView(R.layout.activity_main);

        bindTabViews();
        demoStatusText = findViewById(R.id.demo_status_text);
        licenseSummaryText = findViewById(R.id.license_summary_text);
        playerSummaryText = findViewById(R.id.player_summary_text);
        captureSummaryText = findViewById(R.id.capture_summary_text);
        desktopCaptureSummaryText = findViewById(R.id.desktop_capture_summary_text);
        publisherSummaryText = findViewById(R.id.publisher_summary_text);
        gb28181SummaryText = findViewById(R.id.gb28181_summary_text);
        playerOnvifControls = findViewById(R.id.player_onvif_controls);
        gbVideoFileRow = findViewById(R.id.gb_video_file_row);
        publisherCameraSourceRow = findViewById(R.id.publisher_camera_source_row);
        publisherMediaFileRow = findViewById(R.id.publisher_media_file_row);
        playerPreviewSurface = findViewById(R.id.player_preview_surface);
        cameraPreviewSurface = findViewById(R.id.camera_preview_surface);
        publisherPreviewSurface = findViewById(R.id.publisher_preview_surface);
        gbCameraPreviewSurface = findViewById(R.id.gb_camera_preview_surface);
        gbVideoPreviewView = findViewById(R.id.gb_video_preview_view);
        gbPreviewHintText = findViewById(R.id.gb_preview_hint_text);
        playerMetricsOverlayText = findViewById(R.id.player_metrics_overlay_text);
        publisherPreviewHintText = findViewById(R.id.publisher_preview_hint_text);
        publisherMetricsOverlayText = findViewById(R.id.publisher_metrics_overlay_text);
        gbMetricsOverlayText = findViewById(R.id.gb_metrics_overlay_text);
        playerPreviewFrame = findViewById(R.id.player_preview_frame);
        publisherPreviewFrame = findViewById(R.id.publisher_preview_frame);
        gbPreviewFrame = findViewById(R.id.gb_preview_frame);
        publisherUrlRow = findViewById(R.id.publisher_url_row);
        playerActionRow = findViewById(R.id.player_action_row);
        publisherActionRow = findViewById(R.id.publisher_action_row);
        gbPreviewActionRow = findViewById(R.id.gb_preview_action_row);
        gbStartActionRow = findViewById(R.id.gb_start_action_row);
        startPlayerPreviewButton = findViewById(R.id.start_player_preview_button);
        stopPlayerPreviewButton = findViewById(R.id.stop_player_preview_button);
        startCameraPreviewButton = findViewById(R.id.start_camera_preview_button);
        stopCameraPreviewButton = findViewById(R.id.stop_camera_preview_button);
        startPublisherPreviewButton = findViewById(R.id.start_publisher_preview_button);
        stopPublisherPreviewButton = findViewById(R.id.stop_publisher_preview_button);
        shareLogsButton = findViewById(R.id.share_logs_button);
        uploadLogsButton = findViewById(R.id.upload_logs_button);
        startGbPreviewButton = findViewById(R.id.start_gb_preview_button);
        stopGbPreviewButton = findViewById(R.id.stop_gb_preview_button);
        toggleOnvifPanelButton = findViewById(R.id.toggle_onvif_panel_button);
        searchOnvifButton = findViewById(R.id.search_onvif_button);
        applyOnvifStreamButton = findViewById(R.id.apply_onvif_stream_button);
        evaluatePublisherSourceButton =
                findViewById(R.id.evaluate_publisher_source_button);
        runPublisherLocalCaptureButton =
                findViewById(R.id.run_publisher_local_capture_button);
        stopPublisherLocalCaptureButton =
                findViewById(R.id.stop_publisher_local_capture_button);
        choosePublisherMediaFileButton =
                findViewById(R.id.choose_publisher_media_file_button);
        requestDesktopCaptureButton = findViewById(R.id.request_desktop_capture_button);
        stopDesktopCaptureButton = findViewById(R.id.stop_desktop_capture_button);
        chooseGbVideoFileButton = findViewById(R.id.choose_gb_video_file_button);
        startGb28181Button = findViewById(R.id.start_gb28181_button);
        stopGb28181Button = findViewById(R.id.stop_gb28181_button);
        playerDecodeModeSpinner = findViewById(R.id.player_decode_mode_spinner);
        playerRenderBackendSpinner = findViewById(R.id.player_render_backend_spinner);
        cameraFacingSpinner = findViewById(R.id.camera_facing_spinner);
        publisherSourceSpinner = findViewById(R.id.publisher_source_spinner);
        publisherAudioSpinner = findViewById(R.id.publisher_audio_spinner);
        gbInviteSourceSpinner = findViewById(R.id.gb_invite_source_spinner);
        playerOnvifDeviceSpinner = findViewById(R.id.player_onvif_device_spinner);
        previewDisplayModeSpinners.clear();
        previewDisplayModeSpinners.add(findViewById(R.id.player_preview_display_mode_spinner));
        previewDisplayModeSpinners.add(findViewById(R.id.camera_preview_display_mode_spinner));
        previewDisplayModeSpinners.add(findViewById(R.id.publisher_preview_display_mode_spinner));
        previewDisplayModeSpinners.add(findViewById(R.id.gb_preview_display_mode_spinner));
        languageButton = findViewById(R.id.language_button);
        playerSourceUrlInput = findViewById(R.id.player_source_url_input);
        playerBufferMsInput = findViewById(R.id.player_buffer_ms_input);
        playerAudioQueueInput = findViewById(R.id.player_audio_queue_input);
        playerVideoQueueInput = findViewById(R.id.player_video_queue_input);
        playerOnvifUsernameInput = findViewById(R.id.player_onvif_username_input);
        playerOnvifPasswordInput = findViewById(R.id.player_onvif_password_input);
        cameraSourceIdInput = findViewById(R.id.camera_source_id_input);
        cameraFpsInput = findViewById(R.id.camera_fps_input);
        desktopWidthInput = findViewById(R.id.desktop_width_input);
        desktopHeightInput = findViewById(R.id.desktop_height_input);
        desktopBitrateInput = findViewById(R.id.desktop_bitrate_input);
        desktopKeyIntervalInput = findViewById(R.id.desktop_key_interval_input);
        publisherPublishUrlInput = findViewById(R.id.publisher_publish_url_input);
        publisherMediaPathInput = findViewById(R.id.publisher_media_path_input);
        publisherImagePathInput = findViewById(R.id.publisher_image_path_input);
        publisherVideoCodecSpinner = findViewById(R.id.publisher_video_codec_input);
        publisherAudioCodecSpinner = findViewById(R.id.publisher_audio_codec_input);
        publisherResolutionSpinner = findViewById(R.id.publisher_video_resolution_input);
        publisherVideoBitrateInput = findViewById(R.id.publisher_video_bitrate_input);
        publisherFpsInput = findViewById(R.id.publisher_fps_input);
        publisherGopInput = findViewById(R.id.publisher_gop_input);
        gbLocalIdInput = findViewById(R.id.gb_local_id_input);
        gbLocalDomainInput = findViewById(R.id.gb_local_domain_input);
        gbUpperIdInput = findViewById(R.id.gb_upper_id_input);
        gbUpperDomainInput = findViewById(R.id.gb_upper_domain_input);
        gbUpperPasswordInput = findViewById(R.id.gb_upper_password_input);
        gbLocalIpInput = findViewById(R.id.gb_local_ip_input);
        gbLocalPortInput = findViewById(R.id.gb_local_port_input);
        gbUpperIpInput = findViewById(R.id.gb_upper_ip_input);
        gbUpperPortInput = findViewById(R.id.gb_upper_port_input);
        gbTransportSpinner = findViewById(R.id.gb_transport_spinner);
        gbMediaIpInput = findViewById(R.id.gb_media_ip_input);
        gbMediaPortInput = findViewById(R.id.gb_media_port_input);
        gbRegisterExpiresInput = findViewById(R.id.gb_register_expires_input);
        gbKeepaliveInput = findViewById(R.id.gb_keepalive_input);
        gbMediaSourceInput = findViewById(R.id.gb_media_source_input);
        publisherActionStatusText = findViewById(R.id.publisher_action_status_text);
        initializeDefaultEndpointInputs();
        gb28181StatusText = getString(R.string.gb_preview_hint);
        applyDemoTextSizes(findViewById(android.R.id.content));
        configureLanguageSwitcher();
        configurePrimaryPanelLayout();
        installMobilePreviewAspectRatio();
        configureParameterControls();
        desktopCaptureGrantLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                result -> handleDesktopCaptureGrantResult(result.getResultCode(), result.getData()));
        cameraPermissionLauncher = registerForActivityResult(
                new ActivityResultContracts.RequestMultiplePermissions(),
                this::handleCameraPermissionResult);
        publisherPermissionLauncher = registerForActivityResult(
                new ActivityResultContracts.RequestMultiplePermissions(),
                this::handlePublisherPermissionResult);
        publisherMediaFilePickerLauncher = registerForActivityResult(
                new ActivityResultContracts.OpenDocument(),
                this::handlePublisherMediaFilePicked);
        gbPermissionLauncher = registerForActivityResult(
                new ActivityResultContracts.RequestMultiplePermissions(),
                this::handleGbPermissionResult);
        gbVideoFilePickerLauncher = registerForActivityResult(
                new ActivityResultContracts.OpenDocument(),
                this::handleGbVideoFilePicked);

        installSurfaceLifecycleCallbacks();
        installGbVideoPreviewCallbacks();
        startPlayerPreviewButton.setOnClickListener(view -> {
            if (activePlayerPreviewSession != null) {
                stopPlayerPreview();
            } else {
                startPlayerPreview();
            }
        });
        stopPlayerPreviewButton.setOnClickListener(view -> stopPlayerPreview());
        toggleOnvifPanelButton.setOnClickListener(view -> toggleOnvifPanel());
        searchOnvifButton.setOnClickListener(view -> searchOnvifDevices());
        applyOnvifStreamButton.setOnClickListener(view -> applySelectedOnvifStream());
        startCameraPreviewButton.setOnClickListener(view -> startCameraPreview());
        stopCameraPreviewButton.setOnClickListener(view -> stopCameraPreview());
        startPublisherPreviewButton.setOnClickListener(view -> {
            if (activePublisherPreviewSession != null) {
                stopPublisherPreview();
            } else {
                startPublisherPreview();
            }
        });
        stopPublisherPreviewButton.setOnClickListener(view -> stopPublisherPreview());
        shareLogsButton.setOnClickListener(view -> shareLogPackage());
        uploadLogsButton.setOnClickListener(view -> showLogUploadReserved());
        evaluatePublisherSourceButton.setOnClickListener(
                view -> evaluateSelectedPublisherSource());
        runPublisherLocalCaptureButton.setOnClickListener(view -> {
            if (activePublisherLocalCaptureSession != null) {
                stopPublisherLocalCaptureNativeTransport();
                return;
            }
            if (activePublisherPreviewSession != null) {
                stopPublisherPreview();
                return;
            }
            final boolean directPublishSource =
                    selectedPublisherSourceChoice() == PublisherSourceChoice.CAMERA_MICROPHONE
                            && selectedPublisherAudioChoice() == PublisherAudioChoice.MICROPHONE;
            if (directPublishSource) {
                runPublisherLocalCaptureNativeTransport();
            } else {
                runSelectedPublisherTransport();
            }
        });
        stopPublisherLocalCaptureButton.setOnClickListener(
                view -> stopPublisherLocalCaptureNativeTransport());
        choosePublisherMediaFileButton.setOnClickListener(
                view -> openPublisherMediaFilePicker());
        requestDesktopCaptureButton.setOnClickListener(view -> requestDesktopCapturePermission());
        stopDesktopCaptureButton.setOnClickListener(view -> stopDesktopCapture());
        chooseGbVideoFileButton.setOnClickListener(view -> openGbVideoFilePicker());
        gbMediaSourceInput.setOnClickListener(view -> openGbVideoFilePicker());
        gbMediaSourceInput.setFocusable(false);
        startGbPreviewButton.setOnClickListener(view -> {
            if (activeGbPreviewSession != null || isGbVideoPreviewPlaying()) {
                stopGbSourcePreview();
            } else {
                startGbSourcePreview();
            }
        });
        stopGbPreviewButton.setOnClickListener(view -> stopGbSourcePreview());
        startGb28181Button.setOnClickListener(view -> {
            if (activeGb28181Session != null) {
                stopGb28181Device();
                return;
            }
            if (activeGbPreviewSession != null || isGbVideoPreviewPlaying()) {
                stopGbSourcePreview();
                return;
            }
            startGb28181Device();
        });
        stopGb28181Button.setOnClickListener(view -> stopGb28181Device());

        startDemoInitialization();
        renderInfoText();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        applyInitialLanguageFromIntent(intent);
        autorunDispatched = false;
        runAutorunIfRequested();
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        initializeDefaultEndpointInputs();
        renderInfoText();
    }

    @Override
    protected void onDestroy() {
        releaseVisualPreviewSessions();
        releasePublisherLocalCaptureSession();
        releaseDesktopCaptureSession();
        releaseGb28181Session();
        super.onDestroy();
    }

    private void runAutorunIfRequested() {
        if (autorunDispatched || !demoInitialized || !initializationFailureMessage.isEmpty()) {
            return;
        }
        final Intent intent = getIntent();
        final String mode = intent == null ? "" : intent.getStringExtra(EXTRA_AUTORUN);
        if (TextUtils.isEmpty(mode)) {
            return;
        }

        autorunDispatched = true;
        applyAutorunInputs(intent);
        recordDemoStatus(
                "autorun." + mode,
                "0",
                "starting",
                "Android demo autorun started for " + mode + '.');
        renderInfoText();
        if (AUTORUN_PLAYER.equals(mode)) {
            runPlayerAutorun(intent);
        } else if (AUTORUN_PUBLISHER_MATRIX.equals(mode)) {
            runPublisherMatrixAutorun(intent);
        } else if (AUTORUN_PUBLISHER_LOCAL_CAPTURE.equals(mode)) {
            runPublisherLocalCaptureAutorun(intent);
        } else if (AUTORUN_PUBLISHER_PREVIEW.equals(mode)) {
            runPublisherPreviewAutorun(intent);
        } else if (AUTORUN_CAMERA_PREVIEW.equals(mode)) {
            runCameraPreviewAutorun(intent);
        } else if (AUTORUN_GB28181.equals(mode)) {
            runGb28181Autorun(intent);
        } else if (AUTORUN_LOGS.equals(mode)) {
            runLogPackageAutorun(intent);
        } else if (AUTORUN_UPLOAD.equals(mode)) {
            showLogUploadReserved();
            scheduleAutorunFinish(intent, null);
        } else {
            recordDemoStatus(
                    "autorun." + mode,
                    "-1",
                    "unknown",
                    "Unknown Android demo autorun mode: " + mode);
            renderDemoStatusText();
            scheduleAutorunFinish(intent, null);
        }
    }

    private void applyAutorunInputs(Intent intent) {
        final String playerUrl = readIntentString(intent, EXTRA_PLAYER_URL);
        if (!playerUrl.isEmpty()) {
            playerSourceUrlInput.setText(playerUrl);
        }
        final String publishUrl = readIntentString(intent, EXTRA_PUBLISH_URL);
        if (!publishUrl.isEmpty()) {
            publisherPublishUrlInput.setText(publishUrl);
        }
        final String gbUpperIp = readIntentString(intent, EXTRA_GB_UPPER_IP);
        if (!gbUpperIp.isEmpty()) {
            gbUpperIpInput.setText(gbUpperIp);
        }
        final String gbUpperPort = readIntentString(intent, EXTRA_GB_UPPER_PORT);
        if (!gbUpperPort.isEmpty()) {
            gbUpperPortInput.setText(gbUpperPort);
        }
        setSpinnerSelectionFromIntent(intent, playerDecodeModeSpinner, EXTRA_PLAYER_DECODE_INDEX);
        setSpinnerSelectionFromIntent(intent, playerRenderBackendSpinner, EXTRA_PLAYER_RENDER_INDEX);
        setSpinnerSelectionFromIntent(intent, publisherSourceSpinner, EXTRA_PUBLISHER_SOURCE_INDEX);
        setSpinnerSelectionFromIntent(intent, gbInviteSourceSpinner, EXTRA_GB_SOURCE_INDEX);
        final int displayModeIndex = intent.getIntExtra(EXTRA_DISPLAY_MODE_INDEX, -1);
        if (displayModeIndex >= 0) {
            updatePreviewDisplayModeSelection(displayModeIndex);
        }
        applyPublisherSourceUiState();
        applyGb28181SourceUiState();
        renderInfoText();
    }

    private static String readIntentString(Intent intent, String name) {
        final String value = intent == null ? "" : intent.getStringExtra(name);
        return value == null ? "" : value.trim();
    }

    private static void setSpinnerSelectionFromIntent(
            Intent intent,
            Spinner spinner,
            String name) {
        if (intent == null || spinner == null || !intent.hasExtra(name)) {
            return;
        }
        final int position = intent.getIntExtra(name, -1);
        if (position >= 0 && position < spinner.getCount()) {
            spinner.setSelection(position);
        }
    }

    private void runPlayerAutorun(Intent intent) {
        selectTab(R.id.tab_player);
        startPlayerAutorunWhenReady(0);
        scheduleAutorunFinish(intent, this::stopPlayerPreview);
    }

    private void startPlayerAutorunWhenReady(int attempt) {
        if (hasUsableSurface(playerPreviewSurface)) {
            startPlayerPreview();
            return;
        }
        if (attempt >= AUTORUN_SURFACE_RETRY_LIMIT) {
            recordDemoStatus(
                    "autorun.player",
                    "-1",
                    "surface_unavailable",
                    "Player TextureView did not become ready before autorun timeout.");
            renderInfoText();
            return;
        }
        playerPreviewSurface.postDelayed(
                () -> startPlayerAutorunWhenReady(attempt + 1),
                AUTORUN_SURFACE_RETRY_DELAY_MS);
    }

    private void runPublisherMatrixAutorun(Intent intent) {
        selectTab(R.id.tab_publisher);
        updateGeneratedPublisherPathInputs();
        for (int index = 0; index < PublisherSourceChoice.values().length; ++index) {
            publisherSourceSpinner.setSelection(index);
            applyPublisherSourceUiState();
            evaluateSelectedPublisherSource();
        }
        recordDemoStatus(
                "autorun.publisher_matrix",
                "0",
                "checked",
                "Publisher source preflight matrix completed.");
        renderInfoText();
        scheduleAutorunFinish(intent, null);
    }

    private void runPublisherLocalCaptureAutorun(Intent intent) {
        selectTab(R.id.tab_publisher);
        publisherSourceSpinner.setSelection(PublisherSourceChoice.CAMERA_MICROPHONE.ordinal());
        applyPublisherSourceUiState();
        final int microphoneIndex = publisherAudioChoices.indexOf(PublisherAudioChoice.MICROPHONE);
        if (microphoneIndex >= 0) {
            publisherAudioSpinner.setSelection(microphoneIndex);
            applyPublisherSourceUiState();
        }
        runPublisherLocalCaptureNativeTransport();
        scheduleAutorunFinish(intent, this::stopPublisherLocalCaptureNativeTransport);
    }

    private void runPublisherPreviewAutorun(Intent intent) {
        selectTab(R.id.tab_publisher);
        publisherSourceSpinner.setSelection(PublisherSourceChoice.CAMERA_MICROPHONE.ordinal());
        applyPublisherSourceUiState();
        startPublisherPreviewAutorunWhenReady(0);
        scheduleAutorunFinish(intent, this::stopPublisherPreview);
    }

    private void startPublisherPreviewAutorunWhenReady(int attempt) {
        if (hasUsableSurface(publisherPreviewSurface)) {
            startPublisherPreview();
            return;
        }
        if (attempt >= AUTORUN_SURFACE_RETRY_LIMIT) {
            recordDemoStatus(
                    "autorun.publisher_preview",
                    "-1",
                    "surface_unavailable",
                    "Publisher TextureView did not become ready before autorun timeout.");
            renderInfoText();
            return;
        }
        publisherPreviewSurface.postDelayed(
                () -> startPublisherPreviewAutorunWhenReady(attempt + 1),
                AUTORUN_SURFACE_RETRY_DELAY_MS);
    }

    private void runCameraPreviewAutorun(Intent intent) {
        selectTab(R.id.tab_publisher);
        publisherSourceSpinner.setSelection(PublisherSourceChoice.CAMERA_MICROPHONE.ordinal());
        applyPublisherSourceUiState();
        startPublisherPreviewAutorunWhenReady(0);
        scheduleAutorunFinish(intent, this::stopPublisherPreview);
    }

    private void startCameraPreviewAutorunWhenReady(int attempt) {
        if (hasUsableSurface(cameraPreviewSurface)) {
            startCameraPreview();
            return;
        }
        if (attempt >= AUTORUN_SURFACE_RETRY_LIMIT) {
            recordDemoStatus(
                    "autorun.camera_preview",
                    "-1",
                    "surface_unavailable",
                    "Camera TextureView did not become ready before autorun timeout.");
            renderInfoText();
            return;
        }
        cameraPreviewSurface.postDelayed(
                () -> startCameraPreviewAutorunWhenReady(attempt + 1),
                AUTORUN_SURFACE_RETRY_DELAY_MS);
    }

    private void runGb28181Autorun(Intent intent) {
        selectTab(R.id.tab_gb28181);
        startGb28181Device();
        scheduleAutorunFinish(intent, this::stopGb28181Device);
    }

    private void runLogPackageAutorun(Intent intent) {
        try {
            final File zipFile = createLogPackageZip();
            latestLogZipPath = zipFile.getAbsolutePath();
            recordDemoStatus(
                    "logs.package",
                    "0",
                    "ready",
                    zipFile.getAbsolutePath());
        } catch (IOException | RuntimeException failure) {
            final String message = failure.getMessage() == null
                    ? failure.getClass().getSimpleName()
                    : failure.getMessage();
            recordDemoStatus("logs.package", "-1", "failed", message);
        }
        renderDemoStatusText();
        scheduleAutorunFinish(intent, null);
    }

    private void scheduleAutorunFinish(Intent intent, Runnable beforeFinish) {
        final int quitMs = intent == null ? 0 : intent.getIntExtra(EXTRA_QUIT_MS, 0);
        if (quitMs <= 0) {
            return;
        }
        final View rootView = getWindow().getDecorView();
        rootView.postDelayed(() -> {
            if (beforeFinish != null) {
                beforeFinish.run();
            }
            recordDemoStatus(
                    "autorun.finish",
                    "0",
                    "done",
                    "Android demo autorun finish requested.");
            renderDemoStatusText();
            rootView.postDelayed(this::finish, 700);
        }, quitMs);
    }

    private void configureLanguageSwitcher() {
        updateLanguageButtonText();
        languageButton.setOnClickListener(this::showLanguageSelectionMenu);
    }

    private void applyInitialLanguageFromIntent(Intent intent) {
        final String value = readIntentString(intent, EXTRA_LANGUAGE).toLowerCase(Locale.ROOT);
        if (value.isEmpty()) {
            return;
        }
        if ("en".equals(value) || "english".equals(value)) {
            applyLanguageSelection(1);
        } else if ("zh".equals(value) || "cn".equals(value) || "chinese".equals(value)) {
            applyLanguageSelection(2);
        } else if ("auto".equals(value) || "system".equals(value)) {
            applyLanguageSelection(0);
        }
    }

    private void showLanguageSelectionMenu(View anchor) {
        if (anchor == null) {
            return;
        }
        final PopupMenu popupMenu = new PopupMenu(this, anchor);
        popupMenu.getMenu().add(0, 0, 0, getString(R.string.language_auto));
        popupMenu.getMenu().add(0, 1, 1, getString(R.string.language_english));
        popupMenu.getMenu().add(0, 2, 2, getString(R.string.language_chinese));
        popupMenu.setOnMenuItemClickListener(item -> {
            applyLanguageSelection(item.getItemId());
            updateLanguageButtonText();
            return true;
        });
        popupMenu.show();
    }

    private int currentLanguageSelectionIndex() {
        final LocaleListCompat locales = AppCompatDelegate.getApplicationLocales();
        if (locales.isEmpty()) {
            return 0;
        }
        final Locale locale = locales.get(0);
        if (locale == null) {
            return 0;
        }
        final String language = locale.getLanguage();
        if (Locale.CHINESE.getLanguage().equals(language)) {
            return 2;
        }
        if (Locale.ENGLISH.getLanguage().equals(language)) {
            return 1;
        }
        return 0;
    }

    private void applyLanguageSelection(int position) {
        final String languageTags;
        if (position == 1) {
            languageTags = "en";
        } else if (position == 2) {
            languageTags = "zh";
        } else {
            languageTags = "";
        }
        final LocaleListCompat targetLocales =
                LocaleListCompat.forLanguageTags(languageTags);
        if (!targetLocales.equals(AppCompatDelegate.getApplicationLocales())) {
            AppCompatDelegate.setApplicationLocales(targetLocales);
        }
    }

    private void updateLanguageButtonText() {
        if (languageButton == null) {
            return;
        }
        final int labelResId;
        switch (currentLanguageSelectionIndex()) {
            case 1:
                labelResId = R.string.language_english;
                break;
            case 2:
                labelResId = R.string.language_chinese;
                break;
            default:
                labelResId = R.string.language_auto;
                break;
        }
        languageButton.setText(labelResId);
    }

    private void configurePrimaryPanelLayout() {
        movePanelChildToIndex(playerPanel, playerPreviewFrame, 1);
        movePanelChildToIndex(playerPanel, playerActionRow, 2);
        moveDisplayModeSpinnerToRow(
                playerActionRow,
                findViewById(R.id.player_preview_display_mode_spinner));

        movePanelChildToIndex(publisherPanel, publisherPreviewFrame, 1);
        movePanelChildToIndex(publisherPanel, publisherActionRow, 2);
        movePanelChildToIndex(publisherPanel, publisherUrlRow, 3);
        moveDisplayModeSpinnerToRow(
                publisherActionRow,
                findViewById(R.id.publisher_preview_display_mode_spinner));

        movePanelChildToIndex(gb28181Panel, gbPreviewFrame, 0);
        movePanelChildToIndex(gb28181Panel, gbPreviewActionRow, 1);
        moveActionButtonToRow(gbPreviewActionRow, startGb28181Button);
        moveDisplayModeSpinnerToRow(
                gbPreviewActionRow,
                findViewById(R.id.gb_preview_display_mode_spinner));

        hideOptionalAction(stopPlayerPreviewButton);
        hideOptionalAction(startPublisherPreviewButton);
        hideOptionalAction(stopPublisherPreviewButton);
        hideOptionalAction(evaluatePublisherSourceButton);
        hideOptionalAction(stopPublisherLocalCaptureButton);
        hideOptionalAction(startGbPreviewButton);
        hideOptionalAction(stopGbPreviewButton);
        hideOptionalAction(stopGb28181Button);
        if (gbStartActionRow != null) {
            gbStartActionRow.setVisibility(View.GONE);
        }
        if (publisherActionStatusText != null) {
            publisherActionStatusText.setVisibility(View.GONE);
        }
        hideMobileMediaSummaryViews();
    }

    private void installMobilePreviewAspectRatio() {
        installMobilePreviewAspectRatio(playerPreviewFrame);
        if (cameraPreviewSurface != null && cameraPreviewSurface.getParent() instanceof View) {
            installMobilePreviewAspectRatio((View) cameraPreviewSurface.getParent());
        }
        installMobilePreviewAspectRatio(publisherPreviewFrame);
        installMobilePreviewAspectRatio(gbPreviewFrame);
    }

    private void installMobilePreviewAspectRatio(View previewFrame) {
        if (previewFrame == null) {
            return;
        }

        previewFrame.addOnLayoutChangeListener((view, left, top, right, bottom,
                                                oldLeft, oldTop, oldRight, oldBottom) ->
                applyMobilePreviewAspectRatio(view));
        previewFrame.post(() -> applyMobilePreviewAspectRatio(previewFrame));
    }

    private void applyMobilePreviewAspectRatio(View previewFrame) {
        final int width = previewFrame.getWidth();
        if (width <= 0) {
            return;
        }

        final int targetHeight = Math.round(width * MOBILE_PREVIEW_HEIGHT_RATIO);
        final ViewGroup.LayoutParams layoutParams = previewFrame.getLayoutParams();
        if (layoutParams == null || layoutParams.height == targetHeight) {
            return;
        }

        layoutParams.height = targetHeight;
        previewFrame.setLayoutParams(layoutParams);
    }

    private void hideMobileMediaSummaryViews() {
        if (playerSummaryText != null) {
            playerSummaryText.setVisibility(View.GONE);
        }
        if (publisherSummaryText != null) {
            publisherSummaryText.setVisibility(View.GONE);
        }
        if (publisherActionStatusText != null) {
            publisherActionStatusText.setVisibility(View.GONE);
        }
        if (gb28181SummaryText != null) {
            gb28181SummaryText.setVisibility(View.GONE);
        }
    }

    private void movePanelChildToIndex(View panel, View child, int index) {
        if (!(panel instanceof LinearLayout) || child == null) {
            return;
        }
        final LinearLayout parent = (LinearLayout) panel;
        final ViewGroup currentParent = (ViewGroup) child.getParent();
        if (currentParent != null) {
            currentParent.removeView(child);
        }
        parent.addView(child, Math.max(0, Math.min(index, parent.getChildCount())));
    }

    private void moveDisplayModeSpinnerToRow(LinearLayout row, View spinner) {
        if (row == null || spinner == null) {
            return;
        }
        moveActionButtonToRow(row, spinner);
        final LinearLayout.LayoutParams params =
                new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1.1f);
        params.setMarginStart(dp(6));
        spinner.setLayoutParams(params);
    }

    private void moveActionButtonToRow(LinearLayout row, View action) {
        if (row == null || action == null) {
            return;
        }
        final ViewGroup currentParent = (ViewGroup) action.getParent();
        if (currentParent != null) {
            currentParent.removeView(action);
        }
        row.addView(action);
        action.setVisibility(View.VISIBLE);
    }

    private void hideOptionalAction(View view) {
        if (view != null) {
            view.setVisibility(View.GONE);
        }
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private void applyDemoTextSizes(View view) {
        if (view == null || view == languageButton) {
            return;
        }
        if (view instanceof Button || view instanceof EditText) {
            ((TextView) view).setTextSize(PARAMETER_CONTROL_TEXT_SIZE_SP);
        } else if (view instanceof TextView) {
            final int id = view.getId();
            final float targetSize = id == R.id.demo_status_text
                    || id == R.id.license_summary_text
                    || id == R.id.player_summary_text
                    || id == R.id.capture_summary_text
                    || id == R.id.desktop_capture_summary_text
                    || id == R.id.publisher_action_status_text
                    || id == R.id.publisher_summary_text
                    || id == R.id.gb28181_summary_text
                    ? PARAMETER_LOG_TEXT_SIZE_SP
                    : PARAMETER_LABEL_TEXT_SIZE_SP;
            ((TextView) view).setTextSize(targetSize);
        }
        if (view instanceof ViewGroup) {
            final ViewGroup group = (ViewGroup) view;
            for (int index = 0; index < group.getChildCount(); ++index) {
                applyDemoTextSizes(group.getChildAt(index));
            }
        }
    }

    private void configureParameterControls() {
        final ArrayAdapter<String> decodeAdapter = buildSpinnerAdapter(
                getString(R.string.player_decode_software_option),
                getString(R.string.player_decode_hardware_option));
        playerDecodeModeSpinner.setAdapter(decodeAdapter);
        playerDecodeModeSpinner.setSelection(0);
        final ArrayAdapter<String> renderAdapter = buildSpinnerAdapter(
                getString(R.string.player_render_software_frame_option),
                getString(R.string.player_render_gpu_frame_option),
                getString(R.string.player_render_direct_surface_option),
                getString(R.string.player_render_sdk_auto_option));
        playerRenderBackendSpinner.setAdapter(renderAdapter);
        playerRenderBackendSpinner.setSelection(0);
        final ArrayAdapter<String> displayModeAdapter = buildSpinnerAdapter(
                getString(R.string.preview_display_stretch_option),
                getString(R.string.preview_display_fit_option),
                getString(R.string.preview_display_crop_option));
        for (Spinner spinner : previewDisplayModeSpinners) {
            if (spinner == null) {
                continue;
            }
            spinner.setAdapter(displayModeAdapter);
            spinner.setSelection(selectedPreviewDisplayModeIndex, false);
            spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(
                        AdapterView<?> parent,
                        View view,
                        int position,
                        long id) {
                    updatePreviewDisplayModeSelection(position);
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                    updatePreviewDisplayModeSelection(PreviewDisplayMode.FIT.ordinal());
                }
            });
        }
        playerOnvifDeviceSpinner.setAdapter(
                buildSpinnerAdapter(getString(R.string.player_onvif_empty)));
        playerOnvifDeviceSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                updateOnvifStatusForSelection();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                updateOnvifStatusForSelection();
            }
        });

        cameraSourceOptions = buildCameraSourceOptions();
        final List<String> cameraLabels = new ArrayList<>();
        for (CameraSourceOption option : cameraSourceOptions) {
            cameraLabels.add(option.label);
        }
        final ArrayAdapter<String> cameraAdapter = buildSpinnerAdapter(cameraLabels);
        cameraFacingSpinner.setAdapter(cameraAdapter);
        cameraFacingSpinner.setSelection(defaultCameraSelectionIndex());
        cameraFacingSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                refreshPublisherResolutionOptions(selectedPublisherSourceChoice());
                renderInfoText();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                renderInfoText();
            }
        });

        final ArrayAdapter<String> sourceAdapter = buildSpinnerAdapter(
                getString(R.string.publisher_source_camera_microphone),
                getString(R.string.publisher_source_desktop),
                getString(R.string.publisher_source_video_file),
                getString(R.string.publisher_source_image),
                getString(R.string.publisher_source_none));
        publisherSourceSpinner.setAdapter(sourceAdapter);
        publisherSourceSpinner.setSelection(0);
        publisherSourceSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                applyPublisherSourceUiState();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                applyPublisherSourceUiState();
            }
        });
        publisherAudioSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (!updatingPublisherAudioChoices) {
                    applyPublisherSourceUiState();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                if (!updatingPublisherAudioChoices) {
                    applyPublisherSourceUiState();
                }
            }
        });
        publisherVideoCodecSpinner.setAdapter(buildSpinnerAdapter("H.264", "H.265 / HEVC"));
        publisherVideoCodecSpinner.setSelection(0);
        publisherVideoCodecSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                renderInfoText();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                renderInfoText();
            }
        });
        publisherAudioCodecSpinner.setAdapter(buildSpinnerAdapter("AAC"));
        publisherAudioCodecSpinner.setSelection(0);
        publisherAudioCodecSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                renderInfoText();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                renderInfoText();
            }
        });
        publisherResolutionSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                applyPreviewDisplayMode();
                renderInfoText();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                applyPreviewDisplayMode();
                renderInfoText();
            }
        });
        final ArrayAdapter<String> gbSourceAdapter = buildSpinnerAdapter(
                getString(R.string.gb_source_camera_microphone),
                getString(R.string.gb_source_video_file));
        gbInviteSourceSpinner.setAdapter(gbSourceAdapter);
        gbInviteSourceSpinner.setSelection(0);
        gbInviteSourceSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                applyGb28181SourceUiState();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                applyGb28181SourceUiState();
            }
        });
        if (gbTransportSpinner != null) {
            gbTransportSpinner.setAdapter(buildSpinnerAdapter(
                    getString(R.string.gb_transport_tcp),
                    getString(R.string.gb_transport_udp)));
            gbTransportSpinner.setSelection(0);
        }
        applyPublisherSourceUiState();
        applyGb28181SourceUiState();
        if (!previewDisplayModeSpinners.isEmpty()
                && previewDisplayModeSpinners.get(0) != null) {
            previewDisplayModeSpinners.get(0).post(this::applyPreviewDisplayMode);
        }
    }

    private void updatePreviewDisplayModeSelection(int position) {
        final int safePosition;
        if (position == PreviewDisplayMode.STRETCH.ordinal()
                || position == PreviewDisplayMode.CROP.ordinal()) {
            safePosition = position;
        } else {
            safePosition = PreviewDisplayMode.FIT.ordinal();
        }
        if (selectedPreviewDisplayModeIndex != safePosition) {
            selectedPreviewDisplayModeIndex = safePosition;
        }
        for (Spinner spinner : previewDisplayModeSpinners) {
            if (spinner != null && spinner.getSelectedItemPosition() != safePosition) {
                spinner.setSelection(safePosition, false);
            }
        }
        applyPreviewDisplayMode();
        renderInfoText();
    }

    private ArrayAdapter<String> buildSpinnerAdapter(String... labels) {
        final List<String> values = new ArrayList<>(labels.length);
        Collections.addAll(values, labels);
        return buildSpinnerAdapter(values);
    }

    private ArrayAdapter<String> buildSpinnerAdapter(List<String> labels) {
        final ArrayAdapter<String> adapter = new ArrayAdapter<String>(
                this,
                0,
                labels) {
            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                return createSpinnerTextView(getItem(position), false);
            }

            @Override
            public View getDropDownView(int position, View convertView, ViewGroup parent) {
                return createSpinnerTextView(getItem(position), true);
            }
        };
        return adapter;
    }

    private TextView createSpinnerTextView(String text, boolean dropdown) {
        final TextView textView = new TextView(this);
        final int horizontalPadding = dpToPx(SPINNER_ROW_HORIZONTAL_PADDING_DP);
        final int verticalPadding = dpToPx(SPINNER_ROW_VERTICAL_PADDING_DP);
        textView.setText(text == null ? "" : text);
        textView.setTextColor(TAB_TEXT_IDLE);
        textView.setTextSize(PARAMETER_CONTROL_TEXT_SIZE_SP);
        textView.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
        textView.setSingleLine(true);
        textView.setEllipsize(TextUtils.TruncateAt.END);
        textView.setIncludeFontPadding(false);
        textView.setMinHeight(dpToPx(dropdown ? 44 : 42));
        textView.setPadding(
                horizontalPadding,
                verticalPadding,
                horizontalPadding,
                verticalPadding);
        return textView;
    }

    private int dpToPx(int dp) {
        return Math.round(dp * getResources().getDisplayMetrics().density);
    }

    private void initializeDefaultEndpointInputs() {
        initializeRtmpEndpointInput(playerSourceUrlInput, defaultPlayerSourceUrl());
        initializeRtmpEndpointInput(publisherPublishUrlInput, defaultSelectedPublisherUrl());
        if (gbLocalIpInput != null && gbLocalIpInput.getText().toString().trim().isEmpty()) {
            gbLocalIpInput.setText("0.0.0.0");
        }
        if (gbUpperIpInput != null && gbUpperIpInput.getText().toString().trim().isEmpty()) {
            gbUpperIpInput.setText(DEFAULT_DEVELOPMENT_RTMP_HOST);
        }
        if (gbUpperPortInput != null && gbUpperPortInput.getText().toString().trim().isEmpty()) {
            gbUpperPortInput.setText("5060");
        }
        if (gbMediaIpInput != null && gbMediaIpInput.getText().toString().trim().isEmpty()) {
            gbMediaIpInput.setText(resolveFirstNonLoopbackIpv4("0.0.0.0"));
        }
    }

    private static void initializeRtmpEndpointInput(EditText input, String defaultUrl) {
        if (input == null) {
            return;
        }

        final String currentUrl = input.getText().toString().trim();
        if (currentUrl.isEmpty() || isLegacyLoopbackRtmpEndpoint(currentUrl)) {
            input.setText(defaultUrl);
        }
    }

    // 旧版 demo 曾把 adb reverse 的 loopback 写进可编辑 RTMP 字段；启动时主动迁移，避免继续误导真机用户。
    private static boolean isLegacyLoopbackRtmpEndpoint(String url) {
        try {
            final URI uri = new URI(url);
            final String host = uri.getHost();
            return "rtmp".equalsIgnoreCase(uri.getScheme())
                    && ("127.0.0.1".equals(host)
                    || "localhost".equalsIgnoreCase(host)
                    || "::1".equals(host));
        } catch (URISyntaxException failure) {
            return false;
        }
    }

    private List<CameraSourceOption> buildCameraSourceOptions() {
        final List<CameraSourceOption> options = new ArrayList<>();
        final CameraManager cameraManager = getSystemService(CameraManager.class);
        if (cameraManager != null) {
            try {
                for (String cameraId : cameraManager.getCameraIdList()) {
                    final CameraCharacteristics characteristics =
                            cameraManager.getCameraCharacteristics(cameraId);
                    final Integer facing =
                            characteristics.get(CameraCharacteristics.LENS_FACING);
                    final boolean frontFacing = facing != null
                            && facing == CameraCharacteristics.LENS_FACING_FRONT;
                    final String facingLabel = frontFacing
                            ? getString(R.string.camera_facing_front)
                            : (facing != null
                            && facing == CameraCharacteristics.LENS_FACING_BACK
                            ? getString(R.string.camera_facing_back)
                            : "External camera");
                    options.add(new CameraSourceOption(
                            facingLabel + " / camera2:" + cameraId,
                            "camera2:" + cameraId,
                            frontFacing,
                            cameraResolutionOptions(characteristics)));
                }
            } catch (CameraAccessException | RuntimeException ignored) {
                options.clear();
            }
        }
        if (options.isEmpty()) {
            options.add(new CameraSourceOption(
                    getString(R.string.camera_facing_front),
                    "",
                    true,
                    defaultPublisherResolutionOptions()));
            options.add(new CameraSourceOption(
                    getString(R.string.camera_facing_back),
                    "camera2:0",
                    false,
                    defaultPublisherResolutionOptions()));
        }
        return options;
    }

    private List<PublisherResolutionOption> cameraResolutionOptions(
            CameraCharacteristics characteristics) {
        final List<PublisherResolutionOption> options = new ArrayList<>();
        final StreamConfigurationMap map =
                characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        final Size[] sizes = map == null ? null : map.getOutputSizes(SurfaceTexture.class);
        if (sizes != null) {
            final List<Size> sortedSizes = new ArrayList<>();
            Collections.addAll(sortedSizes, sizes);
            Collections.sort(sortedSizes, (left, right) ->
                    Integer.compare(right.getWidth() * right.getHeight(),
                            left.getWidth() * left.getHeight()));
            for (Size size : sortedSizes) {
                if (size.getWidth() <= 0 || size.getHeight() <= 0) {
                    continue;
                }
                final String label = size.getWidth() + "x" + size.getHeight();
                if (!containsResolution(options, size.getWidth(), size.getHeight())) {
                    options.add(new PublisherResolutionOption(
                            label,
                            size.getWidth(),
                            size.getHeight()));
                }
                if (options.size() >= 12) {
                    break;
                }
            }
        }
        return options.isEmpty() ? defaultPublisherResolutionOptions() : options;
    }

    private static boolean containsResolution(
            List<PublisherResolutionOption> options,
            int width,
            int height) {
        for (PublisherResolutionOption option : options) {
            if (option.width == width && option.height == height) {
                return true;
            }
        }
        return false;
    }

    private List<PublisherResolutionOption> defaultPublisherResolutionOptions() {
        final List<PublisherResolutionOption> options = new ArrayList<>();
        options.add(new PublisherResolutionOption("1920x1080", 1920, 1080));
        options.add(new PublisherResolutionOption("1280x720", 1280, 720));
        options.add(new PublisherResolutionOption("640x480", 640, 480));
        return options;
    }

    private int defaultCameraSelectionIndex() {
        for (int index = 0; index < cameraSourceOptions.size(); ++index) {
            if (cameraSourceOptions.get(index).frontFacing) {
                return index;
            }
        }
        return 0;
    }

    private CameraSourceOption selectedCameraSourceOption() {
        if (cameraSourceOptions.isEmpty()) {
            cameraSourceOptions = buildCameraSourceOptions();
        }
        final int index = cameraFacingSpinner == null ? 0 : cameraFacingSpinner.getSelectedItemPosition();
        if (index < 0 || index >= cameraSourceOptions.size()) {
            return cameraSourceOptions.get(0);
        }
        return cameraSourceOptions.get(index);
    }

    private String selectedCameraSourceId() {
        final String manualSourceId = readTextInput(cameraSourceIdInput, "");
        if (!manualSourceId.isEmpty()) {
            return manualSourceId;
        }
        return selectedCameraSourceOption().sourceId;
    }

    private String selectedCameraLabel() {
        final String manualSourceId = readTextInput(cameraSourceIdInput, "");
        if (!manualSourceId.isEmpty()) {
            return selectedCameraSourceOption().label + " / override=" + manualSourceId;
        }
        final String sourceId = selectedCameraSourceOption().sourceId;
        return selectedCameraSourceOption().label
                + (sourceId.isEmpty() ? " / SDK default" : " / " + sourceId);
    }

    private void installSurfaceLifecycleCallbacks() {
        installPreviewTextureLifecycle(playerPreviewSurface, this::stopPlayerPreview);
        installPreviewTextureLifecycle(cameraPreviewSurface, this::stopCameraPreview);
        installPreviewTextureLifecycle(publisherPreviewSurface, this::stopPublisherPreview);
        installPreviewTextureLifecycle(gbCameraPreviewSurface, this::stopGbSourcePreview);
    }

    private void installGbVideoPreviewCallbacks() {
        gbVideoPreviewView.setOnPreparedListener(mediaPlayer -> {
            if (mediaPlayer.getVideoWidth() > 0 && mediaPlayer.getVideoHeight() > 0) {
                gbVideoPreviewWidth = mediaPlayer.getVideoWidth();
                gbVideoPreviewHeight = mediaPlayer.getVideoHeight();
            }
            applyPreviewDisplayMode();
            mediaPlayer.setLooping(true);
            gbVideoPreviewView.start();
            gb28181StatusText = "Video file preview is running.";
            renderInfoText();
        });
        gbVideoPreviewView.setOnErrorListener((mediaPlayer, what, extra) -> {
            gb28181StatusText = "Video file preview failed: what=" + what + ", extra=" + extra;
            renderInfoText();
            return true;
        });
    }

    private void openGbVideoFilePicker() {
        if (selectedGb28181SourceChoice() != Gb28181SourceChoice.VIDEO_FILE) {
            gbInviteSourceSpinner.setSelection(Gb28181SourceChoice.VIDEO_FILE.ordinal());
        }
        try {
            gbVideoFilePickerLauncher.launch(new String[] {"video/*"});
        } catch (RuntimeException failure) {
            gb28181StatusText = "Failed to open video picker: " + safeMessage(failure);
            renderInfoText();
        }
    }

    private void openPublisherMediaFilePicker() {
        final PublisherSourceChoice sourceChoice = selectedPublisherSourceChoice();
        final boolean audioFileSource =
                selectedPublisherAudioChoice() == PublisherAudioChoice.FILE_AUDIO;
        if (!audioFileSource && sourceChoice != PublisherSourceChoice.VIDEO_FILE) {
            publisherSourceSpinner.setSelection(PublisherSourceChoice.VIDEO_FILE.ordinal());
        }
        final String[] mimeTypes = audioFileSource
                ? new String[] {"audio/*"}
                : new String[] {"video/*"};
        try {
            publisherMediaFilePickerLauncher.launch(mimeTypes);
        } catch (RuntimeException failure) {
            Toast.makeText(
                    this,
                    "Failed to open publisher file picker: " + safeMessage(failure),
                    Toast.LENGTH_LONG).show();
            renderInfoText();
        }
    }

    private void handlePublisherMediaFilePicked(Uri uri) {
        if (uri == null) {
            Toast.makeText(
                    this,
                    "Publisher file selection was cancelled.",
                    Toast.LENGTH_SHORT).show();
            renderInfoText();
            return;
        }
        try {
            getContentResolver().takePersistableUriPermission(
                    uri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } catch (SecurityException ignored) {
            // Some document providers grant transient read access only.
        }
        try {
            final File importedFile = copyPickedMediaUriToAppFile(uri, "publisher_media", ".media");
            publisherMediaPathInput.setText(importedFile.getAbsolutePath());
            Toast.makeText(
                    this,
                    "Selected publisher media file.",
                    Toast.LENGTH_SHORT).show();
            renderInfoText();
        } catch (IOException failure) {
            Toast.makeText(
                    this,
                    "Failed to import the selected publisher media file: "
                            + (failure.getMessage() == null
                            ? failure.getClass().getSimpleName()
                            : failure.getMessage()),
                    Toast.LENGTH_LONG).show();
            renderInfoText();
        }
    }

    private void handleGbVideoFilePicked(Uri uri) {
        if (uri == null) {
            gb28181StatusText = "Video file selection was cancelled.";
            renderInfoText();
            return;
        }
        try {
            getContentResolver().takePersistableUriPermission(
                    uri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } catch (SecurityException ignored) {
            // Some document providers grant transient read access only.
        }
        selectedGbVideoUri = uri;
        try {
            final File importedFile = copyPickedMediaUriToAppFile(uri, "gb28181_video", ".mp4");
            gbMediaSourceInput.setText(importedFile.getAbsolutePath());
            gbInviteSourceSpinner.setSelection(Gb28181SourceChoice.VIDEO_FILE.ordinal());
            gb28181StatusText = "Selected video file: " + importedFile.getAbsolutePath();
            startGbVideoPreview(uri);
            renderInfoText();
        } catch (IOException failure) {
            gb28181StatusText =
                    "Failed to import the selected GB28181 video file: "
                            + (failure.getMessage() == null
                            ? failure.getClass().getSimpleName()
                            : failure.getMessage());
            renderInfoText();
        }
    }

    private void startGbVideoPreview(Uri uri) {
        if (uri == null) {
            return;
        }
        gbVideoPreviewView.setVisibility(View.VISIBLE);
        gbCameraPreviewSurface.setVisibility(View.GONE);
        updateGbPreviewHint();
        applyPreviewDisplayMode();
        gbVideoPreviewView.setVideoURI(uri);
        gbVideoPreviewView.start();
    }

    private boolean stopGbVideoPreview() {
        if (gbVideoPreviewView == null) {
            return false;
        }
        final boolean wasVisible = gbVideoPreviewView.getVisibility() == View.VISIBLE;
        final boolean wasPlaying = gbVideoPreviewView.isPlaying();
        gbVideoPreviewView.stopPlayback();
        gbVideoPreviewView.setVisibility(View.GONE);
        if (selectedGb28181SourceChoice() == Gb28181SourceChoice.CAMERA_MICROPHONE) {
            gbCameraPreviewSurface.setVisibility(View.VISIBLE);
        }
        updateGbPreviewHint();
        return wasVisible || wasPlaying;
    }

    private void updateGbPreviewHint() {
        if (gbPreviewHintText == null) {
            return;
        }
        final boolean cameraPreviewVisible = gbCameraPreviewSurface != null
                && gbCameraPreviewSurface.getVisibility() == View.VISIBLE
                && activeGbPreviewSession != null;
        final boolean videoPreviewVisible = gbVideoPreviewView != null
                && gbVideoPreviewView.getVisibility() == View.VISIBLE;
        gbPreviewHintText.setVisibility(
                cameraPreviewVisible || videoPreviewVisible ? View.GONE : View.VISIBLE);
    }

    private void installPreviewTextureLifecycle(
            TextureView textureView,
            Runnable stopAction) {
        textureView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(
                    SurfaceTexture surfaceTexture,
                    int width,
                    int height) {
                applyPreviewDisplayMode();
                updatePreviewButtons();
            }

            @Override
            public void onSurfaceTextureSizeChanged(
                    SurfaceTexture surfaceTexture,
                    int width,
                    int height) {
                applyPreviewDisplayMode();
                updatePreviewButtons();
            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
                stopAction.run();
                return true;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
            }
        });
    }

    /**
     * Applies the demo-level display policy to host preview Views.
     *
     * <p>The SDK still owns media decode, capture, and Surface binding. This method only
     * changes the Android host View size inside each preview frame so the rendered image is
     * either stretched, aspect-fitted, or aspect-cropped by user choice.</p>
     */
    private void applyPreviewDisplayMode() {
        applyPreviewDisplayMode(
                playerPreviewSurface,
                playerPreviewVideoWidth,
                playerPreviewVideoHeight);
        applyPreviewDisplayMode(
                cameraPreviewSurface,
                DEFAULT_PREVIEW_SOURCE_WIDTH,
                DEFAULT_PREVIEW_SOURCE_HEIGHT);
        applyPreviewDisplayMode(
                publisherPreviewSurface,
                readPublisherVideoWidth(),
                readPublisherVideoHeight());
        applyPreviewDisplayMode(
                gbCameraPreviewSurface,
                DEFAULT_PREVIEW_SOURCE_WIDTH,
                DEFAULT_PREVIEW_SOURCE_HEIGHT);
        applyPreviewDisplayMode(
                gbVideoPreviewView,
                gbVideoPreviewWidth,
                gbVideoPreviewHeight);
    }

    /**
     * Sizes a preview child within its frame according to the selected display mode.
     */
    private void applyPreviewDisplayMode(View previewView, int sourceWidth, int sourceHeight) {
        if (previewView == null || previewView.getParent() == null) {
            return;
        }
        final View parentView = (View) previewView.getParent();
        final int parentWidth = parentView.getWidth();
        final int parentHeight = parentView.getHeight();
        if (parentWidth <= 0 || parentHeight <= 0) {
            if (previewView.isShown()) {
                previewView.post(() -> applyPreviewDisplayMode(
                        previewView,
                        sourceWidth,
                        sourceHeight));
            }
            return;
        }

        final int safeSourceWidth = sourceWidth > 0 ? sourceWidth : DEFAULT_PREVIEW_SOURCE_WIDTH;
        final int safeSourceHeight = sourceHeight > 0 ? sourceHeight : DEFAULT_PREVIEW_SOURCE_HEIGHT;
        final PreviewDisplayMode mode = selectedPreviewDisplayMode();
        int targetWidth = parentWidth;
        int targetHeight = parentHeight;
        if (mode != PreviewDisplayMode.STRETCH) {
            final double parentRatio = (double) parentWidth / (double) parentHeight;
            final double sourceRatio = (double) safeSourceWidth / (double) safeSourceHeight;
            final boolean parentWider = parentRatio > sourceRatio;
            if (mode == PreviewDisplayMode.FIT) {
                if (parentWider) {
                    targetHeight = parentHeight;
                    targetWidth = Math.max(1, (int) Math.round(parentHeight * sourceRatio));
                } else {
                    targetWidth = parentWidth;
                    targetHeight = Math.max(1, (int) Math.round(parentWidth / sourceRatio));
                }
            } else {
                if (parentWider) {
                    targetWidth = parentWidth;
                    targetHeight = Math.max(1, (int) Math.round(parentWidth / sourceRatio));
                } else {
                    targetHeight = parentHeight;
                    targetWidth = Math.max(1, (int) Math.round(parentHeight * sourceRatio));
                }
            }
        }

        applyFrameLayoutSize(previewView, targetWidth, targetHeight);
        if (previewView instanceof TextureView) {
            final SurfaceTexture texture = ((TextureView) previewView).getSurfaceTexture();
            if (texture != null) {
                texture.setDefaultBufferSize(targetWidth, targetHeight);
            }
        }
    }

    /**
     * Updates a preview child size while keeping it centered in its FrameLayout parent.
     */
    private static void applyFrameLayoutSize(View view, int width, int height) {
        final ViewGroup.LayoutParams current = view.getLayoutParams();
        final FrameLayout.LayoutParams params = current instanceof FrameLayout.LayoutParams
                ? new FrameLayout.LayoutParams((FrameLayout.LayoutParams) current)
                : new FrameLayout.LayoutParams(width, height, Gravity.CENTER);
        if (params.width == width
                && params.height == height
                && params.gravity == Gravity.CENTER) {
            return;
        }
        params.width = width;
        params.height = height;
        params.gravity = Gravity.CENTER;
        view.setLayoutParams(params);
    }

    /**
     * Returns the user-selected preview display policy, defaulting to aspect fit.
     */
    private PreviewDisplayMode selectedPreviewDisplayMode() {
        if (selectedPreviewDisplayModeIndex == PreviewDisplayMode.STRETCH.ordinal()) {
            return PreviewDisplayMode.STRETCH;
        }
        if (selectedPreviewDisplayModeIndex == PreviewDisplayMode.CROP.ordinal()) {
            return PreviewDisplayMode.CROP;
        }
        return PreviewDisplayMode.FIT;
    }

    private String selectedPreviewDisplayModeLabel() {
        for (Spinner spinner : previewDisplayModeSpinners) {
            if (spinner != null && spinner.getSelectedItem() != null) {
                return spinner.getSelectedItem().toString();
            }
        }
        return selectedPreviewDisplayMode().name();
    }

    private void bindTabViews() {
        tabContentScroll = findViewById(R.id.tab_content_scroll);
        licenseTabButton = findViewById(R.id.tab_license);
        playerTabButton = findViewById(R.id.tab_player);
        publisherTabButton = findViewById(R.id.tab_publisher);
        gb28181TabButton = findViewById(R.id.tab_gb28181);
        licensePanel = findViewById(R.id.panel_license);
        playerPanel = findViewById(R.id.panel_player);
        publisherPanel = findViewById(R.id.panel_publisher);
        gb28181Panel = findViewById(R.id.panel_gb28181);

        // Camera and desktop helper blocks remain automation-only regions.
        // Public mobile navigation is limited to Publisher / Player / GB28181 / License.
        licenseTabButton.setOnClickListener(view -> selectTab(R.id.tab_license));
        playerTabButton.setOnClickListener(view -> selectTab(R.id.tab_player));
        publisherTabButton.setOnClickListener(view -> selectTab(R.id.tab_publisher));
        gb28181TabButton.setOnClickListener(view -> selectTab(R.id.tab_gb28181));

        applySelectedTab(false);
    }

    private void selectTab(int tabId) {
        selectedTabId = tabId;
        applySelectedTab(true);
    }

    private void applySelectedTab(boolean resetContentScroll) {
        applyTabState(licenseTabButton, licensePanel, selectedTabId == R.id.tab_license);
        applyTabState(playerTabButton, playerPanel, selectedTabId == R.id.tab_player);
        applyTabState(publisherTabButton, publisherPanel, selectedTabId == R.id.tab_publisher);
        applyTabState(gb28181TabButton, gb28181Panel, selectedTabId == R.id.tab_gb28181);
        if (resetContentScroll && tabContentScroll != null) {
            tabContentScroll.post(() -> tabContentScroll.scrollTo(0, 0));
        }
        if (tabContentScroll != null) {
            tabContentScroll.post(this::applyPreviewDisplayMode);
        }
    }

    private static void applyTabState(Button tabButton, View panel, boolean selected) {
        if (panel == null) {
            return;
        }
        if (tabButton == null) {
            panel.setVisibility(selected ? View.VISIBLE : View.GONE);
            return;
        }
        tabButton.setSelected(selected);
        tabButton.setTextColor(selected ? TAB_TEXT_SELECTED : TAB_TEXT_IDLE);
        tabButton.setTypeface(Typeface.DEFAULT, selected ? Typeface.BOLD : Typeface.NORMAL);
        tabButton.setBackgroundColor(selected ? TAB_COLOR_SELECTED : TAB_COLOR_IDLE);
        panel.setVisibility(selected ? View.VISIBLE : View.GONE);
    }

    private void startDemoInitialization() {
        demoInitialized = false;
        initializationFailureMessage = "";

        // 把正式场景预热移到后台，避免 Activity 首帧长期卡在 splash。
        Thread worker = new Thread(() -> {
            try {
                prepareDemoScenarios();
                runOnUiThread(() -> {
                    demoInitialized = true;
                    recordDemoStatus(
                            "demo.initialize",
                            "0",
                            "ready",
                            "Runtime, license, log, and demo assets are ready.");
                    renderInfoText();
                    runAutorunIfRequested();
                });
            } catch (IOException failure) {
                runOnUiThread(() -> {
                    initializationFailureMessage =
                            "failed to prepare demo license assets: " + failure.getMessage();
                    renderInfoText();
                });
            }
        }, "streamcore-demo-init");
        worker.start();
    }

    private void prepareDemoScenarios() throws IOException {
        final StreamCoreRuntime runtime = StreamCoreSdk.getRuntime();
        final File demoLicenseFile = copyAssetToFilesDir(
                "streamcore_demo.lic",
                "streamcore_demo.lic");
        previewMediaFile = copyAssetToFilesDir(
                "streamcore_demo_preview.mp4",
                "streamcore_demo_preview.mp4");
        demoAudioFile = createDemoAudioFile();
        demoStillImageFile = createDemoStillImageFile();
        final String demoPublicPem = readAssetText("streamcore_demo_public.pem");

        productInfo = StreamCoreSdk.getProductInfo();
        runtimeConfig = runtime.getDefaultConfig()
                .buildUpon()
                .expectedProduct("streamcore_demo")
                .licensePath(demoLicenseFile.getAbsolutePath())
                .publicKeyPem(demoPublicPem)
                .packageId(getPackageName())
                .appId(getPackageName())
                .companyId("hbr")
                .build();
        configureStatus = runtime.configure(runtimeConfig);
        logConfigureStatus = runtime.configureLog(
                "",
                "streamcore_demo.log",
                StreamCoreLogLevel.INFO,
                true);
        logInfo = runtime.getLogInfo();
        recordDemoStatus("log.configure", logConfigureStatus);
        licenseInfo = runtime.getLicenseInfo();
        Log.i(LOG_TAG, "runtime.configure status="
                + configureStatus.statusName
                + " result=" + configureStatus.resultCode
                + " packageId=" + runtimeConfig.packageId
                + " appId=" + runtimeConfig.appId);
        Log.i(LOG_TAG, "license configured="
                + licenseInfo.configured
                + " loaded=" + licenseInfo.licenseLoaded
                + " valid=" + licenseInfo.licenseValid
                + " status=" + licenseInfo.statusName
                + " summary=" + licenseInfo.summary);
        demoFeature = runtime.hasFeature("streamcore_demo", false);
        inputChannelLimit = runtime.getLimit("max_input_channels", 0);
        // Player, camera, and publisher contracts are checked on demand so startup stays
        // responsive on devices where native session teardown or permissions can be slow.
        playerScenario = null;
        cameraCaptureScenario = null;
        publisherScenarios = new ArrayList<>();
    }

    private void recordDemoStatus(String action, StreamCoreOperationStatus status) {
        if (status == null) {
            recordDemoStatus(action, "-", "unknown", "No status object returned.");
            return;
        }
        recordDemoStatus(
                action,
                String.valueOf(status.resultCode),
                status.statusName,
                status.summary);
    }

    private void recordDemoStatus(
            String action,
            String code,
            String statusName,
            String summary) {
        latestStatusAction = TextUtils.isEmpty(action) ? "demo" : action;
        latestStatusCode = TextUtils.isEmpty(code) ? "-" : code;
        latestStatusName = TextUtils.isEmpty(statusName) ? "unknown" : statusName;
        latestStatusSummary = TextUtils.isEmpty(summary) ? "No details." : summary;
        appendStatusLogLine();
    }

    private void appendStatusLogLine() {
        final File logFile = configuredLogFile();
        final File parent = logFile.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            return;
        }
        final String timestamp = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.SSS",
                Locale.US).format(new Date());
        final String line = timestamp
                + " action=" + latestStatusAction
                + " code=" + latestStatusCode
                + " status=" + latestStatusName
                + " summary=" + latestStatusSummary
                + '\n';
        try (FileOutputStream output = new FileOutputStream(logFile, true)) {
            output.write(line.getBytes(java.nio.charset.StandardCharsets.UTF_8));
        } catch (IOException ignored) {
            // The status label remains the primary feedback path if the log file cannot be updated.
        }
    }

    private void renderDemoStatusText() {
        if (demoStatusText == null) {
            return;
        }
        demoStatusText.setText(getString(
                R.string.demo_status_template,
                latestStatusAction,
                latestStatusCode,
                latestStatusName,
                compactText(latestStatusSummary, 64),
                compactPath(currentLogFilePath()),
                TextUtils.isEmpty(latestLogZipPath)
                        ? getString(R.string.demo_status_no_zip)
                        : compactPath(latestLogZipPath)));
    }

    private String compactText(String text, int maxLength) {
        if (TextUtils.isEmpty(text) || text.length() <= maxLength) {
            return text == null ? "" : text;
        }
        return text.substring(0, maxLength) + "...";
    }

    private String compactPath(String path) {
        if (TextUtils.isEmpty(path) || path.length() <= 64) {
            return path == null ? "" : path;
        }
        final String normalized = path.replace('\\', '/');
        final String[] parts = normalized.split("/");
        if (parts.length < 3) {
            return compactText(normalized, 64);
        }
        return ".../" + parts[parts.length - 2] + "/" + parts[parts.length - 1];
    }

    private String compactPackagingStatus(String status) {
        if (TextUtils.isEmpty(status)) {
            return "-";
        }
        return compactText(status
                .replace("native_runtime_", "")
                .replace("platform_adapter_", "adapter_")
                .replace("publisher_", "pub_")
                .replace("capture_", "cap_")
                .replace("player_", "play_")
                .replace("_partial", "")
                .replace("_bundled", ""), 48);
    }

    private String spinnerSelectionLabel(Spinner spinner) {
        final Object selectedItem = spinner == null ? null : spinner.getSelectedItem();
        return selectedItem == null ? "-" : selectedItem.toString();
    }

    private String currentLogFilePath() {
        return configuredLogFile().getAbsolutePath();
    }

    private File configuredLogFile() {
        final String logFileName = logInfo == null
                || TextUtils.isEmpty(logInfo.logFileName)
                ? "streamcore_demo.log"
                : logInfo.logFileName;
        final File configuredFile = new File(logFileName);
        if (configuredFile.isAbsolute()) {
            return configuredFile;
        }
        return new File(streamcoreLogDirectory(), logFileName);
    }

    private File streamcoreLogDirectory() {
        final String configuredDirectory = logInfo == null
                ? ""
                : logInfo.logDirectory;
        return logRelatedDirectory(configuredDirectory, "logs");
    }

    private File streamcoreCrashDirectory() {
        return logRelatedDirectory("", "crash");
    }

    private File logRelatedDirectory(String configuredDirectory, String fallbackLeafName) {
        if (!TextUtils.isEmpty(configuredDirectory)) {
            return new File(configuredDirectory);
        }
        return new File(new File(getFilesDir(), "streamcore"), fallbackLeafName);
    }

    private void shareLogPackage() {
        try {
            final File zipFile = createLogPackageZip();
            latestLogZipPath = zipFile.getAbsolutePath();
            recordDemoStatus("logs.share", "0", "ready", getString(R.string.log_share_ready));
            renderDemoStatusText();
            final Uri zipUri = FileProvider.getUriForFile(
                    this,
                    getPackageName() + ".fileprovider",
                    zipFile);
            final Intent shareIntent = new Intent(Intent.ACTION_SEND)
                    .setType(LOG_ZIP_MIME_TYPE)
                    .putExtra(Intent.EXTRA_STREAM, zipUri)
                    .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            startActivity(Intent.createChooser(
                    shareIntent,
                    getString(R.string.log_share_chooser_title)));
        } catch (IOException | RuntimeException failure) {
            final String message = failure.getMessage() == null
                    ? failure.getClass().getSimpleName()
                    : failure.getMessage();
            recordDemoStatus("logs.share", "-1", "failed", message);
            renderDemoStatusText();
            Toast.makeText(
                    this,
                    getString(R.string.log_share_failed, message),
                    Toast.LENGTH_LONG).show();
        }
    }

    private void showLogUploadReserved() {
        final String message = getString(R.string.log_upload_reserved_message);
        recordDemoStatus("logs.upload", "-", "reserved", message);
        renderDemoStatusText();
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    }

    // 日志包只收集最近且受大小限制的文件，避免历史日志导致分享/上传包无限增长。
    private File createLogPackageZip() throws IOException {
        final File shareDirectory = new File(getCacheDir(), LOG_SHARE_DIR_NAME);
        if (!shareDirectory.exists() && !shareDirectory.mkdirs()) {
            throw new IOException("failed to create " + shareDirectory.getAbsolutePath());
        }
        pruneOldLogPackages(shareDirectory);
        final String timestamp = new SimpleDateFormat(
                "yyyyMMdd_HHmmss",
                Locale.US).format(new Date());
        final File zipFile = new File(
                shareDirectory,
                "streamcore_demo_logs_" + timestamp + ".zip");
        final List<LogPackageFile> candidates = new ArrayList<>();
        final Set<String> seenPaths = new HashSet<>();
        collectLogPackageFiles(
                streamcoreLogDirectory(),
                "logs/",
                candidates,
                seenPaths);
        collectLogPackageFiles(
                streamcoreCrashDirectory(),
                "crash/",
                candidates,
                seenPaths);
        addLogPackageFileIfPresent(
                configuredLogFile(),
                "logs/" + sanitizeZipEntryName(configuredLogFile().getName()),
                candidates,
                seenPaths);
        Collections.sort(
                candidates,
                (left, right) -> Long.compare(right.lastModified, left.lastModified));

        final LogPackageSelection selection = new LogPackageSelection();
        try (ZipOutputStream zip = new ZipOutputStream(new FileOutputStream(zipFile))) {
            addTextEntryToZip(zip, "demo_status.txt", buildLogPackageSummary());
            addAndroidProcessExitInfoEntries(zip);
            addSelectedLogFilesToZip(zip, candidates, selection);
            if (selection.addedFileCount == 0) {
                addTextEntryToZip(
                        zip,
                        "logs/README.txt",
                        "No SDK or demo log file has been created yet.\n");
            }
            if (selection.skippedFiles.length() > 0) {
                addTextEntryToZip(
                        zip,
                        "skipped_files.txt",
                        "The log package is capped to "
                                + LOG_PACKAGE_MAX_FILES
                                + " files and "
                                + LOG_PACKAGE_MAX_BYTES
                                + " bytes.\n"
                                + selection.skippedFiles);
            }
        }
        return zipFile;
    }

    private String buildLogPackageSummary() {
        return "StreamCore SDK Demo status\n"
                + "action=" + latestStatusAction + '\n'
                + "code=" + latestStatusCode + '\n'
                + "status=" + latestStatusName + '\n'
                + "summary=" + latestStatusSummary + '\n'
                + "log=" + currentLogFilePath() + '\n'
                + "log_dir=" + streamcoreLogDirectory().getAbsolutePath() + '\n'
                + "crash_dir=" + streamcoreCrashDirectory().getAbsolutePath() + '\n'
                + "package_limit_files=" + LOG_PACKAGE_MAX_FILES + '\n'
                + "package_limit_bytes=" + LOG_PACKAGE_MAX_BYTES + '\n'
                + "crash_capture_note=StreamCore SDK does not install an Android crash handler"
                + " or expose a crash directory. Demo/server/desktop products should integrate"
                + " Crashlytics, Sentry, Breakpad/Crashpad, or another mature crash solution"
                + " and write artifacts into this product directory."
                + " API 30+ process-exit metadata is included when Android exposes it.\n"
                + "zip=" + (TextUtils.isEmpty(latestLogZipPath)
                ? getString(R.string.demo_status_no_zip)
                : latestLogZipPath)
                + '\n';
    }

    private void collectLogPackageFiles(
            File directory,
            String entryPrefix,
            List<LogPackageFile> outFiles,
            Set<String> seenPaths) throws IOException {
        if (directory == null || !directory.isDirectory()) {
            return;
        }
        final File[] children = directory.listFiles();
        if (children == null) {
            return;
        }
        for (File child : children) {
            final String entryName = entryPrefix + sanitizeZipEntryName(child.getName());
            if (child.isDirectory()) {
                collectLogPackageFiles(child, entryName + "/", outFiles, seenPaths);
            } else if (child.isFile()) {
                addLogPackageFileIfPresent(child, entryName, outFiles, seenPaths);
            }
        }
    }

    private void addLogPackageFileIfPresent(
            File file,
            String entryName,
            List<LogPackageFile> outFiles,
            Set<String> seenPaths) throws IOException {
        if (file == null || !file.isFile()) {
            return;
        }
        final String canonicalPath = file.getCanonicalPath();
        if (!seenPaths.add(canonicalPath)) {
            return;
        }
        outFiles.add(new LogPackageFile(file, entryName));
    }

    private void addSelectedLogFilesToZip(
            ZipOutputStream zip,
            List<LogPackageFile> candidates,
            LogPackageSelection selection) throws IOException {
        for (LogPackageFile candidate : candidates) {
            if (selection.addedFileCount >= LOG_PACKAGE_MAX_FILES) {
                selection.skip(candidate, "skipped by file-count cap");
                continue;
            }
            if (candidate.sizeBytes > LOG_PACKAGE_SINGLE_FILE_MAX_BYTES) {
                if (selection.addedBytes + LOG_PACKAGE_SINGLE_FILE_MAX_BYTES
                        > LOG_PACKAGE_MAX_BYTES) {
                    selection.skip(candidate, "skipped by total-size cap for tail entry");
                    continue;
                }
                final long bytesAdded = addFileTailToZip(
                        zip,
                        candidate.file,
                        candidate.entryName + ".tail",
                        LOG_PACKAGE_SINGLE_FILE_MAX_BYTES);
                selection.addedBytes += bytesAdded;
                selection.addedFileCount += 1;
                selection.skip(
                        candidate,
                        "included tail-only entry "
                                + candidate.entryName
                                + ".tail capped to "
                                + LOG_PACKAGE_SINGLE_FILE_MAX_BYTES
                                + " bytes");
                continue;
            }
            if (selection.addedBytes + candidate.sizeBytes > LOG_PACKAGE_MAX_BYTES) {
                selection.skip(candidate, "skipped by total-size cap");
                continue;
            }
            addFileToZip(zip, candidate.file, candidate.entryName);
            selection.addedBytes += candidate.sizeBytes;
            selection.addedFileCount += 1;
        }
    }

    private void addFileToZip(
            ZipOutputStream zip,
            File file,
            String entryName) throws IOException {
        final ZipEntry entry = new ZipEntry(sanitizeZipEntryName(entryName));
        entry.setTime(file.lastModified());
        zip.putNextEntry(entry);
        final byte[] buffer = new byte[16 * 1024];
        try (FileInputStream input = new FileInputStream(file)) {
            int bytesRead;
            while ((bytesRead = input.read(buffer)) != -1) {
                zip.write(buffer, 0, bytesRead);
            }
        }
        zip.closeEntry();
    }

    private long addFileTailToZip(
            ZipOutputStream zip,
            File file,
            String entryName,
            long maxBytes) throws IOException {
        final long fileSize = file.length();
        final long startOffset = Math.max(0L, fileSize - maxBytes);
        final long bytesToCopy = fileSize - startOffset;
        final ZipEntry entry = new ZipEntry(sanitizeZipEntryName(entryName));
        entry.setTime(file.lastModified());
        zip.putNextEntry(entry);
        final byte[] buffer = new byte[16 * 1024];
        long copiedBytes = 0L;
        try (RandomAccessFile input = new RandomAccessFile(file, "r")) {
            input.seek(startOffset);
            while (copiedBytes < bytesToCopy) {
                final int requestedBytes =
                        (int) Math.min(buffer.length, bytesToCopy - copiedBytes);
                final int bytesRead = input.read(buffer, 0, requestedBytes);
                if (bytesRead <= 0) {
                    break;
                }
                zip.write(buffer, 0, bytesRead);
                copiedBytes += bytesRead;
            }
        }
        zip.closeEntry();
        return copiedBytes;
    }

    // Android 11+ 可在下次启动后读取进程退出摘要；真实 tombstone/logcat 仍不由 SDK 自动复制。
    private void addAndroidProcessExitInfoEntries(ZipOutputStream zip) throws IOException {
        final StringBuilder builder = new StringBuilder();
        builder.append("Android process exit info\n");
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            builder.append("unavailable: ApplicationExitInfo requires Android 11/API 30.\n");
            addTextEntryToZip(zip, "android_process_exit_info.txt", builder.toString());
            return;
        }

        final ActivityManager activityManager = getSystemService(ActivityManager.class);
        if (activityManager == null) {
            builder.append("unavailable: ActivityManager is null.\n");
            addTextEntryToZip(zip, "android_process_exit_info.txt", builder.toString());
            return;
        }

        final List<ApplicationExitInfo> exitInfos =
                activityManager.getHistoricalProcessExitReasons(
                        getPackageName(),
                        0,
                        ANDROID_EXIT_INFO_MAX_ITEMS);
        if (exitInfos == null || exitInfos.isEmpty()) {
            builder.append("No historical process exit records are available.\n");
            addTextEntryToZip(zip, "android_process_exit_info.txt", builder.toString());
            return;
        }

        int index = 0;
        for (ApplicationExitInfo exitInfo : exitInfos) {
            builder
                    .append('#')
                    .append(index)
                    .append(" timestamp=")
                    .append(new Date(exitInfo.getTimestamp()))
                    .append(" reason=")
                    .append(exitInfo.getReason())
                    .append(" status=")
                    .append(exitInfo.getStatus())
                    .append(" importance=")
                    .append(exitInfo.getImportance())
                    .append(" pss=")
                    .append(exitInfo.getPss())
                    .append(" rss=")
                    .append(exitInfo.getRss())
                    .append(" description=")
                    .append(exitInfo.getDescription())
                    .append('\n');
            try (InputStream trace = exitInfo.getTraceInputStream()) {
                if (trace != null) {
                    addInputStreamToZip(
                            zip,
                            trace,
                            "crash/android_exit_trace_" + index + ".txt",
                            LOG_PACKAGE_SINGLE_FILE_MAX_BYTES);
                }
            } catch (IOException failure) {
                builder
                        .append("trace_")
                        .append(index)
                        .append("_error=")
                        .append(failure.getMessage())
                        .append('\n');
            }
            ++index;
        }
        addTextEntryToZip(zip, "android_process_exit_info.txt", builder.toString());
    }

    private void addInputStreamToZip(
            ZipOutputStream zip,
            InputStream input,
            String entryName,
            long maxBytes) throws IOException {
        final ZipEntry entry = new ZipEntry(sanitizeZipEntryName(entryName));
        zip.putNextEntry(entry);
        final byte[] buffer = new byte[16 * 1024];
        long copiedBytes = 0;
        int bytesRead;
        while ((bytesRead = input.read(buffer)) != -1) {
            final long remainingBytes = maxBytes - copiedBytes;
            if (remainingBytes <= 0) {
                break;
            }
            final int bytesToWrite = (int) Math.min(bytesRead, remainingBytes);
            zip.write(buffer, 0, bytesToWrite);
            copiedBytes += bytesToWrite;
            if (bytesToWrite < bytesRead) {
                break;
            }
        }
        zip.closeEntry();
    }

    private void pruneOldLogPackages(File shareDirectory) {
        final File[] packages = shareDirectory.listFiles(
                file -> file.isFile()
                        && file.getName().startsWith("streamcore_demo_logs_")
                        && file.getName().endsWith(".zip"));
        if (packages == null || packages.length <= LOG_SHARE_MAX_ZIP_FILES) {
            return;
        }
        final List<File> sortedPackages = new ArrayList<>();
        Collections.addAll(sortedPackages, packages);
        Collections.sort(
                sortedPackages,
                (left, right) -> Long.compare(right.lastModified(), left.lastModified()));
        for (int index = LOG_SHARE_MAX_ZIP_FILES; index < sortedPackages.size(); ++index) {
            final File file = sortedPackages.get(index);
            if (!file.delete()) {
                // Old shared packages are cache files; failure to delete should not block sharing.
            }
        }
    }

    private void addTextEntryToZip(
            ZipOutputStream zip,
            String entryName,
            String text) throws IOException {
        final ZipEntry entry = new ZipEntry(sanitizeZipEntryName(entryName));
        zip.putNextEntry(entry);
        zip.write(text.getBytes(java.nio.charset.StandardCharsets.UTF_8));
        zip.closeEntry();
    }

    private static String sanitizeZipEntryName(String name) {
        return name.replace('\\', '/').replace("../", "").replace("..", "");
    }

    private void requestDesktopCapturePermission() {
        if (!demoInitialized || !initializationFailureMessage.isEmpty()) {
            renderInfoText();
            return;
        }

        final MediaProjectionManager projectionManager =
                getSystemService(MediaProjectionManager.class);
        if (projectionManager == null) {
            desktopCaptureScenario = DesktopCaptureScenarioResult.failedBeforeGrant(
                    buildStatus(
                            StreamCoreResultCode.OPERATION_FAILED,
                            "desktop_capture_projection_manager_missing",
                            "desktop capture permission flow is unavailable on this host.",
                            "MediaProjectionManager is null."));
            renderInfoText();
            return;
        }

        releaseDesktopCaptureSession();
        desktopCaptureScenario = DesktopCaptureScenarioResult.awaitingGrant();
        renderInfoText();

        try {
            desktopCaptureGrantLauncher.launch(projectionManager.createScreenCaptureIntent());
        } catch (RuntimeException failure) {
            desktopCaptureScenario = DesktopCaptureScenarioResult.failedBeforeGrant(
                    buildStatus(
                            StreamCoreResultCode.OPERATION_FAILED,
                            "desktop_capture_grant_launch_failed",
                            "desktop capture permission request failed to launch.",
                            safeMessage(failure)));
            renderInfoText();
        }
    }

    private void handleDesktopCaptureGrantResult(int resultCode, Intent projectionData) {
        releaseDesktopCaptureSession();

        if (resultCode != Activity.RESULT_OK || projectionData == null) {
            desktopCaptureScenario = DesktopCaptureScenarioResult.grantRejected(
                    resultCode,
                    projectionData != null);
            renderInfoText();
            return;
        }

        desktopCaptureStartInProgress = true;
        desktopCaptureScenario = DesktopCaptureScenarioResult.starting();
        renderInfoText();

        final int desktopWidth = readIntInput(desktopWidthInput, 1280, 1, 7680);
        final int desktopHeight = readIntInput(desktopHeightInput, 720, 1, 4320);
        final int desktopBitrate =
                readIntInput(desktopBitrateInput, 1_200_000, 1, 100_000_000);
        final int desktopKeyInterval =
                readIntInput(desktopKeyIntervalInput, 2, 1, 60);

        Thread worker = new Thread(() -> {
            final StreamCoreCapture.Session session = new StreamCoreCapture.Session();
            session.setConfig(StreamCoreCapture.Config.newBuilder()
                    .sessionName("android_demo_capture_desktop")
                    .sourceKind(StreamCoreCapture.SourceKind.DESKTOP)
                    .displayId("main_display")
                    .targetFrameRate(15)
                    .screenWidth(desktopWidth)
                    .screenHeight(desktopHeight)
                    .screenBitrateBps(desktopBitrate)
                    .screenKeyIntervalSeconds(desktopKeyInterval)
                    .projectionGrantResultCode(resultCode)
                    .projectionGrantData(projectionData)
                    .enableAudio(false)
                    .enableVideo(true)
                    .build());

            final StreamCoreCapture.Preflight preflight = session.preflight();
            final StreamCoreOperationStatus startStatus = session.start();
            final StreamCoreCapture.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
            final StreamCoreCapture.RuntimeInfo runtimeAfterStop;
            final DesktopCaptureScenarioResult scenario;
            final boolean keepSession = startStatus.resultCode == StreamCoreResultCode.OK;
            if (keepSession) {
                runtimeAfterStop = null;
                scenario = DesktopCaptureScenarioResult.started(
                        preflight,
                        startStatus,
                        runtimeAfterStart);
            } else {
                session.stop();
                runtimeAfterStop = session.getRuntimeInfo();
                scenario = DesktopCaptureScenarioResult.failedAfterStart(
                        preflight,
                        startStatus,
                        runtimeAfterStart,
                        runtimeAfterStop);
            }

            runOnUiThread(() -> {
                desktopCaptureStartInProgress = false;
                if (keepSession) {
                    activeDesktopCaptureSession = session;
                }
                desktopCaptureScenario = scenario;
                recordDemoStatus("desktop_capture.start", startStatus);
                renderInfoText();
            });
        }, "streamcore-desktop-capture-start");
        worker.start();
    }

    private void stopDesktopCapture() {
        if (activeDesktopCaptureSession == null) {
            final StreamCoreOperationStatus status = buildStatus(
                    StreamCoreResultCode.OK,
                    "desktop_capture_not_running",
                    "desktop capture is not running.",
                    "MediaProjection grant is not active.");
            desktopCaptureScenario = desktopCaptureScenario.withStop(
                    status,
                    null,
                    "idle",
                    "desktop capture is not running.",
                    "foreground screen capture is idle.");
            recordDemoStatus("desktop_capture.stop", status);
            renderInfoText();
            return;
        }

        final StreamCoreCapture.Session session = activeDesktopCaptureSession;
        session.stop();
        final StreamCoreCapture.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        activeDesktopCaptureSession = null;
        final StreamCoreOperationStatus status = buildStatus(
                StreamCoreResultCode.OK,
                "desktop_capture_stopped",
                "desktop capture session stopped.",
                "Foreground screen capture service has been released.");
        desktopCaptureScenario = desktopCaptureScenario.withStop(
                status,
                runtimeAfterStop,
                "stopped",
                "desktop capture session stopped.",
                "foreground screen capture is idle.");
        recordDemoStatus("desktop_capture.stop", status);
        renderInfoText();
    }

    private void startGb28181Device() {
        if (!demoInitialized
                || !initializationFailureMessage.isEmpty()
                || gb28181StartInProgress
                || activeGb28181Session != null) {
            renderInfoText();
            return;
        }
        if (selectedGb28181SourceChoice() == Gb28181SourceChoice.CAMERA_MICROPHONE
                && activePublisherLocalCaptureSession != null) {
            gb28181StatusText = "Stop the active publisher before starting GB28181 from camera.";
            renderInfoText();
            return;
        }
        if (selectedGb28181SourceChoice() == Gb28181SourceChoice.CAMERA_MICROPHONE
                && (!hasCameraPermission() || !hasRecordAudioPermission())) {
            gb28181PermissionRequestInProgress = true;
            gb28181StatusText = "Requesting camera and microphone permission before GB28181 start.";
            renderInfoText();
            gbPermissionLauncher.launch(
                    DemoPermissionSupport.missingCameraAndMicrophonePermissions(this));
            return;
        }
        if (selectedGb28181SourceChoice() == Gb28181SourceChoice.VIDEO_FILE
                && selectedGbVideoUri == null
                && readTextInput(gbMediaSourceInput, "").isEmpty()) {
            gb28181StatusText = "Select a GB28181 video file before starting.";
            renderInfoText();
            openGbVideoFilePicker();
            return;
        }

        final String localId = readTextInput(gbLocalIdInput, "34020000001320000001");
        final String localDomain = readTextInput(gbLocalDomainInput, "3402000000");
        final String upperId = readTextInput(gbUpperIdInput, "34020000002000000001");
        final String upperDomain = readTextInput(gbUpperDomainInput, localDomain);
        final String sipPassword = readTextInput(gbUpperPasswordInput, "123456");
        final String localIp = readTextInput(gbLocalIpInput, "0.0.0.0");
        final int localPort = readIntInput(gbLocalPortInput, 5060, 1, 65535);
        final String upperIp = readTextInput(gbUpperIpInput, "");
        final int upperPort = readIntInput(gbUpperPortInput, 5060, 1, 65535);
        final StreamCoreGB28181.TransportMode transportMode = selectedGb28181TransportMode();
        final String mediaIp = readTextInput(gbMediaIpInput, resolveFirstNonLoopbackIpv4("0.0.0.0"));
        final int mediaPort = readIntInput(gbMediaPortInput, 15060, 1, 65535);
        final int registerExpires =
                readIntInput(gbRegisterExpiresInput, 3600, 60, 86_400);
        final int keepalive = readIntInput(gbKeepaliveInput, 60, 5, 3600);
        if (upperIp.isEmpty()) {
            gb28181StatusText = "Enter the upper-platform SIP IP before starting GB28181.";
            renderInfoText();
            return;
        }
        final Gb28181SourceChoice sourceChoice = selectedGb28181SourceChoice();
        final String sourceLabel = selectedGb28181SourceLabel();
        final String sourceBinding = selectedGb28181SourceBinding();
        final String gbCameraSourceId =
                sourceChoice == Gb28181SourceChoice.CAMERA_MICROPHONE
                        ? selectedCameraSourceId()
                        : "";
        final StreamCoreGB28181.Config config = buildGb28181Config(
                localId,
                localDomain,
                upperId,
                upperDomain,
                sipPassword,
                localIp,
                localPort,
                upperIp,
                upperPort,
                transportMode,
                mediaIp,
                mediaPort,
                registerExpires,
                keepalive,
                sourceLabel);
        final List<StreamCoreGB28181.CatalogItem> catalog =
                buildGb28181Catalog(localId, sourceLabel);

        gb28181StartInProgress = true;
        gb28181StatusText = "Starting GB28181 device runtime for " + sourceBinding + ".";
        renderInfoText();

        Thread worker = new Thread(() -> {
            StreamCoreGB28181.DeviceSession session = null;
            boolean keepSession = false;
            final StringBuilder logBuilder = new StringBuilder();
            final StreamCoreOperationStatus[] statusForUi = new StreamCoreOperationStatus[] {
                    buildStatus(
                            StreamCoreResultCode.OPERATION_FAILED,
                            "gb28181_start_failed",
                            "GB28181 start failed before SDK status was returned.",
                            "See the GB28181 status section.")
            };
            try {
                session = StreamCoreGB28181.createDeviceSession();
                final StreamCoreOperationStatus configStatus = session.setConfig(config);
                final StreamCoreOperationStatus observerStatus = session.setObserver(
                        buildGb28181Observer(
                                session,
                                sourceChoice,
                                sourceBinding,
                                gbCameraSourceId));
                final StreamCoreOperationStatus infoStatus = session.setDeviceInfo(
                        new StreamCoreGB28181.DeviceInfo(
                                "StreamCore SDK Demo",
                                "HBR",
                                "AndroidDemo",
                                "1.0.0"));
                final StreamCoreOperationStatus statusStatus = session.setDeviceStatus(
                        new StreamCoreGB28181.DeviceStatus("OK", true, false));
                final StreamCoreOperationStatus catalogStatus = session.setCatalog(catalog);
                final StreamCoreOperationStatus startStatus =
                        configStatus.isOk() && observerStatus.isOk()
                                ? session.start()
                                : (configStatus.isOk() ? observerStatus : configStatus);
                final StreamCoreOperationStatus registerStatus =
                        startStatus.isOk() ? session.registerToPlatform() : notApplicableStatus();
                final StreamCoreOperationStatus pollStatus =
                        startStatus.isOk() ? session.poll() : notApplicableStatus();
                statusForUi[0] = startStatus.isOk() ? registerStatus : startStatus;
                final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
                keepSession = startStatus.isOk();
                logBuilder.append("source=")
                        .append(sourceBinding)
                        .append(", upperSip=")
                        .append(upperIp)
                        .append(':')
                        .append(upperPort)
                        .append(", localSip=")
                        .append(localIp)
                        .append(':')
                        .append(localPort)
                        .append(", mediaRtp=")
                        .append(mediaIp)
                        .append(':')
                        .append(mediaPort)
                        .append('\n');
                appendStatusLine(logBuilder, "config", configStatus);
                appendStatusLine(logBuilder, "observer", observerStatus);
                appendStatusLine(logBuilder, "device info", infoStatus);
                appendStatusLine(logBuilder, "device status", statusStatus);
                appendStatusLine(logBuilder, "catalog", catalogStatus);
                appendStatusLine(logBuilder, "start", startStatus);
                appendStatusLine(logBuilder, "register", registerStatus);
                appendStatusLine(logBuilder, "poll", pollStatus);
                logBuilder.append("runtime: started=")
                        .append(runtimeInfo.started)
                        .append(", registered=")
                        .append(runtimeInfo.registered)
                        .append(", transportStarted=")
                        .append(runtimeInfo.transportStarted)
                        .append(", activeSessions=")
                        .append(runtimeInfo.activeSessionCount)
                        .append(", sourceBindings=")
                        .append(runtimeInfo.activeSourceBindingCount)
                        .append(", sentAudio=")
                        .append(runtimeInfo.sentAudioPacketCount)
                        .append(", sentVideo=")
                        .append(runtimeInfo.sentVideoPacketCount)
                        .append('\n')
                        .append(runtimeInfo.stateSummary);
            } catch (RuntimeException failure) {
                logBuilder.append("GB28181 start failed: ")
                        .append(safeMessage(failure));
            }

            final StreamCoreGB28181.DeviceSession sessionForUi = session;
            final boolean keepSessionForUi = keepSession;
            runOnUiThread(() -> {
                gb28181StartInProgress = false;
                if (keepSessionForUi) {
                    activeGb28181Session = sessionForUi;
                    startGb28181PollLoop(sessionForUi);
                } else if (sessionForUi != null) {
                    sessionForUi.close();
                }
                gb28181StatusText = logBuilder.toString();
                recordDemoStatus("gb28181.start", statusForUi[0]);
                renderInfoText();
            });
        }, "streamcore-gb28181-start");
        worker.start();
    }

    private void stopGb28181Device() {
        final boolean stoppedPreview = stopGbSourcePreviewInternal(false);
        stopGb28181PollLoop();
        if (activeGb28181Session == null) {
            gb28181StatusText = stoppedPreview
                    ? "GB28181 is not running; source preview has been stopped."
                    : "GB28181 is not running.";
            renderInfoText();
            return;
        }
        final StreamCoreGB28181.DeviceSession session = activeGb28181Session;
        activeGb28181Session = null;
        Thread worker = new Thread(() -> {
            final StringBuilder logBuilder = new StringBuilder();
            final StreamCoreOperationStatus[] statusForUi = new StreamCoreOperationStatus[] {
                    buildStatus(
                            StreamCoreResultCode.OK,
                            "gb28181_stop_done",
                            "GB28181 stop completed.",
                            "Device session closed.")
            };
            try {
                final StreamCoreOperationStatus unregisterStatus =
                        session.unregisterFromPlatform();
                statusForUi[0] = unregisterStatus;
                session.stop();
                final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
                appendStatusLine(logBuilder, "unregister", unregisterStatus);
                logBuilder.append("stop: done\n")
                        .append("runtime: started=")
                        .append(runtimeInfo.started)
                        .append(", registered=")
                        .append(runtimeInfo.registered)
                        .append(", activeSessions=")
                        .append(runtimeInfo.activeSessionCount)
                        .append(", sourceBindings=")
                        .append(runtimeInfo.activeSourceBindingCount)
                        .append(", sentAudio=")
                        .append(runtimeInfo.sentAudioPacketCount)
                        .append(", sentVideo=")
                        .append(runtimeInfo.sentVideoPacketCount);
            } finally {
                session.close();
            }
            runOnUiThread(() -> {
                gb28181StatusText = stoppedPreview
                        ? logBuilder + "\nsource preview stopped."
                        : logBuilder.toString();
                recordDemoStatus("gb28181.stop", statusForUi[0]);
                renderInfoText();
            });
        }, "streamcore-gb28181-stop");
        worker.start();
        renderInfoText();
    }

    private void releaseDesktopCaptureSession() {
        if (activeDesktopCaptureSession == null) {
            return;
        }
        activeDesktopCaptureSession.stop();
        activeDesktopCaptureSession = null;
    }

    private void releaseGb28181Session() {
        stopGb28181PollLoop();
        if (activeGb28181Session == null) {
            return;
        }
        activeGb28181Session.stop();
        activeGb28181Session.close();
        activeGb28181Session = null;
    }

    private StreamCoreGB28181.Observer buildGb28181Observer(
            StreamCoreGB28181.DeviceSession session,
            Gb28181SourceChoice sourceChoice,
            String sourceBinding,
            String cameraSourceId) {
        return new StreamCoreGB28181.Observer() {
            @Override
            public void onRegisterResult(boolean success, @NonNull String reason) {
                final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
                final StreamCoreOperationStatus status = buildStatus(
                        success ? StreamCoreResultCode.OK : StreamCoreResultCode.OPERATION_FAILED,
                        success ? "gb28181_register_result_ok" : "gb28181_register_result_failed",
                        success ? "GB28181 register intent succeeded." : "GB28181 register intent failed.",
                        reason.isEmpty() ? runtimeInfo.stateSummary : reason);
                postGb28181CallbackStatus(
                        "gb28181.register_result",
                        status,
                        "registerResult=" + success + ", reason=" + (reason.isEmpty() ? "-" : reason),
                        runtimeInfo);
            }

            @Override
            public void onInviteReceived(@NonNull StreamCoreGB28181.Invite invite) {
                final StreamCoreOperationStatus acceptStatus = session.replyInviteAccepted(invite);
                final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
                postGb28181CallbackStatus(
                        "gb28181.invite",
                        acceptStatus,
                        "invite "
                                + invite.target.channelId
                                + " / "
                                + invite.target.streamKind
                                + " accepted via SDK auto-answer.",
                        runtimeInfo);
            }

            @Override
            public void onSessionUpdated(@NonNull StreamCoreGB28181.SessionInfo sessionInfo) {
                final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
                postGb28181CallbackStatus(
                        "gb28181.session_updated",
                        buildStatus(
                                StreamCoreResultCode.OK,
                                "gb28181_session_updated",
                                "GB28181 session updated.",
                                sessionInfo.sessionKey),
                        "session "
                                + sessionInfo.target.channelId
                                + " state="
                                + sessionInfo.state
                                + ", stream="
                                + sessionInfo.target.streamKind,
                        runtimeInfo);
            }

            @Override
            public void onSessionClosed(@NonNull StreamCoreGB28181.SessionInfo sessionInfo) {
                final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
                postGb28181CallbackStatus(
                        "gb28181.session_closed",
                        buildStatus(
                                StreamCoreResultCode.OK,
                                "gb28181_session_closed",
                                "GB28181 session closed.",
                                sessionInfo.sessionKey),
                        "session "
                                + sessionInfo.target.channelId
                                + " closed after "
                                + sessionInfo.state,
                        runtimeInfo);
            }

            @Override
            public void onMediaRequest(@NonNull StreamCoreGB28181.MediaRequest request) {
                final StreamCoreOperationStatus bindingStatus = applyGb28181SourceBinding(
                        session,
                        request,
                        sourceChoice,
                        sourceBinding,
                        cameraSourceId);
                final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
                postGb28181CallbackStatus(
                        "gb28181.media_request",
                        bindingStatus,
                        "mediaRequest "
                                + request.target.channelId
                                + " -> "
                                + sourceBinding
                                + ", direction="
                                + request.mediaDirection,
                        runtimeInfo);
            }
        };
    }

    private StreamCoreOperationStatus applyGb28181SourceBinding(
            StreamCoreGB28181.DeviceSession session,
            StreamCoreGB28181.MediaRequest request,
            Gb28181SourceChoice sourceChoice,
            String sourceBinding,
            String cameraSourceId) {
        if (!request.requiresLocalSource) {
            return session.clearSourceBindings();
        }
        final boolean enableAudio = !request.audioCodecName.isEmpty();
        final boolean enableVideo = !request.videoCodecName.isEmpty() || !enableAudio;
        switch (sourceChoice) {
            case VIDEO_FILE:
                if (!request.supportsMediaFile()) {
                    return buildStatus(
                            StreamCoreResultCode.OPERATION_FAILED,
                            "gb28181_media_file_unsupported",
                            "Current GB28181 request does not accept a media-file binding.",
                            "supportedSourceMask=" + request.supportedSourceMask);
                }
                if (sourceBinding.isEmpty()) {
                    return buildStatus(
                            StreamCoreResultCode.OPERATION_FAILED,
                            "gb28181_media_file_missing",
                            "GB28181 media-file binding requires a readable file path.",
                            "Select a GB28181 video file before the upper platform sends INVITE.");
                }
                return session.setSourceBindings(Collections.singletonList(
                        StreamCoreGB28181.SourceBinding.newBuilder(request.target)
                                .sourceKind(StreamCoreGB28181.MediaSourceKind.MEDIA_FILE)
                                .filePath(sourceBinding)
                                .enableAudio(enableAudio)
                                .enableVideo(enableVideo)
                                .repeatFile(true)
                                .readAsFastAsPossible(false)
                                .build()));
            case CAMERA_MICROPHONE:
            default:
                if (!request.supportsLocalDevice()) {
                    return buildStatus(
                            StreamCoreResultCode.OPERATION_FAILED,
                            "gb28181_local_device_unsupported",
                            "Current GB28181 request does not accept a local-device binding.",
                            "supportedSourceMask=" + request.supportedSourceMask);
                }
                return session.setSourceBindings(Collections.singletonList(
                        StreamCoreGB28181.SourceBinding.newBuilder(request.target)
                                .sourceKind(StreamCoreGB28181.MediaSourceKind.LOCAL_DEVICE)
                                .captureSource(StreamCoreCapture.SourceKind.CAMERA, cameraSourceId)
                                .audioCaptureSource(StreamCoreCapture.SourceKind.MICROPHONE, "")
                                .enableAudio(enableAudio)
                                .enableVideo(enableVideo)
                                .allowRawFramePacket(true)
                                .videoShape(DEFAULT_PREVIEW_SOURCE_WIDTH, DEFAULT_PREVIEW_SOURCE_HEIGHT, 25)
                                .audioShape(48000, 2)
                                .build()));
        }
    }

    private void postGb28181CallbackStatus(
            String action,
            StreamCoreOperationStatus status,
            String headline,
            StreamCoreGB28181.RuntimeInfo runtimeInfo) {
        runOnUiThread(() -> {
            gb28181StatusText = headline + "\n" + summarizeGb28181Runtime(runtimeInfo);
            recordDemoStatus(action, status);
            renderInfoText();
        });
    }

    private String summarizeGb28181Runtime(StreamCoreGB28181.RuntimeInfo runtimeInfo) {
        return "runtime: started="
                + runtimeInfo.started
                + ", registered="
                + runtimeInfo.registered
                + ", transportStarted="
                + runtimeInfo.transportStarted
                + ", sessions="
                + runtimeInfo.activeSessionCount
                + ", mediaRuntimes="
                + runtimeInfo.activeMediaRuntimeCount
                + ", sourceBindings="
                + runtimeInfo.activeSourceBindingCount
                + ", sentAudio="
                + runtimeInfo.sentAudioPacketCount
                + ", sentVideo="
                + runtimeInfo.sentVideoPacketCount
                + "\n"
                + runtimeInfo.stateSummary;
    }

    private void startGb28181PollLoop(StreamCoreGB28181.DeviceSession session) {
        stopGb28181PollLoop();
        gb28181PollExecutor = Executors.newSingleThreadScheduledExecutor(
                runnable -> {
                    Thread thread = new Thread(runnable, "streamcore-gb28181-poll");
                    thread.setDaemon(true);
                    return thread;
                });
        gb28181PollFuture = gb28181PollExecutor.scheduleAtFixedRate(() -> {
            if (activeGb28181Session != session) {
                return;
            }
            try {
                final StreamCoreOperationStatus pollStatus = session.poll();
                if (!pollStatus.isOk()) {
                    final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
                    stopGb28181PollLoop();
                    postGb28181CallbackStatus(
                            "gb28181.poll",
                            pollStatus,
                            "GB28181 poll failed after start.",
                            runtimeInfo);
                }
            } catch (RuntimeException failure) {
                stopGb28181PollLoop();
                runOnUiThread(() -> {
                    gb28181StatusText = "GB28181 poll loop failed: " + safeMessage(failure);
                    renderInfoText();
                });
            }
        }, 1, 1, TimeUnit.SECONDS);
    }

    private void stopGb28181PollLoop() {
        if (gb28181PollFuture != null) {
            gb28181PollFuture.cancel(true);
            gb28181PollFuture = null;
        }
        if (gb28181PollExecutor != null) {
            gb28181PollExecutor.shutdownNow();
            gb28181PollExecutor = null;
        }
    }

    private int readIntInput(EditText input, int fallback, int min, int max) {
        if (input == null || input.getText() == null) {
            return fallback;
        }
        try {
            final int value = Integer.parseInt(input.getText().toString().trim());
            return Math.max(min, Math.min(max, value));
        } catch (NumberFormatException ignored) {
            return fallback;
        }
    }

    private String readTextInput(EditText input, String fallback) {
        if (input == null || input.getText() == null) {
            return fallback;
        }
        final String value = input.getText().toString().trim();
        return value.isEmpty() ? fallback : value;
    }

    private PublisherSourceChoice selectedPublisherSourceChoice() {
        final int position = publisherSourceSpinner.getSelectedItemPosition();
        final PublisherSourceChoice[] choices = PublisherSourceChoice.values();
        if (position < 0 || position >= choices.length) {
            return PublisherSourceChoice.CAMERA_MICROPHONE;
        }
        return choices[position];
    }

    private PublisherAudioChoice selectedPublisherAudioChoice() {
        final int position = publisherAudioSpinner.getSelectedItemPosition();
        if (position < 0 || position >= publisherAudioChoices.size()) {
            return PublisherAudioChoice.NONE;
        }
        return publisherAudioChoices.get(position);
    }

    private boolean selectedPublisherTransportSupported() {
        final PublisherSourceChoice sourceChoice = selectedPublisherSourceChoice();
        if (sourceChoice == PublisherSourceChoice.DESKTOP) {
            return false;
        }
        return sourceChoice != PublisherSourceChoice.NONE
                || selectedPublisherAudioChoice() != PublisherAudioChoice.NONE;
    }

    private Gb28181SourceChoice selectedGb28181SourceChoice() {
        final int position = gbInviteSourceSpinner.getSelectedItemPosition();
        final Gb28181SourceChoice[] choices = Gb28181SourceChoice.values();
        if (position < 0 || position >= choices.length) {
            return Gb28181SourceChoice.CAMERA_MICROPHONE;
        }
        return choices[position];
    }

    private void applyGb28181SourceUiState() {
        final Gb28181SourceChoice sourceChoice = selectedGb28181SourceChoice();
        final boolean videoFileSource = sourceChoice == Gb28181SourceChoice.VIDEO_FILE;
        final Uri videoSourceUri = effectiveGbVideoUri();
        gbVideoFileRow.setVisibility(videoFileSource ? View.VISIBLE : View.GONE);
        gbMediaSourceInput.setEnabled(videoFileSource);
        gbMediaSourceInput.setVisibility(videoFileSource ? View.VISIBLE : View.GONE);
        if (videoFileSource) {
            final String defaultVideoPath = buildPreviewMediaPath();
            if (readTextInput(gbMediaSourceInput, "").isEmpty() && !defaultVideoPath.isEmpty()) {
                gbMediaSourceInput.setText(defaultVideoPath);
            }
            stopGbCameraPreviewInternal(false);
            gbCameraPreviewSurface.setVisibility(View.GONE);
            gbVideoPreviewView.setVisibility(
                    videoSourceUri != null ? View.VISIBLE : View.GONE);
            if (videoSourceUri != null) {
                startGbVideoPreview(videoSourceUri);
            }
        } else {
            stopGbVideoPreview();
            gbCameraPreviewSurface.setVisibility(View.VISIBLE);
        }
        if (videoFileSource
                && videoSourceUri == null
                && readTextInput(gbMediaSourceInput, "").isEmpty()) {
            gbVideoFileRow.post(this::openGbVideoFilePicker);
        }
        updateGbPreviewHint();
        renderInfoText();
    }

    private void applyPublisherSourceUiState() {
        final PublisherSourceChoice sourceChoice = selectedPublisherSourceChoice();
        final boolean cameraSource = sourceChoice == PublisherSourceChoice.CAMERA_MICROPHONE;
        final boolean videoFileSource = sourceChoice == PublisherSourceChoice.VIDEO_FILE;
        final boolean imageSource = sourceChoice == PublisherSourceChoice.STILL_IMAGE;
        final PublisherAudioChoice previousAudioChoice = selectedPublisherAudioChoice();
        publisherAudioChoices = audioChoicesForPublisherSource(sourceChoice);
        final List<String> audioLabels = new ArrayList<>();
        for (PublisherAudioChoice choice : publisherAudioChoices) {
            audioLabels.add(publisherAudioChoiceLabel(choice));
        }
        final ArrayAdapter<String> audioAdapter = buildSpinnerAdapter(audioLabels);
        updatingPublisherAudioChoices = true;
        publisherAudioSpinner.setAdapter(audioAdapter);
        final int selection = Math.max(0, publisherAudioChoices.indexOf(previousAudioChoice));
        publisherAudioSpinner.setSelection(selection);
        updatingPublisherAudioChoices = false;
        refreshPublisherResolutionOptions(sourceChoice);
        final boolean audioFileSource =
                selectedPublisherAudioChoice() == PublisherAudioChoice.FILE_AUDIO;

        publisherCameraSourceRow.setVisibility(cameraSource ? View.VISIBLE : View.GONE);
        publisherMediaFileRow.setVisibility(
                videoFileSource || audioFileSource ? View.VISIBLE : View.GONE);
        publisherMediaPathInput.setEnabled(videoFileSource || audioFileSource);
        publisherMediaPathInput.setVisibility(
                videoFileSource || audioFileSource ? View.VISIBLE : View.GONE);
        choosePublisherMediaFileButton.setEnabled(videoFileSource || audioFileSource);
        choosePublisherMediaFileButton.setVisibility(
                videoFileSource || audioFileSource ? View.VISIBLE : View.GONE);
        publisherImagePathInput.setEnabled(imageSource);
        publisherImagePathInput.setVisibility(imageSource ? View.VISIBLE : View.GONE);
        updateGeneratedPublisherPathInputs();
        updatePublisherActionLabels();
        renderInfoText();
    }

    private void refreshPublisherResolutionOptions(PublisherSourceChoice sourceChoice) {
        if (publisherResolutionSpinner == null) {
            return;
        }
        final PublisherResolutionOption previous = selectedPublisherResolutionOption();
        final List<PublisherResolutionOption> options = new ArrayList<>();
        switch (sourceChoice) {
            case CAMERA_MICROPHONE:
                options.addAll(selectedCameraSourceOption().resolutions);
                break;
            case DESKTOP:
                options.add(new PublisherResolutionOption(
                        readIntInput(desktopWidthInput, 1280, 1, 7680)
                                + "x"
                                + readIntInput(desktopHeightInput, 720, 1, 4320),
                        readIntInput(desktopWidthInput, 1280, 1, 7680),
                        readIntInput(desktopHeightInput, 720, 1, 4320)));
                break;
            case VIDEO_FILE:
                options.add(new PublisherResolutionOption("Video file original", 1280, 720));
                options.add(new PublisherResolutionOption("1280x720", 1280, 720));
                break;
            case STILL_IMAGE:
                options.add(new PublisherResolutionOption("Still image 640x360", 640, 360));
                options.add(new PublisherResolutionOption("1280x720", 1280, 720));
                break;
            case NONE:
            default:
                options.add(new PublisherResolutionOption("No video source", 0, 0));
                break;
        }
        publisherResolutionOptions = options.isEmpty()
                ? defaultPublisherResolutionOptions()
                : options;
        final List<String> labels = new ArrayList<>();
        for (PublisherResolutionOption option : publisherResolutionOptions) {
            labels.add(option.label);
        }
        publisherResolutionSpinner.setAdapter(buildSpinnerAdapter(labels));
        int selection = 0;
        if (previous != null) {
            for (int index = 0; index < publisherResolutionOptions.size(); ++index) {
                final PublisherResolutionOption option = publisherResolutionOptions.get(index);
                if (option.width == previous.width && option.height == previous.height) {
                    selection = index;
                    break;
                }
            }
        }
        publisherResolutionSpinner.setSelection(selection);
        publisherResolutionSpinner.setEnabled(sourceChoice != PublisherSourceChoice.NONE);
    }

    private void updatePublisherActionLabels() {
        if (startPublisherPreviewButton == null
                || stopPublisherPreviewButton == null
                || runPublisherLocalCaptureButton == null
                || stopPublisherLocalCaptureButton == null) {
            return;
        }
        if (activePublisherLocalCaptureSession != null) {
            runPublisherLocalCaptureButton.setText(R.string.stop_publisher_local_capture);
        } else if (activePublisherPreviewSession != null) {
            runPublisherLocalCaptureButton.setText(R.string.stop_publisher_preview);
        } else {
            runPublisherLocalCaptureButton.setText(R.string.run_publisher_local_capture);
        }
    }

    private List<PublisherAudioChoice> audioChoicesForPublisherSource(
            PublisherSourceChoice sourceChoice) {
        final List<PublisherAudioChoice> choices = new ArrayList<>();
        switch (sourceChoice) {
            case DESKTOP:
            case CAMERA_MICROPHONE:
            case STILL_IMAGE:
                choices.add(PublisherAudioChoice.NONE);
                choices.add(PublisherAudioChoice.MICROPHONE);
                choices.add(PublisherAudioChoice.FILE_AUDIO);
                break;
            case NONE:
                choices.add(PublisherAudioChoice.NONE);
                choices.add(PublisherAudioChoice.MICROPHONE);
                choices.add(PublisherAudioChoice.FILE_AUDIO);
                break;
            case VIDEO_FILE:
                choices.add(PublisherAudioChoice.NONE);
                break;
            default:
                choices.add(PublisherAudioChoice.NONE);
                break;
        }
        return choices;
    }

    private String publisherAudioChoiceLabel(PublisherAudioChoice choice) {
        switch (choice) {
            case MICROPHONE:
                return getString(R.string.publisher_audio_microphone);
            case SYSTEM_AUDIO:
                return getString(R.string.publisher_audio_system);
            case FILE_AUDIO:
                return getString(R.string.publisher_audio_file);
            case NONE:
            default:
                return getString(R.string.publisher_audio_none);
        }
    }

    private void searchOnvifDevices() {
        if (!demoInitialized || !initializationFailureMessage.isEmpty()
                || onvifSearchInProgress || onvifResolveInProgress) {
            renderInfoText();
            return;
        }

        final StreamCoreOnvif.DiscoveryConfig config =
                StreamCoreOnvif.defaultDiscoveryConfig();
        config.responseWaitMs = 1200;
        config.sendProbeCount = 2;

        onvifSearchInProgress = true;
        showOnvifStatus(getString(R.string.player_onvif_searching));
        renderInfoText();

        Thread worker = new Thread(() -> {
            final StreamCoreOnvif.DiscoveryResult result =
                    StreamCoreOnvif.discoverDevices(getApplicationContext(), config);
            runOnUiThread(() -> {
                onvifSearchInProgress = false;
                onvifDevices = new ArrayList<>(result.devices);
                refreshOnvifDeviceSpinner();
                showOnvifStatus(getString(R.string.player_onvif_result,
                        result.status.statusName,
                        result.devices.size(),
                        result.status.detail));
                if (!result.devices.isEmpty()) {
                    updateOnvifStatusForSelection();
                }
                renderInfoText();
            });
        }, "streamcore-onvif-search");
        worker.start();
    }

    private void toggleOnvifPanel() {
        final boolean expanded = playerOnvifControls.getVisibility() != View.VISIBLE;
        playerOnvifControls.setVisibility(expanded ? View.VISIBLE : View.GONE);
    }

    private void applySelectedOnvifStream() {
        final StreamCoreOnvif.Device device = selectedOnvifDevice();
        if (device == null) {
            return;
        }
        showOnvifStatus("Selected ONVIF device endpoint: "
                + emptyMarker(device.serviceUrl)
                + "\nEnter the device RTSP URL in Player URL to start playback.");
        renderInfoText();
    }

    private static String onvifProbeEndpointForDevice(StreamCoreOnvif.Device device) {
        if (device == null) {
            return "";
        }
        if (!device.responseEndpoint.isEmpty()) {
            return device.responseEndpoint;
        }
        if (device.serviceUrl.isEmpty()) {
            return "";
        }
        try {
            final URI uri = new URI(device.serviceUrl);
            final String host = uri.getHost();
            return host.isEmpty() ? "" : host + ":3702";
        } catch (URISyntaxException e) {
            return "";
        }
    }

    private void refreshOnvifDeviceSpinner() {
        final List<String> labels = new ArrayList<>();
        for (StreamCoreOnvif.Device device : onvifDevices) {
            labels.add(device.displayLabel());
        }
        if (labels.isEmpty()) {
            labels.add(getString(R.string.player_onvif_empty));
        }
        playerOnvifDeviceSpinner.setAdapter(buildSpinnerAdapter(labels));
        playerOnvifDeviceSpinner.setSelection(0);
    }

    private void updateOnvifStatusForSelection() {
        final StreamCoreOnvif.Device device = selectedOnvifDevice();
        if (device == null) {
            applyOnvifStreamButton.setEnabled(false);
            if (!onvifSearchInProgress) {
                showOnvifStatus("");
                renderInfoText();
            }
            return;
        }
        applyOnvifStreamButton.setEnabled(!onvifSearchInProgress && !onvifResolveInProgress);
        showOnvifStatus("Selected ONVIF device:"
                + "\n- urn: " + emptyMarker(device.deviceUrn)
                + "\n- response: " + emptyMarker(device.responseEndpoint)
                + "\n- service: " + emptyMarker(device.serviceUrl)
                + "\n- Profile S advertised: " + device.profileSAdvertised
                + "\n- Profile T advertised: " + device.profileTAdvertised);
        renderInfoText();
    }

    private StreamCoreOnvif.Device selectedOnvifDevice() {
        final int position = playerOnvifDeviceSpinner.getSelectedItemPosition();
        if (position < 0 || position >= onvifDevices.size()) {
            return null;
        }
        return onvifDevices.get(position);
    }

    private void showOnvifStatus(String text) {
        playerOnvifStatus = text == null ? "" : text;
    }

    private static String emptyMarker(String value) {
        return value == null || value.isEmpty() ? "<empty>" : value;
    }

    private void updateGeneratedPublisherPathInputs() {
        final String currentMediaPath = publisherMediaPathInput.getText().toString().trim();
        final String generatedVideoPath =
                previewMediaFile == null ? "" : previewMediaFile.getAbsolutePath();
        final String generatedAudioPath =
                demoAudioFile == null ? "" : demoAudioFile.getAbsolutePath();
        if (currentMediaPath.isEmpty()
                || currentMediaPath.equals(generatedVideoPath)
                || currentMediaPath.equals(generatedAudioPath)) {
            final File defaultFile =
                    selectedPublisherAudioChoice() == PublisherAudioChoice.FILE_AUDIO
                            ? demoAudioFile
                            : previewMediaFile;
            if (defaultFile != null) {
                publisherMediaPathInput.setText(defaultFile.getAbsolutePath());
            }
        }
        if (publisherImagePathInput.getText().toString().trim().isEmpty()
                && demoStillImageFile != null) {
            publisherImagePathInput.setText(demoStillImageFile.getAbsolutePath());
        }
    }

    private StreamCorePlayer.VideoPresentPath selectedPlayerPresentPath() {
        final int position = playerRenderBackendSpinner.getSelectedItemPosition();
        if (position == 0) {
            return StreamCorePlayer.VideoPresentPath.SOFTWARE_FRAME;
        }
        if (position == 1) {
            return StreamCorePlayer.VideoPresentPath.GPU_FRAME;
        }
        if (position == 2) {
            return StreamCorePlayer.VideoPresentPath.DIRECT_SURFACE;
        }
        return StreamCorePlayer.VideoPresentPath.AUTO;
    }

    private boolean selectedPlayerPrefersSoftwareBackend() {
        return selectedPlayerPresentPath() == StreamCorePlayer.VideoPresentPath.SOFTWARE_FRAME;
    }

    private boolean selectedPlayerUsesHardwareDecode() {
        return playerDecodeModeSpinner != null
                && playerDecodeModeSpinner.getSelectedItemPosition() == 1;
    }

    private void startPlayerPreview() {
        if (!isPreviewActionReady()) {
            renderInfoText();
            return;
        }
        if (!hasUsableSurface(playerPreviewSurface)) {
            final StreamCoreOperationStatus status = buildStatus(
                    StreamCoreResultCode.INVALID_ARGUMENT,
                    "player_preview_surface_unavailable",
                    "player preview surface is not ready.",
                    "TextureView has not created a valid Surface yet.");
            playerPreviewScenario = PlayerPreviewScenarioResult.blocked(status);
            recordDemoStatus("player.preview.start", status);
            renderInfoText();
            return;
        }

        stopPlayerPreviewInternal(false);
        playerPreviewStartInProgress = true;
        playerPreviewScenario = PlayerPreviewScenarioResult.starting();
        applyPreviewDisplayMode();
        final StreamCoreSurfaceTarget previewTarget =
                StreamCoreSurfaceTarget.fromView(playerPreviewSurface);
        final int playbackBufferMs =
                readIntInput(playerBufferMsInput, 300, -1, 10_000);
        final int audioQueueLimit =
                readIntInput(playerAudioQueueInput, 12, 0, 400);
        final int videoQueueLimit =
                readIntInput(playerVideoQueueInput, 6, 0, 200);
        final StreamCorePlayer.VideoPresentPath presentPath =
                selectedPlayerPresentPath();
        final boolean preferSoftwareBackend =
                selectedPlayerPrefersSoftwareBackend();
        final String playerSourceUrl = readPlayerSourceUrl();
        final int previewWidth = Math.max(0, playerPreviewSurface.getWidth());
        final int previewHeight = Math.max(0, playerPreviewSurface.getHeight());
        renderInfoText();

        Thread worker = new Thread(() -> {
            final StreamCorePlayer.Session session = new StreamCorePlayer.Session();
            final StreamCoreOperationStatus configStatus = session.setConfig(
                    StreamCorePlayer.Config.newBuilder()
                            .sessionName("android_demo_player_preview")
                            .sourceKind(StreamCorePlayer.SourceKind.URL)
                            .sourceUrl(playerSourceUrl)
                            .allowReconnect(!playerSourceUrl.startsWith("file://"))
                            .renderMode(StreamCorePlayer.RenderMode.NATIVE_WINDOW)
                            .renderTarget(previewTarget)
                            .videoPresentPath(presentPath)
                            .preferSoftwareRenderBackend(preferSoftwareBackend)
                            .allowVideoSoftwareFallback(true)
                            .requestedSize(previewWidth, previewHeight)
                            .useHardwareDecode(selectedPlayerUsesHardwareDecode())
                            .playbackCacheMilliseconds(playbackBufferMs, playbackBufferMs)
                            .playbackQueueLimits(audioQueueLimit, videoQueueLimit)
                            .realtimePlaybackProfile(
                                    playbackBufferMs == 0,
                                    audioQueueLimit,
                                    videoQueueLimit)
                            .enableAudio(true)
                            .build());
            final StreamCorePlayer.Preflight preflight = session.preflight();
            final StreamCoreOperationStatus startStatus =
                    configStatus.isOk() ? session.start() : configStatus;
            final StreamCorePlayer.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
            final boolean keepSession = startStatus.resultCode == StreamCoreResultCode.OK;
            if (!keepSession) {
                session.stop();
                session.close();
            }
            runOnUiThread(() -> {
                playerPreviewStartInProgress = false;
                if (keepSession) {
                    activePlayerPreviewSession = session;
                }
                playerPreviewScenario = keepSession
                        ? PlayerPreviewScenarioResult.started(
                        preflight, startStatus, runtimeAfterStart)
                        : PlayerPreviewScenarioResult.failed(
                        preflight, startStatus, runtimeAfterStart);
                recordDemoStatus("player.preview.start", startStatus);
                renderInfoText();
            });
        }, "streamcore-player-preview-start");
        worker.start();
    }

    private void stopPlayerPreview() {
        stopPlayerPreviewInternal(true);
    }

    private void stopPlayerPreviewInternal(boolean renderAfterStop) {
        if (activePlayerPreviewSession == null) {
            if (renderAfterStop) {
                final StreamCoreOperationStatus status = buildStatus(
                        StreamCoreResultCode.OK,
                        "player_preview_not_running",
                        "player preview is not running.",
                        "Start the player preview first.");
                playerPreviewScenario = playerPreviewScenario.withStop(status);
                recordDemoStatus("player.preview.stop", status);
                renderInfoText();
            }
            return;
        }
        final StreamCorePlayer.Session session = activePlayerPreviewSession;
        session.stop();
        session.close();
        activePlayerPreviewSession = null;
        final StreamCoreOperationStatus status = buildStatus(
                StreamCoreResultCode.OK,
                "player_preview_stopped",
                "player preview stopped and the Surface target was released.",
                "player session stopped.");
        playerPreviewScenario = playerPreviewScenario.withStop(status);
        recordDemoStatus("player.preview.stop", status);
        if (renderAfterStop) {
            renderInfoText();
        }
    }

    private void startCameraPreview() {
        startCameraBackedPreview("camera_preview");
    }

    private void startPublisherPreview() {
        if (selectedPublisherSourceChoice() != PublisherSourceChoice.CAMERA_MICROPHONE) {
            publisherPreviewScenario = CapturePreviewScenarioResult.blocked(
                    "publisher_preview",
                    buildStatus(
                            StreamCoreResultCode.INVALID_ARGUMENT,
                            "publisher_preview_source_not_camera",
                            "publisher preview uses the camera source only.",
                            "Select Camera + microphone before opening the live preview surface."));
            renderInfoText();
            return;
        }
        startCameraBackedPreview("publisher_preview");
    }

    private void startGbSourcePreview() {
        if (selectedGb28181SourceChoice() == Gb28181SourceChoice.VIDEO_FILE) {
            final Uri videoSourceUri = effectiveGbVideoUri();
            if (videoSourceUri == null) {
                gb28181StatusText = "Select a video file before starting the GB28181 source preview.";
                renderInfoText();
                openGbVideoFilePicker();
                return;
            }
            startGbVideoPreview(videoSourceUri);
            gb28181StatusText = "GB28181 video-file source preview is running.";
            renderInfoText();
            return;
        }
        startCameraBackedPreview("gb28181_preview");
    }

    private void stopGbSourcePreview() {
        stopGbSourcePreviewInternal(true);
    }

    private boolean stopGbSourcePreviewInternal(boolean renderAfterStop) {
        final boolean stoppedCamera = stopGbCameraPreviewInternal(false);
        final boolean stoppedVideo = stopGbVideoPreview();
        if (renderAfterStop) {
            gb28181StatusText = stoppedCamera || stoppedVideo
                    ? "GB28181 source preview stopped."
                    : "GB28181 source preview is not running.";
            renderInfoText();
        }
        return stoppedCamera || stoppedVideo;
    }

    private void startCameraBackedPreview(String scenarioKey) {
        if (!isPreviewActionReady()) {
            renderInfoText();
            return;
        }
        if (!canStartCameraBackedPreview(scenarioKey)) {
            setCapturePreviewScenario(
                    scenarioKey,
                    CapturePreviewScenarioResult.blocked(
                            scenarioKey,
                            buildStatus(
                                    StreamCoreResultCode.OPERATION_FAILED,
                                    scenarioKey + "_camera_busy",
                                    "another camera preview is already running.",
                                    "Stop the current camera-backed preview before starting a different preview surface.")));
            renderInfoText();
            return;
        }
        if (!hasCameraPermission()) {
            pendingCameraPermissionScenario = scenarioKey;
            pendingCameraPermissionAction = () -> startCameraBackedPreview(scenarioKey);
            cameraPermissionLauncher.launch(
                    DemoPermissionSupport.missingCameraPreviewPermissions(this));
            setPreviewPermissionPending(scenarioKey);
            renderInfoText();
            return;
        }

        final TextureView targetSurface = "publisher_preview".equals(scenarioKey)
                ? publisherPreviewSurface
                : ("gb28181_preview".equals(scenarioKey)
                ? gbCameraPreviewSurface
                : cameraPreviewSurface);
        if (!hasUsableSurface(targetSurface)) {
            setCapturePreviewScenario(
                    scenarioKey,
                    CapturePreviewScenarioResult.blocked(
                            scenarioKey,
                            buildStatus(
                                    StreamCoreResultCode.INVALID_ARGUMENT,
                                    scenarioKey + "_surface_unavailable",
                                    "camera preview surface is not ready.",
                                    "TextureView has not created a valid Surface yet.")));
            renderInfoText();
            return;
        }

        if ("publisher_preview".equals(scenarioKey)) {
            stopPublisherPreviewInternal(false);
            publisherPreviewStartInProgress = true;
        } else if ("gb28181_preview".equals(scenarioKey)) {
            stopGbSourcePreviewInternal(false);
            gbPreviewStartInProgress = true;
            gbCameraPreviewSurface.setVisibility(View.VISIBLE);
            gbVideoPreviewView.setVisibility(View.GONE);
            updateGbPreviewHint();
        } else {
            stopCameraPreviewInternal(false);
            cameraPreviewStartInProgress = true;
        }
        setCapturePreviewScenario(scenarioKey, CapturePreviewScenarioResult.starting(scenarioKey));
        applyPreviewDisplayMode();
        final StreamCoreSurfaceTarget previewTarget =
                StreamCoreSurfaceTarget.fromView(targetSurface);
        final String cameraSourceId =
                selectedCameraSourceId();
        final int cameraFps =
                readIntInput(cameraFpsInput, 30, 1, 120);
        final String publisherVideoCodec =
                selectedPublisherVideoCodec();
        final String publisherAudioCodec =
                selectedPublisherAudioCodec();
        final int publisherVideoBitrate =
                readPublisherVideoBitrateKbps();
        final int publisherVideoWidth = readPublisherVideoWidth();
        final int publisherVideoHeight = readPublisherVideoHeight();
        final int publisherFps =
                readPublisherFps();
        final int publisherGop =
                readPublisherGopFrames();
        renderInfoText();

        Thread worker = new Thread(() -> {
            final StreamCoreCapture.Session session = new StreamCoreCapture.Session();
            final StreamCoreOperationStatus configStatus = session.setConfig(
                    StreamCoreCapture.Config.newBuilder()
                            .sessionName("android_demo_" + scenarioKey)
                            .sourceKind(StreamCoreCapture.SourceKind.CAMERA)
                            .sourceId(cameraSourceId)
                            .targetFrameRate(cameraFps)
                            .previewTarget(previewTarget)
                            .enableAudio(false)
                            .enableVideo(true)
                            .build());
            final StreamCoreCapture.Preflight preflight = session.preflight();
            final StreamCoreOperationStatus startStatus =
                    configStatus.isOk() ? session.start() : configStatus;
            final StreamCoreCapture.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
            final StreamCoreOperationStatus publisherStatus =
                    "publisher_preview".equals(scenarioKey)
                            ? buildPublisherPreviewTransportStatus(
                            publisherAudioCodec,
                            publisherVideoCodec,
                            publisherVideoBitrate,
                            publisherVideoWidth,
                            publisherVideoHeight,
                            publisherFps,
                            publisherGop)
                            : null;
            final boolean keepSession = startStatus.resultCode == StreamCoreResultCode.OK;
            if (!keepSession) {
                session.stop();
            }
            runOnUiThread(() -> {
                if ("publisher_preview".equals(scenarioKey)) {
                    publisherPreviewStartInProgress = false;
                    if (keepSession) {
                        activePublisherPreviewSession = session;
                    }
                } else if ("gb28181_preview".equals(scenarioKey)) {
                    gbPreviewStartInProgress = false;
                    if (keepSession) {
                        activeGbPreviewSession = session;
                    }
                } else {
                    cameraPreviewStartInProgress = false;
                    if (keepSession) {
                        activeCameraPreviewSession = session;
                    }
                }
                setCapturePreviewScenario(
                        scenarioKey,
                        keepSession
                                ? CapturePreviewScenarioResult.started(
                                scenarioKey, preflight, startStatus, runtimeAfterStart,
                                publisherStatus)
                                : CapturePreviewScenarioResult.failed(
                                scenarioKey, preflight, startStatus, runtimeAfterStart,
                                publisherStatus));
                if ("gb28181_preview".equals(scenarioKey)) {
                    gb28181StatusText = keepSession
                            ? "GB28181 camera source preview is running."
                            : "GB28181 camera source preview failed to start.";
                    updateGbPreviewHint();
                }
                recordDemoStatus(scenarioKey + ".start", startStatus);
                renderInfoText();
            });
        }, "streamcore-" + scenarioKey + "-start");
        worker.start();
    }

    private boolean canStartCameraBackedPreview(String scenarioKey) {
        return ("camera_preview".equals(scenarioKey) || activeCameraPreviewSession == null)
                && ("publisher_preview".equals(scenarioKey) || activePublisherPreviewSession == null)
                && ("gb28181_preview".equals(scenarioKey) || activeGbPreviewSession == null);
    }

    private void stopCameraPreview() {
        stopCameraPreviewInternal(true);
    }

    private void stopCameraPreviewInternal(boolean renderAfterStop) {
        if (activeCameraPreviewSession == null) {
            if (renderAfterStop) {
                final StreamCoreOperationStatus status = buildStatus(
                        StreamCoreResultCode.OK,
                        "camera_preview_not_running",
                        "camera preview is not running.",
                        "Start the camera preview first.");
                cameraPreviewScenario = cameraPreviewScenario.withStop(status);
                recordDemoStatus("camera_preview.stop", status);
                renderInfoText();
            }
            return;
        }
        activeCameraPreviewSession.stop();
        activeCameraPreviewSession = null;
        final StreamCoreOperationStatus status = buildStatus(
                StreamCoreResultCode.OK,
                "camera_preview_stopped",
                "camera preview stopped and the Surface target was released.",
                "capture session stopped.");
        cameraPreviewScenario = cameraPreviewScenario.withStop(status);
        recordDemoStatus("camera_preview.stop", status);
        if (renderAfterStop) {
            renderInfoText();
        }
    }

    private void stopPublisherPreview() {
        stopPublisherPreviewInternal(true);
    }

    private void stopPublisherPreviewInternal(boolean renderAfterStop) {
        if (activePublisherPreviewSession == null) {
            if (renderAfterStop) {
                final StreamCoreOperationStatus status = buildStatus(
                        StreamCoreResultCode.OK,
                        "publisher_preview_not_running",
                        "publisher source preview is not running.",
                        "Start the publisher preview first.");
                publisherPreviewScenario = publisherPreviewScenario.withStop(status);
                recordDemoStatus("publisher_preview.stop", status);
                renderInfoText();
            }
            return;
        }
        activePublisherPreviewSession.stop();
        activePublisherPreviewSession = null;
        final StreamCoreOperationStatus status = buildStatus(
                StreamCoreResultCode.OK,
                "publisher_preview_stopped",
                "publisher camera-source preview stopped and the Surface target was released.",
                "publisher preview is idle.");
        publisherPreviewScenario = publisherPreviewScenario.withStop(status);
        recordDemoStatus("publisher_preview.stop", status);
        if (renderAfterStop) {
            renderInfoText();
        }
    }

    private boolean stopGbCameraPreviewInternal(boolean renderAfterStop) {
        if (activeGbPreviewSession == null) {
            if (renderAfterStop) {
                final StreamCoreOperationStatus status = buildStatus(
                        StreamCoreResultCode.OK,
                        "gb28181_preview_not_running",
                        "GB28181 camera source preview is not running.",
                        "Start the GB28181 source preview first.");
                gbPreviewScenario = gbPreviewScenario.withStop(status);
                recordDemoStatus("gb28181_preview.stop", status);
                renderInfoText();
            }
            updateGbPreviewHint();
            return false;
        }
        activeGbPreviewSession.stop();
        activeGbPreviewSession = null;
        final StreamCoreOperationStatus status = buildStatus(
                StreamCoreResultCode.OK,
                "gb28181_preview_stopped",
                "GB28181 camera source preview stopped.",
                "The StreamCoreCapture preview session bound to the GB28181 page was released.");
        gbPreviewScenario = gbPreviewScenario.withStop(status);
        recordDemoStatus("gb28181_preview.stop", status);
        updateGbPreviewHint();
        if (renderAfterStop) {
            renderInfoText();
        }
        return true;
    }

    private void evaluateSelectedPublisherSource() {
        if (!demoInitialized || !initializationFailureMessage.isEmpty()) {
            renderInfoText();
            return;
        }
        updateGeneratedPublisherPathInputs();
        final PublisherScenarioResult scenario = buildSelectedPublisherSourceScenario();
        upsertPublisherScenario(scenario);
        recordDemoStatus("publisher.source.check", scenario.startStatus);
        renderInfoText();
    }

    private void runSelectedPublisherTransport() {
        if (!demoInitialized
                || !initializationFailureMessage.isEmpty()
                || publisherLocalCaptureRunInProgress
                || activePublisherLocalCaptureSession != null) {
            renderInfoText();
            return;
        }
        if (!selectedPublisherTransportSupported()) {
            evaluateSelectedPublisherSource();
            return;
        }
        if (activeCameraPreviewSession != null
                || activePublisherPreviewSession != null
                || activeGbPreviewSession != null) {
            upsertPublisherScenario(buildSelectedPublisherSkippedScenario(
                    StreamCoreResultCode.NOT_ENABLED,
                    "publisher_preview_active",
                    "Publish is waiting for preview sessions to stop.",
                    "Stop camera, publisher, or GB28181 preview before starting publish."));
            renderInfoText();
            return;
        }

        updateGeneratedPublisherPathInputs();
        final String publishUrl = readPublisherPublishUrl();
        if (!isPublisherUrlSyntaxValid(publishUrl)) {
            upsertPublisherScenario(buildSelectedPublisherSkippedScenario(
                    StreamCoreResultCode.NOT_ENABLED,
                    "publisher_url_invalid",
                    "Publish URL is invalid.",
                    "Enter a URL such as rtmp://"
                            + DEFAULT_DEVELOPMENT_RTMP_HOST
                            + ":1935/live/mobile_publish or rtsp://"
                            + DEFAULT_DEVELOPMENT_RTMP_HOST
                            + ":8554/live/mobile_publish."));
            renderInfoText();
            return;
        }

        final StreamCorePublisher.Config config =
                buildSelectedPublisherPermissionConfig();
        final String[] missingPermissions =
                DemoPermissionSupport.missingPublisherLocalCapturePermissions(this, config);
        if (missingPermissions.length > 0) {
            publisherPermissionRequestInProgress = true;
            publisherPermissionResumeDirectTransport = false;
            upsertPublisherScenario(buildSelectedPublisherSkippedScenario(
                    StreamCoreResultCode.NOT_ENABLED,
                    "publisher_permission_requesting",
                    "Requesting publisher permission.",
                    "Grant the required runtime permissions to start publish."));
            renderInfoText();
            publisherPermissionLauncher.launch(missingPermissions);
            return;
        }

        publisherLocalCaptureRunInProgress = true;
        upsertPublisherScenario(buildSelectedPublisherSkippedScenario(
                StreamCoreResultCode.NOT_ENABLED,
                "publisher_starting",
                "Publish is starting.",
                "The selected source is preparing its publish transport."));
        renderInfoText();

        Thread worker = new Thread(() -> {
            final boolean endpointReachable = isPublisherUrlReachable(publishUrl);
            final PublisherLocalCaptureStartResult startResult =
                    startSelectedPublisherTransportScenario(
                            config,
                            publishUrl,
                            endpointReachable);
            runOnUiThread(() -> {
                publisherLocalCaptureRunInProgress = false;
                if (startResult.activeSession != null) {
                    activePublisherLocalCaptureSession = startResult.activeSession;
                }
                upsertPublisherScenario(startResult.scenario);
                recordDemoStatus(
                        "publisher.publish.start",
                        startResult.scenario.startStatus);
                renderInfoText();
            });
        }, "streamcore-publisher-selected-transport");
        worker.start();
    }

    private void runPublisherLocalCaptureNativeTransport() {
        if (!demoInitialized
                || !initializationFailureMessage.isEmpty()
                || publisherLocalCaptureRunInProgress
                || activePublisherLocalCaptureSession != null) {
            renderInfoText();
            return;
        }
        if (selectedPublisherSourceChoice() != PublisherSourceChoice.CAMERA_MICROPHONE
                || selectedPublisherAudioChoice() != PublisherAudioChoice.MICROPHONE) {
            upsertPublisherScenario(buildLocalCaptureNativeSkippedScenario(
                    "publisher_local_capture_source_not_camera",
                    "direct publish currently supports camera + microphone only.",
                    "selected source="
                            + publisherSourceLabel(selectedPublisherSourceChoice())
                            + "; use Check source config to validate this source contract.",
                    readPublisherPublishUrl()));
            renderInfoText();
            return;
        }
        if (activeCameraPreviewSession != null
                || activePublisherPreviewSession != null
                || activeGbPreviewSession != null) {
            upsertPublisherScenario(buildLocalCaptureNativeSkippedScenario(
                    "publisher_local_capture_preview_active",
                    "direct publish is waiting for preview sessions to stop.",
                    "camera or publisher preview is active.",
                    readPublisherPublishUrl()));
            renderInfoText();
            return;
        }
        if (!hasCameraPermission() || !hasRecordAudioPermission()) {
            publisherPermissionRequestInProgress = true;
            publisherPermissionResumeDirectTransport = true;
            upsertPublisherScenario(buildLocalCaptureNativeSkippedScenario(
                    "publisher_local_capture_permission_requesting",
                    "requesting camera and microphone permission for publish.",
                    "permission prompt pending.",
                    readPublisherPublishUrl()));
            renderInfoText();
            publisherPermissionLauncher.launch(
                    DemoPermissionSupport.missingPublisherLocalCapturePermissions(
                            this,
                            buildPublisherLocalCapturePermissionConfig()));
            return;
        }

        final String publishUrl = readPublisherPublishUrl();
        if (!isPublisherUrlSyntaxValid(publishUrl)) {
            upsertPublisherScenario(buildLocalCaptureNativeSkippedScenario(
                    "publisher_local_capture_url_invalid",
                    "Publish URL is invalid.",
                    "Enter a URL such as rtmp://"
                            + DEFAULT_DEVELOPMENT_RTMP_HOST
                            + ":1935/live/local_native or rtsp://"
                            + DEFAULT_DEVELOPMENT_RTMP_HOST
                            + ":8554/live/local_native.",
                    publishUrl));
            renderInfoText();
            return;
        }

        publisherLocalCaptureRunInProgress = true;
        final String audioCodec = selectedPublisherAudioCodec();
        final String videoCodec = selectedPublisherVideoCodec();
        final int videoWidth = readPublisherVideoWidth();
        final int videoHeight = readPublisherVideoHeight();
        final int videoBitrate = readPublisherVideoBitrateKbps();
        final int fps = readPublisherFps();
        final int gop = readPublisherGopFrames();
        upsertPublisherScenario(buildLocalCaptureNativeSkippedScenario(
                "publisher_local_capture_starting",
                "Publish is starting.",
                "camera + microphone source selected.",
                publishUrl,
                audioCodec,
                videoCodec,
                videoBitrate,
                videoWidth,
                videoHeight,
                fps,
                gop));
        renderInfoText();

        final boolean cameraPermissionGranted = hasCameraPermission();
        final boolean audioPermissionGranted = hasRecordAudioPermission();
        Thread worker = new Thread(() -> {
            final boolean endpointReachable = isPublisherUrlReachable(publishUrl);
            final PublisherLocalCaptureStartResult startResult =
                    startLocalCaptureNativeTransportScenario(
                    cameraPermissionGranted,
                    audioPermissionGranted,
                    publishUrl,
                    endpointReachable,
                    audioCodec,
                    videoCodec,
                    videoBitrate,
                    videoWidth,
                    videoHeight,
                    fps,
                    gop);
            runOnUiThread(() -> {
                publisherLocalCaptureRunInProgress = false;
                if (startResult.activeSession != null) {
                    activePublisherLocalCaptureSession = startResult.activeSession;
                }
                upsertPublisherScenario(startResult.scenario);
                recordDemoStatus(
                        "publisher.publish.start",
                        startResult.scenario.startStatus);
                renderInfoText();
            });
        }, "streamcore-publisher-local-capture-native");
        worker.start();
    }

    private void stopPublisherLocalCaptureNativeTransport() {
        if (activePublisherLocalCaptureSession == null) {
            upsertPublisherScenario(buildLocalCaptureNativeSkippedScenario(
                    "publisher_local_capture_not_running",
                    "Publish is not running.",
                    "Start publish first.",
                    readPublisherPublishUrl()));
            renderInfoText();
            return;
        }

        final StreamCorePublisher.Session session = activePublisherLocalCaptureSession;
        PublisherScenarioResult previous =
                findPublisherScenario("selected_publish_transport");
        if (previous == null) {
            previous = findPublisherScenario("local_capture_native_transport");
        }
        final PublisherScenarioResult previousScenario = previous;
        final String publishUrl = readPublisherPublishUrl();
        final String audioCodec = selectedPublisherAudioCodec();
        final String videoCodec = selectedPublisherVideoCodec();
        final int videoBitrate = readPublisherVideoBitrateKbps();
        final int videoWidth = readPublisherVideoWidth();
        final int videoHeight = readPublisherVideoHeight();
        final int fps = readPublisherFps();
        final int gop = readPublisherGopFrames();
        activePublisherLocalCaptureSession = null;
        Thread worker = new Thread(() -> {
            session.stop();
            final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
            final PublisherScenarioResult stoppedScenario = previousScenario == null
                    ? buildLocalCaptureNativeSkippedScenario(
                    "publisher_local_capture_stopped",
                    "Publish stopped.",
                    "publisher runtime released.",
                    publishUrl,
                    audioCodec,
                    videoCodec,
                    videoBitrate,
                    videoWidth,
                    videoHeight,
                    fps,
                    gop)
                    : new PublisherScenarioResult(
                    previousScenario.scenarioKey,
                    previousScenario.preflight,
                    previousScenario.startStatus,
                    previousScenario.audioPushStatus,
                    previousScenario.videoPushStatus,
                    previousScenario.runtimeAfterStart,
                    runtimeAfterStop,
                    previousScenario.callbackMessage);
            runOnUiThread(() -> {
                upsertPublisherScenario(stoppedScenario);
                recordDemoStatus(
                        "publisher.publish.stop",
                        stoppedScenario.runtimeAfterStop == null
                                ? stoppedScenario.startStatus
                                : buildStatus(
                                StreamCoreResultCode.OK,
                                "publisher_local_capture_stopped",
                                "Publish stopped.",
                                "publisher runtime released."));
                renderInfoText();
            });
        }, "streamcore-publisher-local-capture-stop");
        worker.start();
        renderInfoText();
    }

    private void releaseVisualPreviewSessions() {
        stopPlayerPreviewInternal(false);
        stopCameraPreviewInternal(false);
        stopPublisherPreviewInternal(false);
        stopGbSourcePreviewInternal(false);
    }

    private void releasePublisherLocalCaptureSession() {
        if (activePublisherLocalCaptureSession == null) {
            return;
        }
        activePublisherLocalCaptureSession.stop();
        activePublisherLocalCaptureSession = null;
    }

    private void renderInfoText() {
        updatePreviewButtons();
        updateDesktopCaptureButtons();
        updateGb28181Buttons();
        updateMediaMetricOverlays();
        if (!initializationFailureMessage.isEmpty()) {
            recordDemoStatus(
                    "demo.initialize",
                    "-1",
                    "failed",
                    initializationFailureMessage);
            renderDemoStatusText();
            licenseSummaryText.setText(initializationFailureMessage);
            playerSummaryText.setText("");
            playerSummaryText.setVisibility(View.GONE);
            captureSummaryText.setText("");
            desktopCaptureSummaryText.setText("");
            publisherActionStatusText.setText("");
            publisherActionStatusText.setVisibility(View.GONE);
            publisherSummaryText.setText("");
            publisherSummaryText.setVisibility(View.GONE);
            gb28181SummaryText.setText("");
            hideMobileMediaSummaryViews();
            return;
        }
        if (!demoInitialized) {
            recordDemoStatus(
                    "demo.initialize",
                    "-",
                    "pending",
                    "Runtime, license, log, and demo scenarios are loading.");
            renderDemoStatusText();
            licenseSummaryText.setText("Please wait for runtime, capture, and publisher contracts to finish loading.");
            playerSummaryText.setText("");
            playerSummaryText.setVisibility(View.GONE);
            captureSummaryText.setText("");
            desktopCaptureSummaryText.setText("");
            publisherActionStatusText.setText("");
            publisherActionStatusText.setVisibility(View.GONE);
            publisherSummaryText.setText("");
            publisherSummaryText.setVisibility(View.GONE);
            gb28181SummaryText.setText("");
            hideMobileMediaSummaryViews();
            return;
        }
        renderDemoStatusText();
        updateGeneratedPublisherPathInputs();
        licenseSummaryText.setText(buildLicenseSummaryText());
        final String playerSummary = buildPlayerSummaryText();
        playerSummaryText.setText(playerSummary);
        captureSummaryText.setText(buildCaptureSummaryText());
        desktopCaptureSummaryText.setText(buildDesktopCaptureSummaryText());
        publisherActionStatusText.setText("");
        publisherActionStatusText.setVisibility(View.GONE);
        final String publisherSummary = buildPublisherSummaryText();
        publisherSummaryText.setText(publisherSummary);
        gb28181SummaryText.setText(buildGb28181SummaryText());
        hideMobileMediaSummaryViews();
    }

    private void updateMediaMetricOverlays() {
        if (playerMetricsOverlayText != null) {
            playerMetricsOverlayText.setText(buildPlayerMetricOverlayText());
        }
        updatePublisherPreviewHint();
        if (publisherMetricsOverlayText != null) {
            publisherMetricsOverlayText.setText(buildPublisherMetricOverlayText());
            publisherMetricsOverlayText.setVisibility(
                    shouldShowPublisherPassthroughHint() ? View.GONE : View.VISIBLE);
        }
        if (gbMetricsOverlayText != null) {
            gbMetricsOverlayText.setText(buildGbMetricOverlayText());
        }
        applyPreviewDisplayMode();
    }

    private boolean shouldShowPublisherPassthroughHint() {
        return selectedPublisherSourceChoice() == PublisherSourceChoice.VIDEO_FILE;
    }

    private void updatePublisherPreviewHint() {
        if (publisherPreviewHintText == null) {
            return;
        }
        publisherPreviewHintText.setVisibility(
                shouldShowPublisherPassthroughHint() ? View.VISIBLE : View.GONE);
    }

    private String buildPlayerMetricOverlayText() {
        final boolean running = activePlayerPreviewSession != null;
        final StreamCorePlayer.RuntimeInfo runtimeInfo = running
                ? activePlayerPreviewSession.getRuntimeInfo()
                : null;
        final String videoSize = formatPlayerVideoSize(runtimeInfo, running);
        return getString(
                R.string.player_metrics_overlay_template,
                videoSize,
                selectedPlayerDecoderSummary(),
                selectedPlayerRenderSummary(),
                readIntInput(playerBufferMsInput, 300, -1, 10_000),
                readIntInput(playerAudioQueueInput, 12, 0, 400),
                readIntInput(playerVideoQueueInput, 6, 0, 200));
    }

    private String buildPublisherMetricOverlayText() {
        final boolean active = activePublisherPreviewSession != null
                || publisherLocalCaptureRunInProgress
                || activePublisherLocalCaptureSession != null;
        return getString(
                R.string.publisher_metrics_overlay_template,
                readPublisherVideoWidth(),
                readPublisherVideoHeight(),
                readPublisherVideoBitrateKbps(),
                readPublisherFps(),
                readPublisherGopFrames(),
                active
                        ? getString(R.string.publisher_live_bitrate_pending)
                        : getString(R.string.publisher_live_bitrate_waiting));
    }

    private String formatPlayerVideoSize(
            StreamCorePlayer.RuntimeInfo runtimeInfo,
            boolean running) {
        if (runtimeInfo != null
                && runtimeInfo.videoWidth > 0
                && runtimeInfo.videoHeight > 0) {
            playerPreviewVideoWidth = runtimeInfo.videoWidth;
            playerPreviewVideoHeight = runtimeInfo.videoHeight;
            return runtimeInfo.videoWidth + "x" + runtimeInfo.videoHeight;
        }
        return running
                ? getString(R.string.player_metrics_stream_pending)
                : getString(R.string.player_metrics_stream_waiting);
    }

    private String selectedPlayerDecoderSummary() {
        return selectedPlayerUsesHardwareDecode()
                ? getString(R.string.player_decode_summary_hardware)
                : getString(R.string.player_decode_summary_software);
    }

    private String selectedPlayerRenderSummary() {
        final StreamCorePlayer.VideoPresentPath presentPath = selectedPlayerPresentPath();
        if (presentPath == StreamCorePlayer.VideoPresentPath.SOFTWARE_FRAME) {
            return getString(R.string.player_render_summary_software_frame);
        }
        if (presentPath == StreamCorePlayer.VideoPresentPath.GPU_FRAME) {
            return getString(R.string.player_render_summary_gpu_frame);
        }
        if (presentPath == StreamCorePlayer.VideoPresentPath.DIRECT_SURFACE) {
            return getString(R.string.player_render_summary_direct_surface);
        }
        return getString(R.string.player_render_summary_sdk_auto);
    }

    private void updateDesktopCaptureButtons() {
        final boolean initialized = demoInitialized && initializationFailureMessage.isEmpty();
        requestDesktopCaptureButton.setEnabled(initialized && !desktopCaptureStartInProgress);
        stopDesktopCaptureButton.setEnabled(
                initialized && !desktopCaptureStartInProgress && activeDesktopCaptureSession != null);
    }

    private void updateGb28181Buttons() {
        final boolean initialized = demoInitialized && initializationFailureMessage.isEmpty();
        final boolean videoSource = selectedGb28181SourceChoice() == Gb28181SourceChoice.VIDEO_FILE;
        final boolean previewActive = isGbVideoPreviewPlaying() || activeGbPreviewSession != null;
        chooseGbVideoFileButton.setEnabled(initialized && videoSource);
        startGbPreviewButton.setEnabled(
                initialized
                        && !gbPreviewStartInProgress
                        && (previewActive
                        || (activeCameraPreviewSession == null
                        && activePublisherPreviewSession == null
                        && activePublisherLocalCaptureSession == null
                        && (videoSource
                        ? selectedGbVideoUri != null || !readTextInput(gbMediaSourceInput, "").isEmpty()
                        : activeGbPreviewSession == null && hasUsableSurface(gbCameraPreviewSurface)))));
        startGbPreviewButton.setText(previewActive
                ? R.string.stop_gb_preview
                : R.string.start_gb_preview);
        stopGbPreviewButton.setEnabled(
                initialized
                        && !gbPreviewStartInProgress
                        && previewActive);
        startGb28181Button.setEnabled(
                initialized
                        && !gb28181StartInProgress
                        && (activeGb28181Session != null
                        || previewActive
                        || (!gb28181PermissionRequestInProgress
                        && (selectedGb28181SourceChoice() != Gb28181SourceChoice.CAMERA_MICROPHONE
                        || activePublisherLocalCaptureSession == null))));
        startGb28181Button.setText(activeGb28181Session != null
                ? R.string.stop_gb28181
                : (previewActive ? R.string.stop_gb_preview : R.string.start_gb28181));
        stopGb28181Button.setEnabled(
                initialized
                        && !gb28181StartInProgress
                        && (activeGb28181Session != null || previewActive));
    }

    private boolean isGbVideoPreviewPlaying() {
        return gbVideoPreviewView != null
                && (gbVideoPreviewView.getVisibility() == View.VISIBLE
                || gbVideoPreviewView.isPlaying());
    }

    private void updatePreviewButtons() {
        final boolean initialized = demoInitialized && initializationFailureMessage.isEmpty();
        updatePublisherActionLabels();
        final boolean playerSurfaceReady = hasUsableSurface(playerPreviewSurface);
        final boolean cameraSurfaceReady = hasUsableSurface(cameraPreviewSurface);
        final boolean publisherSurfaceReady = hasUsableSurface(publisherPreviewSurface);
        final boolean publisherCameraSource =
                selectedPublisherSourceChoice() == PublisherSourceChoice.CAMERA_MICROPHONE;
        final boolean publisherTransportSupported = selectedPublisherTransportSupported();
        startPlayerPreviewButton.setEnabled(
                initialized
                        && !playerPreviewStartInProgress
                        && (activePlayerPreviewSession != null || playerSurfaceReady));
        startPlayerPreviewButton.setText(activePlayerPreviewSession != null
                ? R.string.stop_player_preview
                : R.string.start_player_preview);
        stopPlayerPreviewButton.setEnabled(
                initialized
                        && !playerPreviewStartInProgress
                        && activePlayerPreviewSession != null);
        searchOnvifButton.setEnabled(
                initialized && !onvifSearchInProgress && !onvifResolveInProgress);
        final StreamCoreOnvif.Device selectedOnvifDevice = selectedOnvifDevice();
        applyOnvifStreamButton.setEnabled(
                initialized
                        && !onvifSearchInProgress
                        && !onvifResolveInProgress
                        && selectedOnvifDevice != null);
        startCameraPreviewButton.setEnabled(
                initialized
                        && cameraSurfaceReady
                        && !cameraPreviewStartInProgress
                        && activeGbPreviewSession == null
                        && activeCameraPreviewSession == null
                        && activePublisherLocalCaptureSession == null);
        stopCameraPreviewButton.setEnabled(
                initialized
                        && !cameraPreviewStartInProgress
                        && activeCameraPreviewSession != null);
        startPublisherPreviewButton.setEnabled(
                initialized
                        && publisherCameraSource
                        && !publisherPreviewStartInProgress
                        && (activePublisherPreviewSession != null
                        || (publisherSurfaceReady
                        && activeGbPreviewSession == null
                        && activePublisherPreviewSession == null
                        && activePublisherLocalCaptureSession == null)));
        stopPublisherPreviewButton.setEnabled(
                initialized
                        && !publisherPreviewStartInProgress
                        && activePublisherPreviewSession != null);
        evaluatePublisherSourceButton.setEnabled(initialized);
        runPublisherLocalCaptureButton.setEnabled(
                initialized
                        && !cameraPreviewStartInProgress
                        && !publisherPermissionRequestInProgress
                        && !publisherLocalCaptureRunInProgress
                        && (publisherTransportSupported
                        || activePublisherLocalCaptureSession != null
                        || activePublisherPreviewSession != null));
        stopPublisherLocalCaptureButton.setEnabled(
                initialized
                        && !publisherLocalCaptureRunInProgress
                        && activePublisherLocalCaptureSession != null);
    }

    private boolean isPreviewActionReady() {
        return demoInitialized
                && initializationFailureMessage.isEmpty()
                && previewMediaFile != null;
    }

    private boolean hasCameraPermission() {
        return DemoPermissionSupport.hasCameraPreviewPermission(this);
    }

    private boolean hasRecordAudioPermission() {
        return DemoPermissionSupport.hasMicrophoneCapturePermission(this);
    }

    private void handleCameraPermissionResult(Map<String, Boolean> grants) {
        final Runnable action = pendingCameraPermissionAction;
        final String scenarioKey = pendingCameraPermissionScenario;
        pendingCameraPermissionAction = null;
        pendingCameraPermissionScenario = "";
        if (hasCameraPermission() && action != null) {
            action.run();
            return;
        }
        final String effectiveScenarioKey =
                scenarioKey.isEmpty() ? "camera_preview" : scenarioKey;
        setCapturePreviewScenario(
                effectiveScenarioKey,
                CapturePreviewScenarioResult.blocked(
                        effectiveScenarioKey,
                        buildStatus(
                                StreamCoreResultCode.NOT_ENABLED,
                                "camera_permission_denied",
                                "camera preview permission was not granted.",
                                "Grant camera permission before starting camera preview.")));
        renderInfoText();
    }

    private void handlePublisherPermissionResult(Map<String, Boolean> grants) {
        publisherPermissionRequestInProgress = false;
        final boolean cameraGranted = hasCameraPermission();
        final boolean audioGranted = hasRecordAudioPermission();
        final boolean resumeDirectTransport = publisherPermissionResumeDirectTransport;
        publisherPermissionResumeDirectTransport = false;
        final StreamCorePublisher.Config permissionConfig = resumeDirectTransport
                ? buildPublisherLocalCapturePermissionConfig()
                : buildSelectedPublisherPermissionConfig();
        if (DemoPermissionSupport.hasPublisherLocalCapturePermissions(
                this,
                permissionConfig)) {
            if (resumeDirectTransport) {
                runPublisherLocalCaptureNativeTransport();
            } else {
                runSelectedPublisherTransport();
            }
            return;
        }
        upsertPublisherScenario(buildSelectedPublisherSkippedScenario(
                StreamCoreResultCode.NOT_ENABLED,
                "publisher_permission_missing",
                "Publish needs required runtime permission.",
                "cameraPermission=" + cameraGranted + ", recordAudioPermission=" + audioGranted));
        renderInfoText();
    }

    private void handleGbPermissionResult(Map<String, Boolean> grants) {
        gb28181PermissionRequestInProgress = false;
        final boolean cameraGranted = hasCameraPermission();
        final boolean audioGranted = hasRecordAudioPermission();
        if (DemoPermissionSupport.missingCameraAndMicrophonePermissions(this).length == 0) {
            startGb28181Device();
            return;
        }
        gb28181StatusText = "GB28181 camera source needs camera and microphone permission. "
                + "cameraPermission="
                + cameraGranted
                + ", recordAudioPermission="
                + audioGranted;
        renderInfoText();
    }

    private void setPreviewPermissionPending(String scenarioKey) {
        setCapturePreviewScenario(
                scenarioKey,
                CapturePreviewScenarioResult.blocked(
                        scenarioKey,
                        buildStatus(
                                StreamCoreResultCode.NOT_ENABLED,
                                "camera_permission_pending",
                                "waiting for camera permission.",
                                "permission pending.")));
    }

    private void setCapturePreviewScenario(
            String scenarioKey,
            CapturePreviewScenarioResult scenario) {
        if ("publisher_preview".equals(scenarioKey)) {
            publisherPreviewScenario = scenario;
        } else if ("gb28181_preview".equals(scenarioKey)) {
            gbPreviewScenario = scenario;
        } else {
            cameraPreviewScenario = scenario;
        }
    }

    private boolean hasUsableSurface(TextureView textureView) {
        return textureView != null
                && textureView.isAvailable()
                && textureView.getSurfaceTexture() != null;
    }

    private String readPlayerSourceUrl() {
        return readTextInput(playerSourceUrlInput, buildPreviewMediaUrl());
    }

    private String buildPreviewMediaUrl() {
        if (previewMediaFile == null) {
            return "";
        }
        return "file://" + previewMediaFile.getAbsolutePath();
    }

    private StreamCoreOperationStatus buildPublisherPreviewTransportStatus(
            String audioCodec,
            String videoCodec,
            int videoBitrateKbps,
            int width,
            int height,
            int fps,
            int gopFrames) {
        final StreamCorePublisher.Session publisherSession = new StreamCorePublisher.Session();
        final String publishUrl = readPublisherPublishUrl();
        publisherSession.setConfig(StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publisher_preview")
                .publishUrl(publishUrl)
                .inputKind(StreamCorePublisher.InputKind.LOCAL_CAPTURE)
                .inputBindingId("camera_preview")
                .transcodeOptions(StreamCorePublisher.TranscodeOptions.newBuilder()
                        .audioMode(StreamCorePublisher.TranscodeMode.FORCE_TRANSCODE)
                        .videoMode(StreamCorePublisher.TranscodeMode.FORCE_TRANSCODE)
                        .targetAudioCodecName(audioCodec)
                        .targetVideoCodecName(videoCodec)
                        .targetVideoBitrateKbps(videoBitrateKbps)
                        .targetVideoFormat(width, height, fps, gopFrames)
                        .build())
                .build());
        final StreamCorePublisher.Preflight preflight = publisherSession.preflight();
        publisherSession.stop();
        return new StreamCoreOperationStatus(
                preflight.status.resultCode,
                "publisher_preview_transport_deferred",
                "publisher preflight completed; transport start is deferred.",
                "preflight="
                        + preflight.status.statusName
                        + ", detail="
                        + preflight.detail
                        + "; local camera preview is active on the publisher tab; encoder="
                        + videoCodec
                        + "/"
                        + audioCodec
                        + ", size="
                        + width
                        + "x"
                        + height
                        + ", bitrateKbps="
                        + videoBitrateKbps
                        + ", fps="
                        + fps
                        + ", gop="
                        + gopFrames
                        + ".");
    }

    private PublisherScenarioResult buildSelectedPublisherSourceScenario() {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_in_selected_source"};
        final StreamCorePublisher.Config config =
                buildSelectedPublisherConfig(report -> callbackMessage[0] = "required: "
                        + report.summary);
        final StreamCoreOperationStatus configStatus = session.setConfig(config);
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus = configStatus.isOk()
                ? deferredBootstrapStatus(
                "selected publisher source preflight completed.",
                "selected source, track, and transcode contract checked.")
                : configStatus;
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                "selected_source_" + selectedPublisherSourceChoice().name().toLowerCase(Locale.ROOT),
                preflight,
                startStatus,
                notApplicableStatus(),
                notApplicableStatus(),
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }

    private PublisherScenarioResult buildSelectedPublisherSkippedScenario(
            int resultCode,
            String statusName,
            String summary,
            String detail) {
        final PublisherScenarioResult base = buildSelectedPublisherSourceScenario();
        return new PublisherScenarioResult(
                base.scenarioKey,
                base.preflight,
                new StreamCoreOperationStatus(
                        resultCode,
                        statusName,
                        summary,
                        detail),
                base.audioPushStatus,
                base.videoPushStatus,
                base.runtimeAfterStart,
                base.runtimeAfterStop,
                base.callbackMessage);
    }

    private StreamCorePublisher.Config buildSelectedPublisherConfig(
            StreamCorePublisher.TranscodeRequirementCallback callback) {
        final PublisherSourceChoice sourceChoice = selectedPublisherSourceChoice();
        final PublisherAudioChoice audioChoice = selectedPublisherAudioChoice();
        final String audioCodec = selectedPublisherAudioCodec();
        final String videoCodec = selectedPublisherVideoCodec();
        final int videoWidth = readPublisherVideoWidth();
        final int videoHeight = readPublisherVideoHeight();
        final int videoBitrate = readPublisherVideoBitrateKbps();
        final int fps = readPublisherFps();
        final int gop = readPublisherGopFrames();
        final StreamCorePublisher.Config.Builder builder = StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_selected")
                .publishUrl(readPublisherPublishUrl())
                .androidTransportPolicy(StreamCorePublisher.AndroidTransportPolicy.AUTO)
                .transcodeOptions(StreamCorePublisher.TranscodeOptions.newBuilder()
                        .audioMode(StreamCorePublisher.TranscodeMode.AUTO)
                        .videoMode(StreamCorePublisher.TranscodeMode.AUTO)
                        .targetAudioCodecName(audioCodec)
                        .targetVideoCodecName(videoCodec)
                        .targetAudioFormat(48000, 2)
                        .targetAudioBitrateKbps(96)
                        .targetVideoBitrateKbps(videoBitrate)
                        .targetVideoFormat(videoWidth, videoHeight, fps, gop)
                        .build())
                .transcodeRequirementCallback(callback);

        switch (sourceChoice) {
            case DESKTOP:
                builder.inputKind(StreamCorePublisher.InputKind.LOCAL_CAPTURE)
                        .inputBindingId(audioChoice == PublisherAudioChoice.MICROPHONE
                                ? "desktop:+microphone"
                                : "desktop")
                        .enableAudio(audioChoice == PublisherAudioChoice.MICROPHONE)
                        .enableVideo(true);
                break;
            case VIDEO_FILE:
                builder.inputKind(StreamCorePublisher.InputKind.APP_ENCODED_FEED)
                        .inputBindingId("file:"
                                + readTextInput(publisherMediaPathInput, buildPreviewMediaPath()))
                        .enableAudio(true)
                        .enableVideo(true)
                        .sourceMediaProfile(StreamCorePublisher.SourceMediaProfile.newBuilder()
                                .containerName("mp4")
                                .audioCodecName("aac")
                                .videoCodecName("h264")
                                .hasAudio(true)
                                .hasVideo(true)
                                .videoFormat(videoWidth, videoHeight, fps)
                                .audioFormat(48000, 2)
                                .build());
                break;
            case STILL_IMAGE:
                builder.inputKind(StreamCorePublisher.InputKind.APP_RAW_FEED)
                        .inputBindingId("image:"
                                + readTextInput(publisherImagePathInput, buildDemoStillImagePath()))
                        .enableAudio(audioChoice != PublisherAudioChoice.NONE)
                        .enableVideo(true)
                        .sourceMediaProfile(StreamCorePublisher.SourceMediaProfile.newBuilder()
                                .containerName("raw")
                                .audioCodecName(audioChoice == PublisherAudioChoice.NONE ? "" : "pcm_s16le")
                                .videoCodecName("raw")
                                .hasAudio(audioChoice != PublisherAudioChoice.NONE)
                                .hasVideo(true)
                                .videoFormat(videoWidth, videoHeight, fps)
                                .audioFormat(48000, 2)
                                .build());
                break;
            case NONE:
                if (audioChoice == PublisherAudioChoice.FILE_AUDIO) {
                    builder.inputKind(StreamCorePublisher.InputKind.APP_ENCODED_FEED)
                            .inputBindingId("audio-file:"
                                    + readTextInput(publisherMediaPathInput, buildDemoAudioPath()))
                            .enableAudio(true)
                            .enableVideo(false)
                            .sourceMediaProfile(StreamCorePublisher.SourceMediaProfile.newBuilder()
                                    .containerName("wav")
                                    .audioCodecName("pcm_s16le")
                                    .hasAudio(true)
                                    .hasVideo(false)
                                    .audioFormat(48000, 1)
                                    .build());
                } else {
                    builder.inputKind(StreamCorePublisher.InputKind.LOCAL_CAPTURE)
                            .inputBindingId("microphone")
                            .enableAudio(audioChoice == PublisherAudioChoice.MICROPHONE)
                            .enableVideo(false);
                }
                break;
            case CAMERA_MICROPHONE:
            default:
                builder.inputKind(StreamCorePublisher.InputKind.LOCAL_CAPTURE)
                        .inputBindingId(localCameraPublisherBinding(
                                audioChoice == PublisherAudioChoice.MICROPHONE))
                        .enableAudio(audioChoice == PublisherAudioChoice.MICROPHONE)
                        .enableVideo(true);
                break;
        }
        return builder.build();
    }

    private String localCameraPublisherBinding(boolean includeMicrophone) {
        final String cameraSourceId = selectedCameraSourceId();
        final String cameraBinding =
                cameraSourceId.isEmpty() ? "camera:" : "camera:" + cameraSourceId;
        return includeMicrophone ? cameraBinding + "+microphone" : cameraBinding;
    }

    private String buildPreviewMediaPath() {
        return previewMediaFile == null ? "" : previewMediaFile.getAbsolutePath();
    }

    private Uri effectiveGbVideoUri() {
        if (selectedGbVideoUri != null) {
            return selectedGbVideoUri;
        }
        if (previewMediaFile != null) {
            return Uri.fromFile(previewMediaFile);
        }
        return null;
    }

    private File copyPickedMediaUriToAppFile(
            @NonNull Uri uri,
            @NonNull String baseName,
            @NonNull String fallbackSuffix) throws IOException {
        final File importDirectory = new File(new File(getFilesDir(), "streamcore"), "imports");
        if (!importDirectory.exists() && !importDirectory.mkdirs()) {
            throw new IOException("failed to create import directory: " + importDirectory);
        }
        final File outputFile = new File(
                importDirectory,
                baseName + pickedMediaFileSuffix(uri, fallbackSuffix));
        try (InputStream inputStream = getContentResolver().openInputStream(uri)) {
            if (inputStream == null) {
                throw new IOException("content resolver returned no readable stream for " + uri);
            }
            try (FileOutputStream outputStream = new FileOutputStream(outputFile, false)) {
                final byte[] buffer = new byte[8192];
                while (true) {
                    final int read = inputStream.read(buffer);
                    if (read < 0) {
                        break;
                    }
                    outputStream.write(buffer, 0, read);
                }
            }
        }
        return outputFile;
    }

    private static String pickedMediaFileSuffix(@NonNull Uri uri, @NonNull String fallbackSuffix) {
        final String path = uri.getLastPathSegment();
        if (TextUtils.isEmpty(path)) {
            return fallbackSuffix;
        }
        final int dotIndex = path.lastIndexOf('.');
        if (dotIndex < 0 || dotIndex >= path.length() - 1) {
            return fallbackSuffix;
        }
        final String suffix = path.substring(dotIndex);
        if (suffix.length() > 12) {
            return fallbackSuffix;
        }
        for (int index = 1; index < suffix.length(); index++) {
            final char ch = suffix.charAt(index);
            if (!Character.isLetterOrDigit(ch) && ch != '_' && ch != '-') {
                return fallbackSuffix;
            }
        }
        return suffix;
    }

    private String readPublisherPublishUrl() {
        return readTextInput(publisherPublishUrlInput, defaultSelectedPublisherUrl());
    }

    private String selectedPublisherVideoCodec() {
        return publisherVideoCodecSpinner != null
                && publisherVideoCodecSpinner.getSelectedItemPosition() == 1
                ? "h265"
                : "h264";
    }

    private String selectedPublisherAudioCodec() {
        return "aac";
    }

    private PublisherResolutionOption selectedPublisherResolutionOption() {
        if (publisherResolutionOptions.isEmpty()) {
            return null;
        }
        final int position = publisherResolutionSpinner == null
                ? 0
                : publisherResolutionSpinner.getSelectedItemPosition();
        if (position < 0 || position >= publisherResolutionOptions.size()) {
            return publisherResolutionOptions.get(0);
        }
        return publisherResolutionOptions.get(position);
    }

    private int readPublisherVideoWidth() {
        final PublisherResolutionOption option = selectedPublisherResolutionOption();
        return option == null || option.width <= 0 ? 1280 : option.width;
    }

    private int readPublisherVideoHeight() {
        final PublisherResolutionOption option = selectedPublisherResolutionOption();
        return option == null || option.height <= 0 ? 720 : option.height;
    }

    private int readPublisherVideoBitrateKbps() {
        return readIntInput(publisherVideoBitrateInput, 1200, 1, 100_000);
    }

    private int readPublisherFps() {
        return readIntInput(publisherFpsInput, 25, 1, 120);
    }

    private int readPublisherGopFrames() {
        return readIntInput(publisherGopInput, 50, 1, 600);
    }

    private StreamCorePublisher.Config buildPublisherLocalCapturePermissionConfig() {
        return StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_permission_publisher_local_capture")
                .publishUrl(readPublisherPublishUrl())
                .inputKind(StreamCorePublisher.InputKind.LOCAL_CAPTURE)
                .inputBindingId(localCameraPublisherBinding(true))
                .enableAudio(true)
                .enableVideo(true)
                .build();
    }

    private StreamCorePublisher.Config buildSelectedPublisherPermissionConfig() {
        return buildSelectedPublisherConfig(report -> { });
    }

    private String defaultSelectedPublisherUrl() {
        return getString(R.string.default_publisher_publish_url);
    }

    private String defaultPlayerSourceUrl() {
        return getString(R.string.default_player_source_url);
    }

    private String buildDemoAudioPath() {
        return demoAudioFile == null ? "" : demoAudioFile.getAbsolutePath();
    }

    private String buildDemoStillImagePath() {
        return demoStillImageFile == null ? "" : demoStillImageFile.getAbsolutePath();
    }

    private String buildLicenseSummaryText() {
        final String productName = productInfo == null ? "StreamCore SDK" : productInfo.productName;
        final String productCode = productInfo == null ? "-" : productInfo.productCode;
        final String targetName = productInfo == null ? "-" : productInfo.primaryTargetName;
        final String productVersion = productInfo == null ? "-" : productInfo.version;
        final String licenseState = licenseInfo == null
                ? "pending"
                : licenseInfo.statusName
                + " | valid="
                + licenseInfo.licenseValid
                + " | wm="
                + licenseInfo.needWatermark;
        final String featureState = demoFeature == null
                ? "demo=?"
                : "demo=" + demoFeature.enabled;
        final String limitState = inputChannelLimit == null
                ? "inputs=?"
                : "inputs=" + inputChannelLimit.limitValue;
        final String logState = logInfo == null
                ? "streamcore_demo.log"
                : logInfo.logFileName;
        return "App: "
                + productName
                + " ("
                + productCode
                + ") "
                + productVersion
                + "\nTarget: "
                + targetName
                + "\nLicense: "
                + licenseState
                + "\nFeature: "
                + featureState
                + " | "
                + limitState
                + "\nPackage: "
                + compactPackagingStatus(StreamCoreSdk.getAndroidPackagingStatus())
                + "\nRuntime bundled: "
                + StreamCoreSdk.isNativeRuntimeBundled()
                + "\nLog: "
                + logState;
    }

    private String buildPlayerSummaryText() {
        final StringBuilder builder = new StringBuilder();
        builder.append("State: ")
                .append(playerPreviewScenario.lifecycleState)
                .append(" | ")
                .append(compactText(playerPreviewScenario.summary, 60))
                .append('\n')
                .append("Mode: ")
                .append(spinnerSelectionLabel(playerDecodeModeSpinner))
                .append(" / ")
                .append(spinnerSelectionLabel(playerRenderBackendSpinner))
                .append(" | ")
                .append(selectedPreviewDisplayModeLabel())
                .append('\n')
                .append("Buffer: ")
                .append(readIntInput(playerBufferMsInput, 300, -1, 10_000))
                .append(" ms | A")
                .append(readIntInput(playerAudioQueueInput, 12, 0, 400))
                .append(" / V")
                .append(readIntInput(playerVideoQueueInput, 6, 0, 200));
        if (playerPreviewScenario.startStatus != null) {
            builder.append('\n')
                    .append("Start: ")
                    .append(playerPreviewScenario.startStatus.statusName)
                    .append(" / ")
                    .append(playerPreviewScenario.startStatus.resultCode)
                    .append(" | ")
                    .append(compactText(playerPreviewScenario.startStatus.summary, 48));
        }
        if (!TextUtils.isEmpty(playerOnvifStatus)) {
            builder.append('\n')
                    .append("ONVIF: ")
                    .append(compactText(playerOnvifStatus, 54));
        }
        return builder.toString();
    }

    private String buildCaptureSummaryText() {
        final StringBuilder builder = new StringBuilder();
        appendCaptureParameterSection(builder);
        appendCapturePreviewScenarioSection(builder, cameraPreviewScenario);
        appendCaptureScenarioSection(builder, cameraCaptureScenario);
        return builder.toString();
    }

    private String buildDesktopCaptureSummaryText() {
        final StringBuilder builder = new StringBuilder();
        appendDesktopCaptureParameterSection(builder);
        appendDesktopCaptureScenarioSection(builder, desktopCaptureScenario);
        return builder.toString();
    }

    private String buildPublisherSummaryText() {
        final boolean active = activePublisherPreviewSession != null
                || publisherLocalCaptureRunInProgress
                || activePublisherLocalCaptureSession != null;
        return "Source: "
                + spinnerSelectionLabel(publisherSourceSpinner)
                + " | Audio: "
                + spinnerSelectionLabel(publisherAudioSpinner)
                + "\nVideo: "
                + selectedPublisherVideoCodec()
                + " "
                + readPublisherVideoWidth()
                + "x"
                + readPublisherVideoHeight()
                + "\nRate: "
                + readPublisherVideoBitrateKbps()
                + " kbps | "
                + readPublisherFps()
                + " fps | GOP "
                + readPublisherGopFrames()
                + "\nState: "
                + (active ? "running" : "idle")
                + " | "
                + (active
                        ? getString(R.string.publisher_live_bitrate_pending)
                        : getString(R.string.publisher_live_bitrate_waiting));
    }

    private String buildGbMetricOverlayText() {
        final boolean running = activeGb28181Session != null;
        final String state = running ? "running" : "idle";
        final String source = spinnerSelectionLabel(gbInviteSourceSpinner);
        final String upperIp = readTextInput(gbUpperIpInput, DEFAULT_DEVELOPMENT_RTMP_HOST);
        final int upperPort = readIntInput(gbUpperPortInput, 5060, 1, 65535);
        final int mediaPort = readIntInput(gbMediaPortInput, 15060, 1, 65535);
        return "GB28181 " + state
                + "\n" + compactText(source, 22)
                + " | " + upperIp + ":" + upperPort
                + "\nRTP " + mediaPort;
    }

    private String buildPublisherSimpleStatusText() {
        final PublisherScenarioResult localCaptureScenario =
                findPublisherScenario("local_capture_native_transport");
        final String lifecycle = activePublisherLocalCaptureSession != null
                ? "running"
                : (publisherLocalCaptureRunInProgress ? "starting" : "idle");
        final String status = localCaptureScenario == null
                ? getString(R.string.publisher_publish_not_started)
                : localCaptureScenario.startStatus.statusName
                + " / "
                + localCaptureScenario.startStatus.resultCode;
        final String detail = localCaptureScenario == null
                ? getString(R.string.publisher_publish_not_started)
                : compactText(localCaptureScenario.startStatus.summary, 52);
        return "Publish: "
                + lifecycle
                + " | "
                + compactText(status, 20)
                + "\n"
                + compactText(detail, 52);
    }

    private PublisherScenarioResult findPublisherScenario(String scenarioKey) {
        for (PublisherScenarioResult scenario : publisherScenarios) {
            if (scenario.scenarioKey.equals(scenarioKey)) {
                return scenario;
            }
        }
        return null;
    }

    private void upsertPublisherScenario(PublisherScenarioResult scenario) {
        final List<PublisherScenarioResult> updatedScenarios =
                new ArrayList<>(publisherScenarios);
        for (int index = 0; index < updatedScenarios.size(); ++index) {
            if (updatedScenarios.get(index).scenarioKey.equals(scenario.scenarioKey)) {
                updatedScenarios.set(index, scenario);
                publisherScenarios = updatedScenarios;
                return;
            }
        }
        updatedScenarios.add(scenario);
        publisherScenarios = updatedScenarios;
    }

    private String buildGb28181SummaryText() {
        return "State: "
                + compactText(gb28181StatusText, 58)
                + "\nSource: "
                + spinnerSelectionLabel(gbInviteSourceSpinner)
                + " | Local: "
                + readTextInput(gbLocalIdInput, "34020000001320000001")
                + "\nUpper: "
                + readTextInput(gbUpperIpInput, "192.0.2.10")
                + ":"
                + readIntInput(gbUpperPortInput, 5060, 1, 65535)
                + " | Media: "
                + readIntInput(gbMediaPortInput, 15060, 1, 65535)
                + "\nAddon: "
                + StreamCoreGB28181.getAndroidPackagingStatus();
    }

    private void appendPlayerParameterSection(StringBuilder builder) {
        builder.append("Player parameters:\n")
                .append("- source url: ")
                .append(readPlayerSourceUrl())
                .append('\n')
                .append("- decode mode: ")
                .append(playerDecodeModeSpinner.getSelectedItem())
                .append('\n')
                .append("- render path: ")
                .append(playerRenderBackendSpinner.getSelectedItem())
                .append('\n')
                .append("- present path: ")
                .append(selectedPlayerPresentPath().name())
                .append('\n')
                .append("- display mode: ")
                .append(selectedPreviewDisplayModeLabel())
                .append('\n')
                .append("- video resolution: ")
                .append(formatPlayerVideoSize(
                        activePlayerPreviewSession != null
                                ? activePlayerPreviewSession.getRuntimeInfo()
                                : null,
                        activePlayerPreviewSession != null))
                .append('\n')
                .append("- decode request: ")
                .append(selectedPlayerDecoderSummary())
                .append('\n')
                .append("- render request: ")
                .append(selectedPlayerRenderSummary())
                .append('\n')
                .append("- playback buffer ms: ")
                .append(readIntInput(playerBufferMsInput, 300, -1, 10_000))
                .append('\n')
                .append("- audio queue limit: ")
                .append(readIntInput(playerAudioQueueInput, 12, 0, 400))
                .append('\n')
                .append("- video queue limit: ")
                .append(readIntInput(playerVideoQueueInput, 6, 0, 200))
                .append('\n')
                .append("- ONVIF devices: ")
                .append(onvifDevices.size())
                .append(", selected endpoint: ")
                .append(selectedOnvifDevice() == null
                        ? "<none>"
                        : emptyMarker(selectedOnvifDevice().serviceUrl))
                .append("\n\n");
    }

    private void appendCaptureParameterSection(StringBuilder builder) {
        builder.append("Capture parameters:\n")
                .append("- selected camera: ")
                .append(selectedCameraLabel())
                .append('\n')
                .append("- camera source id: ")
                .append(selectedCameraSourceId().isEmpty() ? "SDK default" : selectedCameraSourceId())
                .append('\n')
                .append("- frame rate: ")
                .append(readIntInput(cameraFpsInput, 30, 1, 120))
                .append('\n')
                .append("- display mode: ")
                .append(selectedPreviewDisplayModeLabel())
                .append("\n\n");
    }

    private void appendDesktopCaptureParameterSection(StringBuilder builder) {
        builder.append("Desktop capture parameters:\n")
                .append("- size: ")
                .append(readIntInput(desktopWidthInput, 1280, 1, 7680))
                .append('x')
                .append(readIntInput(desktopHeightInput, 720, 1, 4320))
                .append('\n')
                .append("- encoder bitrate bps: ")
                .append(readIntInput(desktopBitrateInput, 1_200_000, 1, 100_000_000))
                .append('\n')
                .append("- key interval seconds: ")
                .append(readIntInput(desktopKeyIntervalInput, 2, 1, 60))
                .append("\n\n");
    }

    private void appendPublisherParameterSection(StringBuilder builder) {
        final PublisherSourceChoice sourceChoice = selectedPublisherSourceChoice();
        final PublisherAudioChoice audioChoice = selectedPublisherAudioChoice();
        builder.append("Publisher source selection:\n")
                .append("- selected source: ")
                .append(publisherSourceLabel(sourceChoice))
                .append('\n')
                .append("- display mode: ")
                .append(selectedPreviewDisplayModeLabel())
                .append('\n')
                .append("- video source: ")
                .append(selectedPublisherVideoSourceLabel(sourceChoice))
                .append('\n')
                .append("- audio source: ")
                .append(selectedPublisherAudioSourceLabel(sourceChoice, audioChoice))
                .append('\n')
                .append("- camera selection: ")
                .append(selectedCameraLabel())
                .append('\n')
                .append("- system audio: unavailable\n");
        if (sourceChoice == PublisherSourceChoice.VIDEO_FILE
                || audioChoice == PublisherAudioChoice.FILE_AUDIO) {
            builder.append("- media path: ")
                    .append(readTextInput(publisherMediaPathInput, ""))
                    .append('\n');
        }
        if (sourceChoice == PublisherSourceChoice.STILL_IMAGE) {
            builder.append("- image path: ")
                    .append(readTextInput(publisherImagePathInput, ""))
                    .append('\n');
        }
        builder.append('\n');

        builder.append("Publisher encoding parameters:\n")
                .append("- publish url: ")
                .append(readPublisherPublishUrl())
                .append('\n')
                .append("- target codecs: ")
                .append(selectedPublisherVideoCodec())
                .append(" / ")
                .append(selectedPublisherAudioCodec())
                .append('\n')
                .append("- target size: ")
                .append(readPublisherVideoWidth())
                .append('x')
                .append(readPublisherVideoHeight())
                .append('\n')
                .append("- target video bitrate kbps: ")
                .append(readPublisherVideoBitrateKbps())
                .append('\n')
                .append("- live bitrate: waiting for stream stats\n")
                .append("- target fps: ")
                .append(readPublisherFps())
                .append('\n')
                .append("- target gop frames: ")
                .append(readPublisherGopFrames())
                .append("\n\n");
    }

    private static String publisherSourceLabel(PublisherSourceChoice sourceChoice) {
        switch (sourceChoice) {
            case DESKTOP:
                return "desktop";
            case VIDEO_FILE:
                return "video file";
            case STILL_IMAGE:
                return "still image";
            case NONE:
                return "none";
            case CAMERA_MICROPHONE:
            default:
                return "camera";
        }
    }

    private static String selectedPublisherVideoSourceLabel(
            PublisherSourceChoice sourceChoice) {
        switch (sourceChoice) {
            case DESKTOP:
                return "desktop";
            case VIDEO_FILE:
                return "video file";
            case STILL_IMAGE:
                return "still image";
            case NONE:
                return "none";
            case CAMERA_MICROPHONE:
            default:
                return "camera";
        }
    }

    private static String selectedPublisherAudioSourceLabel(
            PublisherSourceChoice sourceChoice,
            PublisherAudioChoice audioChoice) {
        if (sourceChoice == PublisherSourceChoice.CAMERA_MICROPHONE) {
            if (audioChoice == PublisherAudioChoice.MICROPHONE) {
                return "microphone";
            }
            if (audioChoice == PublisherAudioChoice.FILE_AUDIO) {
                return "audio file";
            }
            return "none";
        }
        if (sourceChoice == PublisherSourceChoice.VIDEO_FILE) {
            return "video file audio track";
        }
        if (audioChoice == PublisherAudioChoice.MICROPHONE) {
            return "microphone fallback";
        }
        if (audioChoice == PublisherAudioChoice.FILE_AUDIO) {
            return "audio file";
        }
        if (audioChoice == PublisherAudioChoice.SYSTEM_AUDIO) {
            return "system audio unavailable";
        }
        return "none";
    }

    private void appendGb28181ParameterSection(StringBuilder builder) {
        builder.append("GB28181 parameters:\n")
                .append("- INVITE live source: ")
                .append(selectedGb28181SourceLabel())
                .append('\n')
                .append("- source binding: ")
                .append(selectedGb28181SourceBinding())
                .append('\n')
                .append("- local device/channel id: ")
                .append(readTextInput(gbLocalIdInput, "34020000001320000001"))
                .append('\n')
                .append("- local sip domain: ")
                .append(readTextInput(gbLocalDomainInput, "3402000000"))
                .append('\n')
                .append("- local sip endpoint: ")
                .append(readTextInput(gbLocalIpInput, "0.0.0.0"))
                .append(':')
                .append(readIntInput(gbLocalPortInput, 5060, 1, 65535))
                .append('\n')
                .append("- upper platform id: ")
                .append(readTextInput(gbUpperIdInput, "34020000002000000001"))
                .append('\n')
                .append("- upper sip domain: ")
                .append(readTextInput(gbUpperDomainInput, readTextInput(gbLocalDomainInput, "3402000000")))
                .append('\n')
                .append("- upper sip password: ")
                .append(readTextInput(gbUpperPasswordInput, "").isEmpty() ? "empty" : "set")
                .append('\n')
                .append("- upper sip endpoint: ")
                .append(readTextInput(gbUpperIpInput, ""))
                .append(':')
                .append(readIntInput(gbUpperPortInput, 5060, 1, 65535))
                .append(" ")
                .append(selectedGb28181TransportMode().name())
                .append('\n')
                .append("- media rtp endpoint: ")
                .append(readTextInput(gbMediaIpInput, resolveFirstNonLoopbackIpv4("0.0.0.0")))
                .append(':')
                .append(readIntInput(gbMediaPortInput, 15060, 1, 65535))
                .append('\n')
                .append("- register expires seconds: ")
                .append(readIntInput(gbRegisterExpiresInput, 3600, 60, 86_400))
                .append('\n')
                .append("- keepalive interval seconds: ")
                .append(readIntInput(gbKeepaliveInput, 60, 5, 3600))
                .append("\n\n");
    }

    private String selectedGb28181SourceLabel() {
        final Object selected = gbInviteSourceSpinner == null
                ? null
                : gbInviteSourceSpinner.getSelectedItem();
        if (selected != null) {
            return selected.toString();
        }
        return selectedGb28181SourceChoice() == Gb28181SourceChoice.VIDEO_FILE
                ? getString(R.string.gb_source_video_file)
                : getString(R.string.gb_source_camera_microphone);
    }

    private String selectedGb28181SourceBinding() {
        switch (selectedGb28181SourceChoice()) {
            case VIDEO_FILE:
                return readTextInput(gbMediaSourceInput, buildPreviewMediaPath());
            case CAMERA_MICROPHONE:
            default:
                return localCameraPublisherBinding(true);
        }
    }

    private StreamCoreGB28181.Config buildGb28181Config(
            String localId,
            String localDomain,
            String upperId,
            String upperDomain,
            String sipPassword,
            String localIp,
            int localPort,
            String upperIp,
            int upperPort,
            StreamCoreGB28181.TransportMode transportMode,
            String mediaIp,
            int mediaPort,
            int registerExpires,
            int keepalive,
            String sourceLabel) {
        return StreamCoreGB28181.Config.newBuilder()
                .sessionName("demo-gb28181-device")
                .localIdentity(new StreamCoreGB28181.Identity(
                        localId,
                        localDomain,
                        sipPassword,
                        "StreamCore SDK Demo"))
                .upperPlatformIdentity(new StreamCoreGB28181.Identity(
                        upperId,
                        upperDomain,
                        sipPassword,
                        "Demo Upper Platform"))
                .localEndpoint(new StreamCoreGB28181.Endpoint(
                        localIp,
                        localPort,
                        transportMode))
                .upperPlatformEndpoint(new StreamCoreGB28181.Endpoint(
                        upperIp,
                        upperPort,
                        transportMode))
                .registerExpiresSeconds(registerExpires)
                .keepaliveIntervalSeconds(keepalive)
                .defaultAnswer(StreamCoreGB28181.AnswerConfig.newBuilder()
                        .sessionName("StreamCore " + sourceLabel)
                        .mediaEndpoint(new StreamCoreGB28181.Endpoint(
                                mediaIp,
                                mediaPort,
                                transportMode))
                        .build())
                .build();
    }

    private StreamCoreGB28181.TransportMode selectedGb28181TransportMode() {
        return gbTransportSpinner != null && gbTransportSpinner.getSelectedItemPosition() == 1
                ? StreamCoreGB28181.TransportMode.UDP
                : StreamCoreGB28181.TransportMode.TCP;
    }

    private List<StreamCoreGB28181.CatalogItem> buildGb28181Catalog(
            String localId,
            String sourceLabel) {
        return Collections.singletonList(new StreamCoreGB28181.CatalogItem(
                localId,
                "Android Demo " + sourceLabel,
                "",
                "HBR",
                "AndroidDemo",
                "StreamCore SDK Demo",
                "340200",
                "Android demo",
                false,
                true,
                "ON"));
    }

    private String buildGb28181DeviceWrapperSummary() {
        if (activeGb28181Session != null) {
            final StreamCoreGB28181.RuntimeInfo runtimeInfo =
                    activeGb28181Session.getRuntimeInfo();
            return "active, started=" + runtimeInfo.started
                    + ", registered=" + runtimeInfo.registered
                    + ", inviteSource=" + selectedGb28181SourceBinding()
                    + ", upperSip=" + readTextInput(gbUpperIpInput, "")
                    + ":" + readIntInput(gbUpperPortInput, 5060, 1, 65535)
                    + ", sessions=" + runtimeInfo.activeSessionCount;
        }
        try (StreamCoreGB28181.DeviceSession session =
                     StreamCoreGB28181.createDeviceSession()) {
            final String localId = readTextInput(gbLocalIdInput, "34020000001320000001");
            final String localDomain = readTextInput(gbLocalDomainInput, "3402000000");
            final String upperId = readTextInput(gbUpperIdInput, "34020000002000000001");
            final String upperDomain = readTextInput(gbUpperDomainInput, localDomain);
            final String sipPassword = readTextInput(gbUpperPasswordInput, "123456");
            final String localIp = readTextInput(gbLocalIpInput, "0.0.0.0");
            final int localPort = readIntInput(gbLocalPortInput, 5060, 1, 65535);
            final String upperIp = readTextInput(gbUpperIpInput, "");
            final int upperPort = readIntInput(gbUpperPortInput, 5060, 1, 65535);
            final StreamCoreGB28181.TransportMode transportMode = selectedGb28181TransportMode();
            final String mediaIp = readTextInput(
                    gbMediaIpInput,
                    resolveFirstNonLoopbackIpv4("0.0.0.0"));
            final int mediaPort = readIntInput(gbMediaPortInput, 15060, 1, 65535);
            final int registerExpires =
                    readIntInput(gbRegisterExpiresInput, 3600, 60, 86_400);
            final int keepalive = readIntInput(gbKeepaliveInput, 60, 5, 3600);
            final String gbSourceLabel = selectedGb28181SourceLabel();
            final String gbSourceBinding = selectedGb28181SourceBinding();
            final StreamCoreGB28181.Config config = buildGb28181Config(
                    localId,
                    localDomain,
                    upperId,
                    upperDomain,
                    sipPassword,
                    localIp,
                    localPort,
                    upperIp,
                    upperPort,
                    transportMode,
                    mediaIp,
                    mediaPort,
                    registerExpires,
                    keepalive,
                    gbSourceLabel);
            final StreamCoreOperationStatus configStatus = session.setConfig(config);
            final StreamCoreOperationStatus infoStatus = session.setDeviceInfo(
                    new StreamCoreGB28181.DeviceInfo(
                            "StreamCore SDK Demo",
                            "HBR",
                            "AndroidDemo",
                            "1.0.0"));
            final StreamCoreOperationStatus statusStatus = session.setDeviceStatus(
                    new StreamCoreGB28181.DeviceStatus("OK", true, false));
            final StreamCoreOperationStatus catalogStatus = session.setCatalog(
                    buildGb28181Catalog(localId, gbSourceLabel));
            final StreamCoreGB28181.RuntimeInfo runtimeInfo = session.getRuntimeInfo();
            return configStatus.statusName
                    + ", info=" + infoStatus.statusName
                    + ", status=" + statusStatus.statusName
                    + ", catalog=" + catalogStatus.statusName
                    + ", inviteSource=" + gbSourceBinding
                    + ", started=" + runtimeInfo.started
                    + ", sessions=" + runtimeInfo.activeSessionCount;
        } catch (RuntimeException ex) {
            return "gb28181_java_wrapper_unavailable: " + ex.getMessage();
        }
    }

    private static void appendRuntimeSection(
            StringBuilder builder,
            StreamCoreRuntimeConfig runtimeConfig,
            StreamCoreOperationStatus configureStatus,
            StreamCoreRuntimeLicenseInfo licenseInfo) {
        builder.append("Runtime:\n")
                .append("- expectedProduct: ")
                .append(runtimeConfig.expectedProduct)
                .append('\n')
                .append("- packageId: ")
                .append(runtimeConfig.packageId)
                .append('\n')
                .append("- appId: ")
                .append(runtimeConfig.appId)
                .append('\n')
                .append("- companyId: ")
                .append(runtimeConfig.companyId)
                .append('\n')
                .append("- configure result: ")
                .append(configureStatus.resultCode)
                .append(" / ")
                .append(configureStatus.statusName)
                .append('\n')
                .append("  ")
                .append(configureStatus.summary)
                .append('\n')
                .append("- license configured: ")
                .append(licenseInfo.configured)
                .append('\n')
                .append("- license loaded: ")
                .append(licenseInfo.licenseLoaded)
                .append('\n')
                .append("- license valid: ")
                .append(licenseInfo.licenseValid)
                .append('\n')
                .append("- need watermark: ")
                .append(licenseInfo.needWatermark)
                .append('\n')
                .append("- license status: ")
                .append(licenseInfo.statusName)
                .append('\n')
                .append("  ")
                .append(licenseInfo.summary)
                .append("\n\n");
    }

    private static void appendFeatureAndLimitSection(
            StringBuilder builder,
            StreamCoreFeatureResult demoFeature,
            StreamCoreLimitResult inputChannelLimit) {
        builder.append("Feature / limit queries:\n")
                .append("- feature ")
                .append(demoFeature.featureName)
                .append(": ")
                .append(demoFeature.enabled)
                .append(" (default=")
                .append(demoFeature.defaultValue)
                .append(", status=")
                .append(demoFeature.status.statusName)
                .append(")\n")
                .append("  ")
                .append(demoFeature.status.summary)
                .append('\n')
                .append("- limit ")
                .append(inputChannelLimit.limitName)
                .append(": ")
                .append(inputChannelLimit.limitValue)
                .append(" (default=")
                .append(inputChannelLimit.defaultValue)
                .append(", status=")
                .append(inputChannelLimit.status.statusName)
                .append(")\n")
                .append("  ")
                .append(inputChannelLimit.status.summary)
                .append("\n\n");
    }

    private static void appendSessionSection(StringBuilder builder) {
        builder.append("Session scopes:\n");
        for (StreamCoreSessionScope scope : StreamCoreSdk.getSessionScopes()) {
            builder.append("- ")
                    .append(scope.sessionKey)
                    .append(": ")
                    .append(scope.summary)
                    .append('\n');
        }
        builder.append('\n');
    }

    private static void appendCapabilitySection(StringBuilder builder) {
        builder.append("Capabilities:\n");
        for (StreamCoreCapabilityDescriptor capability : StreamCoreSdk.getCapabilityDescriptors()) {
            builder.append("- ")
                    .append(capability.capabilityKey)
                    .append(" [")
                    .append(capability.group.stableName)
                    .append("]: ")
                    .append(capability.summary)
                    .append(" | build=")
                    .append(capability.enabledInCurrentBuild)
                    .append(", default=")
                    .append(capability.enabledByDefault)
                    .append(", license=")
                    .append(capability.requiresExplicitLicense)
                    .append('\n');
        }
        builder.append('\n');
    }

    private static void appendLogSection(
            StringBuilder builder,
            StreamCoreOperationStatus configureStatus,
            StreamCoreLogInfo logInfo) {
        builder.append("Log:\n")
                .append("- configure result: ")
                .append(configureStatus.resultCode)
                .append(" / ")
                .append(configureStatus.statusName)
                .append('\n')
                .append("  ")
                .append(configureStatus.summary)
                .append('\n')
                .append("- configured: ")
                .append(logInfo.configured)
                .append('\n')
                .append("- platform: ")
                .append(logInfo.enablePlatformLog)
                .append(", file: ")
                .append(logInfo.enableFileLog)
                .append(", level: ")
                .append(logInfo.minimumLogLevel.stableName)
                .append('\n')
                .append("- log file: ")
                .append(logInfo.logFileName)
                .append('\n')
                .append("- summary: ")
                .append(logInfo.stateSummary)
                .append("\n\n");
    }

    private static void appendPackagePlanSection(StringBuilder builder) {
        builder.append("Package plans:\n");
        for (StreamCorePlatformPackagePlan plan : StreamCoreSdk.getPlatformPackagePlans()) {
            builder.append("- ")
                    .append(plan.platformKey)
                    .append(": ")
                    .append(plan.packageFormat)
                    .append(" / ")
                    .append(plan.artifactName)
                    .append(" / ")
                    .append(plan.integrationEntry)
                    .append(" / status=")
                    .append(plan.deliveryStatus)
                    .append('\n');
        }
        builder.append('\n');
    }

    private static void appendPlayerPreviewScenarioSection(
            StringBuilder builder,
            PlayerPreviewScenarioResult scenario) {
        builder.append("Player preview surface:\n")
                .append("- lifecycle=")
                .append(scenario.lifecycleState)
                .append('\n')
                .append("  ")
                .append(scenario.summary)
                .append('\n')
                .append("  ")
                .append(scenario.detail)
                .append('\n');
        if (scenario.preflight != null) {
            builder.append("- preflight ready: ")
                    .append(scenario.preflight.readyToStart)
                    .append(", state=")
                    .append(scenario.preflight.sessionState.stableName)
                    .append('\n');
            appendStatusLine(builder, "preview preflight", scenario.preflight.status);
        }
        if (scenario.startStatus != null) {
            appendStatusLine(builder, "preview start", scenario.startStatus);
        }
        if (scenario.runtimeAfterStart != null) {
            appendPlayerRuntimeSnapshot(builder, "preview runtime(start)",
                    scenario.runtimeAfterStart);
        }
        if (scenario.stopStatus != null) {
            appendStatusLine(builder, "preview stop", scenario.stopStatus);
        }
        builder.append('\n');
    }

    private static void appendCapturePreviewScenarioSection(
            StringBuilder builder,
            CapturePreviewScenarioResult scenario) {
        builder.append("Camera preview surface (")
                .append(scenario.scenarioKey)
                .append("):\n")
                .append("- lifecycle=")
                .append(scenario.lifecycleState)
                .append('\n')
                .append("  ")
                .append(scenario.summary)
                .append('\n')
                .append("  ")
                .append(scenario.detail)
                .append('\n');
        if (scenario.preflight != null) {
            builder.append("- preflight ready: ")
                    .append(scenario.preflight.readyToStart)
                    .append(", state=")
                    .append(scenario.preflight.sessionState.stableName)
                    .append('\n');
            appendStatusLine(builder, "preview preflight", scenario.preflight.status);
        }
        if (scenario.startStatus != null) {
            appendStatusLine(builder, "preview start", scenario.startStatus);
        }
        if (scenario.publisherStatus != null) {
            appendStatusLine(builder, "publisher transport", scenario.publisherStatus);
        }
        if (scenario.runtimeAfterStart != null) {
            appendCaptureRuntimeSnapshot(builder, "preview runtime(start)",
                    scenario.runtimeAfterStart);
        }
        if (scenario.stopStatus != null) {
            appendStatusLine(builder, "preview stop", scenario.stopStatus);
        }
        builder.append('\n');
    }

    private static void appendPlayerScenarioSection(
            StringBuilder builder,
            PlayerScenarioResult scenario) {
        if (scenario == null) {
            builder.append("Player scenario:\n")
                    .append("- not checked at startup; use playback actions to validate this path.\n\n");
            return;
        }
        builder.append("Player scenario:\n")
                .append("- preflight ready: ")
                .append(scenario.preflight.readyToStart)
                .append(", requiresNetwork=")
                .append(scenario.preflight.requiresNetwork)
                .append(", state=")
                .append(scenario.preflight.sessionState.stableName)
                .append('\n');
        appendStatusLine(builder, "preflight", scenario.preflight.status);
        appendStatusLine(builder, "start", scenario.startStatus);
        appendPlayerRuntimeInfo(builder, scenario.runtimeAfterStart, scenario.runtimeAfterStop);
        builder.append('\n');
    }

    private static void appendCaptureScenarioSection(
            StringBuilder builder,
            CaptureScenarioResult scenario) {
        if (scenario == null) {
            builder.append("Capture scenario (camera video):\n")
                    .append("- not checked at startup; use preview or publish actions to validate this path.\n\n");
            return;
        }
        builder.append("Capture scenario (camera video):\n")
                .append("- preflight ready: ")
                .append(scenario.preflight.readyToStart)
                .append(", requiresPermission=")
                .append(scenario.preflight.requiresPermission)
                .append(", state=")
                .append(scenario.preflight.sessionState.stableName)
                .append('\n');
        appendStatusLine(builder, "preflight", scenario.preflight.status);
        appendStatusLine(builder, "start", scenario.startStatus);
        appendCaptureRuntimeInfo(builder, scenario.runtimeAfterStart, scenario.runtimeAfterStop);
        builder.append('\n');
    }

    private static void appendDesktopCaptureScenarioSection(
            StringBuilder builder,
            DesktopCaptureScenarioResult scenario) {
        builder.append("Desktop capture scenario:\n")
                .append("- lifecycle=")
                .append(scenario.lifecycleState)
                .append('\n')
                .append("  ")
                .append(scenario.summary)
                .append('\n')
                .append("  ")
                .append(scenario.detail)
                .append('\n');
        if (scenario.preflight != null) {
            builder.append("- preflight ready: ")
                    .append(scenario.preflight.readyToStart)
                    .append(", requiresPermission=")
                    .append(scenario.preflight.requiresPermission)
                    .append(", state=")
                    .append(scenario.preflight.sessionState.stableName)
                    .append('\n');
            appendStatusLine(builder, "preflight", scenario.preflight.status);
        }
        if (scenario.startStatus != null) {
            appendStatusLine(builder, "start", scenario.startStatus);
        }
        if (scenario.runtimeAfterStart != null) {
            appendCaptureRuntimeSnapshot(builder, "runtime(start)", scenario.runtimeAfterStart);
        }
        if (scenario.stopStatus != null) {
            appendStatusLine(builder, "stop", scenario.stopStatus);
        }
        if (scenario.runtimeAfterStop != null) {
            appendCaptureRuntimeSnapshot(builder, "runtime(stop)", scenario.runtimeAfterStop);
        }
        builder.append('\n');
    }

    private static void appendPublisherScenarioSection(
            StringBuilder builder,
            List<PublisherScenarioResult> scenarios) {
        builder.append("Publisher scenarios:\n");
        for (PublisherScenarioResult scenario : scenarios) {
            appendPublisherScenario(builder, scenario);
        }
        builder.append("- startup mode: publisher transport is started only by explicit UI actions.\n");
        builder.append("- native transport note: when the device can reach ")
                .append(EMULATOR_HOST_LOOPBACK)
                .append(" or ")
                .append(DEFAULT_DEVELOPMENT_RTMP_HOST)
                .append(':')
                .append(RTMP_PORT)
                .append(", direct publish uses the editable RTMP/RTSP URL field and requires the camera+microphone source.\n");
        builder.append("- transcode policy: prefer passthrough when source codecs already match; expose callback and target parameters when transcode is required.\n");
    }

    private static void appendPublisherScenario(
            StringBuilder builder,
            PublisherScenarioResult scenario) {
        builder.append("- ")
                .append(scenario.scenarioKey)
                .append(": input=")
                .append(scenario.preflight.inputKind)
                .append(", ready=")
                .append(scenario.preflight.readyToStart)
                .append(", state=")
                .append(scenario.preflight.sessionState.stableName)
                .append('\n');
        appendStatusLine(builder, "preflight", scenario.preflight.status);
        appendStatusLine(builder, "start", scenario.startStatus);
        builder.append("  transcode audio=")
                .append(scenario.preflight.transcodeReport.audioNeedsTranscode)
                .append(" (")
                .append(scenario.preflight.transcodeReport.audioReason)
                .append("), video=")
                .append(scenario.preflight.transcodeReport.videoNeedsTranscode)
                .append(" (")
                .append(scenario.preflight.transcodeReport.videoReason)
                .append(")\n")
                .append("  ")
                .append(scenario.preflight.transcodeReport.summary)
                .append('\n')
                .append("  callback=")
                .append(scenario.callbackMessage)
                .append('\n');
        if (!scenario.audioPushStatus.statusName.equals("not_applicable")) {
            appendStatusLine(builder, "pushAudio", scenario.audioPushStatus);
        }
        if (!scenario.videoPushStatus.statusName.equals("not_applicable")) {
            appendStatusLine(builder, "pushVideo", scenario.videoPushStatus);
        }
        appendPublisherRuntimeInfo(builder, scenario.runtimeAfterStart, scenario.runtimeAfterStop);
    }

    private static void appendStatusLine(
            StringBuilder builder,
            String label,
            StreamCoreOperationStatus status) {
        builder.append("  ")
                .append(label)
                .append(": ")
                .append(status.statusName)
                .append(" / ")
                .append(status.resultCode)
                .append('\n')
                .append("    ")
                .append(status.summary)
                .append('\n')
                .append("    ")
                .append(status.detail)
                .append('\n');
    }

    private static void appendPlayerRuntimeInfo(
            StringBuilder builder,
            StreamCorePlayer.RuntimeInfo runtimeAfterStart,
            StreamCorePlayer.RuntimeInfo runtimeAfterStop) {
        appendPlayerRuntimeSnapshot(builder, "runtime(start)", runtimeAfterStart);
        appendPlayerRuntimeSnapshot(builder, "runtime(stop)", runtimeAfterStop);
    }

    private static void appendPlayerRuntimeSnapshot(
            StringBuilder builder,
            String label,
            StreamCorePlayer.RuntimeInfo runtimeInfo) {
        builder.append("  ")
                .append(label)
                .append(": state=")
                .append(runtimeInfo.sessionState.stableName)
                .append(", source=")
                .append(runtimeInfo.sourceIdentity)
                .append(", lastReady=")
                .append(runtimeInfo.lastPreflightReady)
                .append('\n')
                .append("    ")
                .append(runtimeInfo.stateSummary)
                .append('\n');
    }

    private static void appendCaptureRuntimeInfo(
            StringBuilder builder,
            StreamCoreCapture.RuntimeInfo runtimeAfterStart,
            StreamCoreCapture.RuntimeInfo runtimeAfterStop) {
        appendCaptureRuntimeSnapshot(builder, "runtime(start)", runtimeAfterStart);
        appendCaptureRuntimeSnapshot(builder, "runtime(stop)", runtimeAfterStop);
    }

    private static void appendCaptureRuntimeSnapshot(
            StringBuilder builder,
            String label,
            StreamCoreCapture.RuntimeInfo runtimeInfo) {
        builder.append("  ")
                .append(label)
                .append(": state=")
                .append(runtimeInfo.sessionState.stableName)
                .append(", source=")
                .append(runtimeInfo.sourceIdentity)
                .append(", lastReady=")
                .append(runtimeInfo.lastPreflightReady)
                .append('\n')
                .append("    ")
                .append(runtimeInfo.stateSummary)
                .append('\n');
    }

    private static void appendPublisherRuntimeInfo(
            StringBuilder builder,
            StreamCorePublisher.RuntimeInfo runtimeAfterStart,
            StreamCorePublisher.RuntimeInfo runtimeAfterStop) {
        builder.append("  runtime(start): state=")
                .append(runtimeAfterStart.sessionState.stableName)
                .append(", input=")
                .append(runtimeAfterStart.inputIdentity)
                .append(", publish=")
                .append(runtimeAfterStart.publishIdentity)
                .append(", lastReady=")
                .append(runtimeAfterStart.lastPreflightReady)
                .append(", transcodePolicy=")
                .append(runtimeAfterStart.lastTranscodePolicySatisfied)
                .append(", rawUsed=")
                .append(runtimeAfterStart.acceptedRawInput)
                .append(", encodedUsed=")
                .append(runtimeAfterStart.acceptedEncodedInput)
                .append('\n')
                .append("    ")
                .append(runtimeAfterStart.stateSummary)
                .append('\n');
        if (runtimeAfterStop != null) {
            builder.append("  runtime(stop): state=")
                    .append(runtimeAfterStop.sessionState.stableName)
                    .append(", summary=")
                    .append(runtimeAfterStop.stateSummary)
                    .append('\n');
        }
    }

    private static String buildMediaPayloadSummary() {
        final StreamCoreMediaPayload payload = StreamCoreMediaPayload.newBuilder()
                .data(new byte[] {0x00, 0x01, 0x02, 0x03})
                .range(1, 2)
                .timestampMs(33L)
                .keyFrame(true)
                .codecName("h264")
                .build();
        return "codec=" + payload.codecName
                + ", timestampMs=" + payload.timestampMs
                + ", keyFrame=" + payload.keyFrame
                + ", copiedBytes=" + payload.copyBytes().length;
    }

    private static String buildSurfaceTargetSummary() {
        final StreamCoreSurfaceTarget target = StreamCoreSurfaceTarget.none();
        return "targetId=" + target.targetId + ", hasSurface=" + target.hasSurface();
    }

    private File copyAssetToFilesDir(String assetName, String outputName) throws IOException {
        final File outputFile = new File(getFilesDir(), outputName);
        try (InputStream inputStream = getAssets().open(assetName);
             FileOutputStream outputStream = new FileOutputStream(outputFile, false)) {
            final byte[] buffer = new byte[4096];
            int readSize;
            while ((readSize = inputStream.read(buffer)) >= 0) {
                if (readSize == 0) {
                    continue;
                }
                outputStream.write(buffer, 0, readSize);
            }
        }
        return outputFile;
    }

    private File createDemoAudioFile() throws IOException {
        final File outputFile = new File(getFilesDir(), "streamcore_demo_tone.wav");
        final int sampleRate = 48000;
        final int durationMs = 500;
        final int sampleCount = sampleRate * durationMs / 1000;
        final int bytesPerSample = 2;
        final int dataSize = sampleCount * bytesPerSample;
        try (FileOutputStream outputStream = new FileOutputStream(outputFile, false)) {
            writeAscii(outputStream, "RIFF");
            writeLittleEndianInt(outputStream, 36 + dataSize);
            writeAscii(outputStream, "WAVEfmt ");
            writeLittleEndianInt(outputStream, 16);
            writeLittleEndianShort(outputStream, 1);
            writeLittleEndianShort(outputStream, 1);
            writeLittleEndianInt(outputStream, sampleRate);
            writeLittleEndianInt(outputStream, sampleRate * bytesPerSample);
            writeLittleEndianShort(outputStream, bytesPerSample);
            writeLittleEndianShort(outputStream, 16);
            writeAscii(outputStream, "data");
            writeLittleEndianInt(outputStream, dataSize);
            for (int index = 0; index < sampleCount; ++index) {
                final double phase = 2.0 * Math.PI * 440.0 * index / sampleRate;
                final short sample = (short) (Math.sin(phase) * 12000.0);
                writeLittleEndianShort(outputStream, sample);
            }
        }
        return outputFile;
    }

    private File createDemoStillImageFile() throws IOException {
        final File outputFile = new File(getFilesDir(), "streamcore_demo_still.png");
        final Bitmap bitmap = Bitmap.createBitmap(640, 360, Bitmap.Config.ARGB_8888);
        final Canvas canvas = new Canvas(bitmap);
        final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        canvas.drawColor(Color.rgb(20, 27, 34));
        paint.setColor(Color.rgb(45, 132, 191));
        canvas.drawRect(0, 0, 640, 120, paint);
        paint.setColor(Color.rgb(248, 250, 252));
        paint.setTextSize(40.0f);
        paint.setTypeface(Typeface.DEFAULT_BOLD);
        canvas.drawText("StreamCore SDK Demo", 44.0f, 78.0f, paint);
        paint.setTextSize(24.0f);
        paint.setTypeface(Typeface.DEFAULT);
        canvas.drawText("still image publisher source", 44.0f, 190.0f, paint);
        canvas.drawText("video-only contract", 44.0f, 232.0f, paint);
        try (FileOutputStream outputStream = new FileOutputStream(outputFile, false)) {
            if (!bitmap.compress(Bitmap.CompressFormat.PNG, 100, outputStream)) {
                throw new IOException("failed to encode demo still image");
            }
        } finally {
            bitmap.recycle();
        }
        return outputFile;
    }

    private static void writeAscii(FileOutputStream outputStream, String value)
            throws IOException {
        outputStream.write(value.getBytes(java.nio.charset.StandardCharsets.US_ASCII));
    }

    private static void writeLittleEndianInt(FileOutputStream outputStream, int value)
            throws IOException {
        outputStream.write(value & 0xff);
        outputStream.write((value >> 8) & 0xff);
        outputStream.write((value >> 16) & 0xff);
        outputStream.write((value >> 24) & 0xff);
    }

    private static void writeLittleEndianShort(FileOutputStream outputStream, int value)
            throws IOException {
        outputStream.write(value & 0xff);
        outputStream.write((value >> 8) & 0xff);
    }

    private String readAssetText(String assetName) throws IOException {
        try (InputStream inputStream = getAssets().open(assetName);
             ByteArrayOutputStream outputStream = new ByteArrayOutputStream()) {
            final byte[] buffer = new byte[4096];
            int readSize;
            while ((readSize = inputStream.read(buffer)) >= 0) {
                if (readSize == 0) {
                    continue;
                }
                outputStream.write(buffer, 0, readSize);
            }
            return outputStream.toString(java.nio.charset.StandardCharsets.UTF_8.name());
        }
    }

    private List<PublisherScenarioResult> buildPublisherScenarios() {
        final List<PublisherScenarioResult> scenarios = new ArrayList<>();
        scenarios.add(buildLocalCapturePublisherScenario());
        scenarios.add(buildRawFeedPublisherScenario(false));
        scenarios.add(buildEncodedFeedForcedScenario(false));
        scenarios.add(buildMediaFilePassthroughScenario());
        scenarios.add(buildStillImageScenario());
        return scenarios;
    }

    private static final class PublisherLocalCaptureStartResult {
        final PublisherScenarioResult scenario;
        final StreamCorePublisher.Session activeSession;

        PublisherLocalCaptureStartResult(
                PublisherScenarioResult scenario,
                StreamCorePublisher.Session activeSession) {
            this.scenario = scenario;
            this.activeSession = activeSession;
        }
    }

    private PublisherLocalCaptureStartResult startSelectedPublisherTransportScenario(
            StreamCorePublisher.Config config,
            String publishUrl,
            boolean endpointReachable) {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_in_selected_publish"};
        final StreamCoreOperationStatus configStatus = session.setConfig(config);
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus;
        if (!endpointReachable) {
            startStatus = new StreamCoreOperationStatus(
                    StreamCoreResultCode.NOT_ENABLED,
                    "publisher_endpoint_unreachable",
                    "Publish requires a reachable RTMP/RTSP endpoint.",
                    "current URL="
                            + publishUrl
                            + "; expected format such as rtmp://"
                            + DEFAULT_DEVELOPMENT_RTMP_HOST
                            + ":1935/live/mobile_publish or rtsp://"
                            + DEFAULT_DEVELOPMENT_RTMP_HOST
                            + ":8554/live/mobile_publish");
        } else if (!configStatus.isOk()) {
            startStatus = configStatus;
        } else {
            startStatus = session.start();
            if (startStatus.isOk()) {
                waitForPublisherLocalCaptureTransportDrain();
            }
        }
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        final boolean keepSession = startStatus.isOk();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop;
        final StreamCorePublisher.Session activeSession;
        if (keepSession) {
            runtimeAfterStop = null;
            activeSession = session;
        } else {
            session.stop();
            runtimeAfterStop = session.getRuntimeInfo();
            activeSession = null;
        }
        return new PublisherLocalCaptureStartResult(
                new PublisherScenarioResult(
                        "selected_publish_transport",
                        preflight,
                        startStatus,
                        notApplicableStatus(),
                        notApplicableStatus(),
                        runtimeAfterStart,
                        runtimeAfterStop,
                        callbackMessage[0]),
                activeSession);
    }

    private PublisherLocalCaptureStartResult startLocalCaptureNativeTransportScenario(
            boolean cameraPermissionGranted,
            boolean audioPermissionGranted,
            String publishUrl,
            boolean endpointReachable,
            String audioCodec,
            String videoCodec,
            int videoBitrateKbps,
            int videoWidth,
            int videoHeight,
            int fps,
            int gopFrames) {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_in_local_capture_native"};
        session.setConfig(StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_local_native")
                .publishUrl(publishUrl)
                .inputKind(StreamCorePublisher.InputKind.LOCAL_CAPTURE)
                .inputBindingId(localCameraPublisherBinding(true))
                .enableAudio(true)
                .enableVideo(true)
                .androidTransportPolicy(
                        StreamCorePublisher.AndroidTransportPolicy.FORCE_NATIVE_TRANSPORT)
                .transcodeOptions(StreamCorePublisher.TranscodeOptions.newBuilder()
                        .audioMode(StreamCorePublisher.TranscodeMode.FORCE_TRANSCODE)
                        .videoMode(StreamCorePublisher.TranscodeMode.FORCE_TRANSCODE)
                        .targetAudioCodecName(audioCodec)
                        .targetVideoCodecName(videoCodec)
                        .targetAudioFormat(48000, 2)
                        .targetAudioBitrateKbps(96)
                        .targetVideoBitrateKbps(videoBitrateKbps)
                        .targetVideoFormat(videoWidth, videoHeight, fps, gopFrames)
                        .build())
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary)
                .build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCoreOperationStatus startStatus;
        if (!endpointReachable) {
            startStatus = new StreamCoreOperationStatus(
                    StreamCoreResultCode.NOT_ENABLED,
                    "publisher_native_endpoint_unreachable",
                    "Publish requires a reachable RTMP/RTSP endpoint.",
                    "current URL="
                            + publishUrl
                            + "; expected format such as rtmp://"
                            + DEFAULT_DEVELOPMENT_RTMP_HOST
                            + ":1935/live/local_native or rtsp://"
                            + DEFAULT_DEVELOPMENT_RTMP_HOST
                            + ":8554/live/local_native");
        } else if (!cameraPermissionGranted || !audioPermissionGranted) {
            startStatus = new StreamCoreOperationStatus(
                    StreamCoreResultCode.NOT_ENABLED,
                    "publisher_local_capture_permission_missing",
                    "Publish requires camera and microphone permission.",
                    "cameraPermission="
                            + cameraPermissionGranted
                            + ", recordAudioPermission="
                            + audioPermissionGranted);
        } else {
            final StreamCoreOperationStatus nativeStartStatus = session.start();
            startStatus = nativeStartStatus.isOk()
                    ? new StreamCoreOperationStatus(
                    nativeStartStatus.resultCode,
                    "publisher_local_capture_start_succeeded",
                    nativeStartStatus.summary,
                    nativeStartStatus.detail)
                    : nativeStartStatus;
            if (startStatus.isOk()) {
                waitForPublisherLocalCaptureTransportDrain();
            }
        }
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        final boolean keepSession = startStatus.isOk();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop;
        final StreamCorePublisher.Session activeSession;
        if (keepSession) {
            runtimeAfterStop = null;
            activeSession = session;
        } else {
            session.stop();
            runtimeAfterStop = session.getRuntimeInfo();
            activeSession = null;
        }
        return new PublisherLocalCaptureStartResult(
                new PublisherScenarioResult(
                        "local_capture_native_transport",
                        preflight,
                        startStatus,
                        notApplicableStatus(),
                        notApplicableStatus(),
                        runtimeAfterStart,
                        runtimeAfterStop,
                        callbackMessage[0]),
                activeSession);
    }

    private PublisherScenarioResult buildLocalCaptureNativeSkippedScenario(
            String statusName,
            String summary,
            String detail,
            String publishUrl) {
        return buildLocalCaptureNativeSkippedScenario(
                statusName,
                summary,
                detail,
                publishUrl,
                selectedPublisherAudioCodec(),
                selectedPublisherVideoCodec(),
                readPublisherVideoBitrateKbps(),
                readPublisherVideoWidth(),
                readPublisherVideoHeight(),
                readPublisherFps(),
                readPublisherGopFrames());
    }

    private PublisherScenarioResult buildLocalCaptureNativeSkippedScenario(
            String statusName,
            String summary,
            String detail,
            String publishUrl,
            String audioCodec,
            String videoCodec,
            int videoBitrateKbps,
            int videoWidth,
            int videoHeight,
            int fps,
            int gopFrames) {
        final StreamCorePublisher.Session session = new StreamCorePublisher.Session();
        final String[] callbackMessage = new String[] {"not_triggered_explicit_action"};
        session.setConfig(StreamCorePublisher.Config.newBuilder()
                .sessionName("android_demo_publish_local_native")
                .publishUrl(publishUrl)
                .inputKind(StreamCorePublisher.InputKind.LOCAL_CAPTURE)
                .inputBindingId(localCameraPublisherBinding(true))
                .enableAudio(true)
                .enableVideo(true)
                .androidTransportPolicy(
                        StreamCorePublisher.AndroidTransportPolicy.FORCE_NATIVE_TRANSPORT)
                .transcodeOptions(StreamCorePublisher.TranscodeOptions.newBuilder()
                        .audioMode(StreamCorePublisher.TranscodeMode.FORCE_TRANSCODE)
                        .videoMode(StreamCorePublisher.TranscodeMode.FORCE_TRANSCODE)
                        .targetAudioCodecName(audioCodec)
                        .targetVideoCodecName(videoCodec)
                        .targetAudioFormat(48000, 2)
                        .targetAudioBitrateKbps(96)
                        .targetVideoBitrateKbps(videoBitrateKbps)
                        .targetVideoFormat(videoWidth, videoHeight, fps, gopFrames)
                        .build())
                .transcodeRequirementCallback(
                        report -> callbackMessage[0] = "required: " + report.summary)
                .build());
        final StreamCorePublisher.Preflight preflight = session.preflight();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStart = session.getRuntimeInfo();
        session.stop();
        final StreamCorePublisher.RuntimeInfo runtimeAfterStop = session.getRuntimeInfo();
        return new PublisherScenarioResult(
                "local_capture_native_transport",
                preflight,
                new StreamCoreOperationStatus(
                        StreamCoreResultCode.NOT_ENABLED,
                        statusName,
                        summary,
                        detail),
                notApplicableStatus(),
                notApplicableStatus(),
                runtimeAfterStart,
                runtimeAfterStop,
                callbackMessage[0]);
    }
}
