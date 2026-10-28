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
#include "ffx_framegeneration_api_types.h"

#define FFX_FRAMEGENERATION_VERSION_MAJOR 4
#define FFX_FRAMEGENERATION_VERSION_MINOR 0
#define FFX_FRAMEGENERATION_VERSION_PATCH 2

#define FFX_FRAMEGENERATION_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define FFX_FRAMEGENERATION_VERSION                           FFX_FRAMEGENERATION_MAKE_VERSION(FFX_FRAMEGENERATION_VERSION_MAJOR, FFX_FRAMEGENERATION_VERSION_MINOR, FFX_FRAMEGENERATION_VERSION_PATCH)

#define FFX_FRAMEGENERATION_PREPARE_REPLACEMENT_SUFFIX "PrepareV2"

#if defined(__cplusplus)
extern "C" {
#endif
enum FfxApiCreateContextFramegenerationFlags
{
    FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT             = (1 << 0),  ///< A bit indicating that async compute workloads should be supported. Enables generation work on async compute queues.
    FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS  = (1 << 1),  ///< A bit indicating if the motion vectors are rendered at display resolution.
    FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION = (1 << 2),  ///< A bit indicating that the motion vectors have the jittering pattern applied to them.
    FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED                     = (1 << 3),  ///< A bit indicating that the input depth buffer data provided is inverted [1..0].
    FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE                     = (1 << 4),  ///< A bit indicating that the input depth buffer data provided is using an infinite far plane.
    FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE                 = (1 << 5),  ///< A bit indicating if the input color data provided to all inputs is using a high-dynamic range.
    FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING                     = (1 << 6),  ///< A bit indicating that the runtime should check some API values and report issues.
};

enum FfxApiDispatchFramegenerationFlags
{
    FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES       = (1 << 0),  ///< A bit indicating that the debug tear lines will be drawn to the generated output.
    FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS = (1 << 1),  ///< A bit indicating that the debug reset indicators will be drawn to the generated output.
    FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW             = (1 << 2),  ///< A bit indicating that the generated output resource will contain debug views with relevant information.
    FFX_FRAMEGENERATION_FLAG_NO_SWAPCHAIN_CONTEXT_NOTIFY = (1 << 3),  ///< A bit indicating that the context should only run frame interpolation and not modify the swapchain.
    FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_PACING_LINES     = (1 << 4),  ///< A bit indicating that the debug pacing lines will be drawn to the generated output.
    FFX_FRAMEGENERATION_FLAG_RESERVED_1                  = (1 << 5),  ///< Reserved for future use.
    FFX_FRAMEGENERATION_FLAG_RESERVED_2                  = (1 << 6),  ///< Reserved for future use.
};

enum FfxApiUiCompositionFlags
{
    FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_USE_PREMUL_ALPHA                    = (1 << 0),  ///< A bit indicating that we use premultiplied alpha for UI composition.
    FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING = (1 << 1),  ///< A bit indicating that the swapchain should doublebuffer the UI resource.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x01)
struct ffxCreateContextDescFrameGeneration
{
    ffxCreateContextDescHeader header;            ///< Description header for frame generation context creation.
    uint32_t                   flags;             ///< A combination of zero or more values from FfxApiCreateContextFramegenerationFlags.
    struct FfxApiDimensions2D  displaySize;       ///< The resolution at which both rendered and generated frames will be displayed.
    struct FfxApiDimensions2D  maxRenderSize;     ///< The maximum rendering resolution.
    uint32_t                   backBufferFormat;  ///< The surface format for the backbuffer. One of the values from FfxApiSurfaceFormat.
};

#define FFX_API_CALLBACK_DESC_TYPE_FRAMEGENERATION_PRESENT FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x05)
typedef struct ffxCallbackDescFrameGenerationPresent
{
    ffxDispatchDescHeader header;                 ///< Description header for present callback dispatch.
    void*                 device;                 ///< The GPU device (ID3D12Device for DX12, VkDevice for Vulkan) passed during context creation.
    void*                 commandList;            ///< A command list (ID3D12GraphicsCommandList for DX12, VkCommandBuffer for Vulkan) that will be executed before presentation.
    struct FfxApiResource currentBackBuffer;      ///< Backbuffer image either rendered or generated.
    struct FfxApiResource currentUI;              ///< UI image for composition if passed. Otherwise empty.
    struct FfxApiResource outputSwapChainBuffer;  ///< Output image that will be presented.
    bool                  isGeneratedFrame;       ///< true if this frame is generated, false if rendered.
    uint64_t              frameID;                ///< Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
} ffxCallbackDescFrameGenerationPresent;

#define FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x03)
typedef struct ffxDispatchDescFrameGeneration
{
    ffxDispatchDescHeader header;                      ///< Description header for frame generation dispatch.
    void*                 commandList;                 ///< The command list (ID3D12GraphicsCommandList for DX12, VkCommandBuffer for Vulkan) to record frame generation commands.
    struct FfxApiResource presentColor;                ///< The current presentation color buffer to be used as source for frame generation.
    struct FfxApiResource outputs[4];                  ///< Output destination targets for generated frames (1 for each frame in numGeneratedFrames).
    uint32_t              numGeneratedFrames;          ///< The number of frames to generate from the input color target.
    bool                  reset;                       ///< A boolean value which when set to true, indicates the camera has moved discontinuously and frame generation should reset.
    uint32_t              backbufferTransferFunction;  ///< The transfer function used to convert frame generation source color data to linear RGB. One of the values from FfxApiBackbufferTransferFunction.
    float                 minMaxLuminance[2];          ///< Min and max luminance values (in nits), used when converting HDR colors to linear RGB.
    struct FfxApiRect2D   generationRect;              ///< The area of the backbuffer that should be used for frame generation in case only a part of the screen is active (e.g. due to movie bars).
    uint64_t              frameID;                     ///< Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
} ffxDispatchDescFrameGeneration;

typedef ffxReturnCode_t (*FfxApiPresentCallbackFunc)(ffxCallbackDescFrameGenerationPresent* params, void* pUserCtx);
typedef ffxReturnCode_t (*FfxApiFrameGenerationDispatchFunc)(ffxDispatchDescFrameGeneration* params, void* pUserCtx);

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x02)
struct ffxConfigureDescFrameGeneration
{
    ffxConfigureDescHeader            header;                              ///< Description header for frame generation configuration.
    void*                             swapChain;                           ///< The swapchain to use with frame generation (IDXGISwapChain for DX12, VkSwapchainKHR for Vulkan).
    FfxApiPresentCallbackFunc         presentCallback;                     ///< Callback function called when finalizing the frame image for presentation and UI composition.
    void*                             presentCallbackUserContext;          ///< User context pointer to be passed to the present callback function.
    FfxApiFrameGenerationDispatchFunc frameGenerationCallback;             ///< Callback function called to dispatch frame generation work and generate interpolated frames.
    void*                             frameGenerationCallbackUserContext;  ///< User context pointer to be passed to the frame generation callback function.
    bool                              frameGenerationEnabled;              ///< Sets the state of frame generation. Set to false to disable frame generation.
    bool                              allowAsyncWorkloads;                 ///< Sets the state of async workloads. Set to true to enable generation work on async compute.
    struct FfxApiResource             HUDLessColor;                        ///< The hudless back buffer image to use for UI extraction from backbuffer resource. May be empty.
    uint32_t                          flags;                               ///< Zero or combination of flags from FfxApiDispatchFrameGenerationFlags.
    bool                              onlyPresentGenerated;                ///< Set to true to only present generated frames.
    struct FfxApiRect2D               generationRect;                      ///< The area of the backbuffer that should be used for generation in case only a part of the screen is used e.g. due to movie bars
    uint64_t                          frameID;                             ///< Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
};

#pragma FFX_PRAGMA_WARNING_PUSH
#pragma FFX_PRAGMA_WARNING_DISABLE_DEPRECATIONS

#define FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x04)
struct FFX_DEPRECATION("ffxDispatchDescFrameGenerationPrepare is deprecated, use ffxDispatchDescFrameGeneration" FFX_FRAMEGENERATION_PREPARE_REPLACEMENT_SUFFIX) ffxDispatchDescFrameGenerationPrepare
{
    ffxDispatchDescHeader      header;             ///< Description header for frame generation prepare dispatch.
    uint64_t                   frameID;            ///< Frame identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
    uint32_t                   flags;              ///< Zero or combination of values from FfxApiDispatchFrameGenerationFlags.
    void*                      commandList;        ///< The command list (ID3D12GraphicsCommandList for DX12, VkCommandBuffer for Vulkan) to record frame generation prepare commands.
    struct FfxApiDimensions2D  renderSize;         ///< The dimensions used to render game content, dilatedDepth, dilatedMotionVectors are expected to be of ths size.
    struct FfxApiFloatCoords2D jitterOffset;       ///< The subpixel jitter offset applied to the camera.
    struct FfxApiFloatCoords2D motionVectorScale;  ///< The scale factor to convert motion vectors to pixel space. Internally: pixelSpaceMV = motionVector * motionVectorScale. Set to renderSize if motion vectors are in UV space [0,1], or to (1.0, 1.0) if motion vectors are already in pixel space. When using display-resolution motion vectors, set to displaySize.

    float                 frameTimeDelta;           ///< Time elapsed in milliseconds since the last frame.
    bool                  unused_reset;             ///< A (currently unused) boolean value which when set to true, indicates FrameGeneration will be called in reset mode
    float                 cameraNear;               ///< The distance to the near plane of the camera.
    float                 cameraFar;                ///< The distance to the far plane of the camera. This is used only used in case of non infinite depth.
    float                 cameraFovAngleVertical;   ///< The camera angle field of view in the vertical direction (expressed in radians).
    float                 viewSpaceToMetersFactor;  ///< The scale factor to convert view space units to meters
    struct FfxApiResource depth;                    ///< The depth buffer data for the current frame.
    struct FfxApiResource motionVectors;            ///< The motion vector data for the current frame.
};

#pragma FFX_PRAGMA_WARNING_POP

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_KEYVALUE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x06)
struct ffxConfigureDescFrameGenerationKeyValue
{
    ffxConfigureDescHeader header;  ///< Description header for frame generation key-value configuration.
    uint64_t               key;     ///< Configuration key, member of the FfxApiConfigureFrameGenerationKey enumeration.
    uint64_t               u64;     ///< Integer value or enum value to set.
    void*                  ptr;     ///< Pointer value to set or pointer to value to set.
};

