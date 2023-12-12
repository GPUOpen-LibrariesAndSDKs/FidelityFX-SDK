// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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


/// @defgroup CauldronBackend Cauldron Backend
/// Custom FidelityFX SDK Backend Implementation for Cauldron
/// 
/// @ingroup SDKSample

#pragma once

#include <FidelityFX/host/ffx_interface.h>

#if defined(__cplusplus)
extern "C" {
#endif  // #if defined(__cplusplus)

namespace cauldron
{
    class Device;
    enum class CommandQueue;
    class CommandList;
    class SwapChain;
    class GPUResource;
}  // namespace cauldron

/// Query how much memory is required for the Cauldron backend's scratch buffer.
///
/// @param [in] maxContexts              The maximum number of simultaneous effect contexts that will share the backend.
///                                      (Note that some effects contain internal contexts which count towards this maximum)
/// 
/// @returns
/// The size (in bytes) of the required scratch memory buffer for the Cauldron backend.
/// 
/// @ingroup CauldronBackend
FFX_API size_t ffxGetScratchMemorySizeCauldron(size_t maxContexts);

/// Create a <c><i>FfxInterface</i></c> from a <c><i>Device</i></c>.
///
/// @param [in] device                      A pointer to the Cauldron device.
///
/// @returns
/// An abstract FidelityFX device.
///
/// @ingroup CauldronBackend
FFX_API FfxDevice ffxGetDeviceCauldron(cauldron::Device* cauldronDevice);

FFX_API FfxCommandQueue ffxGetCommandQueueCauldron(cauldron::CommandQueue cmdQueue);

    /// Populate an interface with pointers for the Cauldron backend.
///
/// @param [out] fsr2Interface              A pointer to a <c><i>FfxInterface</i></c> structure to populate with pointers.
/// @param [in] device                      A pointer to the Cauldron device.
/// @param [in] scratchBuffer               A pointer to a buffer of memory which can be used by the Cauldron backend.
/// @param [in] scratchBufferSize           The size (in bytes) of the buffer pointed to by <c><i>scratchBuffer</i></c>.
/// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
///                                         (Note that some effects contain internal contexts which count towards this maximum)
///
/// @retval
/// FFX_OK                                  The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_INVALID_POINTER          The <c><i>interface</i></c> pointer was <c><i>NULL</i></c>.
///
/// @ingroup CauldronBackend
FFX_API FfxErrorCode ffxGetInterfaceCauldron(FfxInterface* backendInterface, FfxDevice device, void* scratchBuffer, size_t scratchBufferSize, size_t maxContexts);

/// Create a <c><i>FfxCommandList</i></c> from a <c><i>CommandList</i></c>.
///
/// @param [in] cmdList                     A pointer to the Cauldron command list.
///
/// @returns
/// An abstract FidelityFX command list.
///
/// @ingroup CauldronBackend
FFX_API FfxCommandList ffxGetCommandListCauldron(cauldron::CommandList* cauldronCmdList);

FFX_API FfxSwapchain ffxGetSwapchainCauldron(cauldron::SwapChain* pSwapchain);

/// Fetch a <c><i>FfxResource</i></c> from a <c><i>GPUResource</i></c>.
///
/// @param [in] cauldronResource            A pointer to the Cauldron resource.
/// @param [in] ffxDescription              An <c><i>FfxResourceDescription</i></c> for the resource representation.
/// @param [in] name                        (optional) A name string to identify the resource in debug mode.
/// @param [in] state                       The state the resource is currently in.
///
/// @returns
/// An abstract FidelityFX resources.
///
/// @ingroup CauldronBackend
FFX_API FfxResource ffxGetResourceCauldron(const cauldron::GPUResource* cauldronResource,
                                           FfxResourceDescription       ffxResDescription,
                                           wchar_t*                     ffxResName,
                                           FfxResourceStates            state = FFX_RESOURCE_STATE_COMPUTE_READ);

#if defined(__cplusplus)
}
#endif  // #if defined(__cplusplus)
