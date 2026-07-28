#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE
#endif

#include "streamcore_demo_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

#ifndef STREAMCORE_DEMO_LICENSE_PATH
#define STREAMCORE_DEMO_LICENSE_PATH ""
#endif

#ifndef STREAMCORE_DEMO_PUBLIC_KEY_PATH
#define STREAMCORE_DEMO_PUBLIC_KEY_PATH ""
#endif

#ifndef STREAMCORE_DEMO_APP_ID
#define STREAMCORE_DEMO_APP_ID ""
#endif

#define STREAMCORE_DEMO_PATH_CAPACITY 1024

typedef struct streamcore_demo_transcode_callback_state_t
{
    int count;
    streamcore_publisher_transcode_report_t report;
} streamcore_demo_transcode_callback_state_t;

static void streamcore_demo_write_text(
    char* outText,
    size_t capacity,
    const char* text)
{
    if ((outText == NULL) || (capacity == 0))
    {
        return;
    }

    if (text == NULL)
    {
        outText[0] = '\0';
        return;
    }

#if defined(_MSC_VER)
    strncpy_s(outText, capacity, text, _TRUNCATE);
#else
    strncpy(outText, text, capacity - 1);
    outText[capacity - 1] = '\0';
#endif
}

static void streamcore_demo_append_text(
    char* outText,
    size_t capacity,
    const char* text)
{
    size_t current_length = 0;

    if (outText == NULL || capacity == 0 || text == NULL || text[0] == '\0')
    {
        return;
    }

    current_length = strlen(outText);
    if (current_length >= capacity - 1)
    {
        return;
    }

#if defined(_MSC_VER)
    strncpy_s(outText + current_length, capacity - current_length, text, _TRUNCATE);
#else
    strncpy(outText + current_length, text, capacity - current_length - 1);
    outText[capacity - 1] = '\0';
#endif
}

static const char* streamcore_demo_env_text(
    const char* name,
    const char* fallback)
{
    const char* value = getenv(name);
    if (value != NULL && value[0] != '\0')
    {
        return value;
    }
    return fallback;
}

static int streamcore_demo_file_exists(const char* path)
{
    FILE* file = NULL;

    if (path == NULL || path[0] == '\0')
    {
        return 0;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return 0;
    }

    fclose(file);
    return 1;
}

static int streamcore_demo_is_absolute_path(const char* path)
{
    if (path == NULL || path[0] == '\0')
    {
        return 0;
    }

#if defined(_WIN32)
    if ((path[0] == '\\' && path[1] == '\\') ||
            (path[1] == ':' && (path[2] == '\\' || path[2] == '/')))
    {
        return 1;
    }
#endif

    return path[0] == '/' || path[0] == '\\';
}

static int streamcore_demo_get_executable_directory(
    char* outPath,
    size_t capacity)
{
    size_t length = 0;
    size_t index = 0;

    streamcore_demo_write_text(outPath, capacity, "");

    if (outPath == NULL || capacity == 0)
    {
        return 0;
    }

#if defined(_WIN32)
    length = (size_t)GetModuleFileNameA(NULL, outPath, (DWORD)capacity);
    if (length == 0 || length >= capacity)
    {
        outPath[0] = '\0';
        return 0;
    }
#elif defined(__APPLE__)
    {
        uint32_t buffer_size = (uint32_t)capacity;
        if (_NSGetExecutablePath(outPath, &buffer_size) != 0)
        {
            outPath[0] = '\0';
            return 0;
        }
        length = strlen(outPath);
    }
#else
    {
        const ssize_t read_length =
            readlink("/proc/self/exe", outPath, capacity - 1);
        if (read_length <= 0 || (size_t)read_length >= capacity)
        {
            outPath[0] = '\0';
            return 0;
        }
        outPath[read_length] = '\0';
        length = (size_t)read_length;
    }
#endif

    for (index = length; index > 0; --index)
    {
        if (outPath[index - 1] == '/' || outPath[index - 1] == '\\')
        {
            outPath[index - 1] = '\0';
            return 1;
        }
    }

    outPath[0] = '\0';
    return 0;
}

