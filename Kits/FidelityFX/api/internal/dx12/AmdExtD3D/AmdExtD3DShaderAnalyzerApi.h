/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <unknwn.h>
#include <cstdint>
#include <d3d12.h>

typedef void* AmdExtD3DPipelineHandle;
typedef void* AmdExtD3DPipelineElfHandle;
// Process environment variable to set virtual GPU Id in call to PAL CreatePlatform during Adapter initialization.
static const char AmdExtVirtualGpuIdEnvVarStringName[] = "AmdVirtualGpuId";

/**
***********************************************************************************************************************
* @brief GPU Id and string name
***********************************************************************************************************************
*/
struct AmdExtD3DGpuIdEntry
{
    UINT        gpuId;       ///< GPU Id
    const char* pGpuIdName;  ///< GPU string name in the form of "gpuName:gfxIp"
};

/**
***********************************************************************************************************************
* @brief GPU Id list with Env. varaible name for virtual GPU.
***********************************************************************************************************************
*/
struct AmdExtD3DGpuIdList
{
    UINT                 numGpuIdEntries; // Number of GPU Ids
    AmdExtD3DGpuIdEntry* pGpuIdEntries;   // GPU Id entries
};

/**
***********************************************************************************************************************
* @brief Shader stage type flags
***********************************************************************************************************************
*/
enum AmdExtD3DShaderStageTypeFlags : UINT
{
    AmdExtD3DShaderStageCompute  = 0x00000001,
    AmdExtD3DShaderStageVertex   = 0x00000002,
    AmdExtD3DShaderStageHull     = 0x00000004,
    AmdExtD3DShaderStageDomain   = 0x00000008,
    AmdExtD3DShaderStageGeometry = 0x00000010,
    AmdExtD3DShaderStagePixel    = 0x00000020
};

/**
***********************************************************************************************************************
* @brief Shader usage statistics
***********************************************************************************************************************
*/
struct AmdExtD3DShaderUsageStats
{
    UINT    numUsedVgprs;              ///< Number of VGPRs used by shader
    UINT    numUsedSgprs;              ///< Number of SGPRs used by shader
    UINT    ldsSizePerThreadGroup;     ///< LDS size per thread group in bytes.
    size_t  ldsUsageSizeInBytes;       ///< LDS usage by shader.
    size_t  scratchMemUsageInBytes;    ///< Amount of scratch mem used by shader.
};

/**
***********************************************************************************************************************
* @brief Shader statistics, multiple bits set in the shader stage mask indicates that multiple shaders have been
*        combined by HW. The same information will be repeated for both the constituent shaders in this case.
***********************************************************************************************************************
*/
struct AmdExtD3DShaderStats
{
    UINT                      shaderStageMask;      ///< Indicates stages of pipeline shader stats are for.  Multiple
                                                    ///  bits set indicate shaders stages were merged.
    AmdExtD3DShaderUsageStats usageStats;           ///< Shader reg/LDS/Scratch mem usage.
    UINT                      numPhysicalVgprs;     ///< Number of physical Vgprs.
    UINT                      numPhysicalSgprs;     ///< Number of physical Sgprs.
    UINT                      numAvailableVgprs;    ///< Number of Vgprs made available to shader
    UINT                      numAvailableSgprs;    ///< Number of Sgprs made available to shader
    size_t                    isaSizeInBytes;       ///< Size of the shader ISA disassembly for this shader
};

/**
***********************************************************************************************************************
* @brief  DXR pipeline hardware resource usage statistics for a shader.
***********************************************************************************************************************
*/
struct AmdExtD3DShaderStatsRayTracing : public AmdExtD3DShaderStats
{
    uint32_t    stackSizeInBytes;     ///< Stack size used by this shader export.
    bool        isInlined;            ///< True if uber shader was generated.
};

/**
***********************************************************************************************************************
* @brief Shader statistics for graphics pipeline
***********************************************************************************************************************
*/
struct AmdExtD3DGraphicsShaderStats
{
    AmdExtD3DShaderStats vertexShaderStats;     ///< Vertex Shader stats
    AmdExtD3DShaderStats hullShaderStats;       ///< Hull Shader stats
    AmdExtD3DShaderStats domainShaderStats;     ///< Domain Shader stats
    AmdExtD3DShaderStats geometryShaderStats;   ///< Geometry Shader stats
    AmdExtD3DShaderStats pixelShaderStats;      ///< Pixel Shader stats
};

