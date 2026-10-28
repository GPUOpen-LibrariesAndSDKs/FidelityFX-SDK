// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "../../api/include/ffx_api.h"
#include "../include/ffx_framegeneration.h"

/// Legacy declaration for SDK1 resource structure.
///
/// @ingroup SDKTypes
#if defined(__cplusplus)
typedef struct FfxResourceSDK1 : public FfxApiResource
{
    wchar_t name[FFX_RESOURCE_NAME_SIZE];
    FfxResourceSDK1& operator=(FfxApiResource const& other)
    {
        if (this != &other)
        {
            this->description = other.description;
            this->resource = other.resource;
            this->state = other.state;
        }
        return *this;
    }
} FfxResourceSDK1;
#else
typedef struct FfxResourceSDK1
{
    FfxApiResource header;
    wchar_t name[FFX_RESOURCE_NAME_SIZE];
} FfxResourceSDK1;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct FfxPresentCallbackDescriptionSDK2
{
    FfxDevice       device;                    ///< The active device
    FfxCommandList  commandList;               ///< The command list on which to register render commands
    FfxApiResource  currentBackBuffer;         ///< The backbuffer resource with scene information
    FfxApiResource  currentUI;                 ///< Optional UI texture (when doing backbuffer + ui blend)
    FfxApiResource  outputSwapChainBuffer;     ///< The swapchain target into which to render ui composition
    bool            isInterpolatedFrame;       ///< Whether this is an interpolated or real frame
    bool            usePremulAlpha;            ///< Toggles whether UI gets premultiplied alpha blending or not
    uint64_t        frameID;
} FfxPresentCallbackDescriptionSDK2;

typedef struct FfxPresentCallbackDescriptionSDK1
{
    FfxDevice       device;                    ///< The active device
    FfxCommandList  commandList;               ///< The command list on which to register render commands
    FfxResourceSDK1  currentBackBuffer;         ///< The backbuffer resource with scene information
    FfxResourceSDK1  currentUI;                 ///< Optional UI texture (when doing backbuffer + ui blend)
    FfxResourceSDK1  outputSwapChainBuffer;     ///< The swapchain target into which to render ui composition
    bool            isInterpolatedFrame;       ///< Whether this is an interpolated or real frame
    bool            usePremulAlpha;            ///< Toggles whether UI gets premultiplied alpha blending or not
    uint64_t        frameID;
} FfxPresentCallbackDescriptionSDK1;

typedef struct FfxFrameGenerationDispatchDescriptionSDK2 {
    FfxCommandList                  commandList;                    ///< The command list on which to register render commands
    FfxApiResource                  presentColor;                   ///< The current presentation color, this will be used as interpolation source data.
    FfxApiResource                  outputs[4];                     ///< Interpolation destination targets (1 for each frame in numInterpolatedFrames)
    uint32_t                        numInterpolatedFrames;          ///< The number of frames to interpolate from the passed in color target
    bool                            reset;                          ///< A boolean value which when set to true, indicates the camera has moved discontinuously.
    FfxApiBackbufferTransferFunction   backBufferTransferFunction;     ///< The transfer function use to convert interpolation source color data to linear RGB.
    float                           minMaxLuminance[2];             ///< Min and max luminance values, used when converting HDR colors to linear RGB
    FfxApiRect2D                       interpolationRect;              ///< The area of the backbuffer that should be used for interpolation in case only a part of the screen is used e.g. due to movie bars
    uint64_t                        frameID;
} FfxFrameGenerationDispatchDescriptionSDK2;

typedef struct FfxFrameGenerationDispatchDescriptionSDK1 {
    FfxCommandList                  commandList;                    ///< The command list on which to register render commands
    FfxResourceSDK1                  presentColor;                   ///< The current presentation color, this will be used as interpolation source data.
    FfxResourceSDK1                  outputs[4];                     ///< Interpolation destination targets (1 for each frame in numInterpolatedFrames)
    uint32_t                        numInterpolatedFrames;          ///< The number of frames to interpolate from the passed in color target
    bool                            reset;                          ///< A boolean value which when set to true, indicates the camera has moved discontinuously.
    FfxApiBackbufferTransferFunction   backBufferTransferFunction;     ///< The transfer function use to convert interpolation source color data to linear RGB.
    float                           minMaxLuminance[2];             ///< Min and max luminance values, used when converting HDR colors to linear RGB
    FfxApiRect2D                       interpolationRect;              ///< The area of the backbuffer that should be used for interpolation in case only a part of the screen is used e.g. due to movie bars
    uint64_t                        frameID;
} FfxFrameGenerationDispatchDescriptionSDK1;

