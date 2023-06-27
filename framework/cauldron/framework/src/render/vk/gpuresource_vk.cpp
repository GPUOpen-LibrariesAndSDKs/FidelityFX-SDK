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

#if defined(_VK)

#include "render/vk/gpuresource_vk.h"   // This needs to come first or we get a linker error (that is very weird)
#include "render/vk/device_vk.h"

#include "core/framework.h"
#include "misc/assert.h"
#include "helpers.h"

namespace cauldron
{
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        DeviceInternal* pDevice = GetDevice()->GetImpl();

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(pDevice->VKPhysicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        CauldronWarning(L"failed to find suitable memory type!");
        return -1;
    }

    GPUResource* GPUResource::CreateGPUResource(const wchar_t* resourceName, void* pOwner, ResourceState initialState, void* pInitParams, bool resizable /* = false */)
    {
        GPUResourceInitParams* pParams = (GPUResourceInitParams*)pInitParams;
        switch (pParams->type)
        {
        case GPUResourceType::Texture:
            return new GPUResourceInternal(pParams->imageInfo, initialState, resourceName, pOwner, resizable);
        case GPUResourceType::Buffer:
            return new GPUResourceInternal(pParams->bufferInfo, pParams->memoryUsage, resourceName, pOwner,
                                            initialState, resizable, pParams->alignment);
        case GPUResourceType::Swapchain:
            return new GPUResourceInternal(pParams->image, pParams->imageInfo, resourceName, initialState, resizable);
        default:
            CauldronCritical(L"Unsupported GPUResourceType creation requested");
            break;
        }

        return nullptr;
    }

    GPUResourceInternal::GPUResourceInternal(VkImageCreateInfo info, ResourceState initialState, const wchar_t* resourceName, void* pOwner, bool resizable) :
        GPUResource(resourceName, pOwner, g_UndefinedState, resizable),
        m_ImageCreateInfo(info)
    {
        m_Type = ResourceType::Image;
        m_OwnerType = OwnerType::Texture;
        CreateImage(initialState);

        // Setup sub-resource states
        InitSubResourceCount(m_ImageCreateInfo.arrayLayers * m_ImageCreateInfo.mipLevels);
    }

    GPUResourceInternal::GPUResourceInternal(VkImage image, VkImageCreateInfo info, const wchar_t* resourceName, ResourceState initialState, bool resizable) :
        GPUResource(resourceName, nullptr, initialState, resizable),
        m_ImageCreateInfo(info)
    {
        m_Type = ResourceType::Image;
        m_Image = image;
        m_External = true;

        // Setup sub-resource states
        InitSubResourceCount(m_ImageCreateInfo.arrayLayers * m_ImageCreateInfo.mipLevels);
    }

    GPUResourceInternal::GPUResourceInternal(VkBufferCreateInfo info,
                             VmaMemoryUsage     memoryUsage,
                             const wchar_t*     resourceName,
                             void*              pOwner,
                             ResourceState      initialState,
                             bool               resizable /*=false*/,
                             size_t             alignment /*=0*/) : 
        GPUResource(resourceName, pOwner, initialState, resizable),
        m_BufferCreateInfo(info),
        m_MemoryUsage(memoryUsage)
    {
        m_Type = ResourceType::Buffer;
        if (m_pOwner)
        {
            if (memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU)
                m_OwnerType = OwnerType::Memory;
            else
                m_OwnerType = OwnerType::Buffer;
        }

        CreateBuffer(alignment);
    }

    GPUResourceInternal::~GPUResourceInternal()
    {
        ClearResource();
    }

    void GPUResourceInternal::SetAllocationName()
    {
        std::string name = WStringToString(m_Name);
        DeviceInternal* pDevice = GetDevice()->GetImpl();
        vmaSetAllocationName(pDevice->GetVmaAllocator(), m_Allocation, name.c_str());
    }

    void GPUResourceInternal::SetOwner(void* pOwner)
    {
        m_pOwner = pOwner;

        // What type of resource is this?
        if (m_pOwner)
        {
            if (m_Type == ResourceType::Buffer)
                m_OwnerType = OwnerType::Buffer;

            else if (m_Type == ResourceType::Image)
                m_OwnerType = OwnerType::Texture;

            else
                m_OwnerType = OwnerType::Memory;
        }
    }

