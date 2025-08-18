/* Copyright(C) 2021-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once


#include <d3d12.h>
#include "AmdExtD3D.h"

enum class AmdExtD3DHeapAndResourceFlags
{
    PreferVideoMemory       = 0x1,
    CpuVisible              = 0x2,
    StorageDst              = 0x4,
    KmdShareUmdSysMem       = 0x8,
    ForceLinear             = 0x10,
};

/**
***********************************************************************************************************************
* Layout information for one subresource
***********************************************************************************************************************
*/
struct AmdExtD3DSubresourceInfo
{
    uint64_t rowPitch;   ///< Subresource row pitch in bytes
    uint64_t depthPitch; ///< Subresource depth pitch in bytes
    uint64_t offset;     ///< Offset from texture base address in bytes
};

/**
***********************************************************************************************************************
* @brief Extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("a3c56f9f-888f-4a98-a121-70ed5ae207e6")
IAmdExtD3DCreateResource : public IUnknown
{
    virtual HRESULT CreateCommittedResource(
        AmdExtD3DHeapAndResourceFlags   heapAndResourceFlags,
        const D3D12_HEAP_PROPERTIES*    pHeapProperties,
        D3D12_HEAP_FLAGS                heapFlags,
        const D3D12_RESOURCE_DESC*      pDesc,
        D3D12_RESOURCE_STATES           initialResourceState,
        const D3D12_CLEAR_VALUE*        pOptimizedClearValue,
        REFIID                          riidResource,
        void**                          ppvResource) = 0;

    virtual HRESULT CreateHeap(
        AmdExtD3DHeapAndResourceFlags   heapAndResourceFlags,
        const D3D12_HEAP_DESC*          pDesc,
        REFIID                          riid,
        void**                          ppvHeap) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 1 extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("b41bd061-1eb0-4f50-beab-c8ed3ac1f509")
IAmdExtD3DCreateResource1 : public IAmdExtD3DCreateResource
{
    virtual D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(
        AmdExtD3DHeapAndResourceFlags    heapAndResourceFlags,
        uint32_t                         visibleMask,
        uint32_t                         numResourceDescs,
        const D3D12_RESOURCE_DESC*       pResourceDescs) = 0;

    virtual D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo1(
        AmdExtD3DHeapAndResourceFlags    heapAndResourceFlags,
        uint32_t                         visibleMask,
        uint32_t                         numResourceDescs,
        const D3D12_RESOURCE_DESC*       pResourceDescs,
        D3D12_RESOURCE_ALLOCATION_INFO1* pResourceAllocationInfo1) = 0;

    virtual HRESULT CreatePlacedResource(
        AmdExtD3DHeapAndResourceFlags   heapAndResourceFlags,
        ID3D12Heap*                     pHeap,
        uint64_t                        heapOffset,
        const D3D12_RESOURCE_DESC*      pDesc,
        D3D12_RESOURCE_STATES           initialState,
        const D3D12_CLEAR_VALUE*        pOptimizedClearValue,
        REFIID                          riid,
        void**                          ppvResource) = 0;

    virtual AmdExtD3DSubresourceInfo GetSubresourceInfo(
        ID3D12Resource* pResource,
        uint32_t        subresourceIndex) = 0;
};

using IAmdExtD3DCreateResourceLatest = IAmdExtD3DCreateResource1;
