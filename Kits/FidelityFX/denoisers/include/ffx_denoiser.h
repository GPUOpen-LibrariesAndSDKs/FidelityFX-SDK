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

//------------------------------------------------------------------------------
// FFX Includes
//------------------------------------------------------------------------------

// FFX_API_EFFECT_ID_DENOISER
// FFX_API_MAKE_EFFECT_SUB_ID
// ffxConfigureDescHeader
// ffxCreateContextDescHeader
// ffxDispatchDescHeader
// ffxQueryDescHeader
#include "../../api/include/ffx_api.h"
// FfxApiDimensions2D
// FfxApiFloatBounds
// FfxApiFloatCoords2D
// FfxApiFloatCoords3D
// FfxApiMatrix4x4
// FfxApiResource
#include "../../api/include/ffx_api_types.h"

//------------------------------------------------------------------------------
// External Includes
//------------------------------------------------------------------------------

// uint32_t
// uint64_t
#include <stdint.h>

//------------------------------------------------------------------------------
// FFX Denoiser Defines
//------------------------------------------------------------------------------

/// Denoiser major version number
#define FFX_DENOISER_VERSION_MAJOR 1
/// Denoiser minor version number
#define FFX_DENOISER_VERSION_MINOR 2
/// Denoiser patch version number
#define FFX_DENOISER_VERSION_PATCH 1

#define FFX_DENOISER_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
/// Denoiser combined version number
#define FFX_DENOISER_VERSION FFX_DENOISER_MAKE_VERSION(FFX_DENOISER_VERSION_MAJOR, FFX_DENOISER_VERSION_MINOR, FFX_DENOISER_VERSION_PATCH)

//------------------------------------------------------------------------------
// FFX Denoiser Declarations
//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

//------------------------------------------------------------------------------
// FFX Denoiser Descriptions: Create Context
//------------------------------------------------------------------------------

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x01)
/// Denoiser description for context creation.
typedef struct ffxCreateContextDescDenoiser
{
    /// Header description
    /// * @c type: FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER
    ffxCreateContextDescHeader header;

    /// The version of the API the application was built against.
    /// Must be set to @c FFX_DENOISER_VERSION.
    uint32_t version;

    /// Maximum size that rendering will be performed at.
    struct FfxApiDimensions2D maxRenderSize;

    /// Signal flags corresponding to a combination of values from @c FfxApiDenoiserSignalFlags.
    uint32_t signalFlags;

    /// Signal flags corresponding to a combination of values from @c FfxApiDenoiserSignalFlags
    /// for signals that use checkerboard reconstruction.
    /// Must be a subset of @c signalFlags.
    uint32_t checkerboardSignalFlags;

    /// Create flags corresponding to zero or a combination of values from @c FfxApiCreateContextDenoiserFlags.
    uint32_t flags;
} ffxCreateContextDescDenoiser;

//------------------------------------------------------------------------------
// FFX Denoiser Descriptions: Configure
//------------------------------------------------------------------------------

#define FFX_API_CONFIGURE_DESC_TYPE_DENOISER_KEYVALUE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x21)
/// Denoiser description for configuring.
/// @pre Required to be passed to @c ffxConfigure with a non-nullptr denoiser @c FfxContext*.
typedef struct ffxConfigureDescDenoiserKeyValue
{
    /// Header description
    /// * @c type: FFX_API_CONFIGURE_DESC_TYPE_DENOISER_KEYVALUE
    ffxConfigureDescHeader header;

    /// Configuration key corresponding to an entry of @c FfxApiConfigureDenoiserKey.
    uint64_t key;

    /// The number of elements to configure.
    uint64_t count;

    /// Pointer to an array with the configuration values for the elements to configure.
    const void* data;
} ffxConfigureDescDenoiserKeyValue;

//------------------------------------------------------------------------------
// FFX Denoiser Descriptions: Dispatch
//------------------------------------------------------------------------------

