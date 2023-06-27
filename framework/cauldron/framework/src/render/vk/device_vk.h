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

#include "render/device.h"
#include "misc/threadsafe_queue.h"
#include "memoryallocator.h"

#include <queue>
#include <mutex>
#include <vector>

namespace cauldron
{
    struct QueueFamilies
    {
        uint32_t PresentQueueFamilyIndex  = UINT32_MAX;
        uint32_t GraphicsQueueFamilyIndex = UINT32_MAX;
        uint32_t ComputeQueueFamilyIndex  = UINT32_MAX;
        uint32_t CopyQueueFamilyIndex     = UINT32_MAX;

        VkQueueFamilyProperties PresentQueueProperties  = {};
        VkQueueFamilyProperties GraphicsQueueProperties = {};
        VkQueueFamilyProperties ComputeQueueProperties  = {};
        VkQueueFamilyProperties CopyQueueProperties     = {};

        CommandQueue presentQueueType = CommandQueue::Count;
    };

    struct SwapChainCreationParams
    {
        VkSwapchainCreateInfoKHR swapchainCreateInfo;
    };

    class DeviceInternal final : public Device
    {
    public:
        virtual ~DeviceInternal();

        virtual void     GetFeatureInfo(DeviceFeature feature, void* pFeatureInfo) override;
        void FlushQueue(CommandQueue queueType) override;
        virtual uint64_t QueryPerformanceFrequency(CommandQueue queueType) override;

        virtual CommandList* CreateCommandList(const wchar_t* name, CommandQueue queueType) override;

        // For swapchain creation
        virtual void CreateSwapChain(SwapChain*& pSwapChain, const SwapChainCreationParams& params, CommandQueue queueType) override;

        // For swapchain present and signaling (for synchronization)
        virtual uint64_t PresentSwapChain(SwapChain* pSwapChain) override;

        virtual void WaitOnQueue(uint64_t waitValue, CommandQueue queueType) const override;

        virtual uint64_t ExecuteCommandLists(std::vector<CommandList*>& cmdLists, CommandQueue queueType, bool isLastSubmissionOfFrame = false) override;
        virtual void ExecuteCommandListsImmediate(std::vector<CommandList*>& cmdLists, CommandQueue queueType) override;

        virtual void ExecuteResourceTransitionImmediate(uint32_t barrierCount, const Barrier* pBarriers) override;
        virtual void ExecuteTextureResourceCopyImmediate(uint32_t resourceCopyCount, const TextureCopyDesc* pCopyDescs) override;

        // Vulkan only methods
        // used to transition on any queue. Use it only when necessary
        void ExecuteResourceTransitionImmediate(CommandQueue queueType, uint32_t barrierCount, const Barrier* pBarriers);
        VkSemaphore ExecuteCommandListsWithSignalSemaphore(std::vector<CommandList*>& cmdLists, CommandQueue queueType);
        void ExecuteCommandListsImmediate(std::vector<CommandList*>& cmdLists, CommandQueue queueType, VkSemaphore waitSemaphore, CommandQueue waitQueueType);
        uint64_t ExecuteCommandLists(std::vector<CommandList*>& cmdLists, CommandQueue queueType, VkSemaphore waitSemaphore);

        void ReleaseCommandPool(CommandList* pCmdList);

        const VkDevice VKDevice() const { return m_Device; }
        VkDevice VKDevice() { return m_Device; }
        const VkPhysicalDevice VKPhysicalDevice() const { return m_PhysicalDevice; }
        VmaAllocator GetVmaAllocator() const { return m_VmaAllocator; }
        VkSampler GetDefaultSampler() const { return m_DefaultSampler; }

        VkSurfaceKHR GetSurface() { return m_Surface; }

        void SetResourceName(VkObjectType objectType, uint64_t handle, const char* name);
        void SetResourceName(VkObjectType objectType, uint64_t handle, const wchar_t* name);

        QueueFamilies GetQueueFamilies() const { return m_QueueFamilies; }

        const uint32_t GetMinAccelerationStructureScratchOffsetAlignment() { return m_MinAccelerationStructureScratchOffsetAlignment; }

        virtual const DeviceInternal* GetImpl() const override { return this; }
        virtual DeviceInternal* GetImpl() override { return this; }

        // extensions
        PFN_vkCmdSetPrimitiveTopologyEXT    GetCmdSetPrimitiveTopology()         const { return m_vkCmdSetPrimitiveTopologyEXT;   }
        PFN_vkCmdBeginDebugUtilsLabelEXT    GetCmdBeginDebugUtilsLabel()         const { return m_vkCmdBeginDebugUtilsLabelEXT;   }
        PFN_vkCmdEndDebugUtilsLabelEXT      GetCmdEndDebugUtilsLabel()           const { return m_vkCmdEndDebugUtilsLabelEXT;     }
        PFN_vkCmdBeginRenderingKHR          GetCmdBeginRenderingKHR()            const { return m_vkCmdBeginRenderingKHR;         }
        PFN_vkCmdEndRenderingKHR            GetCmdEndRenderingKHR()              const { return m_vkCmdEndRenderingKHR;           }
        PFN_vkCmdSetFragmentShadingRateKHR  GetCmdSetFragmentShadingRateKHR() const { return m_vkCmdSetFragmentShadingRateKHR; }
        PFN_vkGetAccelerationStructureBuildSizesKHR GetAccelerationStructureBuildSizesKHR() const { return m_vkGetAccelerationStructureBuildSizesKHR; }
        PFN_vkCreateAccelerationStructureKHR GetCreateAccelerationStructureKHR() const { return m_vkCreateAccelerationStructureKHR; }
        PFN_vkDestroyAccelerationStructureKHR GetDestroyAccelerationStructureKHR() const { return m_vkDestroyAccelerationStructureKHR; }
        PFN_vkGetAccelerationStructureDeviceAddressKHR GetGetAccelerationStructureDeviceAddressKHR() const{ return m_vkGetAccelerationStructureDeviceAddressKHR; }
        PFN_vkCmdBuildAccelerationStructuresKHR GetCmdBuildAccelerationStructuresKHR() const { return m_vkCmdBuildAccelerationStructuresKHR; }

        PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR GetPhysicalDeviceSurfaceCapabilities2KHR() const { return m_vkGetPhysicalDeviceSurfaceCapabilities2KHR; }
        PFN_vkGetPhysicalDeviceSurfaceFormats2KHR      GetPhysicalDeviceSurfaceFormats2()         const { return m_vkGetPhysicalDeviceSurfaceFormats2KHR; }
        PFN_vkSetHdrMetadataEXT                        GetSetHdrMetadata()                        const { return m_vkSetHdrMetadataEXT; }
        PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR GetPhysicalDeviceFragmentShadingRatesKHR() const { return m_vkGetPhysicalDeviceFragmentShadingRatesKHR; }

    private:
        friend class Device;
        DeviceInternal();

        class QueueSyncPrimitive
        {
        public:
            void Init(Device* pDevice, CommandQueue queueType, uint32_t queueFamilyIndex, uint32_t queueIndex, uint32_t numFramesInFlight, const char* name);
            void Release(VkDevice device);

            VkCommandPool GetCommandPool();
            void ReleaseCommandPool(VkCommandPool commandPool);

            // thread safe
            uint64_t Submit(const std::vector<CommandList*>& cmdLists, const VkSemaphore signalSemaphore, const VkSemaphore waitSemaphore, bool useEndOfFrameSemaphore);
            uint64_t Present(VkSwapchainKHR swapchain, uint32_t imageIndex); // only valid on the present queue
            void Wait(VkDevice device, uint64_t waitValue) const;

            void Flush();

            VkSemaphore GetOwnershipTransferSemaphore();
            void ReleaseOwnershipTransferSemaphore(VkSemaphore semaphore);

        private:
            VkQueue m_Queue = VK_NULL_HANDLE;
            CommandQueue m_QueueType;
            VkSemaphore m_Semaphore;
            uint64_t m_LatestSemaphoreValue = 0;
            uint32_t m_FamilyIndex;

            ThreadSafeQueue<VkCommandPool> m_AvailableCommandPools = {};
            std::vector<VkSemaphore> m_FrameSemaphores = {};

            std::vector<VkSemaphore> m_AvailableOwnershipTransferSemaphores = {};
            std::vector<VkSemaphore> m_UsedOwnershipTransferSemaphores = {};

            std::mutex m_SubmitMutex;
        };
        QueueSyncPrimitive m_QueueSyncPrims[static_cast<int32_t>(CommandQueue::Count)] = {};

        // Internal members
        VkInstance       m_Instance = VK_NULL_HANDLE;
        VkDevice         m_Device = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkSurfaceKHR     m_Surface = VK_NULL_HANDLE;

        QueueFamilies    m_QueueFamilies;

        VmaAllocator     m_VmaAllocator = nullptr;

        // minAccelerationStructureScratchOffsetAlignment
        uint32_t m_MinAccelerationStructureScratchOffsetAlignment;

        // Default objects
        VkSampler m_DefaultSampler = VK_NULL_HANDLE;

        // debug helpers
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

        PFN_vkSetDebugUtilsObjectNameEXT    m_vkSetDebugUtilsObjectNameEXT = nullptr;
        PFN_vkCmdSetPrimitiveTopologyEXT    m_vkCmdSetPrimitiveTopologyEXT = nullptr;
        PFN_vkCmdBeginDebugUtilsLabelEXT    m_vkCmdBeginDebugUtilsLabelEXT = nullptr;
        PFN_vkCmdEndDebugUtilsLabelEXT      m_vkCmdEndDebugUtilsLabelEXT = nullptr;
        PFN_vkCmdBeginRenderingKHR          m_vkCmdBeginRenderingKHR = nullptr;
        PFN_vkCmdEndRenderingKHR            m_vkCmdEndRenderingKHR = nullptr;
        PFN_vkCmdSetFragmentShadingRateKHR  m_vkCmdSetFragmentShadingRateKHR = nullptr;
        PFN_vkGetAccelerationStructureBuildSizesKHR m_vkGetAccelerationStructureBuildSizesKHR = nullptr;
        PFN_vkCreateAccelerationStructureKHR        m_vkCreateAccelerationStructureKHR = nullptr;
        PFN_vkDestroyAccelerationStructureKHR       m_vkDestroyAccelerationStructureKHR = nullptr;
        PFN_vkGetAccelerationStructureDeviceAddressKHR m_vkGetAccelerationStructureDeviceAddressKHR = nullptr;
        PFN_vkCmdBuildAccelerationStructuresKHR     m_vkCmdBuildAccelerationStructuresKHR = nullptr;

        // hdr helpers
        PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR m_vkGetPhysicalDeviceSurfaceCapabilities2KHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceFormats2KHR      m_vkGetPhysicalDeviceSurfaceFormats2KHR = nullptr;
        PFN_vkSetHdrMetadataEXT                        m_vkSetHdrMetadataEXT = nullptr;
        PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR m_vkGetPhysicalDeviceFragmentShadingRatesKHR = nullptr;
    };

} // namespace cauldron

#endif // #if defined(_VK)
