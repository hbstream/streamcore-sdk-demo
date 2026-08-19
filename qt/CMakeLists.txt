cmake_minimum_required(VERSION 3.20)

# StreamCore Qt / desktop demo standalone build entry.
#
# This CMake file lets the Qt demo compile independently by consuming an
# extracted StreamCore SDK release package under
# sdk/<platform_arch>/.

project(streamcore_demo_qt LANGUAGES C CXX)

if (WIN32)
    enable_language(RC)
endif()
if (APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/utf-8)
endif()

set(streamcore_demo_cli_default ON)
if (APPLE)
    # macOS 正式校验绑定 Bundle ID 与 Team ID；无 bundle 身份的 CLI 不能用
    # GUI Demo 授权伪装通过，因此 Apple 默认不构建该辅助程序。
    set(streamcore_demo_cli_default OFF)
endif()
option(STREAMCORE_DEMO_BUILD_CLI
    "Build the desktop command-line snapshot helper" ${streamcore_demo_cli_default})
option(STREAMCORE_DEMO_BUILD_QT_GUI
    "Build the Qt 6 Widgets GUI demo" ON)
option(STREAMCORE_DEMO_ENABLE_GB28181
    "Enable GB28181 demo code when the packaged SDK exposes the public runtime" OFF)
option(STREAMCORE_DEMO_ALLOW_EXTERNAL_SDK_PACKAGE_ROOT
    "Allow STREAMCORE_DEMO_SDK_PACKAGE_ROOT to point outside this demo tree for local development only" OFF)
set(STREAMCORE_DEMO_SDK_PACKAGE_ROOT ""
    CACHE PATH "Extracted StreamCore SDK package root, such as sdk/windows_x86_64")
set(STREAMCORE_DEMO_RUNTIME_DEPENDENCY_DIRS ""
    CACHE STRING "Additional runtime DLL/SO directories copied beside the demo executable")
set(STREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY "$ENV{STREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY}"
    CACHE STRING "macOS code-signing identity for stable TCC permission identity; use '-' for ad-hoc")
set(STREAMCORE_DEMO_APPLE_CODE_SIGN_KEYCHAIN "$ENV{STREAMCORE_DEMO_APPLE_CODE_SIGN_KEYCHAIN}"
    CACHE FILEPATH "Optional keychain path used when signing the macOS demo app")
set(STREAMCORE_DEMO_APPLE_CODE_SIGN_EXTRA_ARGS ""
    CACHE STRING "Extra arguments passed to codesign for the macOS app bundle")

if (APPLE AND STREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY STREQUAL "")
    set(STREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY "-"
        CACHE STRING "macOS code-signing identity for stable TCC permission identity; use '-' for ad-hoc" FORCE)
endif()

if (STREAMCORE_DEMO_ALLOW_EXTERNAL_SDK_PACKAGE_ROOT
        AND NOT STREAMCORE_DEMO_SDK_PACKAGE_ROOT
        AND DEFINED ENV{STREAMCORE_DEMO_SDK_PACKAGE_ROOT})
    set(STREAMCORE_DEMO_SDK_PACKAGE_ROOT "$ENV{STREAMCORE_DEMO_SDK_PACKAGE_ROOT}"
        CACHE PATH "Extracted StreamCore SDK package root" FORCE)
endif()

