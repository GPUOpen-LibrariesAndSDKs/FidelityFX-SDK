// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
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

#include <vulkan/vulkan.h>
#include "FrameInterpolationSwapchainVK_DebugPacing.h"
#include "FrameInterpolationSwapchainVK_Helpers.h"
#include "FrameInterpolationSwapchainVK.h"

#include "FrameInterpolationSwapchainDebugPacingVS.h"
#include "FrameInterpolationSwapchainDebugPacingPS.h"

constexpr uint32_t c_debugPacingRingBufferSize = 4;

VkDevice              s_debugPacingDevice                 = VK_NULL_HANDLE;
VkDescriptorPool      s_debugPacingDescriptorPool         = VK_NULL_HANDLE;
VkPipelineLayout      s_debugPacingPipelineLayout         = VK_NULL_HANDLE;
VkRenderPass          s_debugPacingRenderPass             = VK_NULL_HANDLE;
VkPipeline            s_debugPacingPipeline               = VK_NULL_HANDLE;
VkFormat              s_debugPacingAttachmentFormat       = VK_FORMAT_UNDEFINED;
uint32_t              s_debugPacingFrameIndex             = 0;
uint32_t              s_debugPacingRingIndex              = 0;
VkImageView           s_debugPacingImageRTViews[c_debugPacingRingBufferSize];
VkFramebuffer         s_debugPacingFramebuffers[c_debugPacingRingBufferSize];


#define FFX_BACKEND_API_ERROR_ON_VK_ERROR(res) if (res != VK_SUCCESS) return FFX_ERROR_BACKEND_API_ERROR;


void releaseDebugPacingGpuResources(const VkAllocationCallbacks* pAllocator)
{
    vkDestroyDescriptorPool(s_debugPacingDevice, s_debugPacingDescriptorPool, pAllocator);
    s_debugPacingDescriptorPool = VK_NULL_HANDLE;

    vkDestroyPipeline(s_debugPacingDevice, s_debugPacingPipeline, pAllocator);
    s_debugPacingPipeline = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(s_debugPacingDevice, s_debugPacingPipelineLayout, pAllocator);
    s_debugPacingPipelineLayout = VK_NULL_HANDLE;

    vkDestroyRenderPass(s_debugPacingDevice, s_debugPacingRenderPass, pAllocator);
    s_debugPacingRenderPass = nullptr;

    for (uint32_t i = 0; i < _countof(s_debugPacingImageRTViews); ++i)
    {
        vkDestroyImageView(s_debugPacingDevice, s_debugPacingImageRTViews[i], pAllocator);
        s_debugPacingImageRTViews[i] = VK_NULL_HANDLE;
    }
    for (uint32_t i = 0; i < _countof(s_debugPacingFramebuffers); ++i)
    {
        vkDestroyFramebuffer(s_debugPacingDevice, s_debugPacingFramebuffers[i], pAllocator);
        s_debugPacingFramebuffers[i] = VK_NULL_HANDLE;
    }
}

void DestroyDebugPacingPipeline(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    vkDestroyRenderPass(device, s_debugPacingRenderPass, pAllocator);
    s_debugPacingRenderPass = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(device, s_debugPacingPipelineLayout, pAllocator);
    s_debugPacingPipelineLayout = VK_NULL_HANDLE;
    vkDestroyPipeline(device, s_debugPacingPipeline, pAllocator);
    s_debugPacingPipeline = VK_NULL_HANDLE;
}

