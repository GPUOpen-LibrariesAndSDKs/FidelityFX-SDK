/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <d3d12.h>
#include "AmdExtD3D.h"

/**
***********************************************************************************************************************
* @brief AMD extension source language type enumeration supported by the cross compiler in the driver
***********************************************************************************************************************
*/
enum class AmdShaderType : uint32_t
{
    Hip,
    Ocl,
    Hlsl,
    Spirv,
    ElfHsa,
    ElfPal,
};

/**
***********************************************************************************************************************
* @brief AMD extension structure enumeration supported by the driver
***********************************************************************************************************************
*/
enum AmdExtD3DStructType : uint32_t
{
    AmdExtD3DStructUnknown,                     ///< Unsupported
    AmdExtD3DStructPipelineState,               ///< Pipeline state extension structure (AmdExtD3DPipelineCreateInfo)
    AmdExtD3DStructPipelineElf,                 ///< Pipeline state extension structure (AmdExtD3DPipelineElfInfo)
    AmdExtD3DStructPipelineCrossCompile,        ///< Pipeline state extension structure (AmdExtD3DPipelineCCInfo)
};

/**
***********************************************************************************************************************
* @brief Extension create info base structure
***********************************************************************************************************************
*/
struct AmdExtD3DCreateInfo
{
    AmdExtD3DStructType type;       ///< AMD create info structure. Must be one of the supported types.
    void*               pNext;      ///< Pointer to a valid AMD structure. Must be nullptr if using base version.
                                    ///  Structures defined by multiple extensions (or versions) may be chained
                                    ///  together using this field. When chaining, the driver will process the chain
                                    ///  starting from the base parameter onwards.
};

/**
***********************************************************************************************************************
* @brief Extended pipeline flags. We currently only support one of these being set at a time.
***********************************************************************************************************************
*/
struct AmdExtD3DPipelineFlags
{
    union
    {
        struct
        {
            uint32_t depthBoundsTestEnable          : 1;    ///< Enable depth bounds testing
            uint32_t abortCreateIfPipelineNotCached : 1;    ///< If creation should be aborted if the pipeline
                                                            ///  is not cached
            uint32_t topologyTypeRectangle          : 1;    ///< Override PrimitiveTopologyType to use Rectangles
            uint32_t reserved                       : 29;   ///< Reserved bits (must be 0)
        };
        uint32_t all;
    };
};

/**
***********************************************************************************************************************
* @brief Extended pipeline state create info structure
***********************************************************************************************************************
*/
struct AmdExtD3DPipelineCreateInfo : AmdExtD3DCreateInfo
{
    AmdExtD3DPipelineFlags flags;  ///< Pipeline flags
};

struct AmdExtD3DPipelineElfInfo : AmdExtD3DCreateInfo
{
    const void* pElfBinary;         ///< Pointer to the input blob
    size_t      elfSizeInBytes;     ///< Size of the blob
    struct
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    } threadsPerGroup;
};

struct AmdExtD3DPipelineCrossCompileInfo : AmdExtD3DCreateInfo
{
    const void* pBlob;              ///< Pointer to the input blob
    size_t      blobSizeInBytes;    ///< Size of the blob
                                    ///< Set 0 to avoid extra copy for null terminated string
    struct
    {
        uint32_t dimx;
        uint32_t dimy;
        uint32_t dimz;
    } threadsPerGroup;
    AmdShaderType shType;
    const void*   pOptions;          ///< Pointer to build options
    size_t        optionSizeInBytes; ///< Size of build option array
                                     ///< Set 0 to avoid extra copy for null terminated string
    const char*   pKernelName;       ///< Pointer to kernel name
};

/**
***********************************************************************************************************************
* @brief bitfield of extension features supported by the current driver/hardware
***********************************************************************************************************************
*/
struct AmdExtD3DCheckFeatureSupportFlags
{
    union
    {
        struct
        {
            uint32_t depthBoundsTest                : 1;
            uint32_t abortCreateIfPipelineNotCached : 1;
            uint32_t rectangleListPrimitive         : 1;
            uint32_t pipelinePalElf                 : 1;
            uint32_t pipelineHsaElf                 : 1;
            uint32_t reserved0                      : 1;
            uint32_t pipelineCrossCompileHip        : 1;
            uint32_t pipelineCrossCompileOcl        : 1;
            uint32_t pipelineCrossCompileHlsl       : 1;
            uint32_t pipelineCrossCompileElfHsa     : 1;
            uint32_t waveMatrixSupported            : 1;
            uint32_t reserved                       : 21;
        };
        uint32_t all;
    };
};

/**
***********************************************************************************************************************
* @brief extension features that can be checked if the current driver/hardware supports
***********************************************************************************************************************
*/
enum class AmdExtD3DCheckFeatureSupportType : uint32_t
{
    Flags
};

/**
***********************************************************************************************************************
* @brief AMD specific primitive topology enumeration that includes extension types
***********************************************************************************************************************
*/
enum AmdExtD3DPrimitiveTopology : uint32_t
{
    AmdExtD3DPrimitiveTopologyUndefined      = 0,
    AmdExtD3DPrimitiveTopologyRectangleList  = 1,
};

/**
***********************************************************************************************************************
* @brief AmdExtD3DGpuRtVersion structure for GPURT version.
***********************************************************************************************************************
*/
struct AmdExtD3DGpuRtVersion
{
    uint16_t major;  ///< Major version, incremented for breaking changes
    uint16_t minor;  ///< Minor version, incremented for non-breaking changes
};