if (NOT STREAMCORE_DEMO_SDK_PACKAGE_ROOT)
    if (WIN32)
        set(streamcore_demo_default_platform "windows_x86_64")
    elseif (UNIX AND NOT APPLE)
        set(streamcore_demo_default_platform "linux_x86_64")
    elseif (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(streamcore_demo_default_platform "macos_arm64")
    elseif (APPLE)
        set(streamcore_demo_default_platform "macos_x86_64")
    endif()

    if (streamcore_demo_default_platform)
        set(streamcore_demo_candidate
            "${CMAKE_CURRENT_LIST_DIR}/sdk/${streamcore_demo_default_platform}")
        if (EXISTS "${streamcore_demo_candidate}")
            set(STREAMCORE_DEMO_SDK_PACKAGE_ROOT "${streamcore_demo_candidate}"
                CACHE PATH "Extracted StreamCore SDK package root" FORCE)
        endif()
    endif()
endif()

if (NOT STREAMCORE_DEMO_SDK_PACKAGE_ROOT
        OR NOT EXISTS "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include/streamcore/streamcore_sdk.h")
    message(FATAL_ERROR
        "StreamCore SDK package is missing. Extract/copy a package to "
        "streamcore_demo_qt/sdk/<platform_arch>/ "
        "or pass -DSTREAMCORE_DEMO_SDK_PACKAGE_ROOT=<path>.")
endif()

if (STREAMCORE_DEMO_SDK_PACKAGE_ROOT
        AND NOT STREAMCORE_DEMO_ALLOW_EXTERNAL_SDK_PACKAGE_ROOT)
    file(REAL_PATH "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}" streamcore_demo_sdk_package_real)
    file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}/sdk" streamcore_demo_local_sdk_real)
    string(REPLACE "\\" "/" streamcore_demo_sdk_package_real "${streamcore_demo_sdk_package_real}")
    string(REPLACE "\\" "/" streamcore_demo_local_sdk_real "${streamcore_demo_local_sdk_real}")
    if (NOT streamcore_demo_sdk_package_real MATCHES "^${streamcore_demo_local_sdk_real}(/|$)")
        message(FATAL_ERROR
            "Release demo builds must use SDK packages under streamcore_demo_qt/sdk/. "
            "For local development only, pass "
            "-DSTREAMCORE_DEMO_ALLOW_EXTERNAL_SDK_PACKAGE_ROOT=ON.")
    endif()
endif()