// create the pipeline state to use for Debug Pacing
// pretty similar to FfxCreatePipelineFunc
VkResult CreateDebugPacingPipeline(VkDevice device, VkFormat fmt, const VkAllocationCallbacks* pAllocator)
{
    VkResult res = VK_SUCCESS;

    // create render pass
    if (res == VK_SUCCESS)
    {
        VkAttachmentDescription attachmentDesc = {};
        attachmentDesc.flags                   = 0;
        attachmentDesc.format                  = fmt;
        attachmentDesc.samples                 = VK_SAMPLE_COUNT_1_BIT;
        attachmentDesc.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.initialLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentDesc.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference attachmentRef = {};
        attachmentRef.attachment            = 0;
        attachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subPassDesc    = {};
        subPassDesc.flags                   = 0;
        subPassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subPassDesc.inputAttachmentCount    = 0;
        subPassDesc.pInputAttachments       = nullptr;
        subPassDesc.colorAttachmentCount    = 1;
        subPassDesc.pColorAttachments       = &attachmentRef;
        subPassDesc.pResolveAttachments     = nullptr;
        subPassDesc.pDepthStencilAttachment = nullptr;
        subPassDesc.preserveAttachmentCount = 0;
        subPassDesc.pPreserveAttachments    = nullptr;

        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.pNext                  = nullptr;
        renderPassCreateInfo.flags                  = 0;
        renderPassCreateInfo.attachmentCount        = 1;
        renderPassCreateInfo.pAttachments           = &attachmentDesc;
        renderPassCreateInfo.subpassCount           = 1;
        renderPassCreateInfo.pSubpasses             = &subPassDesc;
        renderPassCreateInfo.dependencyCount        = 0;
        renderPassCreateInfo.pDependencies          = nullptr;

        res = vkCreateRenderPass(device, &renderPassCreateInfo, pAllocator, &s_debugPacingRenderPass);
    }

    // create pipeline layout
    if (res == VK_SUCCESS)
    {
        VkPushConstantRange pushConstant = {};
        pushConstant.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstant.offset              = 0;
        pushConstant.size                = sizeof(uint32_t);

        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.pNext                      = nullptr;
        layoutInfo.flags                      = 0;
        layoutInfo.setLayoutCount             = 0;
        layoutInfo.pSetLayouts                = nullptr;
        layoutInfo.pushConstantRangeCount     = 1;
        layoutInfo.pPushConstantRanges        = &pushConstant;

        res = vkCreatePipelineLayout(device, &layoutInfo, pAllocator, &s_debugPacingPipelineLayout);
    }

    // create VS module
    VkShaderModule moduleVS = VK_NULL_HANDLE;
    if (res == VK_SUCCESS)
    {
        res = CreateShaderModule(device, sizeof(g_mainDebugPacingVS), g_mainDebugPacingVS, &moduleVS, pAllocator);
    }

    // create PS module
    VkShaderModule modulePS = VK_NULL_HANDLE;
    if (res == VK_SUCCESS)
    {
        res = CreateShaderModule(device, sizeof(g_mainDebugPacingPS), g_mainDebugPacingPS, &modulePS, pAllocator);
    }

    // create pipeline
    if (res == VK_SUCCESS)
    {
        VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2];
        shaderStageCreateInfos[0].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfos[0].pNext               = nullptr;
        shaderStageCreateInfos[0].flags               = 0;
        shaderStageCreateInfos[0].stage               = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageCreateInfos[0].module              = moduleVS;
        shaderStageCreateInfos[0].pName               = "main";
        shaderStageCreateInfos[0].pSpecializationInfo = nullptr;

        shaderStageCreateInfos[1].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfos[1].pNext               = nullptr;
        shaderStageCreateInfos[1].flags               = 0;
        shaderStageCreateInfos[1].stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageCreateInfos[1].module              = modulePS;
        shaderStageCreateInfos[1].pName               = "main";
        shaderStageCreateInfos[1].pSpecializationInfo = nullptr;

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
        vertexInputStateCreateInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.pNext                                = nullptr;
        vertexInputStateCreateInfo.flags                                = 0;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount        = 0;
        vertexInputStateCreateInfo.pVertexBindingDescriptions           = nullptr;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount      = 0;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions         = nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
        inputAssemblyStateCreateInfo.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.pNext                                  = nullptr;
        inputAssemblyStateCreateInfo.flags                                  = 0;
        inputAssemblyStateCreateInfo.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCreateInfo.primitiveRestartEnable                 = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
        rasterizationStateCreateInfo.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.pNext                                  = nullptr;
        rasterizationStateCreateInfo.flags                                  = 0;
        rasterizationStateCreateInfo.depthClampEnable                       = VK_FALSE;
        rasterizationStateCreateInfo.rasterizerDiscardEnable                = VK_FALSE;
        rasterizationStateCreateInfo.polygonMode                            = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.cullMode                               = VK_CULL_MODE_NONE;
        rasterizationStateCreateInfo.frontFace                              = VK_FRONT_FACE_CLOCKWISE;
        rasterizationStateCreateInfo.depthBiasEnable                        = VK_FALSE;
        rasterizationStateCreateInfo.depthBiasConstantFactor                = 0.0f;
        rasterizationStateCreateInfo.depthBiasClamp                         = 0.0f;
        rasterizationStateCreateInfo.depthBiasSlopeFactor                   = 0.0f;
        rasterizationStateCreateInfo.lineWidth                              = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
        depthStencilStateCreateInfo.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.pNext                                 = nullptr;
        depthStencilStateCreateInfo.flags                                 = 0;
        depthStencilStateCreateInfo.depthTestEnable                       = VK_FALSE;
        depthStencilStateCreateInfo.depthWriteEnable                      = VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp                        = VK_COMPARE_OP_NEVER;
        depthStencilStateCreateInfo.depthBoundsTestEnable                 = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable                     = VK_FALSE;
        depthStencilStateCreateInfo.front.failOp                          = VK_STENCIL_OP_KEEP;
        depthStencilStateCreateInfo.front.passOp                          = VK_STENCIL_OP_KEEP;
        depthStencilStateCreateInfo.front.depthFailOp                     = VK_STENCIL_OP_KEEP;
        depthStencilStateCreateInfo.front.compareOp                       = VK_COMPARE_OP_NEVER;
        depthStencilStateCreateInfo.front.compareMask                     = 0;
        depthStencilStateCreateInfo.front.writeMask                       = 0;
        depthStencilStateCreateInfo.front.reference                       = 0;
        depthStencilStateCreateInfo.back                                  = depthStencilStateCreateInfo.front;
        depthStencilStateCreateInfo.minDepthBounds                        = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds                        = 1.0f;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
        colorBlendAttachmentState.blendEnable                         = VK_FALSE;
        colorBlendAttachmentState.srcColorBlendFactor                 = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.dstColorBlendFactor                 = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.colorBlendOp                        = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp                        = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
        colorBlendStateCreateInfo.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.pNext                               = nullptr;
        colorBlendStateCreateInfo.flags                               = 0;
        colorBlendStateCreateInfo.logicOpEnable                       = VK_FALSE;
        colorBlendStateCreateInfo.logicOp                             = VK_LOGIC_OP_CLEAR;
        colorBlendStateCreateInfo.attachmentCount                     = 1;
        colorBlendStateCreateInfo.pAttachments                        = &colorBlendAttachmentState;
        colorBlendStateCreateInfo.blendConstants[0]                   = 0.0f;
        colorBlendStateCreateInfo.blendConstants[1]                   = 0.0f;
        colorBlendStateCreateInfo.blendConstants[2]                   = 0.0f;
        colorBlendStateCreateInfo.blendConstants[3]                   = 0.0f;

        VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
        dynamicStateCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.pNext                            = nullptr;
        dynamicStateCreateInfo.flags                            = 0;
        dynamicStateCreateInfo.dynamicStateCount                = _countof(dynamicStates);
        dynamicStateCreateInfo.pDynamicStates                   = dynamicStates;

        // dynamic so put dummy values
        VkViewport viewport;
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = 1920;
        viewport.height   = 1080;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor;
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        scissor.extent.width  = 0;
        scissor.extent.height = 0;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
        viewportStateCreateInfo.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount                     = 1;
        viewportStateCreateInfo.pViewports                        = &viewport;
        viewportStateCreateInfo.scissorCount                      = 1;
        viewportStateCreateInfo.pScissors                         = &scissor;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
        multisampleStateCreateInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.pNext                                = nullptr;
        multisampleStateCreateInfo.flags                                = 0;
        multisampleStateCreateInfo.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
        multisampleStateCreateInfo.sampleShadingEnable                  = VK_FALSE;
        multisampleStateCreateInfo.minSampleShading                     = 0.0f;
        multisampleStateCreateInfo.pSampleMask                          = nullptr;
        multisampleStateCreateInfo.alphaToCoverageEnable                = VK_FALSE;
        multisampleStateCreateInfo.alphaToOneEnable                     = VK_FALSE;

        VkGraphicsPipelineCreateInfo info = {};
        info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.pNext                        = nullptr;
        info.flags                        = 0;
        info.stageCount                   = _countof(shaderStageCreateInfos);
        info.pStages                      = shaderStageCreateInfos;
        info.pVertexInputState            = &vertexInputStateCreateInfo;
        info.pInputAssemblyState          = &inputAssemblyStateCreateInfo;
        info.pTessellationState           = nullptr;
        info.pViewportState               = &viewportStateCreateInfo;
        info.pRasterizationState          = &rasterizationStateCreateInfo;
        info.pMultisampleState            = &multisampleStateCreateInfo;
        info.pDepthStencilState           = &depthStencilStateCreateInfo;
        info.pColorBlendState             = &colorBlendStateCreateInfo;
        info.pDynamicState                = &dynamicStateCreateInfo;
        info.layout                       = s_debugPacingPipelineLayout;
        info.renderPass                   = s_debugPacingRenderPass;
        info.subpass                      = 0;
        info.basePipelineHandle           = VK_NULL_HANDLE;
        info.basePipelineIndex            = -1;

        res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, pAllocator, &s_debugPacingPipeline);
    }

    // cleanup
    vkDestroyShaderModule(device, moduleVS, pAllocator);
    vkDestroyShaderModule(device, modulePS, pAllocator);

    if (res == VK_SUCCESS)
    {
        s_debugPacingAttachmentFormat = fmt;
    }
    else
    {
        DestroyDebugPacingPipeline(device, pAllocator);
    }

    return res;
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
    // need physical device
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((memoryTypeBits & (1 << i)) != 0 && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return 0;
}