/**
***********************************************************************************************************************
* @brief Properties structure for IAmdExtD3DDevice8::GetWaveMatrixProperties()
***********************************************************************************************************************
*/
struct AmdExtWaveMatrixProperties
{
    enum class Type : uint32_t
    {
        float16,
        float32,
        float64,
        sint8,
        sint16,
        sint32,
        sint64,
        uint8,
        uint16,
        uint32,
        uint64,
        fp8,
        bf8,
    };

    size_t mSize;
    size_t nSize;
    size_t kSize;
    Type   aType;
    Type   bType;
    Type   cType;
    Type   resultType;
    bool   saturatingAccumulation;
};

/**
***********************************************************************************************************************
* @brief Extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("8104C0FC-7413-410F-8E83-AA617E908648")
IAmdExtD3DDevice : public IUnknown
{
public:
    virtual HRESULT CreateGraphicsPipelineState(
        const AmdExtD3DCreateInfo*                  pAmdExtCreateInfo,
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC*   pDesc,
        REFIID                                      riid,
        void**                                      ppPipelineState) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 1 extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("4BBCAF68-EAF7-4FA4-B653-CB458C334A4E")
IAmdExtD3DDevice1 : public IAmdExtD3DDevice
{
public:
    virtual void PushMarker(ID3D12GraphicsCommandList* pGfxCmdList, const char* pMarkerData) = 0;
    virtual void PopMarker(ID3D12GraphicsCommandList* pGfxCmdList) = 0;
    virtual void SetMarker(ID3D12GraphicsCommandList* pGfxCmdList, const char* pMarkerData) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 2 extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("A7BECF5D-2930-4FDA-8EEE-C797D8A52B7E")
    IAmdExtD3DDevice2 : public IAmdExtD3DDevice1
{
public:
    virtual HRESULT CheckExtFeatureSupport(AmdExtD3DCheckFeatureSupportType featureType,
                                           void*                            pFeatureData,
                                           size_t                           featureDataSize) const = 0;
    virtual HRESULT CreateComputePipelineState(const AmdExtD3DCreateInfo*               pAmdExtCreateInfo,
                                               const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
                                               REFIID                                   refiid,
                                               void**                                   ppPipelineState) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 3 extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("397E3533-111E-4A9D-A171-2BAE8EF6CB24")
    IAmdExtD3DDevice3 : public IAmdExtD3DDevice2
{
public:
    virtual HRESULT CreatePipelineState(const AmdExtD3DCreateInfo*              pAmdExtCreateInfo,
                                        const D3D12_PIPELINE_STATE_STREAM_DESC* pDesc,
                                        REFIID                                  riid,
                                        void**                                  ppPipelineState) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 4 extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("BE9A8C6A-868E-490D-8FBF-29DAC2650F3B")
    IAmdExtD3DDevice4 : public IAmdExtD3DDevice3
{
public:
    virtual void SetPrimitiveTopology(ID3D12GraphicsCommandList* pGfxCmdList, AmdExtD3DPrimitiveTopology topology) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 5 extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("BDC14598-B7D2-4A8D-9CA5-67848E2AF745")
    IAmdExtD3DDevice5 : public IAmdExtD3DDevice4
{
public:
    virtual HRESULT CreateComputePipelineFromElf(
        AmdExtD3DPipelineElfInfo* pAmdExtCreateInfo,
        REFIID                    refiid,
        void**                    ppPipelineState) = 0;

    virtual void SetKernelArguments(
        ID3D12GraphicsCommandList* pCmdList,
        uint32_t                   firstArg,
        uint32_t                   argCount,
        const void* const*         ppValues) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 6 extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("F764A768-48B4-46A5-9779-928ED6896D2A")
    IAmdExtD3DDevice6 : public IAmdExtD3DDevice5
{
public:
    // Gets interface version of GPURT algorithms
    virtual void GetGpuRtInterfaceVersion(AmdExtD3DGpuRtVersion* pInterfaceVersion) = 0;

    // Gets binary version of GPURT data structures
    virtual void GetGpuRtBinaryVersion(AmdExtD3DGpuRtVersion* pBinaryVersion) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 7 extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("FEE37AFC-3C50-4ABF-86CC-1622349B29C0")
  IAmdExtD3DDevice7 : public IAmdExtD3DDevice6
{
public:
    // AMD cross compiler extension, compiling HIP, OCL shaders for DX12
    virtual HRESULT CreateComputePipelineCrossCompile(
        const AmdExtD3DPipelineCrossCompileInfo* pAmdExtCreateInfo,
        REFIID                                   refiid,
        void**                                   ppPipelineState) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 8 extension device API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("F714E11A-B54E-4E0F-ABC5-DF58B18133D1")
IAmdExtD3DDevice8 : public IAmdExtD3DDevice7
{
public:
    // Query feature support for Wave Matrix intrinsic extension
    virtual HRESULT GetWaveMatrixProperties(
        size_t * pCount,
        AmdExtWaveMatrixProperties * pProperties) = 0;
};

/**
***********************************************************************************************************************
* @brief Most recent extension device API object
***********************************************************************************************************************
*/
using IAmdExtD3DDeviceLatest = IAmdExtD3DDevice8;
