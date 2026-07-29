cmake_minimum_required(VERSION 3.20)

# StreamCore iOS demo package-first build entry.
#
# This target consumes StreamCoreSDK.xcframework from the demo's local
# Frameworks/ directory or an explicit override path. It does not rely on the
# sibling streamcore_sdk source tree.

project(streamcore_demo_ios LANGUAGES C CXX OBJC OBJCXX)

if (NOT APPLE OR NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR
        "streamcore_demo_ios is only available when CMAKE_SYSTEM_NAME=iOS.")
endif()

# Older Xcode/CMake combinations used on the shared Apple validation host can
# drop a target-local LD_RUNPATH_SEARCH_PATHS override for iOS app bundles.
# Use the directory-scope Xcode attribute so regenerated projects keep the
# embedded XCFramework reachable via @rpath without post-link patching.
set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STREAMCORE_DEMO_ALLOW_EXTERNAL_IOS_FRAMEWORK_ROOT
    "Allow STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT to point outside this demo tree for local development only" OFF)
set(STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT ""
    CACHE PATH "Path to StreamCoreSDK.xcframework for the iOS demo")
set(STREAMCORE_DEMO_IOS_BUNDLE_IDENTIFIER "com.hbr.streamcoredemo"
    CACHE STRING "Bundle identifier used by the iOS demo target")

if (STREAMCORE_DEMO_ALLOW_EXTERNAL_IOS_FRAMEWORK_ROOT
        AND NOT STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT
        AND DEFINED ENV{STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT})
    set(STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT "$ENV{STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT}"
        CACHE PATH "Path to StreamCoreSDK.xcframework for the iOS demo" FORCE)
endif()

if (NOT STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT)
    set(streamcore_demo_ios_framework_candidate
        "${CMAKE_CURRENT_LIST_DIR}/Frameworks/StreamCoreSDK.xcframework")
    if (EXISTS "${streamcore_demo_ios_framework_candidate}")
        set(STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT "${streamcore_demo_ios_framework_candidate}"
            CACHE PATH "Path to StreamCoreSDK.xcframework for the iOS demo" FORCE)
    endif()
endif()

if (NOT STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT
        OR NOT EXISTS "${STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT}")
    message(FATAL_ERROR
        "StreamCoreSDK.xcframework is missing. Place/copy it under "
        "streamcore_demo_ios/Frameworks/ or pass "
        "-DSTREAMCORE_DEMO_IOS_FRAMEWORK_ROOT=<path>.")
endif()

if (STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT
        AND NOT STREAMCORE_DEMO_ALLOW_EXTERNAL_IOS_FRAMEWORK_ROOT)
    file(REAL_PATH "${STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT}" streamcore_demo_ios_framework_real)
    file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}/Frameworks" streamcore_demo_ios_local_frameworks_real)
    string(REPLACE "\\" "/" streamcore_demo_ios_framework_real "${streamcore_demo_ios_framework_real}")
    string(REPLACE "\\" "/" streamcore_demo_ios_local_frameworks_real "${streamcore_demo_ios_local_frameworks_real}")
    if (NOT streamcore_demo_ios_framework_real MATCHES "^${streamcore_demo_ios_local_frameworks_real}(/|$)")
        message(FATAL_ERROR
            "Release iOS demo builds must use StreamCoreSDK.xcframework under "
            "streamcore_demo_ios/Frameworks/. For local development only, pass "
            "-DSTREAMCORE_DEMO_ALLOW_EXTERNAL_IOS_FRAMEWORK_ROOT=ON.")
    endif()
endif()

set(streamcore_demo_ios_framework_dir "")
set(streamcore_demo_ios_framework_dirs "")
set(streamcore_demo_ios_sysroot "${CMAKE_OSX_SYSROOT}")
set(streamcore_demo_ios_expect_simulator FALSE)
if (streamcore_demo_ios_sysroot MATCHES "iphonesimulator")
    set(streamcore_demo_ios_expect_simulator TRUE)
endif()

file(GLOB streamcore_demo_ios_framework_dirs
    "${STREAMCORE_DEMO_IOS_FRAMEWORK_ROOT}/ios*/StreamCoreSDK.framework")
if (streamcore_demo_ios_framework_dirs)
    if (streamcore_demo_ios_expect_simulator)
        list(FILTER streamcore_demo_ios_framework_dirs INCLUDE REGEX "simulator/StreamCoreSDK\\.framework$")
    else()
        list(FILTER streamcore_demo_ios_framework_dirs EXCLUDE REGEX "simulator/StreamCoreSDK\\.framework$")
    endif()
    if (streamcore_demo_ios_framework_dirs)
        list(SORT streamcore_demo_ios_framework_dirs)
        list(GET streamcore_demo_ios_framework_dirs 0 streamcore_demo_ios_framework_dir)
    endif()
endif()

if (NOT streamcore_demo_ios_framework_dir
        OR NOT EXISTS "${streamcore_demo_ios_framework_dir}/StreamCoreSDK")
    message(FATAL_ERROR
        "The iOS demo requires an XCFramework slice that matches the current "
        "SDK root (${CMAKE_OSX_SYSROOT}) and contains StreamCoreSDK.framework.")
endif()

add_library(hbr_streamcore_sdk SHARED IMPORTED GLOBAL)
set_target_properties(hbr_streamcore_sdk
    PROPERTIES
        FRAMEWORK TRUE
        IMPORTED_LOCATION "${streamcore_demo_ios_framework_dir}/StreamCoreSDK"
        INTERFACE_INCLUDE_DIRECTORIES "${streamcore_demo_ios_framework_dir}/Headers")

set(streamcore_demo_ios_resources
    Resources/streamcore_demo.lic
    Resources/streamcore_demo_public.pem
    StreamCoreDemoLaunchScreen.storyboard
    Resources/Assets.xcassets
)
set_source_files_properties(${streamcore_demo_ios_resources}
    PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)

add_executable(streamcore_demo_ios MACOSX_BUNDLE
    StreamCoreDemoAppDelegate.h
    StreamCoreDemoAppDelegate.m
    StreamCoreDemoAppMain.m
    StreamCoreDemoViewController.h
    StreamCoreDemoViewController.m
    ${streamcore_demo_ios_resources}
)

target_include_directories(streamcore_demo_ios
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}")
target_compile_options(streamcore_demo_ios
    PRIVATE
        -fobjc-arc)
target_link_libraries(streamcore_demo_ios
    PRIVATE
        hbr_streamcore_sdk
        "-framework CoreGraphics"
        "-framework Foundation"
        "-framework UIKit")

set_target_properties(streamcore_demo_ios
    PROPERTIES
        OUTPUT_NAME "StreamCoreDemo"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/StreamCoreDemoInfo.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "StreamCore SDK Demo"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${STREAMCORE_DEMO_IOS_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.5.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.5.0"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${STREAMCORE_DEMO_IOS_BUNDLE_IDENTIFIER}"
        XCODE_ATTRIBUTE_PRODUCT_NAME "StreamCoreDemo"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
        XCODE_ATTRIBUTE_MARKETING_VERSION "1.5.0"
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "1"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "13.0"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_EMBED_FRAMEWORKS "${streamcore_demo_ios_framework_dir}"
        XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY "NO"
        XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY "YES")