FfxErrorCodes verifyDebugPacingGpuResources(VkPhysicalDevice physicalDevice, VkDevice device, VkFormat fmt, const VkAllocationCallbacks* pAllocator)
{
    FFX_ASSERT(VK_NULL_HANDLE != device);

    if (s_debugPacingDevice != device)
    {
        if (s_debugPacingDevice != VK_NULL_HANDLE)
        {
            // new device
            releaseDebugPacingGpuResources(pAllocator);
        }
        else
        {
            s_debugPacingDevice = device;
        }
    }

    VkResult res = VK_SUCCESS;

    // create pool
    if (res == VK_SUCCESS && s_debugPacingDescriptorPool == VK_NULL_HANDLE)
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo info = {};
        info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.pNext                      = nullptr;
        info.flags                      = 0;
        info.maxSets                    = 1;
        info.poolSizeCount              = 1;
        info.pPoolSizes                 = &poolSize;

        res = vkCreateDescriptorPool(device, &info, pAllocator, &s_debugPacingDescriptorPool);
    }

    if (res == VK_SUCCESS)
    {
        if (s_debugPacingAttachmentFormat != fmt)
        {
            DestroyDebugPacingPipeline(device, pAllocator);
        }
        if (s_debugPacingPipeline == VK_NULL_HANDLE)
        {
            res = CreateDebugPacingPipeline(device, fmt, pAllocator);
        }
    }

    if (res == VK_SUCCESS)
        return FFX_OK;
    else
        return FFX_ERROR_BACKEND_API_ERROR;
}