enum FfxApiConfigureFrameGenerationKey
{
    FFX_API_CONFIGURE_FRAMEGENERATION_KEY_DEBUG_VIEW_MODE       = 0,  ///< A key to set a debug view mode of display.
    FFX_API_CONFIGURE_FRAMEGENERATION_KEY_DEBUG_VIEW_FLOW_SCALE = 1,  ///< A key to set a debug view scale for flow resources.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x07)
struct ffxQueryDescFrameGenerationGetGPUMemoryUsage
{
    ffxQueryDescHeader              header;                         ///< Description header for GPU memory usage query.
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGeneration;  ///< Output pointer to receive GPU memory usage information for frame generation.
};

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_REGISTERDISTORTIONRESOURCE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x08)
//Pass this optional linked struct after ffxConfigureDescFrameGeneration
struct ffxConfigureDescFrameGenerationRegisterDistortionFieldResource
{
    ffxConfigureDescHeader header;           ///< Description header for distortion field resource configuration.
    struct FfxApiResource  distortionField;  ///< A resource containing distortion offset data. Needs to be 2-component (ie. RG). Read by FG shaders via Sample. Pixel value encodes [UV coordinate of pixel after lens distortion effect - UV coordinate of pixel before lens distortion].
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_HUDLESS FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x09)
//Pass this optional linked struct at FG context creation to enable app to use different hudlessBackBufferformat (IE.RGBA8_UNORM) from backBufferFormat (IE. BGRA8_UNORM)
struct ffxCreateContextDescFrameGenerationHudless
{
    ffxCreateContextDescHeader header;                   ///< Description header for hudless context creation.
    uint32_t                   hudlessBackBufferFormat;  ///< The surface format for the hudless back buffer. One of the values from FfxApiSurfaceFormat.
};

