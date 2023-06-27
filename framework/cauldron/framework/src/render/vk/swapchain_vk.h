// AMD Cauldron code
//
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#pragma once

#if defined(_VK)

#include "render/swapchain.h"

#include <vulkan/vulkan.h>

namespace cauldron
{
    class SwapChainInternal final : public SwapChain
    {
    public:
        const VkSwapchainKHR VKSwapChain() const { return m_SwapChain; }
        VkSwapchainKHR VKSwapChain() { return m_SwapChain; }
        VkSwapchainKHR* VKSwapChainPtr() { return &m_SwapChain; }

        const VkSurfaceFormatKHR GetVkSurfaceFormat() const { return m_SurfaceFormat; }

        void OnResize(uint32_t width, uint32_t height) override;
        void WaitForSwapChain() override;
        void Present() override;

        void DumpSwapChainToFile(std::experimental::filesystem::path filePath) override;
        void EnumerateDisplayModesAndFormats(const std::vector<VkSurfaceFormat2KHR>& formats2);
        void EnumerateHDRMetadata(VkPhysicalDevice                 PhysicalDevice,
                                  VkPhysicalDeviceSurfaceInfo2KHR& physicalDeviceSurfaceInfo2,
                                  VkSurfaceCapabilities2KHR&       capabilities2);

        void SetHDRMetadataAndColorspace() override;

        SwapChainInternal* GetImpl() override { return this; }
        const SwapChainInternal* GetImpl() const override { return this; }

    private:
        friend class SwapChain;
        SwapChainInternal();
        virtual ~SwapChainInternal();

        void CreateSwapChainRenderTargets() override;

        void CreateSwapChain(uint32_t width, uint32_t height);

    private:
        // Internal members
        VkSwapchainKHR     m_SwapChain    = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_SurfaceFormat;

        uint32_t m_Width  = 0;
        uint32_t m_Height = 0;

        VkFence m_ImageAvailableFence = VK_NULL_HANDLE;
    };

} // namespace cauldron

#endif // #if defined(_VK)