FFX_API FfxErrorCode ffxFrameInterpolationDebugPacing(const FfxPresentCallbackDescription* params, void* userContext)
{
    FFX_ASSERT(userContext != nullptr);

    const VkAllocationCallbacks* pAllocator  = nullptr;

    VkDevice device            = reinterpret_cast<VkDevice>(params->device);

    const FfxDebugPacingContext* ctx            = reinterpret_cast<const FfxDebugPacingContext*>(userContext);
    VkPhysicalDevice             physicalDevice = ctx->physicalDevice;

    FFX_ASSERT(physicalDevice != VK_NULL_HANDLE);
    FFX_ASSERT(device != VK_NULL_HANDLE);

    // blit backbuffer and composite UI using a VS/PS pass
    FfxErrorCode res =
        verifyDebugPacingGpuResources(physicalDevice, device, ffxGetVkFormatFromSurfaceFormat(params->currentBackBuffer.description.format), pAllocator);
    if (res != FFX_OK)
        return res;

    VkCommandBuffer commandBuffer   = reinterpret_cast<VkCommandBuffer>(params->commandList);
    VkImage         backbufferImage = reinterpret_cast<VkImage>(params->currentBackBuffer.resource);
    
    FFX_ASSERT(commandBuffer != VK_NULL_HANDLE);
    FFX_ASSERT(s_debugPacingPipeline != VK_NULL_HANDLE);
    FFX_ASSERT(backbufferImage != VK_NULL_HANDLE);

    const uint32_t backBufferWidth    = params->currentBackBuffer.description.width;
    const uint32_t backBufferHeight   = params->currentBackBuffer.description.height;

    auto flipBarrier = [](VkImageMemoryBarrier& barrier) {
        VkAccessFlags tmpAccess = barrier.srcAccessMask;
        VkImageLayout tmpLayout = barrier.oldLayout;
        barrier.srcAccessMask   = barrier.dstAccessMask;
        barrier.dstAccessMask   = tmpAccess;
        barrier.oldLayout       = barrier.newLayout;
        barrier.newLayout       = tmpLayout;
    };

    // create views
    vkDestroyImageView(device, s_debugPacingImageRTViews[s_debugPacingRingIndex], pAllocator);
    s_debugPacingImageRTViews[s_debugPacingRingIndex] = VK_NULL_HANDLE;

    VkImageViewCreateInfo viewCreateInfo           = {};
    viewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.pNext                           = nullptr;
    viewCreateInfo.flags                           = 0;
    viewCreateInfo.image                           = backbufferImage;
    viewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format                          = ffxGetVkFormatFromSurfaceFormat(params->currentBackBuffer.description.format);
    viewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCreateInfo.subresourceRange.baseMipLevel   = 0;
    viewCreateInfo.subresourceRange.levelCount     = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount     = 1;

    res = vkCreateImageView(device, &viewCreateInfo, pAllocator, &s_debugPacingImageRTViews[s_debugPacingRingIndex]);
    FFX_BACKEND_API_ERROR_ON_VK_ERROR(res);

    // create framebuffer
    if (s_debugPacingFramebuffers[s_debugPacingRingIndex] != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(device, s_debugPacingFramebuffers[s_debugPacingRingIndex], pAllocator);
        s_debugPacingFramebuffers[s_debugPacingRingIndex] = VK_NULL_HANDLE;
    }

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext                   = nullptr;
    framebufferCreateInfo.flags                   = 0;
    framebufferCreateInfo.renderPass              = s_debugPacingRenderPass;
    framebufferCreateInfo.attachmentCount         = 1;
    framebufferCreateInfo.pAttachments            = &s_debugPacingImageRTViews[s_debugPacingRingIndex];
    framebufferCreateInfo.width                   = backBufferWidth;
    framebufferCreateInfo.height                  = backBufferHeight;
    framebufferCreateInfo.layers                  = 1;
    res = vkCreateFramebuffer(device, &framebufferCreateInfo, pAllocator, &s_debugPacingFramebuffers[s_debugPacingRingIndex]);
    FFX_BACKEND_API_ERROR_ON_VK_ERROR(res);

    VkImageMemoryBarrier barrier;
    VkPipelineStageFlags srcStageMask       = 0;
    VkPipelineStageFlags dstStageMask       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext                           = nullptr;
    barrier.srcAccessMask                   = getVKAccessFlagsFromResourceState(params->currentBackBuffer.state);
    barrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout                       = getVKImageLayoutFromResourceState(params->currentBackBuffer.state);
    barrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex             = 0;
    barrier.dstQueueFamilyIndex             = 0;
    barrier.image                           = backbufferImage;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    if (barrier.oldLayout != barrier.newLayout)
    {
        srcStageMask = srcStageMask | getVKPipelineStageFlagsFromResourceState(params->currentBackBuffer.state);
        vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
        
    VkRenderPassBeginInfo beginInfo    = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.pNext                    = nullptr;
    beginInfo.renderPass               = s_debugPacingRenderPass;
    beginInfo.framebuffer              = s_debugPacingFramebuffers[s_debugPacingRingIndex];
    beginInfo.renderArea.offset.x      = 0;
    beginInfo.renderArea.offset.y      = 0;
    beginInfo.renderArea.extent.width  = backBufferWidth;
    beginInfo.renderArea.extent.height = backBufferHeight;
    beginInfo.clearValueCount          = 0;
    beginInfo.pClearValues             = nullptr;
    vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_debugPacingPipeline);

    vkCmdPushConstants(commandBuffer, s_debugPacingPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &s_debugPacingFrameIndex);

    VkViewport viewport;
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = 32;
    viewport.height   = static_cast<float>(backBufferHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = backBufferWidth;
    scissor.extent.height = backBufferHeight;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (barrier.oldLayout != barrier.newLayout)
    {
        flipBarrier(barrier);
        vkCmdPipelineBarrier(commandBuffer, dstStageMask, srcStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    s_debugPacingFrameIndex = (s_debugPacingFrameIndex + 1) % FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT;
    s_debugPacingRingIndex  = (s_debugPacingRingIndex + 1) % c_debugPacingRingBufferSize;

    return FFX_OK;
}