typedef struct FfxApiDenoiserSignal
{
    /// Input signal to be denoised.
    struct FfxApiResource input;

    /// Resulting denoised signal.
    struct FfxApiResource output;

    /// @brief The X-coordinate of the first traced pixel at Y=0 in a checkerboard pattern.
    ///
    /// In a checkerboard pattern, only half the pixels are traced per frame, alternating as follows:
    ///
    /// @code
    ///      x=0  x=1  x=2  x=3
    /// y=0:  0    1    0    1
    /// y=1:  1    0    1    0
    /// y=2:  0    1    0    1
    /// @endcode
    ///
    /// * @c 0: pixel (0, 0) was traced.
    /// * @c 1: pixel (1, 0) was traced.
    ///
    /// @code checkerboardOrigin = frameIndex & 1 @endcode
    ///
    /// @pre Must be zero-initialized if this signal is not set in @c checkerboardSignalFlags.
    /// @pre Only meaningful if this signal is set in @c checkerboardSignalFlags.
    uint32_t checkerboardOrigin;
} FfxApiDenoiserSignal;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x41)
/// Denoiser description for dispatching.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Always required to be passed to @c ffxDispatch.
typedef struct ffxDispatchDescDenoiser
{
    /// Header description
    ///
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER
    ffxDispatchDescHeader header;

    /// A pointer to the command list.
    ///
    /// * DX12: pointer to ID3D12GraphicsCommandList.
    void* commandList;

    /// @brief The signed linear depth.
    ///
    /// Channels:
    /// * R:   Signed linear depth values for the current frame. @code CurrentLinearDepth @endcode
    struct FfxApiResource linearDepth;

    /// @brief The motion vectors.
    ///
    /// Channels:
    /// * RG:  2D motion vectors. @code PreviousUV - CurrentUV @endcode
    /// * B:   Signed linear depth delta. @code PreviousLinearDepth - CurrentLinearDepth @endcode
    struct FfxApiResource motionVectors;

    /// @brief The normals, roughness and material type.
    ///
    /// Channels:
    /// * RG:  Octahedrally encoded normals
    /// * B:   Linear Roughness
    /// * A:   Material Type - See docs for more info.
    struct FfxApiResource normals;

    /// @brief The specular albedo.
    ///
    /// Channels:
    /// * RGB: Specular albedo. The denoiser expects a software encoding chosen by the
    ///        application: @code sqrt(SpecularAlbedo) @endcode by default, or the
    ///        raw @code SpecularAlbedo @endcode (identity function) when
    ///        @c FFX_DENOISER_DISPATCH_NON_ENCODED_ALBEDO is provided. This software
    ///        encoding is a convention between the application and the denoiser
    ///        and is orthogonal to the resource format: if a hardware sRGB format is
    ///        used, the sampled value is already sRGB-decoded to linear by the hardware,
    ///        and the software encoding above is then applied on top of that linear value.
    /// @note The default @code sqrt @endcode encoding is a perceptual transfer function:
    ///       it allocates more precision to darker values, matching human perception and
    ///       reducing banding when the albedo is stored in a low-bit-depth format. For this
    ///       reason, apply only one transfer function, not both: either use a hardware sRGB
    ///       format together with @c FFX_DENOISER_DISPATCH_NON_ENCODED_ALBEDO (so the
    ///       software step is the identity function), or store into a linear format and use the default
    ///       software @code sqrt @endcode encoding. Combining a hardware sRGB format with
    ///       the software @code sqrt @endcode encoding applies two perceptual transfer
    ///       functions, which defeats the purpose: the precision redistribution is applied
    ///       twice and warps the values.
    /// @pre Can be zero-initialized if and only if @c FFX_DENOISER_SIGNAL_DIRECT_SPECULAR, @c FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR, and @c FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY are not set in the context creation signal flags.
    struct FfxApiResource specularAlbedo;

    /// @brief The diffuse albedo.
    ///
    /// Channels:
    /// * RGB: Diffuse albedo (e.g., @code BaseColor * (1 - Metalness) @endcode).
    ///        The denoiser expects a software encoding chosen by the application:
    ///        @code sqrt(DiffuseAlbedo) @endcode by default, or the raw
    ///        @code DiffuseAlbedo @endcode (identity function) when
    ///        @c FFX_DENOISER_DISPATCH_NON_ENCODED_ALBEDO is provided. This software
    ///        encoding is a convention between the application and the denoiser
    ///        and is orthogonal to the resource format: if a hardware sRGB format is
    ///        used, the sampled value is already sRGB-decoded to linear by the hardware,
    ///        and the software encoding above is then applied on top of that linear value.
    /// @note The default @code sqrt @endcode encoding is a perceptual transfer function:
    ///       it allocates more precision to darker values, matching human perception and
    ///       reducing banding when the albedo is stored in a low-bit-depth format. For this
    ///       reason, apply only one transfer function, not both: either use a hardware sRGB
    ///       format together with @c FFX_DENOISER_DISPATCH_NON_ENCODED_ALBEDO (so the
    ///       software step is the identity function), or store into a linear format and use the default
    ///       software @code sqrt @endcode encoding. Combining a hardware sRGB format with
    ///       the software @code sqrt @endcode encoding applies two perceptual transfer
    ///       functions, which defeats the purpose: the precision redistribution is applied
    ///       twice and warps the values.
    /// @pre Can be zero-initialized if and only if @c FFX_DENOISER_SIGNAL_DIRECT_DIFFUSE, @c FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE, and @c FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY are not set in the context creation signal flags.
    struct FfxApiResource diffuseAlbedo;

    /// @brief The motion vector scale factor.
    ///
    /// Channels:
    /// * RG:  The scale factor for transforming the 2D motion vectors into UV space
    ///   * For 2D motion vectors computed as @code PreviousUV - CurrentUV @endcode, use @code { .x = +1.0f, .y = +1.0f } @endcode.
    ///   * For 2D motion vectors computed as @code PreviousNDC - CurrentNDC @endcode, use @code { .x = +0.5f, .y = -0.5f } @endcode.
    /// * B:   The scale factor for transforming the signed linear depth delta
    ///   * For linear depth deltas computed as @code PreviousLinearDepth - CurrentLinearDepth @endcode, use @code { .z = +1.0f } @endcode.
    struct FfxApiFloatCoords3D motionVectorScale;

    /// The subpixel jitter offset applied to the camera projection. (Expressed in screen pixels)
    struct FfxApiFloatCoords2D jitterOffsets;

    /// The position delta of the camera since last frame. @code PreviousPosition - CurrentPosition @endcode (Expressed in world space)
    struct FfxApiFloatCoords3D cameraPositionDelta;

    /// @brief The world-to-view transformation matrix for the current frame.
    ///
    /// Matrix convention:
    /// * Storage: row-major (rows stored contiguously in memory)
    /// * Multiplication: row vectors @code v' = vM @endcode
    ///
    /// Compatibility:
    /// * Column-major with column vectors (e.g., OpenGL, Cauldron):
    ///       direct copy (memcpy) - no transpose needed
    /// * Column-major with row vectors:
    ///       requires transpose
    /// * Row-major with row vectors (e.g., DirectXMath):
    ///       direct copy (memcpy)
    /// * Row-major with column vectors:
    ///       requires transpose
    ///
    /// Memory Layout Example (translation by (Tx, Ty, Tz)):
    /// [m00 m01 m02 m03]   [1  0  0  0]
    /// [m10 m11 m12 m13] = [0  1  0  0]
    /// [m20 m21 m22 m23]   [0  0  1  0]
    /// [m30 m31 m32 m33]   [Tx Ty Tz 1]
    FfxApiMatrix4x4 view;

    /// @brief The (unjittered) view-to-projection transformation matrix for the current frame.
    ///
    /// Matrix convention:
    /// * Storage: row-major (rows stored contiguously in memory)
    /// * Multiplication: row vectors @code v' = vM @endcode
    ///
    /// Compatibility:
    /// * Column-major with column vectors (e.g., OpenGL, Cauldron):
    ///       direct copy (memcpy) - no transpose needed
    /// * Column-major with row vectors:
    ///       requires transpose
    /// * Row-major with row vectors (e.g., DirectXMath):
    ///       direct copy (memcpy)
    /// * Row-major with column vectors:
    ///       requires transpose
    FfxApiMatrix4x4 projection;

    /// @brief The absolute linear depth bounds considered for denoising.
    /// @post Passthrough is enabled for signal texels where the corresponding absolute linear depth values are outside [linearDepthBounds.min,linearDepthBounds.max].
    FfxApiFloatBounds linearDepthBounds;

    /// The resolution that was used for rendering the input resources.
    struct FfxApiDimensions2D renderSize;

    /// The index of the current frame. Must be @c previousFrameIndex + 1 for normal operation.
    ///
    /// Any other value (jump or reset) is interpreted as a scene discontinuity and implicitly triggers
    /// @c FFX_DENOISER_DISPATCH_RESET, discarding temporal history. For intentional discontinuities
    /// such as camera cuts, set @c FFX_DENOISER_DISPATCH_RESET explicitly instead.
    uint32_t frameIndex;

    /// Dispatch flags corresponding to zero or a combination of values from @c FfxApiDispatchDenoiserFlags.
    uint32_t flags;
} ffxDispatchDescDenoiser;