static void streamcore_demo_resolve_runtime_path(
    const char* configuredPath,
    char* outPath,
    size_t capacity)
{
    char executable_dir[STREAMCORE_DEMO_PATH_CAPACITY];

    if (outPath == NULL || capacity == 0)
    {
        return;
    }

    streamcore_demo_write_text(outPath, capacity, configuredPath);

    if (configuredPath == NULL || configuredPath[0] == '\0')
    {
        return;
    }

    if (streamcore_demo_is_absolute_path(configuredPath) ||
            streamcore_demo_file_exists(configuredPath))
    {
        return;
    }

    if (!streamcore_demo_get_executable_directory(
            executable_dir,
            sizeof(executable_dir)))
    {
        return;
    }

    if (strlen(executable_dir) + 1 + strlen(configuredPath) >= capacity)
    {
        return;
    }

    streamcore_demo_write_text(outPath, capacity, executable_dir);
#if defined(_WIN32)
    streamcore_demo_append_text(outPath, capacity, "\\");
#else
    streamcore_demo_append_text(outPath, capacity, "/");
#endif
    streamcore_demo_append_text(outPath, capacity, configuredPath);
}

static void streamcore_demo_build_transcode_summary(
    const streamcore_publisher_transcode_report_t* report,
    char* outText,
    size_t capacity)
{
    if (report == NULL)
    {
        streamcore_demo_write_text(outText, capacity, "unavailable");
        return;
    }

    streamcore_demo_write_text(outText, capacity, "");
    if (report->audio_summary[0] != '\0')
    {
        streamcore_demo_append_text(outText, capacity, "audio=");
        streamcore_demo_append_text(outText, capacity, report->audio_summary);
    }
    if (report->video_summary[0] != '\0')
    {
        if (outText[0] != '\0')
        {
            streamcore_demo_append_text(outText, capacity, " | ");
        }
        streamcore_demo_append_text(outText, capacity, "video=");
        streamcore_demo_append_text(outText, capacity, report->video_summary);
    }
    if (outText[0] == '\0')
    {
        streamcore_demo_write_text(outText, capacity, "no_transcode_required");
    }
}

static void streamcore_demo_on_transcode_required(
    const streamcore_publisher_transcode_report_t* report,
    void* userContext)
{
    streamcore_demo_transcode_callback_state_t* state =
        (streamcore_demo_transcode_callback_state_t*)userContext;

    if (state == NULL || report == NULL)
    {
        return;
    }

    state->count += 1;
    state->report = *report;
}

