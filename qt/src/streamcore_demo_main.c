#include "streamcore_demo_snapshot.h"

#include <stdio.h>

int main(void)
{
    size_t i = 0;
    char error_text[STREAMCORE_TEXT_CAPACITY];
    streamcore_demo_snapshot_t snapshot;

    printf("Product: %s (%s)\n",
        "StreamCore Demo",
        "desktop-cli");
    printf("\n");

    error_text[0] = '\0';
    const streamcore_result_t snapshot_result = streamcore_demo_collect_snapshot(
            &snapshot,
            error_text,
            sizeof(error_text));
    if (snapshot_result != STREAMCORE_RESULT_OK)
    {
        fprintf(stderr,
            "Demo snapshot failed: result=%d %s\n",
            snapshot_result,
            error_text);
        return 2;
    }

    printf("SDK    : %s (%s)\n",
        snapshot.product_info.product_name,
        snapshot.product_info.product_code);
    printf("Target : %s\n", snapshot.product_info.primary_target_name);
    printf("Stage  : %s\n", snapshot.product_info.stage);
    printf("Version: %s\n", snapshot.product_info.version);
    printf("Machine: %s\n\n", snapshot.machine_id);

    printf("License:\n");
    printf("  - status: %s\n",
        snapshot.license_info.status_name);
    printf("  - configured: %d\n", snapshot.license_info.is_configured);
    printf("  - loaded: %d\n", snapshot.license_info.is_license_loaded);
    printf("  - valid : %d\n", snapshot.license_info.is_license_valid);
    printf("  - watermark: %d\n", snapshot.license_info.need_watermark);
    printf("  - summary: %s\n", snapshot.license_info.summary);
    if (snapshot.license_info.detail[0] != '\0')
    {
        printf("  - detail : %s\n", snapshot.license_info.detail);
    }
    printf("\n");

    printf("Log:\n");
    printf("  - configure result: %d\n", snapshot.log_config_result);
    printf("  - summary: %s\n", snapshot.log_info.state_summary);
    printf("  - output: platform=%d file=%d level=%d file=%s\n",
        snapshot.log_info.enable_platform_log,
        snapshot.log_info.enable_file_log,
        snapshot.log_info.minimum_log_level,
        snapshot.log_info.log_file_name);
    if (snapshot.log_config_error[0] != '\0')
    {
        printf("  - log error: %s\n", snapshot.log_config_error);
    }
    printf("\n");

    if (!snapshot.license_info.is_license_valid)
    {
        return 5;
    }

    printf("Licensed Desktop GUI: Qt=%d\n", snapshot.qt_desktop_enabled);
    printf("Licensed Max Input Channels: %lld\n\n",
        (long long)snapshot.max_input_channels);

    printf("Session Preflight:\n");
    printf("  - player  : state=%s | ready=%d | summary=%s\n",
        streamcore_session_state_name(snapshot.player_runtime.state),
        snapshot.player_preflight.is_ready_to_start,
        snapshot.player_preflight.summary);
    printf("    identity: %s\n", snapshot.player_runtime.source_identity);
    printf("  - capture : state=%s | ready=%d | summary=%s\n",
        streamcore_session_state_name(snapshot.capture_runtime.state),
        snapshot.capture_preflight.is_ready_to_start,
        snapshot.capture_preflight.summary);
    printf("    identity: %s\n\n", snapshot.capture_runtime.source_identity);

    printf("Publisher Examples:\n");
    for (i = 0; i < snapshot.publisher_case_count; ++i)
    {
        const streamcore_demo_publisher_case_t* item =
            &snapshot.publisher_cases[i];

        printf("  - %s : kind=%s | state=%s | ready=%d | policy=%d\n",
            item->case_name,
            streamcore_publisher_input_kind_name(item->runtime.input_kind),
            streamcore_session_state_name(item->runtime.state),
            item->preflight.is_ready_to_start,
            item->preflight.transcode_policy_satisfied);
        printf("    input    : %s\n", item->runtime.input_identity);
        printf("    publish  : %s\n", item->runtime.publish_identity);
        printf("    summary  : %s\n", item->preflight.summary);
        printf("    transcode: %s\n", item->transcode_summary);
        printf("    callback : count=%d | %s\n",
            item->transcode_callback_count,
            item->callback_summary);
    }
    printf("\n");

    printf("GB28181 Device Example:\n");
    printf("  - addon: compiled=%d | %s (%s)\n",
        snapshot.gb28181_case.addon_compiled,
        snapshot.gb28181_case.addon_name,
        snapshot.gb28181_case.target_name);
    printf("  - config/device/status/catalog/runtime: %d / %d / %d / %d / %d\n",
        snapshot.gb28181_case.set_config_result,
        snapshot.gb28181_case.set_device_info_result,
        snapshot.gb28181_case.set_device_status_result,
        snapshot.gb28181_case.set_catalog_result,
        snapshot.gb28181_case.runtime_info_result);
    printf("  - start deferred: %d\n", snapshot.gb28181_case.start_deferred);
    printf("  - summary: %s\n", snapshot.gb28181_case.summary);
#if STREAMCORE_DEMO_ENABLE_GB28181
    printf("  - runtime: started=%d registered=%d activeSessions=%d summary=%s\n",
        snapshot.gb28181_case.runtime_info.is_started,
        snapshot.gb28181_case.runtime_info.is_registered,
        snapshot.gb28181_case.runtime_info.active_session_count,
        snapshot.gb28181_case.runtime_info.state_summary);
#endif
    printf("\n");

    printf("Capabilities:\n");
    for (i = 0; i < snapshot.capability_count; ++i)
    {
        printf("  - [%s] %s | enabled=%d | license=%d\n",
            streamcore_capability_group_name(snapshot.capabilities[i].group),
            snapshot.capabilities[i].summary,
            snapshot.capabilities[i].enabled_in_current_build,
            snapshot.capabilities[i].requires_explicit_license);
    }
    printf("\n");

    printf("Platform Packaging:\n");
    for (i = 0; i < snapshot.package_plan_count; ++i)
    {
        printf("  - %s -> %s (%s) [%s]\n",
            snapshot.package_plans[i].platform_key,
            snapshot.package_plans[i].package_format,
            snapshot.package_plans[i].artifact_name,
            snapshot.package_plans[i].delivery_status);
    }

    return 0;
}
