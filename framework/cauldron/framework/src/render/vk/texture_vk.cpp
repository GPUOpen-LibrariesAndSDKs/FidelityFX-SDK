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

#include "core/framework.h"
#include "core/loaders/textureloader.h"
#include "misc/assert.h"

#include "render/vk/commandlist_vk.h"
#include "render/vk/device_vk.h"
#include "render/vk/gpuresource_vk.h"
#include "render/vk/texture_vk.h"
#include "render/vk/uploadheap_vk.h"

#include "helpers.h"

namespace cauldron
{
    //////////////////////////////////////////////////////////////////////////
    // Helpers

    bool IsBCFormat(ResourceFormat format)
    {
        switch (format)
        {
        case ResourceFormat::BC1_UNORM:
        case ResourceFormat::BC1_SRGB:
        case ResourceFormat::BC2_UNORM:
        case ResourceFormat::BC2_SRGB:
        case ResourceFormat::BC3_UNORM:
        case ResourceFormat::BC3_SRGB:
        case ResourceFormat::BC4_UNORM:
        case ResourceFormat::BC4_SNORM:
        case ResourceFormat::BC5_UNORM:
        case ResourceFormat::BC5_SNORM:
        case ResourceFormat::BC6_UNSIGNED:
        case ResourceFormat::BC6_SIGNED:
        case ResourceFormat::BC7_UNORM:
        case ResourceFormat::BC7_SRGB:
            return true;

        default:
            return false;
        };
    }

    VkImageType GetImageType(const TextureDesc& desc)
    {
        switch (desc.Dimension)
        {
        case TextureDimension::Texture1D:
            return VK_IMAGE_TYPE_1D;
        case TextureDimension::Texture2D:
        case TextureDimension::CubeMap:
            return VK_IMAGE_TYPE_2D;
        case TextureDimension::Texture3D:
            return VK_IMAGE_TYPE_3D;
        default:
            CauldronError(L"Incorrect texture dimension");
            return VK_IMAGE_TYPE_MAX_ENUM;
        }
    }