static void streamcore_demo_collect_gb28181_snapshot(
    streamcore_demo_snapshot_t* outSnapshot)
{
#if STREAMCORE_DEMO_ENABLE_GB28181
    streamcore_gb28181_addon_info_t addon_info;
    streamcore_gb28181_config_t config;
    streamcore_gb28181_device_info_t device_info;
    streamcore_gb28181_device_status_t device_status;
    streamcore_gb28181_catalog_item_t catalog_item;
    streamcore_gb28181_handle gb28181 = NULL;

    if (outSnapshot == NULL)
    {
        return;
    }

    memset(&addon_info, 0, sizeof(addon_info));
    memset(&config, 0, sizeof(config));
    memset(&device_info, 0, sizeof(device_info));
    memset(&device_status, 0, sizeof(device_status));
    memset(&catalog_item, 0, sizeof(catalog_item));
    memset(&outSnapshot->gb28181_case, 0, sizeof(outSnapshot->gb28181_case));

    streamcore_get_gb28181_addon_info(&addon_info);
    outSnapshot->gb28181_case.addon_compiled = 1;
    outSnapshot->gb28181_case.start_deferred = 1;
    streamcore_demo_write_text(
        outSnapshot->gb28181_case.addon_name,
        sizeof(outSnapshot->gb28181_case.addon_name),
        addon_info.addon_name);
    streamcore_demo_write_text(
        outSnapshot->gb28181_case.target_name,
        sizeof(outSnapshot->gb28181_case.target_name),
        addon_info.target_name);

    streamcore_gb28181_get_default_config(&config);
    config.session_name = "desktop_gb28181_device";
    config.local_identity.id = "34020000001320000001";
    config.local_identity.domain = "3402000000";
    config.local_identity.password = "12345678";
    config.local_identity.display_name = "StreamCore Demo Device";
    config.upper_platform_identity.id = "34020000002000000001";
    config.upper_platform_identity.domain = "3402000000";
    config.upper_platform_identity.password = "12345678";
    config.upper_platform_identity.display_name = "Demo SIP Platform";
    config.local_endpoint.ip = "0.0.0.0";
    config.local_endpoint.port = 5060;
    config.local_endpoint.transport = STREAMCORE_GB28181_TRANSPORT_UDP;
    config.upper_platform_endpoint.ip = "192.0.2.1";
    config.upper_platform_endpoint.port = 5060;
    config.upper_platform_endpoint.transport = STREAMCORE_GB28181_TRANSPORT_UDP;
    config.default_answer.session_name = "StreamCore Demo";
    config.default_answer.media_endpoint.ip = "192.0.2.1";
    config.default_answer.media_endpoint.port = 30000;
    config.default_answer.media_endpoint.transport =
        STREAMCORE_GB28181_TRANSPORT_UDP;
    config.default_answer.media_direction = "sendonly";
    config.default_answer.video_codec_name = "h264";
    config.default_answer.video_payload_type = 96;
    config.default_answer.audio_codec_name = "aac";
    config.default_answer.audio_payload_type = 97;
    config.default_answer.audio_clock_rate = 48000;
    config.default_answer.audio_channel_count = 2;

    device_info.device_name = "StreamCore Demo Camera";
    device_info.manufacturer = "HBR";
    device_info.model = "StreamCoreDemo";
    device_info.firmware = "0.1.0";

    device_status.status_text = "OK";
    device_status.is_online = 1;
    device_status.is_recording = 0;

    catalog_item.channel_id = "34020000001320000001";
    catalog_item.name = "Demo Channel 1";
    catalog_item.parent_id = "34020000002000000001";
    catalog_item.manufacturer = "HBR";
    catalog_item.model = "StreamCoreDemo";
    catalog_item.owner = "streamcore_demo";
    catalog_item.civil_code = "340200";
    catalog_item.address = "local demo";
    catalog_item.parental = 0;
    catalog_item.is_online = 1;
    catalog_item.status_text = "ON";

    gb28181 = streamcore_gb28181_create();
    if (gb28181 == NULL)
    {
        streamcore_demo_write_text(
            outSnapshot->gb28181_case.summary,
            sizeof(outSnapshot->gb28181_case.summary),
            "failed to create GB28181 addon handle");
        return;
    }

    outSnapshot->gb28181_case.set_config_result =
        streamcore_gb28181_set_config(gb28181, &config);
    outSnapshot->gb28181_case.set_device_info_result =
        streamcore_gb28181_set_device_info(gb28181, &device_info);
    outSnapshot->gb28181_case.set_device_status_result =
        streamcore_gb28181_set_device_status(gb28181, &device_status);
    outSnapshot->gb28181_case.set_catalog_result =
        streamcore_gb28181_set_catalog(gb28181, &catalog_item, 1);
    outSnapshot->gb28181_case.runtime_info_result =
        streamcore_gb28181_get_runtime_info(
            gb28181,
            &outSnapshot->gb28181_case.runtime_info);
    streamcore_demo_write_text(
        outSnapshot->gb28181_case.summary,
        sizeof(outSnapshot->gb28181_case.summary),
        "GB28181 device config, device info, status, catalog, and runtime snapshot are demonstrated; start/register is left for a real SIP platform.");

    streamcore_gb28181_destroy(gb28181);
#else
    if (outSnapshot != NULL)
    {
        memset(&outSnapshot->gb28181_case, 0, sizeof(outSnapshot->gb28181_case));
        streamcore_demo_write_text(
            outSnapshot->gb28181_case.summary,
            sizeof(outSnapshot->gb28181_case.summary),
            "GB28181 addon is not built in this desktop demo configuration.");
    }
#endif
}

static streamcore_result_t streamcore_demo_collect_player_snapshot(
    streamcore_demo_snapshot_t* outSnapshot,
    char* outErrorText,
    size_t errorCapacity)
{
    streamcore_player_handle player = NULL;
    streamcore_player_config_t config;
    streamcore_result_t result = STREAMCORE_RESULT_OK;

    player = streamcore_player_create();
    if (player == NULL)
    {
        streamcore_demo_write_text(
            outErrorText,
            errorCapacity,
            "failed to create player session");
        return STREAMCORE_RESULT_OPERATION_FAILED;
    }

    streamcore_player_get_default_config(&config);
    config.session_name = "desktop_player_preflight";
    config.source_kind = STREAMCORE_PLAYER_SOURCE_KIND_URL;
    config.source_url = "rtsp://demo.example/live/camera-1";
    config.enable_audio = 1;
    config.enable_video = 1;

    result = streamcore_player_set_config(player, &config);
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_player_preflight(
            player,
            &outSnapshot->player_preflight,
            outErrorText,
            errorCapacity);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_player_get_runtime_info(
            player,
            &outSnapshot->player_runtime);
    }

    streamcore_player_destroy(player);
    return result;
}

