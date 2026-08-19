/*******************************************************************************
 * streamcore_demo_snapshot.h
 * Copyright (c) 2026 HBRun.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Snapshot helper declarations for StreamCore SDK demo tests.
 ******************************************************************************/

#ifndef _STREAMCORE_DEMO_SNAPSHOT_H_
#define _STREAMCORE_DEMO_SNAPSHOT_H_

#include "streamcore/streamcore_sdk.h"

#if STREAMCORE_DEMO_ENABLE_GB28181
#include "streamcore/streamcore_sdk_gb28181.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define STREAMCORE_DEMO_MACHINE_ID_CAPACITY 128
#define STREAMCORE_DEMO_MAX_CAPABILITIES 16
#define STREAMCORE_DEMO_MAX_PACKAGE_PLANS 16
#define STREAMCORE_DEMO_MAX_PUBLISHER_CASES 4
#define STREAMCORE_DEMO_CASE_NAME_CAPACITY 64

/*
 * Minimal GB28181 device-side snapshot used by the demo UI and CLI.
 * The demo prepares configuration, device info, catalog and runtime queries;
 * actual registration still requires a reachable upper GB28181 platform.
 */
typedef struct streamcore_demo_gb28181_case_t
{
    int addon_compiled;
    int start_deferred;
    streamcore_result_t set_config_result;
    streamcore_result_t set_device_info_result;
    streamcore_result_t set_device_status_result;
    streamcore_result_t set_catalog_result;
    streamcore_result_t runtime_info_result;
    char addon_name[STREAMCORE_TEXT_CAPACITY];
    char target_name[STREAMCORE_TEXT_CAPACITY];
    char summary[STREAMCORE_TEXT_CAPACITY];
#if STREAMCORE_DEMO_ENABLE_GB28181
    streamcore_gb28181_runtime_info_t runtime_info;
#endif
} streamcore_demo_gb28181_case_t;

/*
 * Snapshot for one publisher scenario shown by the demo.
 * case_name is a stable display name, transcode_summary summarizes the latest
 * preflight decision, and callback_summary records whether the transcode
 * callback was invoked.
 */
typedef struct streamcore_demo_publisher_case_t
{
    char case_name[STREAMCORE_DEMO_CASE_NAME_CAPACITY];
    int transcode_callback_count;
    streamcore_publisher_preflight_t preflight;
    streamcore_publisher_runtime_info_t runtime;
    streamcore_publisher_transcode_report_t callback_report;
    char transcode_summary[STREAMCORE_TEXT_CAPACITY];
    char callback_summary[STREAMCORE_TEXT_CAPACITY];
} streamcore_demo_publisher_case_t;

/*
 * Snapshot shared by the desktop demo UI and CLI.
 * It groups product metadata, license status, logging status, capabilities,
 * package information and representative player/capture/publisher examples.
 */
typedef struct streamcore_demo_snapshot_t
{
    streamcore_product_info_t product_info;
    streamcore_build_feature_set_t build_feature_set;
    streamcore_runtime_license_info_t license_info;
    streamcore_log_info_t log_info;
    streamcore_result_t log_config_result;
    char machine_id[STREAMCORE_DEMO_MACHINE_ID_CAPACITY];
    char log_config_error[STREAMCORE_TEXT_CAPACITY];
    int qt_desktop_enabled;
    int64_t max_input_channels;
    streamcore_player_preflight_t player_preflight;
    streamcore_player_runtime_info_t player_runtime;
    streamcore_demo_publisher_case_t publisher_cases[STREAMCORE_DEMO_MAX_PUBLISHER_CASES];
    size_t publisher_case_count;
    streamcore_demo_gb28181_case_t gb28181_case;
    streamcore_capture_preflight_t capture_preflight;
    streamcore_capture_runtime_info_t capture_runtime;
    streamcore_capability_descriptor_t capabilities[STREAMCORE_DEMO_MAX_CAPABILITIES];
    size_t capability_count;
    streamcore_platform_package_plan_t package_plans[STREAMCORE_DEMO_MAX_PACKAGE_PLANS];
    size_t package_plan_count;
} streamcore_demo_snapshot_t;

/*
 * Collects the complete runtime snapshot used by the desktop demo and its
 * automation. When configureRuntime is non-zero, this function submits the
 * packaged license through the native C runtime. macOS Qt passes zero after
 * the public Objective-C wrapper has collected the Bundle ID and configured
 * the same process-wide runtime, preventing an identity-free C configuration
 * from overwriting the validated Apple state.
 */
streamcore_result_t streamcore_demo_collect_snapshot(
    int configureRuntime,
    streamcore_demo_snapshot_t* outSnapshot,
    char* outErrorText,
    size_t errorCapacity);

#ifdef __cplusplus
}
#endif

#endif // _STREAMCORE_DEMO_SNAPSHOT_H_
