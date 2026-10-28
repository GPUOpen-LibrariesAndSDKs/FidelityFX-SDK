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

#include "../../api/include/vk/ffx_api_vk.h"
#include "../../api/internal/ffx_backends.h"

#if defined(FFX_FRAMEGENERATION)
#include "../../framegeneration/include/ffx_framegeneration.h"
#include "../../framegeneration/include/vk/ffx_api_framegeneration_vk.h"
#endif // defined(FFX_FRAMEGENERATION)
#if defined(FFX_UPSCALER)
#include "../../upscalers/include/ffx_upscale.h"
#endif // defined(FFX_UPSCALER)
#if defined(FFX_DENOISER)
#include "../../denoisers/include/ffx_denoiser.h"
#endif // defined(FFX_DENOISER)

#include "ffx_vk.h"

ffxReturnCode_t CreateBackend(const ffxCreateContextDescHeader *desc, bool& backendFound, FfxInterface *iface, size_t contexts, Allocator& alloc)
{
    for (const auto* it = desc->pNext; it; it = it->pNext)
    {
        switch (it->type)
        {
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK_DEPRECATED: // Explicit path for FSR3.1.4 VK deprecated desc
        {
            // check for double backend just to make sure.
            //if (backendFound)
            //    return FFX_API_RETURN_ERROR;
            //backendFound = true;

            //const auto *backendDesc = reinterpret_cast<const ffxCreateBackendVKDesc*>(it);
            //FfxDevice device = ffxGetDeviceVK(backendDesc->device);
            //size_t scratchBufferSize = ffxGetScratchMemorySizeVK(contexts);
            //void* scratchBuffer = alloc.alloc(scratchBufferSize);
            //memset(scratchBuffer, 0, scratchBufferSize);
            //TRY2(ffxGetInterfaceVK(iface, device, scratchBuffer, scratchBufferSize, contexts));

            break;
        }
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK:
        {
            // check for double backend just to make sure.
            if (backendFound)
                return FFX_API_RETURN_ERROR;
            backendFound = true;

            const auto *backendDesc = reinterpret_cast<const ffxCreateBackendVKDesc*>(it);
            // A missing device is otherwise dereferenced during backend setup and crashes;
            // fail with a clear error instead, as the DX12 backend does for its device.
            FFX_RETURN_ON_ERROR(backendDesc->deviceContext.vkDevice, FFX_API_RETURN_ERROR_PARAMETER);
            VkDeviceContext deviceContext = backendDesc->deviceContext;
            FfxDevice device = ffxGetDeviceVK(&deviceContext);
            size_t scratchBufferSize = ffxGetScratchMemorySizeVK(backendDesc->deviceContext.vkPhysicalDevice, contexts);
            void* scratchBuffer = alloc.alloc(scratchBufferSize);
            TRY2(ffxGetInterfaceVK(iface, device, scratchBuffer, scratchBufferSize, contexts));

            break;
        }
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK_ALLOCATION_CALLBACKS:
        {
            const auto* allocationCallbacksDesc = reinterpret_cast<const ffxCreateBackendVKAllocationCallbacksDesc*>(it);
			if (allocationCallbacksDesc->pfnFfxResourceAllocator && allocationCallbacksDesc->pfnFfxResourceDeallocator)
			{
                ffxRegisterResourceAllocatorVK(allocationCallbacksDesc->pfnFfxResourceAllocator);
                ffxRegisterResourceDeallocatorVK(allocationCallbacksDesc->pfnFfxResourceDeallocator);
			}
            if (allocationCallbacksDesc->pfnFfxConstantBufferAllocator)
            {
                ffxRegisterConstantBufferAllocatorVK(allocationCallbacksDesc->pfnFfxConstantBufferAllocator);
            }
            break;
        }
        }
    }
    return FFX_API_RETURN_OK;
}

void* GetDevice(const ffxApiHeader* desc)
{
#if defined(FFX_FRAMEGENERATION)
    const ffxCreateContextDescFrameGenerationSwapChainVK* swapchainDesc = nullptr;
    const ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2* gpuMemoryDesc = nullptr;
#endif
    for (const auto* it = desc; it; it = it->pNext)
    {
        switch (it->type)
        {
        case FFX_API_QUERY_DESC_TYPE_GET_VERSIONS:
        {
            return reinterpret_cast<const ffxQueryDescGetVersions*>(it)->device;
        }
#if defined(FFX_UPSCALER)
        case FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE_V2:
        {
            const ffxQueryDescUpscaleGetGPUMemoryUsageV2* queryDesc = reinterpret_cast<const ffxQueryDescUpscaleGetGPUMemoryUsageV2*>(it);
            return queryDesc->device;
        }
#endif //#if defined(FFX_UPSCALER)
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK:
        {
            ffxCreateBackendVKDesc* backendDesc = const_cast<ffxCreateBackendVKDesc*>(reinterpret_cast<const ffxCreateBackendVKDesc*>(it));
            return &backendDesc->deviceContext;
        }
#if defined(FFX_FRAMEGENERATION)
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_FGSWAPCHAIN_VK:
        {
            swapchainDesc = reinterpret_cast<const ffxCreateContextDescFrameGenerationSwapChainVK*>(it);
            break;
        }
        case FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_VK_V2:
        {
            gpuMemoryDesc = reinterpret_cast<const ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2*>(it);
            break;
        }
#endif // defined(FFX_FRAMEGENERATION)
        }
    }
#if defined(FFX_FRAMEGENERATION)
    if (gpuMemoryDesc && gpuMemoryDesc->device && gpuMemoryDesc->physicalDevice)
    {
        // External providers receive a VkDeviceContext*, not a raw VkDevice.
        static thread_local VkDeviceContext gpuMemoryDeviceContext{};
        gpuMemoryDeviceContext = {gpuMemoryDesc->device, gpuMemoryDesc->physicalDevice, vkGetDeviceProcAddr};
        return &gpuMemoryDeviceContext;
    }
    if (swapchainDesc && swapchainDesc->device && swapchainDesc->physicalDevice)
    {
        static thread_local VkDeviceContext swapchainDeviceContext{};
        swapchainDeviceContext = {swapchainDesc->device, swapchainDesc->physicalDevice, vkGetDeviceProcAddr};
        return &swapchainDeviceContext;
    }
#endif
    return nullptr;
}

FfxErrorCode GetResourceSizeFromDescription(FfxDevice device, const FfxCreateResourceDescription* createResourceDescription, uint64_t* sizeInBytes, uint64_t* alignment)
{
    return ffxGetResourceSizeFromDescriptionVK(device, createResourceDescription, sizeInBytes, alignment);
}

FfxErrorCode GetBackendContextGpuMemoryUsage(FfxDevice device, size_t contextCount, uint64_t* byteCount)
{
    return ffxGetBackendContextGpuMemoryUsageVK(device, contextCount, byteCount);
}