add_library(hbr_streamcore_sdk SHARED IMPORTED GLOBAL)
set_target_properties(hbr_streamcore_sdk
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include")
set(STREAMCORE_DEMO_SDK_APPLE_FRAMEWORK_DIR "")
set(STREAMCORE_DEMO_SDK_HAS_GB28181_ADDON OFF)

if (WIN32)
    set(streamcore_demo_sdk_implib
        "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/streamcore_sdk.lib")
    set(streamcore_demo_sdk_library
        "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/bin/streamcore_sdk.dll")
    if (NOT EXISTS "${streamcore_demo_sdk_implib}"
            OR NOT EXISTS "${streamcore_demo_sdk_library}")
        message(FATAL_ERROR
            "Windows SDK package must contain lib/streamcore_sdk.lib and bin/streamcore_sdk.dll.")
    endif()
    set_target_properties(hbr_streamcore_sdk
        PROPERTIES
            IMPORTED_IMPLIB "${streamcore_demo_sdk_implib}"
            IMPORTED_LOCATION "${streamcore_demo_sdk_library}")
elseif (APPLE)
    if (EXISTS "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/StreamCoreSDK.xcframework")
        file(GLOB streamcore_demo_sdk_framework_dirs
            "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/StreamCoreSDK.xcframework/macos*/StreamCoreSDK.framework")
        if (NOT streamcore_demo_sdk_framework_dirs)
            message(FATAL_ERROR
                "macOS SDK package contains StreamCoreSDK.xcframework but no macOS framework slice.")
        endif()
        list(SORT streamcore_demo_sdk_framework_dirs)
        list(GET streamcore_demo_sdk_framework_dirs 0 STREAMCORE_DEMO_SDK_APPLE_FRAMEWORK_DIR)
        set_target_properties(hbr_streamcore_sdk
            PROPERTIES
                IMPORTED_LOCATION "${STREAMCORE_DEMO_SDK_APPLE_FRAMEWORK_DIR}/StreamCoreSDK")
        # Qt 的 macOS 启动桥需要正式 Apple wrapper 采集 Bundle ID；imported
        # framework 的二进制路径不会自动传播 Headers，因此显式加入该公开头目录。
        set_property(TARGET hbr_streamcore_sdk APPEND PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES
                "${STREAMCORE_DEMO_SDK_APPLE_FRAMEWORK_DIR}/Headers")
    else()
        file(GLOB streamcore_demo_sdk_libraries
            "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/libstreamcore_sdk.dylib*"
            "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/libhbr_streamcore_sdk.dylib*")
        if (NOT streamcore_demo_sdk_libraries)
            message(FATAL_ERROR
                "macOS SDK package must contain StreamCoreSDK.xcframework or lib/libstreamcore_sdk.dylib.")
        endif()
        list(SORT streamcore_demo_sdk_libraries)
        list(GET streamcore_demo_sdk_libraries 0 streamcore_demo_sdk_library)
        set_target_properties(hbr_streamcore_sdk
            PROPERTIES
                IMPORTED_LOCATION "${streamcore_demo_sdk_library}")
    endif()
elseif (UNIX)
    file(GLOB streamcore_demo_sdk_libraries
        "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/libstreamcore_sdk.so*"
        "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/libhbr_streamcore_sdk.so*")
    if (NOT streamcore_demo_sdk_libraries)
        message(FATAL_ERROR
            "Linux SDK package must contain lib/libstreamcore_sdk.so.")
    endif()
    list(SORT streamcore_demo_sdk_libraries)
    list(GET streamcore_demo_sdk_libraries 0 streamcore_demo_sdk_library)
    set_target_properties(hbr_streamcore_sdk
        PROPERTIES
            IMPORTED_LOCATION "${streamcore_demo_sdk_library}")
endif()

if (WIN32)
    set(streamcore_demo_gb28181_implib
        "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/streamcore_gb28181.lib")
    set(streamcore_demo_gb28181_library
        "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/bin/streamcore_gb28181.dll")
    if (EXISTS "${streamcore_demo_gb28181_implib}"
            AND EXISTS "${streamcore_demo_gb28181_library}")
        add_library(hbr_streamcore_gb28181 SHARED IMPORTED GLOBAL)
        set_target_properties(hbr_streamcore_gb28181
            PROPERTIES
                IMPORTED_IMPLIB "${streamcore_demo_gb28181_implib}"
                IMPORTED_LOCATION "${streamcore_demo_gb28181_library}"
                INTERFACE_INCLUDE_DIRECTORIES "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include")
        set(STREAMCORE_DEMO_SDK_HAS_GB28181_ADDON ON)
    endif()
elseif (APPLE)
    if (NOT STREAMCORE_DEMO_SDK_APPLE_FRAMEWORK_DIR)
        file(GLOB streamcore_demo_gb28181_libraries
            "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/libstreamcore_gb28181.dylib*"
            "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/libhbr_streamcore_gb28181.dylib*")
        if (streamcore_demo_gb28181_libraries)
            list(SORT streamcore_demo_gb28181_libraries)
            list(GET streamcore_demo_gb28181_libraries 0 streamcore_demo_gb28181_library)
            add_library(hbr_streamcore_gb28181 SHARED IMPORTED GLOBAL)
            set_target_properties(hbr_streamcore_gb28181
                PROPERTIES
                    IMPORTED_LOCATION "${streamcore_demo_gb28181_library}"
                    INTERFACE_INCLUDE_DIRECTORIES "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include")
            set(STREAMCORE_DEMO_SDK_HAS_GB28181_ADDON ON)
        endif()
    endif()
elseif (UNIX)
    file(GLOB streamcore_demo_gb28181_libraries
        "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/libstreamcore_gb28181.so*"
        "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib/libhbr_streamcore_gb28181.so*")
    if (streamcore_demo_gb28181_libraries)
        list(SORT streamcore_demo_gb28181_libraries)
        list(GET streamcore_demo_gb28181_libraries 0 streamcore_demo_gb28181_library)
        add_library(hbr_streamcore_gb28181 SHARED IMPORTED GLOBAL)
        set_target_properties(hbr_streamcore_gb28181
            PROPERTIES
                IMPORTED_LOCATION "${streamcore_demo_gb28181_library}"
                INTERFACE_INCLUDE_DIRECTORIES "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include")
        set(STREAMCORE_DEMO_SDK_HAS_GB28181_ADDON ON)
    endif()
endif()

if (NOT STREAMCORE_DEMO_SDK_HAS_GB28181_ADDON
        AND EXISTS "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include/streamcore/streamcore_sdk_gb28181.h")
    # StreamCore 1.6.2 起，正式包默认把 GB28181 C ABI 编入主 SDK runtime。
    # 保留独立 addon 探测是为了兼容旧包；新包直接复用主 target，不能因为没有
    # streamcore_gb28181.dll/.so 就把已交付的公开能力误判为不可用。
    add_library(hbr_streamcore_gb28181 INTERFACE IMPORTED GLOBAL)
    set_target_properties(hbr_streamcore_gb28181
        PROPERTIES
            INTERFACE_LINK_LIBRARIES hbr_streamcore_sdk
            INTERFACE_INCLUDE_DIRECTORIES "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include")
    set(STREAMCORE_DEMO_SDK_HAS_GB28181_ADDON ON)
endif()

set(streamcore_demo_asset_root "${CMAKE_CURRENT_LIST_DIR}/assets")

set(streamcore_demo_license_root "${CMAKE_CURRENT_LIST_DIR}/license/demo")

if (WIN32)
    set(streamcore_demo_qt_license_file "streamcore_demo_qt.lic")
    set(streamcore_demo_cli_license_file "streamcore_demo_cli.lic")
elseif (APPLE)
    set(streamcore_demo_qt_license_file "streamcore_demo_qt_macos.lic")
    if (STREAMCORE_DEMO_BUILD_CLI)
        message(FATAL_ERROR
            "The macOS Demo license is package-bound. "
            "STREAMCORE_DEMO_BUILD_CLI must remain OFF on Apple platforms.")
    endif()
else()
    set(streamcore_demo_qt_license_file "streamcore_demo_qt_linux.lic")
    set(streamcore_demo_cli_license_file "streamcore_demo_cli_linux.lic")
endif()

if (NOT EXISTS "${streamcore_demo_license_root}/${streamcore_demo_qt_license_file}"
        OR (STREAMCORE_DEMO_BUILD_CLI
            AND NOT EXISTS "${streamcore_demo_license_root}/${streamcore_demo_cli_license_file}"))
    message(FATAL_ERROR
        "Demo license resources are missing. Place them under "
        "streamcore_demo_qt/license/demo/.")
endif()

include(CheckCXXSourceCompiles)
set(streamcore_demo_previous_required_includes "${CMAKE_REQUIRED_INCLUDES}")
set(CMAKE_REQUIRED_INCLUDES "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include")
check_cxx_source_compiles([=[
#include <cstddef>
#include <streamcore/streamcore_sdk.h>
int main()
{
    streamcore_onvif_discovery_config_t config = {};
    streamcore_onvif_device_t device = {};
    config.username = nullptr;
    config.password = nullptr;
    config.resolve_stream_uri = 0;
    device.profile_name[0] = 0;
    device.profile_token[0] = 0;
    device.stream_uri[0] = 0;
    device.resolved_stream_uri = 0;
    return config.username == nullptr &&
        config.password == nullptr &&
        config.resolve_stream_uri == 0 &&
        device.profile_name[0] == 0 &&
        device.profile_token[0] == 0 &&
        device.stream_uri[0] == 0 &&
        device.resolved_stream_uri == 0 ? 0 : 1;
}
]=] STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI_FIELDS)
set(CMAKE_REQUIRED_INCLUDES "${streamcore_demo_previous_required_includes}")
if (STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI_FIELDS)
    message(STATUS "StreamCore SDK ONVIF stream URI fields: enabled")
else()
    message(STATUS "StreamCore SDK ONVIF stream URI fields: unavailable")
endif()

set(streamcore_demo_snapshot_sources
    src/streamcore_demo_snapshot.c)

# Linux Demo 的构建产物会直接进入绿色包，因此必须从生成时就只使用包内相对 RPATH。
function(streamcore_demo_configure_linux_runpath target)
    if (UNIX AND NOT APPLE)
        target_link_options(${target}
            PRIVATE
                "LINKER:--disable-new-dtags")
        set_target_properties(${target}
            PROPERTIES
                BUILD_RPATH "$ORIGIN;$ORIGIN/lib"
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "$ORIGIN;$ORIGIN/lib"
                INSTALL_RPATH_USE_LINK_PATH FALSE)
    endif()
endfunction()

function(streamcore_demo_copy_directory_dlls target sourceDir)
    if (WIN32 AND EXISTS "${sourceDir}")
        file(GLOB streamcore_demo_shared_dlls "${sourceDir}/*.dll")
        if (streamcore_demo_shared_dlls)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${streamcore_demo_shared_dlls}
                    $<TARGET_FILE_DIR:${target}>
                COMMAND_EXPAND_LISTS)
        endif()
    endif()