/**
***********************************************************************************************************************
* @brief Shader statistics for compute pipeline
***********************************************************************************************************************
*/
struct AmdExtD3DComputeShaderStats : public AmdExtD3DShaderStats
{
    UINT numThreadsPerGroupX;           ///< Number of compute threads per thread group in X dimension.
    UINT numThreadsPerGroupY;           ///< Number of compute threads per thread group in Y dimension.
    UINT numThreadsPerGroupZ;           ///< Number of compute threads per thread group in Z dimension.
};


/**
***********************************************************************************************************************
* @brief Shader disassembly code for different shader stage
***********************************************************************************************************************
*/
struct AmdExtD3DPipelineDisassembly
{
    const char* pComputeDisassembly;   ///< Pointer to compute shader disassembly buffer
    const char* pVertexDisassembly;    ///< Pointer to vertex shader disassembly buffer
    const char* pHullDisassembly;      ///< Pointer to hull shader disassembly buffer
    const char* pDomainDisassembly;    ///< Pointer to domain shader disassembly buffer
    const char* pGeometryDisassembly;  ///< Pointer to geometry shader disassembly buffer
    const char* pPixelDisassembly;     ///< Pointer to pixel shader disassembly buffer
};

/**
***********************************************************************************************************************
* @brief Shader Analyzer extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("A2783A2E-FECA-4881-B3FD-7F8F1D6F4A08")
IAmdExtD3DShaderAnalyzer: public IUnknown
{
public:
    // Gets list of available virtual GPU Ids
    // NOTE: The client will need to call this function twice :
    //       First time, client call it with empty AmdExtD3DGpuIdList struct and driver will store the maximum
    //       available virtual GPU number in numGpuIdEntries and return it back to client.
    //       Second time, client is responsible for allocating enough space based on numGpuIdEntries driver returned
    //       and then call this function to get the available GPU ID entries
    virtual HRESULT GetAvailableVirtualGpuIds(AmdExtD3DGpuIdList* pGpuIdList) = 0;

    // Create Graphics pipeline state with shader stats and get the pipeline object handle
    virtual HRESULT CreateGraphicsPipelineState(
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc,
        REFIID                                    riid,
        void**                                    ppPipelineState,
        AmdExtD3DGraphicsShaderStats*             pGraphicsShaderStats,
        AmdExtD3DPipelineHandle*                  pPipelineHandle) = 0;

    // Create Compute pipeline state with shader stats and get the pipeline object handle
    virtual HRESULT CreateComputePipelineState(
        const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
        REFIID                                   riid,
        void**                                   ppPipelineState,
        AmdExtD3DComputeShaderStats*             pComputeShaderStats,
        AmdExtD3DPipelineHandle*                 pPipelineHandle) = 0;

    // Get shader ISA code
    // NOTE: The client will need to call this function twice to get the ISA code :
    //       First time, client call it with null pIsaCode to query the size of whole ISA code buffer
    //       Second time, the client is responsible for allocating enough space for ISA code buffer. And the pointer to
    //       the ISA code buffer will be returned as pIsaCode. Also, client can get the per-shader stage disassembly
    //       (will be nullptr if it is not present) by passing in the empty AmdExtD3DPipelineDisassembly struct
    virtual HRESULT GetShaderIsaCode(
        AmdExtD3DPipelineHandle       pipelineHandle,
        char*                         pIsaCode,
        size_t*                       pIsaCodeSize,
        AmdExtD3DPipelineDisassembly* pDisassembly) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 1 Shader Analyzer extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("A2EA2E25-9709-47E3-B3A0-6DDA8A9372C4")
IAmdExtD3DShaderAnalyzer1 : public IAmdExtD3DShaderAnalyzer
{
public:
    // Create Graphics pipeline state with the pipeline object handle
    virtual HRESULT CreateGraphicsPipelineState1(
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc,
        REFIID                                    riid,
        void**                                    ppPipelineState,
        AmdExtD3DPipelineHandle*                  pPipelineHandle) = 0;

    // Create Compute pipeline state with the pipeline object handle
    virtual HRESULT CreateComputePipelineState1(
        const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
        REFIID                                   riid,
        void**                                   ppPipelineState,
        AmdExtD3DPipelineHandle*                 pPipelineHandle) = 0;

    // Get graphic shader stats
    virtual HRESULT GetGraphicsShaderStats(
        AmdExtD3DPipelineHandle       pipelineHandle,
        AmdExtD3DGraphicsShaderStats* pGraphicsShaderStats) = 0;

    // Get compute shader stats
    virtual HRESULT GetComputeShaderStats(
        AmdExtD3DPipelineHandle      pipelineHandle,
        AmdExtD3DComputeShaderStats* pComputeShaderStats) = 0;

    // Get pipeline ELF binary
    // NOTE: The client will need to call this function twice to get the pipeline ELF binary :
    //       First time, client call it with null pipelineElfHandle to query the size of pipeline ELF binary
    //       Second time, the client is responsible for allocating enough space for pipeline ELF binary. The handle
    //       to the elf binary will be returned as pipelineElfHandle.
    virtual HRESULT GetPipelineElfBinary(
        AmdExtD3DPipelineHandle     pipelineHandle,
        AmdExtD3DPipelineElfHandle  pipelineElfHandle,
        uint32_t*                   pSize) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 2 Shader Analyzer extension API object (adding DXR extension support)
***********************************************************************************************************************
*/
MIDL_INTERFACE("B63EEBE2-411B-494D-A4D1-08676225773C")
IAmdExtD3DShaderAnalyzer2 : public IAmdExtD3DShaderAnalyzer1
{
   // Overload the CreateStateObject compile the pipeline.
    virtual HRESULT CreateStateObject(
        const D3D12_STATE_OBJECT_DESC* pDesc,
        REFIID                        riid, // ID3D12StateObject
        void**                        ppStateObject,
        AmdExtD3DPipelineHandle*      pPipelineHandle) = 0;

    // Get DXR pipeline stats. Since DXR pipelines are dispatched by specifying a ray
    // generation shader, we need the ray generation shader identifier to retrieve the
    // pipeline in the driver.
    // Note: pRayGenerationExportName must match the relevant HLSL export name.
    virtual HRESULT GetRayTracingPipelineStats(
        AmdExtD3DPipelineHandle           pipelineHandle,
        const wchar_t*                    pRayGenerationExportName,
        AmdExtD3DShaderStatsRayTracing*   pRayTracingPipelineStats) = 0;

    // Get DXR pipeline stats based on a pipeline index to support the use-case where export names
    // have not been provided and the desire is to extract all available pipelines.
    // pipelineIndex must be a valid value in the range of 0 to pipelineCount as reported by
    // GetPipelineElfCount().
    virtual HRESULT GetRayTracingPipelineStatsByIndex(
        AmdExtD3DPipelineHandle         pipelineHandle,
        uint32_t                        pipelineIndex,
        AmdExtD3DShaderStatsRayTracing* pRayTracingPipelineStats) = 0;

    // Get DXR shader stats.
    // Note: pShaderExportName must match the relevant HLSL export name.
    virtual HRESULT GetRayTracingPipelineShaderStats(
        AmdExtD3DPipelineHandle         pipelineHandle,
        const wchar_t*                  pShaderExportName,
        AmdExtD3DShaderStatsRayTracing* pRayTracingShaderStats) = 0;

    // Retrieve the disassembly for a ray tracing shader (fails if shader was inlined).
    // This would work, if relevant, for callable shaders as well.
    // If the call to this function is performed with pIsaCode as nullptr, pIsaCodeSize would be set
    // to the number of bytes to be allocated by the caller. If pIsaCode is not nullptr, the
    // disassembly data would be written to pIsaCode.
    // Note: pShaderExportName must match the relevant HLSL export name.
    virtual HRESULT GetRayTracingShaderIsaDisassembly(
        AmdExtD3DPipelineHandle     pipelineHandle,
        const wchar_t*              pShaderExportName,
        char*                       pIsaCode,
        size_t*                     pIsaCodeSize) = 0;

    // Retrieve the disassembly for a ray tracing pipeline.
    // First call performed with pIsaCode as nullptr would return the number of bytes to be
    // allocated. Second call with an allocated buffer would fill in the bytes.
    // Note: pRayGenerationName must match the relevant HLSL export name.
    virtual HRESULT GetRayTracingPipelineIsaDisassembly(
        AmdExtD3DPipelineHandle     pipelineHandle,
        const wchar_t*              pRayGenerationExportName,
        char*                       pIsaCode,
        size_t*                     pIsaCodeSize) = 0;

    // Check if a pipeline was compiled in Indirect mode.
    virtual HRESULT IsRayTracingPipelineIndirect(
        AmdExtD3DPipelineHandle     pipelineHandle,
        uint32_t                    pipelineIndex,
        bool*                       pIsIndirect) const = 0;

    // Gets the number of shaders which have been compiled as part of the pipeline.
    virtual HRESULT GetRayTracingPipelineShaderCount(
        AmdExtD3DPipelineHandle     pipelineHandle,
        uint32_t                    pipelineIndex,
        uint32_t*                   pShaderCount) = 0;

    // Retrieves the name of a shader that was compiled as part of a raytracing pipeline.
    // If pNameBuffer is nullptr - sets the name size in bytes to nameBytes, otherwise copies the name to pNameBuffer.
    // Note: the given index has to be in the range between 0 and
    // the count returned by GetRayTracingPipelineShaderCount().
    virtual HRESULT GetRayTracingPipelineShaderName(
        AmdExtD3DPipelineHandle   pipelineHandle,
        uint32_t                  pipelineIndex,
        uint32_t                  shaderIndex,
        size_t*                   pNameCount,
        wchar_t*                  pNameBuffer) = 0;

    // Retrieve the disassembly for a ray tracing pipeline.
    // If the call to this function is performed with pIsaCode as nullptr, pIsaCodeSize would be set
    // to the number of bytes to be allocated by the caller. If pIsaCode is not nullptr, the
    // disassembly data would be written to pIsaCode.
    virtual HRESULT GetRayTracingPipelineIsaDisassemblyByIndex(
        AmdExtD3DPipelineHandle     pipelineHandle,
        uint32_t                    pipelineIndex,
        char*                       pIsaCode,
        size_t*                     pIsaCodeSize) = 0;

    // Get the number of RayTracing pipeline ELF binaries that are available in the compiled state object.
    // Any value between 0 and pipelineCount-1 would be a valid pipelineIndex argument for
    // GetPipelineElfBinaryDXR().
    // pipelineElfHandle is the handle to the State Object.
    // pipelineCount would be set to the number of pipeline ELF binaries.
    virtual HRESULT GetRayTracingPipelineElfCount(
        AmdExtD3DPipelineHandle     pipelineHandle,
        uint32_t*                   pPipelineCount) = 0;

    // Get RayTracing pipeline ELF binary.
    // pipelineElfHandle is the handle to State Object.
    // pipelineIndex is the index of relevant pipeline if multiple pipelines targeted, or zero
    // otherwise. This value must be between 0 and (pipelineCount-1) as returned by
    // GetPipelineElfCount().
    // pipelineElfHandle is the output buffer. If nullptr, pSize will be set for the number of bytes
    // to be allocated in the buffer (ELF buffer size).
    virtual HRESULT GetRayTracingPipelineElfBinary(
        AmdExtD3DPipelineHandle     pipelineHandle,
        uint32_t                    pipelineIndex,
        AmdExtD3DPipelineElfHandle  pipelineElfHandle,
        uint32_t*                   pSize) = 0;

};

/**
***********************************************************************************************************************
* @brief Version 3 Shader Analyzer extension API object (adding dissasembly support)
***********************************************************************************************************************
*/
MIDL_INTERFACE("D54EA359-BE21-40A5-89FF-4E81ACAD88A1")
IAmdExtD3DShaderAnalyzer3 : public IAmdExtD3DShaderAnalyzer2
{
    // Get shader AMDIL code
    // NOTE: The client will need to call this function twice to get the AMDIL code :
    //       First time, client call it with null pAmdilCode to query the size of whole AMDIL code buffer.
    //       Second time, the client is responsible for allocating enough space for AMDIL code buffer and passes it as
    //       pAmdilCode. Also, if pDisassembly is not null, it will be populated with the AMDIL for the
    //       individual pipeline stages.
    virtual HRESULT GetShaderAmdIlDisassembly(
        AmdExtD3DPipelineHandle       pipelineHandle,
        char*                         pAmdilCode,
        size_t*                       pAmdilCodeSize,
        AmdExtD3DPipelineDisassembly* pDisassembly) = 0;
};