    void GPUResourceInternal::ClearResource()
    {
        DeviceInternal* pDevice = GetDevice()->GetImpl();
        if (!m_External)
        {
            switch (m_Type)
            {
            case ResourceType::Image:
                vmaDestroyImage(pDevice->GetVmaAllocator(), m_Image, m_Allocation);
                m_Image = VK_NULL_HANDLE;
                m_Allocation = VK_NULL_HANDLE;
                break;
            case ResourceType::Buffer:
                vmaDestroyBuffer(pDevice->GetVmaAllocator(), m_Buffer, m_Allocation);
                m_Buffer = VK_NULL_HANDLE;
                m_Allocation = VK_NULL_HANDLE;
                break;
            default:
                break;
            }
        }
    }

    void GPUResourceInternal::RecreateResource(VkImageCreateInfo info, ResourceState initialState)
    {
        CauldronAssert(ASSERT_CRITICAL, m_Type == ResourceType::Image, L"Cannot recreate non-image resource");
        CauldronAssert(ASSERT_CRITICAL, m_Resizable, L"Cannot recreate non-resizable resource");
        m_ImageCreateInfo = info;
        m_CurrentStates.front() = g_UndefinedState;
        InitSubResourceCount(m_ImageCreateInfo.arrayLayers * m_ImageCreateInfo.mipLevels);
        CreateImage(initialState);
    }

    void GPUResourceInternal::RecreateResource(VkBufferCreateInfo info, ResourceState initialState)
    {
        CauldronAssert(ASSERT_CRITICAL, m_Type == ResourceType::Buffer, L"Cannot recreate non-buffer resource");
        CauldronAssert(ASSERT_CRITICAL, m_Resizable, L"Cannot recreate non-resizable resource");
        m_BufferCreateInfo = info;
        m_CurrentStates.front() = initialState; // buffer have no state, we can safely set this state internally
        InitSubResourceCount(1);
        CreateBuffer();
    }

    VkImage GPUResourceInternal::GetImage() const
    {
        CauldronAssert(ASSERT_ERROR, m_Type == ResourceType::Image, L"GPUResource type isn't Image");
        return m_Image;
    }

    VkBuffer GPUResourceInternal::GetBuffer() const
    {
        CauldronAssert(ASSERT_ERROR, m_Type == ResourceType::Buffer, L"GPUResource type isn't Buffer");
        return m_Buffer;
    }

    VkDeviceAddress GPUResourceInternal::GetDeviceAddress() const
    {
        CauldronAssert(ASSERT_ERROR, m_Type == ResourceType::Buffer, L"GPUResource type isn't Buffer");
        return m_DeviceAddress;
    }

    VkImageCreateInfo GPUResourceInternal::GetImageCreateInfo() const
    {
        CauldronAssert(ASSERT_ERROR, m_Type == ResourceType::Image, L"GPUResource type isn't Image");
        return m_ImageCreateInfo;
    }

    VkBufferCreateInfo GPUResourceInternal::GetBufferCreateInfo() const
    {
        CauldronAssert(ASSERT_ERROR, m_Type == ResourceType::Buffer, L"GPUResource type isn't Buffer");
        return m_BufferCreateInfo;
    }