endfunction()

function(streamcore_demo_clean_windows_media_runtime target)
    if (WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "$<TARGET_FILE_DIR:${target}>/avcodec-61.dll"
                "$<TARGET_FILE_DIR:${target}>/avcodec-62.dll"
                "$<TARGET_FILE_DIR:${target}>/avdevice-61.dll"
                "$<TARGET_FILE_DIR:${target}>/avdevice-62.dll"
                "$<TARGET_FILE_DIR:${target}>/avfilter-10.dll"
                "$<TARGET_FILE_DIR:${target}>/avfilter-11.dll"
                "$<TARGET_FILE_DIR:${target}>/avformat-61.dll"
                "$<TARGET_FILE_DIR:${target}>/avformat-62.dll"
                "$<TARGET_FILE_DIR:${target}>/avutil-59.dll"
                "$<TARGET_FILE_DIR:${target}>/avutil-60.dll"
                "$<TARGET_FILE_DIR:${target}>/libfaac.dll"
                "$<TARGET_FILE_DIR:${target}>/libfaad.dll"
                "$<TARGET_FILE_DIR:${target}>/libopenh264.dll"
                "$<TARGET_FILE_DIR:${target}>/libx264-165.dll"
                "$<TARGET_FILE_DIR:${target}>/postproc-58.dll"
                "$<TARGET_FILE_DIR:${target}>/swresample-5.dll"
                "$<TARGET_FILE_DIR:${target}>/swresample-6.dll"
                "$<TARGET_FILE_DIR:${target}>/swscale-8.dll"
                "$<TARGET_FILE_DIR:${target}>/swscale-9.dll")
    endif()