    VkImageUsageFlags GetUsage(ResourceFlags cauldronFlags)
    {
        uint32_t flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (static_cast<bool>(cauldronFlags & ResourceFlags::AllowRenderTarget))
        {
            // add color attachment flag
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            // keep transfer dest flag to clear
        }
        if (static_cast<bool>(cauldronFlags & ResourceFlags::AllowDepthStencil))
        {
            // add depth stencil attachment flag
            flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            // keep transfer dest flag to clear
        }
        if (static_cast<bool>(cauldronFlags & ResourceFlags::AllowUnorderedAccess))
        {
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (static_cast<bool>(cauldronFlags & ResourceFlags::DenyShaderResource))
        {
            flags &= ~(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        }
        if (static_cast<bool>(cauldronFlags & ResourceFlags::AllowShadingRate))
        {
            flags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
        }

        return static_cast<VkImageUsageFlags>(flags);
    }

    struct MipInformation
    {
        VkDeviceSize TotalSize;
        VkDeviceSize Stride;
        uint32_t Rows;
    };

    MipInformation GetMipInformation(uint32_t width, uint32_t height, ResourceFormat format)
    {
        MipInformation info;
        VkDeviceSize blockSize = GetBlockSize(format);
        VkDeviceSize lineSize = 0;
        VkDeviceSize totalSize = 0;
        if (IsBCFormat(format))
        {
            info.Stride = blockSize * DivideRoundingUp(width, 4);
            info.Rows = DivideRoundingUp(height, 4);
            info.TotalSize = info.Stride * info.Rows;
        }
        else
        {
            info.Stride = blockSize * width;
            info.Rows = height;
            info.TotalSize = info.Stride * info.Rows;
        }
        return info;
    }

    VkDeviceSize GetTotalTextureSize(uint32_t width, uint32_t height, ResourceFormat format, uint32_t mipCount)
    {
        VkDeviceSize totalSize = 0;
        for (uint32_t i = 0; i < mipCount; ++i)
        {
            totalSize += GetMipInformation(width, height, format).TotalSize;
            width /= 2;
            height /= 2;
        }
        return totalSize;
    }

    uint32_t CalculateMipLevels(const TextureDesc& desc)
    {
        if (desc.MipLevels == 0)
        {
            if (desc.Dimension == TextureDimension::Texture1D)
            {
                uint32_t w        = desc.Width;
                uint32_t mipCount = 1;
                while (w > 1)
                {
                    w >>= 1;
                    ++mipCount;
                }
                return mipCount;
            }
            else if (desc.Dimension == TextureDimension::Texture2D || desc.Dimension == TextureDimension::CubeMap)
            {
                uint32_t w        = desc.Width;
                uint32_t h        = desc.Height;
                uint32_t mipCount = 1;
                while (w > 1 || h > 1)
                {
                    w >>= 1;
                    h >>= 1;
                    ++mipCount;
                }
                return mipCount;
            }
            else if (desc.Dimension == TextureDimension::Texture3D)
            {
                uint32_t w        = desc.Width;
                uint32_t h        = desc.Height;
                uint32_t d        = desc.DepthOrArraySize;
                uint32_t mipCount = 1;
                while (w > 1 || h > 1 || d > 1)
                {
                    w >>= 1;
                    h >>= 1;
                    d >>= 1;
                    ++mipCount;
                }
                return mipCount;
            }
            else
            {
                CauldronCritical(L"Cannot calculate mip count for unknown texture dimension");
                return 0;
            }
        }
        else
        {
            return desc.MipLevels;
        }
    }

    VkImageCreateInfo ConvertTextureDesc(const TextureDesc& desc)
    {
        // TODO: cube textures
        VkImageType imageType = GetImageType(desc);

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType         = imageType;
        imageInfo.extent.width      = desc.Width;
        imageInfo.extent.height     = desc.Height;
        imageInfo.extent.depth      = imageType == VK_IMAGE_TYPE_3D ? desc.DepthOrArraySize : 1;
        imageInfo.mipLevels         = CalculateMipLevels(desc);
        imageInfo.arrayLayers       = imageType == VK_IMAGE_TYPE_3D ? 1 : desc.DepthOrArraySize;
        imageInfo.format            = GetVkFormat(desc.Format);
        imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage             = GetUsage(desc.Flags);
        imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags             = 0;

        if (desc.Dimension == TextureDimension::CubeMap)
        {
            CauldronAssert(ASSERT_CRITICAL, desc.DepthOrArraySize % 6 == 0, L"The number of slices of the cubemap texture isn't a mutiple of 6");
            imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        // if a sRGB texture will have a UAV view on it, we need to:
        //  - use a non sRGB format for it
        //  - use this format for UAV/storage view
        //  - use sRGB view for SRV and RTV
        if (static_cast<bool>(desc.Flags & ResourceFlags::AllowUnorderedAccess) && IsSRGB(desc.Format))
        {
            imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            imageInfo.format = VKFromGamma(imageInfo.format);
        }

        return imageInfo;
    }
    
    //////////////////////////////////////////////////////////////////////////
    // Texture
    
    Texture::Texture(const TextureDesc* pDesc, ResourceState initialState, ResizeFunction fn) :
        m_TextureDesc(*pDesc),
        m_ResizeFn(fn)
    {
        VkImageCreateInfo imageInfo = ConvertTextureDesc(*pDesc);

        GPUResourceInitParams initParams = {};
        initParams.imageInfo = imageInfo;
        initParams.type = GPUResourceType::Texture;

        m_pResource = GPUResource::CreateGPUResource(pDesc->Name.c_str(), this, initialState, &initParams, fn != nullptr);
        CauldronAssert(ASSERT_ERROR, m_pResource != nullptr, L"Could not create GPU resource for texture %ls", pDesc->Name.c_str());
    }

    Texture::Texture(const TextureDesc* pDesc, GPUResource* pResource) :
        m_TextureDesc(*pDesc),
        m_pResource(pResource),
        m_ResizeFn(nullptr)
    {
    }

    void Texture::CopyData(TextureDataBlock* pTextureDataBlock)
    {
        VkDeviceSize totalSize = GetTotalTextureSize(m_TextureDesc.Width, m_TextureDesc.Height, m_TextureDesc.Format, m_TextureDesc.MipLevels);
        UploadHeap* pUploadHeap = GetUploadHeap();
        DeviceInternal* pDevice = GetDevice()->GetImpl();

        // NOTES
        // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-queue-transfers
        // We assume that all resources end up on the graphics queue. Copying data on vulkan requires some extra steps.
        //   - There is no need to perform a queue ownership transfer for the target texture since we don't care about its previous content.
        //   - Staging buffer is only accessed on the copy queue, so there is also no need to transfer queue ownership.
        //   - We assume that the texture is in a CopyDest state
        //   - We record all the copy commands on the copy queue.
        //   - If the copy queue and the graphics queue aren't the same family, we need to transfer the ownership of the texture from the copy queue to the graphics queue.
        //     This is done by issuing a image memory barrier on the copy queue then on the graphics queue, ensuring that the second barrier occurs after the first one.

        CommandList* pImmediateCopyCmdList = pDevice->CreateCommandList(L"ImmediateCopyCommandList", CommandQueue::Copy);
        
        // Get what we need to transfer data
        TransferInfo* pTransferInfo = pUploadHeap->BeginResourceTransfer(totalSize, 512, m_TextureDesc.DepthOrArraySize);

        uint32_t readOffset = 0;
        for (uint32_t a = 0; a < m_TextureDesc.DepthOrArraySize; ++a)
        {
            // Get the pointer for the entries in this slice (depth slice or array entry)
            uint8_t* pPixels = pTransferInfo->DataPtr(a);

            uint32_t width = m_TextureDesc.Width;
            uint32_t height = m_TextureDesc.Height;
            uint32_t offset = 0;

            // Copy all the mip slices into the offsets specified by the footprint structure
            for (uint32_t mip = 0; mip < m_TextureDesc.MipLevels; ++mip)
            {
                MipInformation info = GetMipInformation(width, height, m_TextureDesc.Format);

                pTextureDataBlock->CopyTextureData(pPixels + offset, static_cast<uint32_t>(info.Stride), static_cast<uint32_t>(info.Stride), info.Rows, readOffset);
                readOffset += static_cast<uint32_t>(info.Stride * info.Rows);
                
                VkDeviceSize bufferOffset = static_cast<VkDeviceSize>(offset) + static_cast<VkDeviceSize>(pPixels - pUploadHeap->BasePtr());

                TextureCopyDesc copyDesc;
                copyDesc.GetImpl()->IsSourceTexture = false;
                copyDesc.GetImpl()->DstImage = m_pResource->GetImpl()->GetImage();
                copyDesc.GetImpl()->SrcBuffer = pUploadHeap->GetResource()->GetImpl()->GetBuffer();
                copyDesc.GetImpl()->Region.bufferOffset = bufferOffset;
                copyDesc.GetImpl()->Region.bufferRowLength = 0; // tightly packed
                copyDesc.GetImpl()->Region.bufferImageHeight = 0; // tightly packed
                copyDesc.GetImpl()->Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyDesc.GetImpl()->Region.imageSubresource.mipLevel = mip;
                copyDesc.GetImpl()->Region.imageSubresource.baseArrayLayer = a;
                copyDesc.GetImpl()->Region.imageSubresource.layerCount = 1;
                copyDesc.GetImpl()->Region.imageOffset = { 0, 0, 0 };
                copyDesc.GetImpl()->Region.imageExtent = { width, height, 1 };

                copyDesc.GetImpl()->pDestResource = m_pResource;

                // record copy command
                CopyTextureRegion(pImmediateCopyCmdList, &copyDesc);
                
                offset += static_cast<uint32_t>(info.TotalSize);
                if (width > 1)
                    width >>= 1;
                if (height > 1)
                    height >>= 1;
            }
        }

        QueueFamilies families = pDevice->GetQueueFamilies();
        bool needsQueueOwnershipTransfer = (families.GraphicsQueueFamilyIndex != families.CopyQueueFamilyIndex);
        VkImageMemoryBarrier imageMemoryBarrier = {};
        if (needsQueueOwnershipTransfer)
        {
            // release ownership from the copy queue
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = nullptr;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcQueueFamilyIndex = families.CopyQueueFamilyIndex;
            imageMemoryBarrier.dstQueueFamilyIndex = families.GraphicsQueueFamilyIndex;
            imageMemoryBarrier.image = m_pResource->GetImpl()->GetImage();
            imageMemoryBarrier.subresourceRange.aspectMask = GetImageAspectMask(m_pResource->GetImpl()->GetImageCreateInfo().format);
            imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
            imageMemoryBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            vkCmdPipelineBarrier(pImmediateCopyCmdList->GetImpl()->VKCmdBuffer(),
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                0, nullptr, 0, nullptr,
                1, &imageMemoryBarrier);
        }
        
        CloseCmdList(pImmediateCopyCmdList);

        std::vector<CommandList*> cmdLists;
        cmdLists.reserve(1);
        cmdLists.push_back(pImmediateCopyCmdList);
        // Copy all immediate
        pDevice->ExecuteCommandListsImmediate(cmdLists, CommandQueue::Copy);

        delete pImmediateCopyCmdList;

        // Kick off the resource transfer. When we get back from here the resource is ready to be used.
        pUploadHeap->EndResourceTransfer(pTransferInfo);

        if (needsQueueOwnershipTransfer)
        {
            // acquire ownership on the graphics queue
            CommandList* pImmediateGraphicsCmdList = pDevice->CreateCommandList(L"ImmediateGraphicsCommandList", CommandQueue::Graphics);

            vkCmdPipelineBarrier(pImmediateGraphicsCmdList->GetImpl()->VKCmdBuffer(),
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr, 0, nullptr,
                1, &imageMemoryBarrier);

            CloseCmdList(pImmediateGraphicsCmdList);

            cmdLists.clear();
            cmdLists.push_back(pImmediateGraphicsCmdList);
            pDevice->ExecuteCommandListsImmediate(cmdLists, CommandQueue::Graphics);

            delete pImmediateGraphicsCmdList;

            // TODO: a possible improvement would be to not wait that the copy finish before issuing the command on the graphics list.
            // If we can pass the wait semaphore to the ExecuteCommandListsImmediate, we only need to call ExecuteCommand on the copy queue.
            // In that case we can only delete the CommandLists and the pTransferInfo at the end of this method.
        }
    }

    void Texture::Recreate()
    {
        VkImageCreateInfo imageInfo = ConvertTextureDesc(m_TextureDesc);

        // recreate the resource
        m_pResource->GetImpl()->RecreateResource(imageInfo, m_pResource->GetImpl()->GetCurrentResourceState());
    }

    //////////////////////////////////////////////////////////////////////////
    // TextureCopyDesc

    TextureCopyDesc::TextureCopyDesc(const GPUResource* pSrc, const GPUResource* pDst)
    {
        const GPUResourceInternal* pSrcResource = pSrc->GetImpl();
        const GPUResourceInternal* pDstResource = pDst->GetImpl();

        CauldronAssert(ASSERT_CRITICAL, pDstResource->GetResourceType() == ResourceType::Image, L"Destination should be an image.");

        if (pSrcResource->GetResourceType() == ResourceType::Image)
        {
            GetImpl()->IsSourceTexture = true;
            GetImpl()->SrcImage = pSrcResource->GetImage();
            GetImpl()->DstImage = pDstResource->GetImage();
            GetImpl()->pDestResource = const_cast<GPUResource*>(pDst);

            GetImpl()->ImageCopy = {};
            GetImpl()->ImageCopy.srcSubresource.aspectMask = GetImageAspectMask(pSrcResource->GetImageCreateInfo().format);
            GetImpl()->ImageCopy.srcSubresource.baseArrayLayer = 0;
            GetImpl()->ImageCopy.srcSubresource.layerCount = 1;
            GetImpl()->ImageCopy.srcSubresource.mipLevel = 0;
            GetImpl()->ImageCopy.srcOffset.x = 0;
            GetImpl()->ImageCopy.srcOffset.y = 0;
            GetImpl()->ImageCopy.srcOffset.z = 0;
            GetImpl()->ImageCopy.dstSubresource.aspectMask = GetImageAspectMask(pDstResource->GetImageCreateInfo().format);
            GetImpl()->ImageCopy.dstSubresource.baseArrayLayer = 0;
            GetImpl()->ImageCopy.dstSubresource.layerCount = 1;
            GetImpl()->ImageCopy.dstSubresource.mipLevel = 0;
            GetImpl()->ImageCopy.dstOffset.x = 0;
            GetImpl()->ImageCopy.dstOffset.y = 0;
            GetImpl()->ImageCopy.dstOffset.z = 0;

            VkExtent3D srcExtent = pSrcResource->GetImageCreateInfo().extent;
            GetImpl()->ImageCopy.extent = pDstResource->GetImageCreateInfo().extent;
            GetImpl()->ImageCopy.extent.width = std::min(GetImpl()->ImageCopy.extent.width, srcExtent.width);
            GetImpl()->ImageCopy.extent.height = std::min(GetImpl()->ImageCopy.extent.height, srcExtent.height);
            GetImpl()->ImageCopy.extent.depth = std::min(GetImpl()->ImageCopy.extent.depth, srcExtent.depth);
        }
        else if (pSrcResource->GetResourceType() == ResourceType::Buffer)
        {
            GetImpl()->IsSourceTexture = false;
            GetImpl()->SrcBuffer = pSrcResource->GetBuffer();
            GetImpl()->DstImage = pDstResource->GetImage();
            GetImpl()->pDestResource = const_cast<GPUResource*>(pDst);

            GetImpl()->Region = {};
            GetImpl()->Region.bufferOffset = 0;
            GetImpl()->Region.bufferRowLength = 0; // tightly packed
            GetImpl()->Region.bufferImageHeight = 0; // tightly packed
            // we are assuming we are copying the whole texture
            // Copying a depth/stencil image will result in a validation error but we won't do that
            GetImpl()->Region.imageSubresource.aspectMask = GetImageAspectMask(pDstResource->GetImageCreateInfo().format);
            GetImpl()->Region.imageSubresource.baseArrayLayer = 0;
            GetImpl()->Region.imageSubresource.layerCount = pDstResource->GetImageCreateInfo().arrayLayers;
            GetImpl()->Region.imageSubresource.mipLevel = 0;
            GetImpl()->Region.imageOffset.x = 0;
            GetImpl()->Region.imageOffset.y = 0;
            GetImpl()->Region.imageOffset.z = 0;
            GetImpl()->Region.imageExtent = pDstResource->GetImageCreateInfo().extent;
        }
    }

} // namespace cauldron

#endif // #if defined(_VK)