/// A structure representing the configuration options to pass to FrameInterpolationSwapChain
/// Version used in SDK2
///
/// @ingroup FfxInterface
typedef struct FfxFrameGenerationConfigSDK2
{
    FfxSwapchain                        swapChain;                       ///< The <c><i>FfxSwapchain</i></c> to use with frame interpolation
    FfxApiPresentCallbackFunc           presentCallback;                 ///< A UI composition callback to call when finalizing the frame image
    void*                               presentCallbackContext;          ///< A pointer to be passed to the UI composition callback
    FfxApiFrameGenerationDispatchFunc   frameGenerationCallback;         ///< The frame generation callback to use to generate the interpolated frame
    void*                               frameGenerationCallbackContext;  ///< A pointer to be passed to the frame generation callback
    bool                                frameGenerationEnabled;          ///< Sets the state of frame generation. Set to false to disable frame generation
    bool                                allowAsyncWorkloads;             ///< Sets the state of async workloads. Set to true to enable interpolation work on async compute
    bool                                allowAsyncPresent;               ///< Sets the state of async presentation (console only). Set to true to enable present from async command queue
    FfxApiResource                      HUDLessColor;                    ///< The hudless back buffer image to use for UI extraction from backbuffer resource
    FfxUInt32                           flags;                           ///< Flags
    bool                                onlyPresentInterpolated;         ///< Set to true to only present interpolated frame
    FfxApiRect2D                        interpolationRect;               ///< Set the area in the backbuffer that will be interpolated 
    uint64_t                            frameID;                         ///< A frame identifier used to synchronize resource usage in workloads
    bool                                drawDebugPacingLines;            ///< Sets the state of pacing debug lines. Set to true to display debug lines
#if defined(__cplusplus)
    FfxFrameGenerationConfigSDK2& operator=(FfxFrameGenerationConfig const& other)
    {
        if (this != (FfxFrameGenerationConfigSDK2*)&other)
        {
            swapChain = other.swapChain;
            presentCallback = other.presentCallback;
            presentCallbackContext = other.presentCallbackContext;
            frameGenerationCallback = other.frameGenerationCallback;
            frameGenerationCallbackContext = other.frameGenerationCallbackContext;
            frameGenerationEnabled = other.frameGenerationEnabled;
            allowAsyncWorkloads = other.allowAsyncWorkloads;
            allowAsyncPresent = other.allowAsyncPresent;
            HUDLessColor = other.HUDLessColor;
            flags = other.flags;
            onlyPresentInterpolated = other.onlyPresentInterpolated;
            interpolationRect = other.interpolationRect;
            frameID = other.frameID;
            drawDebugPacingLines = other.drawDebugPacingLines;
        }
        return *this;
    }
#endif
} FfxFrameGenerationConfigSDK2;

/// A structure representing the configuration options to pass to FrameInterpolationSwapChain
/// Version used in SDK1
///
/// @ingroup FfxInterface
typedef struct FfxFrameGenerationConfigSDK1
{
    FfxSwapchain                        swapChain;                       ///< The <c><i>FfxSwapchain</i></c> to use with frame interpolation
    FfxApiPresentCallbackFunc           presentCallback;                 ///< A UI composition callback to call when finalizing the frame image
    void*                               presentCallbackContext;          ///< A pointer to be passed to the UI composition callback
    FfxApiFrameGenerationDispatchFunc   frameGenerationCallback;         ///< The frame generation callback to use to generate the interpolated frame
    void*                               frameGenerationCallbackContext;  ///< A pointer to be passed to the frame generation callback
    bool                                frameGenerationEnabled;          ///< Sets the state of frame generation. Set to false to disable frame generation
    bool                                allowAsyncWorkloads;             ///< Sets the state of async workloads. Set to true to enable interpolation work on async compute
    bool                                allowAsyncPresent;               ///< Sets the state of async presentation (console only). Set to true to enable present from async command queue
    FfxResourceSDK1                     HUDLessColor;                    ///< The hudless back buffer image to use for UI extraction from backbuffer resource
    FfxUInt32                           flags;                           ///< Flags
    bool                                onlyPresentInterpolated;         ///< Set to true to only present interpolated frame
    FfxApiRect2D                        interpolationRect;               ///< Set the area in the backbuffer that will be interpolated 
    uint64_t                            frameID;                         ///< A frame identifier used to synchronize resource usage in workloads
    bool                                drawDebugPacingLines;            ///< Sets the state of pacing debug lines. Set to true to display debug lines
#if defined(__cplusplus)
    FfxFrameGenerationConfigSDK1& operator=(FfxFrameGenerationConfig const& other)
    {
        if (this != (FfxFrameGenerationConfigSDK1*)&other)
        {
            swapChain = other.swapChain;
            presentCallback = other.presentCallback;
            presentCallbackContext = other.presentCallbackContext;
            frameGenerationCallback = other.frameGenerationCallback;
            frameGenerationCallbackContext = other.frameGenerationCallbackContext;
            frameGenerationEnabled = other.frameGenerationEnabled;
            allowAsyncWorkloads = other.allowAsyncWorkloads;
            allowAsyncPresent = other.allowAsyncPresent;
            HUDLessColor = other.HUDLessColor;
            flags = other.flags;
            onlyPresentInterpolated = other.onlyPresentInterpolated;
            interpolationRect = other.interpolationRect;
            frameID = other.frameID;
            drawDebugPacingLines = other.drawDebugPacingLines;
        }
        return *this;
    }
#endif
} FfxFrameGenerationConfigSDK1;

#if defined(__cplusplus)
} // extern "C"
#endif