static streamcore_result_t streamcore_demo_collect_publisher_case(
    const char* caseName,
    const streamcore_publisher_config_t* templateConfig,
    streamcore_demo_publisher_case_t* outCase,
    char* outErrorText,
    size_t errorCapacity)
{
    streamcore_publisher_handle publisher = NULL;
    streamcore_publisher_config_t config;
    streamcore_demo_transcode_callback_state_t callback_state;
    streamcore_result_t result = STREAMCORE_RESULT_OK;

    if (caseName == NULL || templateConfig == NULL || outCase == NULL)
    {
        streamcore_demo_write_text(
            outErrorText,
            errorCapacity,
            "publisher case arguments are required");
        return STREAMCORE_RESULT_INVALID_ARGUMENT;
    }

    memset(outCase, 0, sizeof(*outCase));
    memset(&callback_state, 0, sizeof(callback_state));
    streamcore_demo_write_text(
        outCase->case_name,
        sizeof(outCase->case_name),
        caseName);

    publisher = streamcore_publisher_create();
    if (publisher == NULL)
    {
        streamcore_demo_write_text(
            outErrorText,
            errorCapacity,
            "failed to create publisher session");
        return STREAMCORE_RESULT_OPERATION_FAILED;
    }

    config = *templateConfig;
    config.transcode_callback = streamcore_demo_on_transcode_required;
    config.transcode_callback_context = &callback_state;

    result = streamcore_publisher_set_config(publisher, &config);
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_publisher_preflight(
            publisher,
            &outCase->preflight,
            outErrorText,
            errorCapacity);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_publisher_get_runtime_info(
            publisher,
            &outCase->runtime);
    }

    outCase->transcode_callback_count = callback_state.count;
    outCase->callback_report = callback_state.report;
    streamcore_demo_build_transcode_summary(
        &outCase->preflight.transcode_report,
        outCase->transcode_summary,
        sizeof(outCase->transcode_summary));
    if (callback_state.count > 0)
    {
        streamcore_demo_build_transcode_summary(
            &callback_state.report,
            outCase->callback_summary,
            sizeof(outCase->callback_summary));
    }
    else
    {
        streamcore_demo_write_text(
            outCase->callback_summary,
            sizeof(outCase->callback_summary),
            "not_invoked");
    }

    streamcore_publisher_destroy(publisher);
    return result;
}