endfunction()

function(streamcore_demo_copy_directory_shared_libraries target sourceDir)
    if (UNIX AND NOT APPLE AND EXISTS "${sourceDir}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target}>/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${sourceDir}"
                "$<TARGET_FILE_DIR:${target}>/lib")
    endif()
endfunction()

function(streamcore_demo_copy_directory_macos_dylibs target sourceDir)
    if (APPLE AND EXISTS "${sourceDir}")
        file(GLOB streamcore_demo_shared_dylibs "${sourceDir}/*.dylib*")
        if (streamcore_demo_shared_dylibs)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${streamcore_demo_shared_dylibs}
                    $<TARGET_FILE_DIR:${target}>
                COMMAND_EXPAND_LISTS)
        endif()
    endif()
endfunction()

function(streamcore_demo_copy_macos_framework target frameworkDir)
    if (APPLE AND EXISTS "${frameworkDir}")
        get_target_property(streamcore_demo_target_is_bundle ${target} MACOSX_BUNDLE)
        if (NOT streamcore_demo_target_is_bundle)
            return()
        endif()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:${target}>/Contents/Frameworks"
            COMMAND ${CMAKE_COMMAND} -E remove_directory
                "$<TARGET_BUNDLE_DIR:${target}>/Contents/Frameworks/StreamCoreSDK.framework"
            COMMAND /bin/cp -R
                "${frameworkDir}"
                "$<TARGET_BUNDLE_DIR:${target}>/Contents/Frameworks/"
            COMMAND /usr/bin/install_name_tool
                -change
                "@rpath/StreamCoreSDK.framework/Versions/A/StreamCoreSDK"
                "@executable_path/../Frameworks/StreamCoreSDK.framework/Versions/A/StreamCoreSDK"
                "$<TARGET_FILE:${target}>")
    endif()