#pragma FFX_PRAGMA_WARNING_PUSH
#pragma FFX_PRAGMA_WARNING_DISABLE_DEPRECATIONS

#define FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_CAMERAINFO FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x0a)
// Link this struct after ffxDispatchDescFrameGenerationPrepare. This is a required input to FSR3.1.4 and FSR3.1.5.
// These fields are now embedded in ffxDispatchDescFrameGenerationPrepareV2.
struct FFX_DEPRECATION("ffxDispatchDescFrameGenerationPrepareCameraInfo is deprecated, use ffxDispatchDescFrameGeneration" FFX_FRAMEGENERATION_PREPARE_REPLACEMENT_SUFFIX " instead") ffxDispatchDescFrameGenerationPrepareCameraInfo
{
    ffxConfigureDescHeader header;             ///< Description header for camera info configuration.
    float                  cameraPosition[3];  ///< The camera position in world space.
    float                  cameraUp[3];        ///< The camera up normalized vector in world space.
    float                  cameraRight[3];     ///< The camera right normalized vector in world space.
    float                  cameraForward[3];   ///< The camera forward normalized vector in world space.
};

#pragma FFX_PRAGMA_WARNING_POP

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE_V2 FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x0b)
struct ffxQueryDescFrameGenerationGetGPUMemoryUsageV2
{
    ffxQueryDescHeader              header;                         ///< Description header for GPU memory usage query.
    void*                           device;                         ///< The GPU device. For DX12: pointer to ID3D12Device. For VK: pointer to VkDeviceContext. App needs to fill out before Query() call.
    struct FfxApiDimensions2D       maxRenderSize;                  ///< The maximum rendering resolution. App needs to fill out before Query() call.
    struct FfxApiDimensions2D       displaySize;                    ///< The resolution at which both rendered and generated frames will be displayed. App needs to fill out before Query() call.
    uint32_t                        createFlags;                    ///< Context creation flags. A combination of zero or more values from FfxApiCreateContextFramegenerationFlags. App needs to fill out before Query() call.
    uint32_t                        dispatchFlags;                  ///< Dispatch flags. A combination of zero or more values from FfxApiDispatchFrameGenerationFlags. App needs to fill out before Query() call.
    uint32_t                        backBufferFormat;               ///< The surface format for the backbuffer. One of the values from FfxApiSurfaceFormat. App needs to fill out before Query() call.
    uint32_t                        hudlessBackBufferFormat;        ///< The surface format for HUDLessColor resource if used. One of the values from FfxApiSurfaceFormat. Otherwise set value to FFX_API_SURFACE_FORMAT_UNKNOWN(0). App needs to fill out before Query() call.
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGeneration;  ///< Output pointer to receive GPU memory usage information for frame generation. Populated by Query() call.
};


