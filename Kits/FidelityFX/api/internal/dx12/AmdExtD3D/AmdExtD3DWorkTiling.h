/* Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once
#include <d3d12.h>

/// Type of additional information for a command in the tiling range
enum AmdExtD3DWorkTilingAnnotationType
{
    TileDilationSize          = 0, ///< Dilation size of a tile relative to previous command
    ResourceFootprintEstimate = 1, ///< Estimate of the total number of bytes read/written for all resources
};

/// Dilation size in units of elements/threads/pixels. This information is required if a shader thread
/// can sample adjacent addresses in a resource that was written by a previous command. Failure to specify
/// this results in undefined behavior for these cases.
struct AmdExtD3DWorkTilingDilationSize
{
    UINT maxDilationRadiusX;
    UINT maxDilationRadiusY;
    UINT maxDilationRadiusZ;
};

/// Estimate of the amount of memory accessed by a command in the tiling range. This provides more
/// information that is used in the heuristic for determining tile sizes. If this is provided, it
/// should include every resource otherwise performance may not be optimal.
struct AmdExtD3DWorkTilingResourceFootprint
{
    UINT64 totalReadBytes;
    UINT64 totalWriteBytes;
};

enum AmdExtD3DWorkTilingError
{
    InvalidCommand,             ///< An unsupported command was used between Begin/End
    NumberOfThreadsMismatch,    ///< Dispatch command was issued that had a different number of threads
                                ///  compared to previous dispatches inside Begin/End
};

typedef void (*AmdExtD3DWorkTilingErrorCallback)(AmdExtD3DWorkTilingError);

MIDL_INTERFACE("212755BC-9D41-445E-A635-35EA3FD207CF")
IAmdExtD3DWorkTiling : public IUnknown
{
public:
    /// Get the support level for work tiling capabilities. The return value should be compared to the expected
    /// minimum support level. Higher levels support all features from lower levels.
    virtual UINT GetWorkTilingSupportLevel() = 0;

    /// Create a command list to be used with work tiling. Equivalent to ID3D12Device::CreateCommandList.
    virtual HRESULT CreateWorkTilingCommandList(
        UINT                    supportLevel,
        UINT                    nodeMask,
        D3D12_COMMAND_LIST_TYPE type,
        ID3D12CommandAllocator* pCommandAllocator,
        ID3D12PipelineState*    pInitialState,
        REFIID                  riid,
        void**                  ppCommandList) = 0;

    /// Create a command list to be used with work tiling. Equivalent to ID3D12Device4::CreateCommandList1.
    virtual HRESULT CreateWorkTilingCommandList1(
        UINT                     supportLevel,
        UINT                     nodeMask,
        D3D12_COMMAND_LIST_TYPE  type,
        D3D12_COMMAND_LIST_FLAGS flags,
        REFIID                   riid,
        void**                   ppCommandList) = 0;

    /// Marks the beginning of a range of commands to tile
    virtual HRESULT WorkTilingBegin(
        ID3D12GraphicsCommandList* pGfxCmdList,
        UINT                       workloadId) = 0;

    /// Marks the end of a range of commands to tile
    virtual HRESULT WorkTilingEnd(
        ID3D12GraphicsCommandList* pGfxCmdList) = 0;

    /// Sets annotation data to be associated with the following commands
    virtual HRESULT WorkTilingSetAnnotation(
        ID3D12GraphicsCommandList*        pGfxCmdList,
        AmdExtD3DWorkTilingAnnotationType type,
        const void*                       pData) = 0;

    /// Sets a function to be called whenever an internal error occurs
    virtual HRESULT WorkTilingSetErrorCallback(
        ID3D12GraphicsCommandList*       pGfxCmdList,
        AmdExtD3DWorkTilingErrorCallback callback) = 0;
};
