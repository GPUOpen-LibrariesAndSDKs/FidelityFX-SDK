/* Copyright (c) 2017-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include "AmdExtD3DCreateDeviceApi.h"

#define AMD_MAKE_VERSION(major,minor,patch) ((major << 22) | (minor << 12) | patch) /// version numbering
#define AMD_UNSPECIFIED_VERSION 0xFFFFAD00                                          /// unspecified verison

struct AmdExtAppRegInfo : public AmdExtD3DCreateDeviceInfo
{
    const WCHAR*    pAppName;          /// application name
    UINT            appVersion;        /// application version
    const WCHAR*    pEngineName;       /// engine name
    UINT            engineVersion;     /// engine version
};