#define FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2 FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x0c)

struct ffxDispatchDescFrameGenerationPrepareV2

{
    ffxDispatchDescHeader      header;             ///< Description header for frame generation prepare dispatch.
    uint64_t                   frameID;            ///< Frame identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
    uint32_t                   flags;              ///< Zero or combination of values from FfxApiDispatchFrameGenerationFlags.
    void*                      commandList;        ///< The command list (ID3D12GraphicsCommandList for DX12, VkCommandBuffer for Vulkan) to record frame generation prepare commands.
    struct FfxApiDimensions2D  renderSize;         ///< The dimensions used to render game content. dilatedDepth and dilatedMotionVectors are expected to be of this size.
    struct FfxApiFloatCoords2D jitterOffset;       ///< The subpixel jitter offset applied to the camera.
    struct FfxApiFloatCoords2D motionVectorScale;  ///< The scale factor to convert motion vectors to pixel space. Internally: pixelSpaceMV = motionVector * motionVectorScale. Set to renderSize if motion vectors are in UV space [0,1], or to (1.0, 1.0) if motion vectors are already in pixel space. When using display-resolution motion vectors, set to displaySize.

    float                 frameTimeDelta;           ///< Time elapsed in milliseconds since the last frame.
    bool                  reset;                    ///< A boolean value which when set to true, indicates the camera has moved discontinuously and frame generation should reset its internal state.
    float                 cameraNear;               ///< The distance to the near plane of the camera.
    float                 cameraFar;                ///< The distance to the far plane of the camera. This is used only in case of non-infinite depth.
    float                 cameraFovAngleVertical;   ///< The camera angle field of view in the vertical direction (expressed in radians).
    float                 viewSpaceToMetersFactor;  ///< The scale factor to convert view space units to meters.
    struct FfxApiResource depth;                    ///< The depth buffer data for the current frame.
    struct FfxApiResource motionVectors;            ///< The motion vector data for the current frame.