static streamcore_result_t streamcore_demo_collect_publisher_snapshots(
    streamcore_demo_snapshot_t* outSnapshot,
    char* outErrorText,
    size_t errorCapacity)
{
    streamcore_publisher_config_t config;
    streamcore_result_t result = STREAMCORE_RESULT_OK;

    outSnapshot->publisher_case_count = 0;

    streamcore_publisher_get_default_config(&config);
    config.session_name = "desktop_publish_local_capture";
    config.publish_url = "rtmp://192.0.2.1:1935/live/desktop";
    config.input_kind = STREAMCORE_PUBLISHER_INPUT_KIND_LOCAL_CAPTURE;
    config.input_binding_id = "desktop+microphone";
    config.enable_audio = 1;
    config.enable_video = 1;
    result = streamcore_demo_collect_publisher_case(
        "local_capture",
        &config,
        &outSnapshot->publisher_cases[outSnapshot->publisher_case_count],
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }
    outSnapshot->publisher_case_count += 1;

    streamcore_publisher_get_default_config(&config);
    config.session_name = "app_publish_encoded_forced";
    config.publish_url = "rtmp://192.0.2.1:1935/live/encoder-1";
    config.input_kind = STREAMCORE_PUBLISHER_INPUT_KIND_APP_ENCODED_FEED;
    config.input_binding_id = "encoder:camera-1";
    config.enable_audio = 1;
    config.enable_video = 1;
    config.source_media_profile.container_name = "annexb";
    config.source_media_profile.audio_codec_name = "aac";
    config.source_media_profile.video_codec_name = "h264";
    config.source_media_profile.has_audio = 1;
    config.source_media_profile.has_video = 1;
    config.source_media_profile.width = 1920;
    config.source_media_profile.height = 1080;
    config.source_media_profile.fps = 30;
    config.transcode.video_mode = STREAMCORE_PUBLISHER_TRANSCODE_MODE_FORCE_TRANSCODE;
    config.transcode.video_codec_name = "h264";
    config.transcode.target_video_bitrate_kbps = 2500;
    config.transcode.target_width = 1920;
    config.transcode.target_height = 1080;
    config.transcode.target_fps = 30;
    result = streamcore_demo_collect_publisher_case(
        "encoded_feed_forced",
        &config,
        &outSnapshot->publisher_cases[outSnapshot->publisher_case_count],
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }
    outSnapshot->publisher_case_count += 1;

    streamcore_publisher_get_default_config(&config);
    config.session_name = "file_publish_passthrough";
    config.publish_url = "rtmp://192.0.2.1:1935/live/file";
    config.input_kind = STREAMCORE_PUBLISHER_INPUT_KIND_APP_ENCODED_FEED;
    config.input_binding_id = "capture:media-file:C:/media/demo_record.mp4";
    config.enable_audio = 1;
    config.enable_video = 1;
    config.source_media_profile.container_name = "mp4";
    config.source_media_profile.audio_codec_name = "aac";
    config.source_media_profile.video_codec_name = "h264";
    config.source_media_profile.has_audio = 1;
    config.source_media_profile.has_video = 1;
    config.source_media_profile.width = 1280;
    config.source_media_profile.height = 720;
    config.source_media_profile.fps = 25;
    config.source_media_profile.sample_rate = 48000;
    config.source_media_profile.channel_count = 2;
    result = streamcore_demo_collect_publisher_case(
        "media_file_passthrough",
        &config,
        &outSnapshot->publisher_cases[outSnapshot->publisher_case_count],
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }
    outSnapshot->publisher_case_count += 1;

    streamcore_publisher_get_default_config(&config);
    config.session_name = "image_publish_fixed_fps";
    config.publish_url = "rtmp://192.0.2.1:1935/live/poster";
    config.input_kind = STREAMCORE_PUBLISHER_INPUT_KIND_APP_RAW_FEED;
    config.input_binding_id = "capture:still-image:C:/media/poster.png";
    config.enable_audio = 0;
    config.enable_video = 1;
    config.transcode.video_codec_name = "h264";
    config.transcode.target_width = 1280;
    config.transcode.target_height = 720;
    config.transcode.target_fps = 25;
    result = streamcore_demo_collect_publisher_case(
        "still_image_fixed_fps",
        &config,
        &outSnapshot->publisher_cases[outSnapshot->publisher_case_count],
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }
    outSnapshot->publisher_case_count += 1;

    return STREAMCORE_RESULT_OK;
}

static streamcore_result_t streamcore_demo_collect_capture_snapshot(
    streamcore_demo_snapshot_t* outSnapshot,
    char* outErrorText,
    size_t errorCapacity)
{
    streamcore_capture_handle capture = NULL;
    streamcore_capture_config_t config;
    streamcore_result_t result = STREAMCORE_RESULT_OK;

    capture = streamcore_capture_create();
    if (capture == NULL)
    {
        streamcore_demo_write_text(
            outErrorText,
            errorCapacity,
            "failed to create capture session");
        return STREAMCORE_RESULT_OPERATION_FAILED;
    }

    streamcore_capture_get_default_config(&config);
    config.session_name = "desktop_capture_preflight";
    config.source_kind = STREAMCORE_CAPTURE_SOURCE_KIND_DESKTOP;
    config.display_id = "display-1";
    config.enable_audio = 0;
    config.enable_video = 1;

    result = streamcore_capture_set_config(capture, &config);
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_capture_preflight(
            capture,
            &outSnapshot->capture_preflight,
            outErrorText,
            errorCapacity);
    }
    if (result == STREAMCORE_RESULT_OK)
    {
        result = streamcore_capture_get_runtime_info(
            capture,
            &outSnapshot->capture_runtime);
    }

    streamcore_capture_destroy(capture);
    return result;
}