endfunction()

function(streamcore_demo_deploy_shared_libraries target)
    streamcore_demo_clean_windows_media_runtime(${target})
    if (STREAMCORE_DEMO_SDK_PACKAGE_ROOT)
        streamcore_demo_copy_directory_dlls(${target}
            "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/bin")
        streamcore_demo_copy_directory_shared_libraries(${target}
            "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib")
        if (STREAMCORE_DEMO_SDK_APPLE_FRAMEWORK_DIR)
            streamcore_demo_copy_macos_framework(${target}
                "${STREAMCORE_DEMO_SDK_APPLE_FRAMEWORK_DIR}")
        else()
            streamcore_demo_copy_directory_macos_dylibs(${target}
                "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/lib")
        endif()
    endif()
    foreach(streamcore_demo_runtime_dependency_dir
            IN LISTS STREAMCORE_DEMO_RUNTIME_DEPENDENCY_DIRS)
        streamcore_demo_copy_directory_dlls(${target}
            "${streamcore_demo_runtime_dependency_dir}")
        streamcore_demo_copy_directory_shared_libraries(${target}
            "${streamcore_demo_runtime_dependency_dir}")
    endforeach()
endfunction()

function(streamcore_demo_deploy_license_resources target licenseFile)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${target}>/license/demo"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${streamcore_demo_license_root}/${licenseFile}"
            "$<TARGET_FILE_DIR:${target}>/license/demo/${licenseFile}")
endfunction()

function(streamcore_demo_codesign_apple_bundle target)
    if (APPLE)
        find_program(STREAMCORE_DEMO_CODESIGN_EXECUTABLE codesign)
        if (STREAMCORE_DEMO_CODESIGN_EXECUTABLE)
            if (STREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY STREQUAL "-")
                message(WARNING
                    "streamcore_demo_qt will be ad-hoc signed. macOS TCC "
                    "Camera/Microphone/Screen Recording permissions may need "
                    "to be granted again after rebuilds. Set "
                    "STREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY to a stable "
                    "local/development signing identity for repeatable tests.")
            endif()
            set(streamcore_demo_codesign_args)
            if (STREAMCORE_DEMO_APPLE_CODE_SIGN_KEYCHAIN)
                list(APPEND streamcore_demo_codesign_args
                    --keychain
                    "${STREAMCORE_DEMO_APPLE_CODE_SIGN_KEYCHAIN}")
            endif()
            if (STREAMCORE_DEMO_APPLE_CODE_SIGN_EXTRA_ARGS)
                separate_arguments(streamcore_demo_extra_codesign_args
                    NATIVE_COMMAND
                    "${STREAMCORE_DEMO_APPLE_CODE_SIGN_EXTRA_ARGS}")
                list(APPEND streamcore_demo_codesign_args
                    ${streamcore_demo_extra_codesign_args})
            endif()
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${STREAMCORE_DEMO_CODESIGN_EXECUTABLE}"
                    --force
                    --deep
                    --sign "${STREAMCORE_DEMO_APPLE_CODE_SIGN_IDENTITY}"
                    --timestamp=none
                    ${streamcore_demo_codesign_args}
                    "$<TARGET_BUNDLE_DIR:${target}>")
        endif()
    endif()