    float cameraPosition[3];  ///< The camera position in world space.
    float cameraUp[3];        ///< The camera up normalized vector in world space.
    float cameraRight[3];     ///< The camera right normalized vector in world space.
    float cameraForward[3];   ///< The camera forward normalized vector in world space.
};

#define FFX_API_CALLBACK_DESC_TYPE_FRAMEGENERATION_PRESENT_PREMUL_ALPHA FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x0d)
// This is a structure that is attached to header.pNext of ffxCallbackDescFrameGenerationPresent
// Provides whether to use premultiplied alpha blending or not for the UI
typedef struct ffxCallbackDescFrameGenerationPresentPremulAlpha
{
    ffxApiHeader header;          ///< Header for versioning & ABI stability.
    bool         usePremulAlpha;  ///< Toggles whether UI gets premultiplied alpha blending or not.
} ffxCallbackDescFrameGenerationPresentPremulAlpha;

/// Callback function for waiting on a named fence to reach a specific value.
/// @param fenceName The name of the fence to wait on.
/// @param fenceValueToWaitFor The fence value to wait for.
/// @return 0 on success, non-zero error code on failure.
typedef int32_t (*FfxWaitCallbackFunc)(wchar_t* fenceName, uint64_t fenceValueToWaitFor);

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x0e)
// Pass this linked struct at FG context creation to enable new API features - omitting this will prevent new APIs from functioning.
struct ffxCreateContextDescFrameGenerationVersion
{
    ffxCreateContextDescHeader header;   ///< Description header for frame generation version context creation.
    uint32_t                   version;  ///< The version of the API the application was built against. This must be set to FFX_FRAMEGENERATION_VERSION.
};

#define FFX_API_FRAME_GENERATION_CONFIG FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x0f)
/// A structure representing the configuration options to pass to FrameInterpolationSwapChain
///
/// @ingroup FfxInterface
typedef struct FfxFrameGenerationConfig
{
    ffxApiHeader                      header;                          ///< Header for versioning & ABI stability.
    void*                             swapChain;                       ///< The <c><i>FfxSwapchain</i></c> to use with frame interpolation.
    FfxApiPresentCallbackFunc         presentCallback;                 ///< A UI composition callback to call when finalizing the frame image.
    void*                             presentCallbackContext;          ///< A pointer to be passed to the UI composition callback.
    FfxApiFrameGenerationDispatchFunc frameGenerationCallback;         ///< The frame generation callback to use to generate the interpolated frame.
    void*                             frameGenerationCallbackContext;  ///< A pointer to be passed to the frame generation callback.
    bool                              frameGenerationEnabled;          ///< Sets the state of frame generation. Set to false to disable frame generation.
    bool                              allowAsyncWorkloads;             ///< Sets the state of async workloads. Set to true to enable interpolation work on async compute.
    bool                              allowAsyncPresent;               ///< Sets the state of async presentation (console only). Set to true to enable present from async command queue.
    struct FfxApiResource             HUDLessColor;                    ///< The hudless back buffer image to use for UI extraction from backbuffer resource.
    uint32_t                          flags;                           ///< Zero or combination of flags from FfxApiDispatchFrameGenerationFlags.
    bool                              onlyPresentInterpolated;         ///< Set to true to only present interpolated frame.
    struct FfxApiRect2D               interpolationRect;               ///< Set the area in the backbuffer that will be interpolated.
    uint64_t                          frameID;                         ///< A frame identifier used to synchronize resource usage in workloads.
    bool                              drawDebugPacingLines;            ///< Sets the state of pacing debug lines. Set to true to display debug lines.
} FfxFrameGenerationConfig;

#if defined(__cplusplus)
}  // extern "C"
#endif