streamcore_result_t streamcore_demo_collect_snapshot(
    streamcore_demo_snapshot_t* outSnapshot,
    char* outErrorText,
    size_t errorCapacity)
{
    streamcore_result_t result = STREAMCORE_RESULT_OK;
    streamcore_runtime_config_t runtime_config;
    char machine_error_text[STREAMCORE_TEXT_CAPACITY];
    char license_path[STREAMCORE_DEMO_PATH_CAPACITY];
    char public_key_path[STREAMCORE_DEMO_PATH_CAPACITY];

    if (outSnapshot == NULL)
    {
        streamcore_demo_write_text(
            outErrorText,
            errorCapacity,
            "snapshot output is required");
        return STREAMCORE_RESULT_INVALID_ARGUMENT;
    }

    memset(outSnapshot, 0, sizeof(*outSnapshot));
    memset(&runtime_config, 0, sizeof(runtime_config));
    memset(machine_error_text, 0, sizeof(machine_error_text));
    memset(license_path, 0, sizeof(license_path));
    memset(public_key_path, 0, sizeof(public_key_path));
    streamcore_demo_write_text(outErrorText, errorCapacity, "");

    streamcore_get_product_info(&outSnapshot->product_info);
    streamcore_get_default_build_feature_set(&outSnapshot->build_feature_set);
    streamcore_runtime_get_default_config(&runtime_config);
    streamcore_demo_resolve_runtime_path(
        STREAMCORE_DEMO_LICENSE_PATH,
        license_path,
        sizeof(license_path));
    streamcore_demo_resolve_runtime_path(
        STREAMCORE_DEMO_PUBLIC_KEY_PATH,
        public_key_path,
        sizeof(public_key_path));
    runtime_config.expected_product = "streamcore_demo";
    runtime_config.license_path = license_path;
    runtime_config.public_key_pem_path = public_key_path;
    runtime_config.app_id = STREAMCORE_DEMO_APP_ID;

    outSnapshot->log_config_result = streamcore_log_configure(
        streamcore_demo_env_text(
            "STREAMCORE_DEMO_LOG_DIRECTORY",
            "streamcore/logs"),
        streamcore_demo_env_text(
            "STREAMCORE_DEMO_LOG_FILE_NAME",
            "streamcore_demo.log"),
        STREAMCORE_LOG_INFO,
        1,
        outSnapshot->log_config_error,
        sizeof(outSnapshot->log_config_error));
    (void)streamcore_log_get_info(&outSnapshot->log_info);

    result = streamcore_collect_machine_fingerprint(
        outSnapshot->machine_id,
        sizeof(outSnapshot->machine_id),
        NULL,
        machine_error_text,
        sizeof(machine_error_text));
    if (result != STREAMCORE_RESULT_OK)
    {
        streamcore_demo_write_text(
            outSnapshot->machine_id,
            sizeof(outSnapshot->machine_id),
            "unavailable");
    }

    result = streamcore_runtime_configure(
        &runtime_config,
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }

    result = streamcore_runtime_get_license_info(
        &outSnapshot->license_info,
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }

    if (outSnapshot->license_info.machine_id[0] != '\0')
    {
        streamcore_demo_write_text(
            outSnapshot->machine_id,
            sizeof(outSnapshot->machine_id),
            outSnapshot->license_info.machine_id);
    }

    (void)streamcore_runtime_has_feature(
        "desktop_gui_qt",
        0,
        &outSnapshot->qt_desktop_enabled,
        outErrorText,
        errorCapacity);

    (void)streamcore_runtime_get_limit(
        "max_input_channels",
        0,
        &outSnapshot->max_input_channels,
        outErrorText,
        errorCapacity);

    result = streamcore_demo_collect_player_snapshot(
        outSnapshot,
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }

    result = streamcore_demo_collect_publisher_snapshots(
        outSnapshot,
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }

    streamcore_demo_collect_gb28181_snapshot(outSnapshot);

    result = streamcore_demo_collect_capture_snapshot(
        outSnapshot,
        outErrorText,
        errorCapacity);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }

    result = streamcore_copy_capability_list(
        &outSnapshot->build_feature_set,
        outSnapshot->capabilities,
        sizeof(outSnapshot->capabilities) / sizeof(outSnapshot->capabilities[0]),
        &outSnapshot->capability_count);
    if (result != STREAMCORE_RESULT_OK)
    {
        return result;
    }

    result = streamcore_copy_platform_package_plans(
        outSnapshot->package_plans,
        sizeof(outSnapshot->package_plans) / sizeof(outSnapshot->package_plans[0]),
        &outSnapshot->package_plan_count);
    return result;
}