endfunction()

function(streamcore_demo_configure_sdk_consumer target licenseFile)
    target_include_directories(${target}
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/src")
    target_link_libraries(${target}
        PRIVATE
            hbr_streamcore_sdk)
    if (STREAMCORE_DEMO_ENABLE_GB28181 AND
            STREAMCORE_DEMO_SDK_HAS_GB28181_ADDON AND
            EXISTS "${STREAMCORE_DEMO_SDK_PACKAGE_ROOT}/include/streamcore/streamcore_sdk_gb28181.h")
        target_link_libraries(${target}
            PRIVATE
                hbr_streamcore_gb28181)
        target_compile_definitions(${target}
            PRIVATE
                STREAMCORE_DEMO_ENABLE_GB28181=1)
    else()
        target_compile_definitions(${target}
            PRIVATE
                STREAMCORE_DEMO_ENABLE_GB28181=0)
    endif()
    target_compile_definitions(${target}
        PRIVATE
            STREAMCORE_DEMO_LICENSE_PATH="license/demo/${licenseFile}")
    if (STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI_FIELDS)
        target_compile_definitions(${target}
            PRIVATE
                STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI=1)
    else()
        target_compile_definitions(${target}
            PRIVATE
                STREAMCORE_DEMO_HAS_ONVIF_STREAM_URI=0)
    endif()
    if (WIN32)
        foreach(streamcore_demo_config Debug Release RelWithDebInfo MinSizeRel)
            string(TOUPPER "${streamcore_demo_config}" streamcore_demo_config_upper)
            set_target_properties(${target}
                PROPERTIES
                    "RUNTIME_OUTPUT_DIRECTORY_${streamcore_demo_config_upper}"
                        "${CMAKE_BINARY_DIR}/${streamcore_demo_config}"
                    "LIBRARY_OUTPUT_DIRECTORY_${streamcore_demo_config_upper}"
                        "${CMAKE_BINARY_DIR}/${streamcore_demo_config}")
        endforeach()
    endif()
    streamcore_demo_configure_linux_runpath(${target})
    streamcore_demo_deploy_shared_libraries(${target})
    streamcore_demo_deploy_license_resources(${target} "${licenseFile}")
endfunction()

if (STREAMCORE_DEMO_BUILD_CLI)
    add_executable(streamcore_demo_cli
        src/streamcore_demo_main.c
        ${streamcore_demo_snapshot_sources})
    streamcore_demo_configure_sdk_consumer(
        streamcore_demo_cli
        "${streamcore_demo_cli_license_file}")
endif()

if (STREAMCORE_DEMO_BUILD_QT_GUI)
    if (DEFINED ENV{QTDIR} AND NOT "$ENV{QTDIR}" STREQUAL "")
        list(PREPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
    endif()

    find_package(Qt6 COMPONENTS Widgets REQUIRED)
    if (UNIX AND NOT APPLE)
        find_package(X11 REQUIRED)
    endif()

    set(streamcore_demo_qt_sources
        src/streamcore_demo_qt_main.cpp
        src/streamcore_demo_qt_window.cpp
        ${streamcore_demo_snapshot_sources})
    if (WIN32)
        list(APPEND streamcore_demo_qt_sources
            src/streamcore_demo_qt.rc)
    elseif (APPLE)
        list(APPEND streamcore_demo_qt_sources
            src/streamcore_demo_qt_macos_permissions.mm)
    endif()

    add_executable(streamcore_demo_qt
        ${streamcore_demo_qt_sources})
    if (WIN32)
        string(REPLACE "\\" "\\\\"
            streamcore_demo_icon_path_rc
            "${streamcore_demo_asset_root}/branding/streamcore_app_icon.ico")
        set_target_properties(streamcore_demo_qt
            PROPERTIES
                WIN32_EXECUTABLE ON)
        target_compile_definitions(streamcore_demo_qt
            PRIVATE
                STREAMCORE_DEMO_ICON_PATH="${streamcore_demo_icon_path_rc}")
    elseif (APPLE)
        # macOS 绿色包直接使用最终 bundle 的相对 RPATH，避免 CMake 为 imported
        # framework 自动写入构建机 SDK staging 绝对路径；该约束必须在链接期生效，
        # 不能依赖打包后修改 Mach-O 来掩盖不可迁移的构建来源。
        set_target_properties(streamcore_demo_qt
            PROPERTIES
                MACOSX_BUNDLE ON
                MACOSX_BUNDLE_INFO_PLIST
                    "${CMAKE_CURRENT_LIST_DIR}/StreamCoreDemoQtInfo.plist.in"
                MACOSX_BUNDLE_BUNDLE_NAME "StreamCore Demo Qt"
                MACOSX_BUNDLE_GUI_IDENTIFIER "com.hbr.streamcoredemo.qt"
                MACOSX_BUNDLE_SHORT_VERSION_STRING "1.6.2"
                MACOSX_BUNDLE_BUNDLE_VERSION "1.6.2"
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "@executable_path/../Frameworks"
                XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
                XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
    endif()

    qt_add_resources(streamcore_demo_qt "streamcore_demo_assets"
        PREFIX "/"
        BASE "${streamcore_demo_asset_root}"
        FILES
            "${streamcore_demo_asset_root}/branding/streamcore_app_icon.png"
            "${streamcore_demo_asset_root}/branding/streamcore_brand_logo.png")

    target_link_libraries(streamcore_demo_qt
        PRIVATE
            Qt6::Widgets)
    if (UNIX AND NOT APPLE)
        target_include_directories(streamcore_demo_qt
            PRIVATE
                ${X11_INCLUDE_DIR})
        target_link_libraries(streamcore_demo_qt
            PRIVATE
                ${X11_LIBRARIES})
    endif()
    if (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        target_link_libraries(streamcore_demo_qt
            PRIVATE
                "-framework ApplicationServices"
                "-framework AVFoundation"
                "-framework Foundation")
    endif()
    streamcore_demo_configure_sdk_consumer(
        streamcore_demo_qt
        "${streamcore_demo_qt_license_file}")

    if (WIN32)
        find_program(STREAMCORE_DEMO_WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt
            HINTS
                "$ENV{QTDIR}/bin"
                "${Qt6_DIR}/../../../bin")
        if (STREAMCORE_DEMO_WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET streamcore_demo_qt POST_BUILD
                COMMAND "${STREAMCORE_DEMO_WINDEPLOYQT_EXECUTABLE}"
                    --no-translations
                    "$<TARGET_FILE:streamcore_demo_qt>")
        else()
            message(WARNING
                "Qt 6 Widgets was found, but windeployqt was not found. "
                "streamcore_demo_qt may miss runtime DLLs and plugins.")
        endif()
    elseif (APPLE)
        find_program(STREAMCORE_DEMO_MACDEPLOYQT_EXECUTABLE
            NAMES macdeployqt
            HINTS
                "$ENV{QTDIR}/bin"
                "${Qt6_DIR}/../../../bin")
        if (STREAMCORE_DEMO_MACDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET streamcore_demo_qt POST_BUILD
                COMMAND "${STREAMCORE_DEMO_MACDEPLOYQT_EXECUTABLE}"
                    "$<TARGET_BUNDLE_DIR:streamcore_demo_qt>"
                    -always-overwrite
                    -verbose=1)
        else()
            message(WARNING
                "Qt 6 Widgets was found, but macdeployqt was not found. "
                "streamcore_demo_qt.app may miss Qt frameworks and plugins.")
        endif()
        streamcore_demo_codesign_apple_bundle(streamcore_demo_qt)
    endif()
endif()