    void GPUResourceInternal::CreateImage(ResourceState initialState)
    {
        CauldronAssert(ASSERT_ERROR, m_Type == ResourceType::Image, L"GPUResource type isn't Image");
        ClearResource();

        DeviceInternal* pDevice = GetDevice()->GetImpl();

        // adjust the image creation structure if mutable views are allowed
        std::array<VkFormat, 2> formats;
        VkImageFormatListCreateInfo imageFormatInfo = {};
        if ((m_ImageCreateInfo.flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) != 0)
        {
            formats[0] = m_ImageCreateInfo.format;
            formats[1] = VKToGamma(m_ImageCreateInfo.format);
            imageFormatInfo.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
            imageFormatInfo.pNext = nullptr;
            imageFormatInfo.viewFormatCount = static_cast<uint32_t>(formats.size());
            imageFormatInfo.pViewFormats = formats.data();
            m_ImageCreateInfo.pNext = &imageFormatInfo;

            // Add some asserts to be sure that we are handling the case of a image with sRGB view on it
            CauldronAssert(ASSERT_CRITICAL, formats[0] != formats[1], L"Image is already a sRGB one, this shouldn't happen if the VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT is set.");
            CauldronAssert(ASSERT_CRITICAL, (m_ImageCreateInfo.usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0, L"VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT should ony be set to handle storage views on a texture.");
        }

        m_MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = m_MemoryUsage;
        VkResult res = vmaCreateImage(pDevice->GetVmaAllocator(), &m_ImageCreateInfo, &allocInfo, &m_Image, &m_Allocation, nullptr);
        CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS && m_Image != VK_NULL_HANDLE, L"Failed to create an image");

        // reset the pointer to the format info structure
        m_ImageCreateInfo.pNext = nullptr;

        pDevice->SetResourceName(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_Image, m_Name.c_str());
        SetAllocationName();

        // In vulkan, after creation the image is in an undefined state. Transition it to the desired state
        if (initialState != g_UndefinedState)
        {
            switch (initialState)
            {
            case ResourceState::PixelShaderResource:
            case ResourceState::NonPixelShaderResource:
            case ResourceState::ShaderResource:
            case ResourceState::RenderTargetResource:
            case ResourceState::DepthRead:
            {
                Barrier barrier;
                barrier.Type = BarrierType::Transition;
                barrier.pResource = this;
                barrier.SourceState = g_UndefinedState;
                barrier.DestState = initialState;
                pDevice->ExecuteResourceTransitionImmediate(CommandQueue::Graphics, 1, &barrier);
                break;
            }
            case ResourceState::UnorderedAccess:
            {
                // we need to transition to our final state. This should be done on the graphics queue.
                // only render targets should call for an inital state like those ones.
                CauldronAssert(ASSERT_CRITICAL,
                               (m_ImageCreateInfo.usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0,
                               L"A non-UAV texture has an unexpected initial state. Please review and implement if necessary");
                Barrier barrier;
                barrier.Type        = BarrierType::Transition;
                barrier.pResource   = this;
                barrier.SourceState = g_UndefinedState;
                barrier.DestState   = initialState;
                pDevice->ExecuteResourceTransitionImmediate(CommandQueue::Graphics, 1, &barrier);
                break;
            }
            case ResourceState::CopyDest:
            {
                // transition on the copy queue as it will be used to load data
                Barrier barrier;
                barrier.Type = BarrierType::Transition;
                barrier.pResource = this;
                barrier.SourceState = g_UndefinedState;
                barrier.DestState = initialState;
                pDevice->ExecuteResourceTransitionImmediate(CommandQueue::Copy, 1, &barrier);
                break;
            }
            case ResourceState::ShadingRateSource:
            {
                // transition on the graphics queue as it will be used to load data
                CauldronAssert(ASSERT_CRITICAL,
                               (m_ImageCreateInfo.usage & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR) != 0,
                               L"Cannot transition to initial state ShadingRateSource because the texture doesn't support this usage.");
                Barrier barrier;
                barrier.Type        = BarrierType::Transition;
                barrier.pResource   = this;
                barrier.SourceState = g_UndefinedState;
                barrier.DestState   = initialState;
                pDevice->ExecuteResourceTransitionImmediate(CommandQueue::Graphics, 1, &barrier);
                break;
            }
            default:
                CauldronCritical(L"Unsupported initial resource state (%d). Please implement the correct transition.", static_cast<uint32_t>(initialState));
                break;
            }
        }
    }

    void GPUResourceInternal::CreateBuffer(size_t alignment /*=0*/)
    {
        ClearResource();

        DeviceInternal* pDevice = GetDevice()->GetImpl();

        m_Buffer = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = m_MemoryUsage;

        // Create with alignment
        VkResult res;
        if (alignment != 0)
        {
            res = vmaCreateBufferWithAlignment(pDevice->GetVmaAllocator(), &m_BufferCreateInfo, &allocInfo, (VkDeviceSize)alignment, &m_Buffer, &m_Allocation, nullptr);
        }
        else
        {
            res = vmaCreateBuffer(pDevice->GetVmaAllocator(), &m_BufferCreateInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr);
        }
        CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS && m_Buffer != VK_NULL_HANDLE, L"Failed to create a buffers");

        pDevice->SetResourceName(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_Buffer, m_Name.c_str());
        SetAllocationName();

        if ((m_BufferCreateInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0)
        {
            VkBufferDeviceAddressInfo bufferAddressInfo = {};
            bufferAddressInfo.sType                     = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bufferAddressInfo.pNext                     = nullptr;
            bufferAddressInfo.buffer                    = m_Buffer;
            m_DeviceAddress                             = vkGetBufferDeviceAddress(pDevice->VKDevice(), &bufferAddressInfo);
        }

    }
} // namespace cauldron

#endif // #if defined(_VK)