/// The maximum number of viewports that can be visualized.
#define FFX_API_DENOISER_DEBUG_VIEW_MAX_VIEWPORTS 12

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_DEBUG_VIEW FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x42)
/// Denoiser description for dispatching debug views.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Can be passed to @c ffxDispatch if and only if @c FFX_DENOISER_ENABLE_DEBUGGING is set in the context creation flags.
typedef struct ffxDispatchDescDenoiserDebugView
{
    /// Header description
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER_DEBUG_VIEW
    ffxDispatchDescHeader header;

    /// @brief The target output resource for debug visualization.
    ///
    /// Channels:
    /// * RGB: Color information to display.
    /// * A:   Indicates which pixels have been written to.
    struct FfxApiResource output;

    /// The resolution of the debug visualization.
    struct FfxApiDimensions2D outputSize;

    /// The debug view mode corresponding to an entry of @c FfxApiDenoiserDebugViewMode.
    uint32_t mode;

    /// The index of the viewport to visualize when @c FFX_API_DENOISER_DEBUG_VIEW_MODE_FULLSCREEN_VIEWPORT mode is active.
    /// Clamped between 0 and @c FFX_API_DENOISER_DEBUG_VIEW_MAX_VIEWPORTS - 1.
    uint32_t viewportIndex;
} ffxDispatchDescDenoiserDebugView;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x43)
/// Denoiser description for dispatching ambient occlusion signals.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Required to be passed to @c ffxDispatch if and only if @c FFX_DENOISER_SIGNAL_AMBIENT_OCCLUSION is set in the context creation signal flags.
typedef struct ffxDispatchDescDenoiserAmbientOcclusion
{
    /// Header description
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION
    ffxDispatchDescHeader header;

    /// Channels:
    /// * R:
    ///   * @c input:  Ambient occlusion (in [0,1] where 0 is fully occluded and 1 is fully exposed) to denoise
    ///   * @c output: Denoised ambient occlusion (in [0,1] where 0 is fully occluded and 1 is fully exposed) (or input if passthrough)
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserAmbientOcclusion;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x44)
/// Denoiser description for dispatching direct diffuse signals.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Required to be passed to @c ffxDispatch if and only if @c FFX_DENOISER_SIGNAL_DIRECT_DIFFUSE is set in the context creation signal flags.
typedef struct ffxDispatchDescDenoiserDirectDiffuse
{
    /// Header description
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE
    ffxDispatchDescHeader header;

    /// Channels:
    /// * RGB:
    ///   * @c input:  Direct diffuse radiance to denoise
    ///   * @c output: Denoised direct diffuse radiance (or input if passthrough)
    /// * A:
    ///   * @c input:  Undefined
    ///   * @c output: Preserved (or input if passthrough)
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserDirectDiffuse;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x45)
/// Denoiser description for dispatching direct specular signals.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Required to be passed to @c ffxDispatch if and only if @c FFX_DENOISER_SIGNAL_DIRECT_SPECULAR is set in the context creation signal flags.
typedef struct ffxDispatchDescDenoiserDirectSpecular
{
    /// Header description
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR
    ffxDispatchDescHeader header;

    /// Channels:
    /// * RGB:
    ///   * @c input:  Direct specular radiance to denoise
    ///   * @c output: Denoised direct specular radiance (or input if passthrough)
    /// * A:
    ///   * @c input:  Undefined
    ///   * @c output: Preserved (or input if passthrough)
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserDirectSpecular;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_DOMINANT_LIGHT FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x46)
/// Denoiser description for dispatching dominant light visibility signals.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Required to be passed to @c ffxDispatch if and only if @c FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY is set in the context creation signal flags.
typedef struct ffxDispatchDescDenoiserDominantLight
{
    /// Header description
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER_DOMINANT_LIGHT
    ffxDispatchDescHeader header;

    /// Channels:
    /// * R:
    ///   * @c input:  Dominant light visibility (ray hit distance [0,FP16_MAX] where FP16_MAX is fully exposed) to denoise
    ///   * @c output: Denoised dominant light visibility (in [0,1] where 0 is fully occluded and 1 is fully exposed) (or input ray hit distance converted to visibility if passthrough)
    struct FfxApiDenoiserSignal signal;

    /// Dominant light direction. (from light source to target)
    struct FfxApiFloatCoords3D direction;

    /// Dominant light emission. (i.e. @c LightColor * LightIntensity)
    struct FfxApiFloatCoords3D emission;

    /// Dominant light angular radius, i.e. the half-angle of the cone the light subtends at the shaded point. (Expressed in radians)
    /// Controls the width of the penumbra and must match the angular radius used when sampling the light to generate the noisy visibility signal.
    float angularRadius;
} ffxDispatchDescDenoiserDominantLight;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x47)
/// Denoiser description for dispatching indirect diffuse signals.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Required to be passed to @c ffxDispatch if and only if @c FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE is set in the context creation signal flags.
typedef struct ffxDispatchDescDenoiserIndirectDiffuse
{
    /// Header description
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE
    ffxDispatchDescHeader header;

    /// Channels:
    /// * RGB:
    ///   * @c input:  Indirect diffuse radiance to denoise
    ///   * @c output: Denoised indirect diffuse radiance (or input if passthrough)
    /// * A:
    ///   * @c input:  Indirect diffuse ray hit distance
    ///   * @c output: Preserved (or input if passthrough)
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserIndirectDiffuse;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x48)
/// Denoiser description for dispatching of indirect specular signals.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Required to be passed to @c ffxDispatch if and only if @c FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR is set in the context creation signal flags.
typedef struct ffxDispatchDescDenoiserIndirectSpecular
{
    /// Header description
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR
    ffxDispatchDescHeader header;

    /// Channels:
    /// * RGB:
    ///   * @c input:  Indirect specular radiance to denoise
    ///   * @c output: Denoised indirect specular radiance (or input if passthrough)
    /// * A:
    ///   * @c input:  Indirect specular ray hit distance
    ///   * @c output: Preserved (or input if passthrough)
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserIndirectSpecular;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_SPECULAR_OCCLUSION FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x49)
/// Denoiser description for dispatching specular occlusion signals.
/// @pre Required to be passed to @c ffxDispatch with a non-nullptr denoiser @c FfxContext*.
/// @pre Required to be passed to @c ffxDispatch if and only if @c FFX_DENOISER_SIGNAL_SPECULAR_OCCLUSION is set in the context creation signal flags.
typedef struct ffxDispatchDescDenoiserSpecularOcclusion
{
    /// Header description
    /// * @c type: FFX_API_DISPATCH_DESC_TYPE_DENOISER_SPECULAR_OCCLUSION
    ffxDispatchDescHeader header;

    /// Channels:
    /// * R:
    ///   * @c input:  Specular occlusion (in [0,1], where 0 is fully occluded and 1 is fully exposed) to denoise
    ///   * @c output: Denoised specular occlusion (in [0,1], where 0 is fully occluded and 1 is fully exposed) (or input if passthrough)
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserSpecularOcclusion;

//------------------------------------------------------------------------------
// FFX Denoiser Descriptions: Query
//------------------------------------------------------------------------------

#define FFX_API_QUERY_DESC_TYPE_DENOISER_GET_DEFAULT_KEYVALUE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x81)
/// Denoiser description for querying default configuration values.
/// @pre Required to be passed to @c ffxQuery with a non-nullptr denoiser @c FfxContext*.
typedef struct ffxQueryDescDenoiserGetDefaultKeyValue
{
    /// Header description
    /// * @c type: FFX_API_QUERY_DESC_TYPE_DENOISER_GET_DEFAULT_KEYVALUE
    ffxQueryDescHeader header;

    /// Configuration key corresponding to an entry of @c FfxApiConfigureDenoiserKey.
    uint64_t key;

    /// The number of elements to query.
    uint64_t count;

    /// Pointer to an array for storing the queried elements.
    void* data;
} ffxQueryDescDenoiserGetDefaultKeyValue;

#define FFX_API_QUERY_DESC_TYPE_DENOISER_GPU_MEMORY_USAGE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x82)
/// Denoiser description for querying GPU memory usage.
/// @pre Can be passed to @c ffxQuery with a non-nullptr denoiser @c FfxContext* to retrieve the actual memory usage.
/// @pre Can be passed to @c ffxQuery with a nullptr @c FfxContext* to retrieve the expected memory usage.
typedef struct ffxQueryDescDenoiserGetGPUMemoryUsage
{
    /// Header description
    /// * @c type: FFX_API_QUERY_DESC_TYPE_DENOISER_GPU_MEMORY_USAGE
    ffxQueryDescHeader header;

    /// A pointer to the device.
    /// * DX12: pointer to ID3D12Device.
    void* device;

    /// Maximum size that rendering will be performed at.
    struct FfxApiDimensions2D maxRenderSize;

    /// Signal flags corresponding to a combination of values from @c FfxApiDenoiserSignalFlags.
    uint32_t signalFlags;

    /// Signal flags corresponding to a combination of values from @c FfxApiDenoiserSignalFlags
    /// for signals that use checkerboard reconstruction.
    /// Must be a subset of @c signalFlags.
    uint32_t checkerboardSignalFlags;

    /// Create flags corresponding to zero or a combination of values from @c FfxApiCreateContextDenoiserFlags.
    uint32_t flags;

    /// A pointer to a @c FfxApiEffectMemoryUsage structure that will hold the GPU memory usage.
    struct FfxApiEffectMemoryUsage* gpuMemoryUsage;
} ffxQueryDescDenoiserGetGPUMemoryUsage;

//------------------------------------------------------------------------------
// FFX Denoiser Enums
//------------------------------------------------------------------------------

typedef enum FfxApiCreateContextDenoiserFlags
{
    /// Flag indicating that debug features may be enabled. Memory usage may increase.
    FFX_DENOISER_ENABLE_DEBUGGING  = (1 << 0),
    /// Flag indicating that exhaustive validation should be enabled. Performance may decrease.
    FFX_DENOISER_ENABLE_VALIDATION = (1 << 1),
} FfxApiCreateContextDenoiserFlags;

typedef enum FfxApiConfigureDenoiserKey
{
    /// Override the strength of the cross bilateral normal term.
    /// * Format: A single @c float.
    FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH = 1,
    /// Override the bias of the temporal accumulation to be more stable but less responsive.
    /// * Format: A single @c float.
    FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS                  = 2,
    /// Override the maximum radiance value.
    /// * Format: A single @c float.
    FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE                    = 3,
    /// Override the standard deviation K value used for radiance clipping.
    /// * Format: A single @c float.
    FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K             = 4,
    /// Override the Gaussian kernel relaxation factor.
    /// * Format: A single @c float.
    FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION      = 5,
    /// Override the disocclusion threshold used for depth comparisons during temporal reprojection.
    /// * Format: A single @c float.
    FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD          = 6,
    /// Override the absolute linear depth bounds used for normalizing the absolute linear depth debug view.
    /// * Format: A single @c FfxApiFloatBounds.
    FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS  = 7,
} FfxApiConfigureDenoiserKey;

typedef enum FfxApiDenoiserDebugViewMode
{
    /// Overview debug view mode.
    FFX_API_DENOISER_DEBUG_VIEW_MODE_OVERVIEW            = 0,
    /// Fullscreen viewport debug view mode.
    FFX_API_DENOISER_DEBUG_VIEW_MODE_FULLSCREEN_VIEWPORT = 1,
} FfxApiDenoiserDebugViewMode;

typedef enum FfxApiDenoiserSignalFlags
{
    FFX_DENOISER_SIGNAL_NONE                      = 0,
    /// Flag indicating that ambient occlusion needs to be denoised.
    FFX_DENOISER_SIGNAL_AMBIENT_OCCLUSION         = (1 << 0),
    /// Flag indicating that direct diffuse needs to be denoised.
    FFX_DENOISER_SIGNAL_DIRECT_DIFFUSE            = (1 << 1),
    /// Flag indicating that direct specular needs to be denoised.
    FFX_DENOISER_SIGNAL_DIRECT_SPECULAR           = (1 << 2),
    /// Flag indicating that dominant light visibility needs to be denoised.
    FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY = (1 << 3),
    /// Flag indicating that indirect diffuse needs to be denoised.
    FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE          = (1 << 4),
    /// Flag indicating that indirect specular needs to be denoised.
    FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR         = (1 << 5),
    /// Flag indicating that specular occlusion needs to be denoised.
    FFX_DENOISER_SIGNAL_SPECULAR_OCCLUSION        = (1 << 6),
} FfxApiDenoiserSignalFlags;

typedef enum FfxApiDispatchDenoiserFlags
{
    /// Flag indicating that the history accumulation needs to be reset.
    FFX_DENOISER_DISPATCH_RESET              = (1 << 0),
    /// Flag controlling the software encode/decode transfer function applied to the
    /// (diffuse and specular) albedo textures. This is a software convention
    /// agreed between the application and the denoiser; it is independent of any
    /// hardware sRGB texture format.
    ///
    /// When set:   the application is expected to encode the albedo in software with
    ///             the identity function; the denoiser decodes with the identity function.
    /// When clear: the application is expected to encode the albedo in software with
    ///             the sqrt function; the denoiser decodes with the square function.
    FFX_DENOISER_DISPATCH_NON_ENCODED_ALBEDO = (1 << 1),

    /// \deprecated Renamed to @c FFX_DENOISER_DISPATCH_NON_ENCODED_ALBEDO. Kept as an
    /// alias for source compatibility; identical value and behavior.
    FFX_DENOISER_DISPATCH_NON_GAMMA_ALBEDO   = FFX_DENOISER_DISPATCH_NON_ENCODED_ALBEDO,
} FfxApiDispatchDenoiserFlags;

#ifdef __cplusplus
}
#endif // __cplusplus
