/* Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <d3d12.h>
#include <dxgi.h>
#include <unknwn.h>
#include "AmdExtD3D.h"

typedef void* AmdExtD3DDCapResHandle;
typedef void* AmdExtD3DPrimaryHandle;

/**
***********************************************************************************************************************
* @brief DirectCapture texture creation status
***********************************************************************************************************************
*/
enum class AmdExtD3DDCapStatus : uint32_t
{
    Success,                 ///< DirectCapture texture is available
    FailedProtectedContent,  ///< DirectCapture texture is unavailable because the attempt to
                             ///  disable protected content failed
    FailePreFlipInUse,       ///< DirectCapture pre-flip access is unavailable because another
                             ///  client is already using it
    FailedPostFlipInUse,     ///< DirectCapture post-flip access is unavailable because another
                             ///  client is already using it
    FailedInUse,             ///< DirectCapture access is unavailable because both pre-flip and post-flip are in use
    PreFlipNotSupported,     ///< DirectCapture pre-flip access is not supported since the fullscreen application's
                             ///  UMD, like OpenGL driver, does not support the required frame metadata controls
    InvalidFrameGenRatio,    ///< An invalid frame generation ratio was provided
    InvalidFrameGenUmd,      ///< Fullscreen application is using a UMD that isn't supported for frame generation
};

/**
***********************************************************************************************************************
* @brief DirectCapture frame generation ratio
***********************************************************************************************************************
*/
enum class AmdExtD3DFrameGenRatio : uint32_t
{
    Unsupported,  ///< No frame generation supported
    OneFrame,     ///< One frame can be generated for a given primary surface
    TwoFrames,    ///< Two frames can be generated for a given primary surface
    ThreeFrames,  ///< Three frames can be generated for a given primary surface
};

/**
***********************************************************************************************************************
* @brief DirectCapture frame generation index
***********************************************************************************************************************
*/
enum class AmdExtD3DFrameGenIndex : uint32_t
{
    OriginalFrame,       ///< The private primary of the original game frame
    GeneratedFrameOne,   ///< The first generated frame
    GeneratedFrameTwo,   ///< The second generated frame
    GeneratedFrameThree, ///< The third generated frame
};

/**
***********************************************************************************************************************
* @brief DirectCapture resource usage type
***********************************************************************************************************************
*/
enum class AmdExtD3DDCapResUsageType : uint32_t
{
    FmfFallbackStatus, ///< FMF fallback status resource from RSX
    MaxUsageType,      ///< Max resource uage type
};

/**
***********************************************************************************************************************
* @brief DirectCapture surface description
***********************************************************************************************************************
*/
struct AmdExtD3DDCapSurfaceDesc
{
    IDXGIOutput*       pOutput;               ///< [in] IDXGIOutput object to indicate which screen to capture
    uint32_t           resFlags;              ///< [in] Combinations of D3D12_RESOURCE_FLAGS value
    union
    {
        struct
        {
            uint32_t  preflip            :  1;     ///< [in] Need pre-flip access
            uint32_t  postflip           :  1;     ///< [in] Need post-flip access
            uint32_t  needAccessDesktop  :  1;     ///< [in] Need to access desktop
            uint32_t  shared             :  1;     ///< [in] Need to share the surface
            uint32_t  paceGeneratedFrame :  1;     ///< [in] Need to pace the generated frames
            uint32_t  reserved           : 27;
        };
        uint32_t      u32All;
    } usageFlags;
    union
    {
        HANDLE      hPreFlipNewFrameEvent;    ///< [in] The event handle for a new frame available for pre-flip access
        HANDLE      hPostFlipNewFrameEvent;   ///< [in] The event handle for a new frame available for post-flip access
    };
    uint32_t               screenId;          ///< [out] The screen ID corresponding to the
                                              ///        IDXGIOutput. To be used in other APIs.
    AmdExtD3DDCapStatus    status;            ///< [out] The DirectCapture texture creation status
    AmdExtD3DFrameGenRatio frameGenRatio;     ///< [in] Frame generation ratio
    HANDLE                 hFatalErrorEvent;  ///< [in] The event handle signaled by KMD of an unrecoverable error.
                                              ///       The captuer app must destroy the current capture session
    uint32_t               reserved[5];
};

/**
***********************************************************************************************************************
* @brief DirectCapture texture HDR metadata
***********************************************************************************************************************
*/
struct AmdExtD3DDCapHdrMetadata
{
    union
    {
        struct
        {
            uint32_t   isLimitedRange : 1;      ///< [out] 1 means the colorspace is studio
            uint32_t   reserved       : 31;
        };
        uint32_t       u32All;
    } flags;
    uint32_t transferFunction;          ///< DAL transfer function
    uint32_t colorSpace;                ///< DAL color space
    uint32_t redPrimaryX;               ///< X chromaticity coordinates of the red value
    uint32_t redPrimaryY;               ///< Y chromaticity coordinates of the red value
    uint32_t greenPrimaryX;             ///< X chromaticity coordinates of the green value
    uint32_t greenPrimaryY;             ///< Y chromaticity coordinates of the green value
    uint32_t bluePrimaryX;              ///< X chromaticity coordinates of the blue value
    uint32_t bluePrimaryY;              ///< Y chromaticity coordinates of the blue value
    uint32_t whitePointX;               ///< X chromaticity coordinates of the white point
    uint32_t whitePointY;               ///< Y chromaticity coordinates of the white point
    uint32_t maxMasteringLuminance;     ///< Maximum number of nites of the display
    uint32_t minMasteringLuminance;     ///< Minimum number of nites of the display
    uint32_t maxContentLightLevel;      ///< Maximum content light level
    uint32_t maxFrameAverageLightLevel; ///< Maximum frame average light level
    uint32_t reserved[17];              ///< Reserved for future extension
};

/**
***********************************************************************************************************************
* @brief DirectCapture texture creation status
***********************************************************************************************************************
*/
enum class AmdExtD3DDCapFrameInfoStatus : uint32_t
{
    Success,                       ///< On-screen frame info is available
    FailedHdrQuery,                ///< KMD failed to retrieve HDR metadata for the frame
    FailedFatalConsecutiveTimeout, ///< Fatal Error: Application failed to respond to work beginevents. Capture must
                                   ///  be terminated
    FailedAbortFrame,              ///< Error: KMD is unable to process the escape for this frame. The capture process
                                   ///  should release the frame and reenter wait state.
};

/**
***********************************************************************************************************************
* @brief DirectCapture frame information
***********************************************************************************************************************
*/
struct AmdExtD3DDCapFrameInfo
{
    union
    {
        struct
        {
            uint32_t  queryDisplayInfo              : 1;  ///< [in] Query display info
            uint32_t  surfPropsChanged              : 1;  ///< [out] On-screen surface properties changed
            uint32_t  frameIndexValid               : 1;  ///< [out] Frame index is valid
            uint32_t  frameFlipTimeStampValid       : 1;  ///< [out] Frame flip timestamp is valid

            uint32_t  noAccessDueToProtectedContent : 1;  ///< [out] Cannot access the on-screen surface due to the
                                                          ///        protected content
            uint32_t  isDesktop                     : 1;  ///< [out] On-screen surface is desktop
            uint32_t  isMpoActive                   : 1;  ///< [out] On-screen surface is MPO active
            uint32_t  reserved                      : 25;
        };
        uint32_t      u32All;
    } flags;
    uint32_t                     dirtyRectsCount;    ///< [in] Number of elements in the pDirtyRectsArray
                                                     ///  [out] Actual number of dirty rectangles returned from driver
    RECT                         srcRect;            ///< [out] Source rect for cropping
    RECT                         dstRect;            ///< [out] Destination rect for cropping
    RECT                         clipRect;           ///< [out] Clip rect for cropping
    uint64_t                     frameIndex;         ///< [out] Frame index
    uint64_t                     frameFlipTimeStamp; ///< [out] Frame flip timestamp
    AmdExtD3DDCapHdrMetadata     hdrMetadata;        ///< [out] HDR metadata
    AmdExtD3DDCapFrameInfoStatus status;             ///< [out] Get on-screen frame info status
    uint32_t                     vsyncInterval;      ///< [out] Vsync interval
    uint32_t                     reserved[12];       ///< Reserved for future extension
    RECT*                        pDirtyRectsArray;   ///< [in] A RECT array to store the dirty rects
};

/**
***********************************************************************************************************************
* @brief DirectCapture surface access type
***********************************************************************************************************************
*/
enum class AmdExtD3DDCapSurfAccessType : uint32_t
{
    AccessTypePreflip,         ///< DirectCapture surface will be pre-flip accessed
    AccessTypePostflip,        ///< DirectCapture surface will be post-flip accessed
};

/**
***********************************************************************************************************************
* @brief DirectCapture RSync control information
***********************************************************************************************************************
*/
struct AmdExtD3DDCapRSyncControl
{
    union
    {
        struct
        {
            uint32_t   enableRSync  : 1;  ///< [in] Enable RSync feature
            uint32_t   disableRSync : 1;  ///< [in] Disable RSync feature
            uint32_t   setMaxFps    : 1;  ///< [in] Set RSync maximum frame rate
            uint32_t   reserved     : 29;
        };
        uint32_t       u32All;
    } flags;
    uint32_t           maxFps;            ///< [in] RSync maximum frame rate
};

/**
***********************************************************************************************************************
* @brief DirectCapture Post-flip access status
***********************************************************************************************************************
*/
enum class AmdExtD3DDCapPostFlipAccessStatus : uint32_t
{
    Success,                       ///< Acquire screen success
    FailedSubmissionInProgress,    ///< KMD failed to acquire screen since capture work is still in progress
    FailedMetadataNotReady,        ///< KMD failed to acquire screen since frame metadata is not ready yet
    FailedUnknown,                 ///< KMD failed to acquire screen for an unknown reason
};

/**
***********************************************************************************************************************
* @brief DirectCapture Post-flip access information
***********************************************************************************************************************
*/
struct AmdExtD3DDCapPostFlipAccessInfo
{
    HANDLE                            hPostFlipEvent; ///< [in] The post-flip event handle
    AmdExtD3DDCapPostFlipAccessStatus status;         ///< [out] Get post-flip access status
};

/**
***********************************************************************************************************************
* @brief Private Flip information
***********************************************************************************************************************
*/
struct AmdExtD3DDCapPrivFlip
{
    union
    {
        struct
        {
            uint32_t        noFlip   : 1;         ///< [in] No flip when submission completes
            uint32_t        reserved : 31;
        };
        uint32_t            u32All;
    } flags;
    AmdExtD3DFrameGenIndex  frameGenerationIndex; ///< [in] Index of the generated frames
    AmdExtD3DPrimaryHandle  hFlipSurface;         ///< [in] Private primary surface handle to flip to
    uint32_t                reserved[28];
};

/**
***********************************************************************************************************************
* @brief Disable capture arguments
***********************************************************************************************************************
*/
struct AmdExtD3DDCapDisableCaptureArg
{
    union
    {
        struct
        {
            uint32_t  preflip  : 1;     ///< [in] Disable preflip access
            uint32_t  postflip : 1;     ///< [in] Disable postflip access
            uint32_t  reserved : 30;
        };
        uint32_t      u32All;
    } usageFlags;
    uint32_t          screenId;         ///< [in] The screen ID of the captured screen
    uint32_t          reserved[6];
};

/**
***********************************************************************************************************************
* @brief DirectCapture command list extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("54fb7a76-24a7-4f27-adc4-947c2a3f5664"))
IAmdExtD3DDCapCommandList : public IUnknown
{
public:
    // Notify driver about the begin of the DirectCapture surface access
    virtual HRESULT BeginAccess(uint32_t                    screenId,
                                AmdExtD3DDCapResHandle      hDCapRes,
                                AmdExtD3DDCapSurfAccessType type,
                                uint64_t                    frameIndex) = 0;

    // Notify driver about the end of access end and the flip primary surface
    virtual HRESULT EndAccess(uint32_t                screenId,
                              AmdExtD3DPrimaryHandle  hFlipSurface) = 0;

    // Discard DirectCapture texture access request
    virtual HRESULT DiscardAccessRequest(uint32_t                    screenId,
                                         AmdExtD3DDCapResHandle      hDCapRes,
                                         AmdExtD3DDCapSurfAccessType type) = 0;

    // Reset DirectCapture command list
    virtual HRESULT Reset(ID3D12CommandAllocator* pAllocator,
                          ID3D12PipelineState*    pPipelineState) = 0;

};

/**
***********************************************************************************************************************
* @brief Version 1 DirectCapture command list extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("63805c3e-88ef-4e37-a9b2-5ead625c0cc9"))
    IAmdExtD3DDCapCommandList1 : public IAmdExtD3DDCapCommandList
{
public:
    // Version 1 discard DirectCapture texture access request
    virtual HRESULT DiscardAccessRequest1(uint32_t                    screenId,
                                          AmdExtD3DDCapResHandle      hDCapRes,
                                          AmdExtD3DDCapSurfAccessType type,
                                          uint64_t                    frameIndex) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 2 DirectCapture command list extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("c7db2ae9-cacd-42b2-b0a5-0e39070b690a"))
IAmdExtD3DDCapCommandList2 : public IAmdExtD3DDCapCommandList1
{
public:
    // Version 1 access end and the flip primary information
    virtual HRESULT EndAccess1(uint32_t               screenId,
                               AmdExtD3DDCapPrivFlip* pPrivFlip) = 0;

    // Notify the driver there is a private flip after the command list submission
    virtual HRESULT PrivFlip(uint32_t               screenId,
                             uint64_t               frameIndex,
                             AmdExtD3DDCapPrivFlip* pPrivFlip) = 0;
};

/**
***********************************************************************************************************************
* @brief Most recent DirectCapture command list extension API object
***********************************************************************************************************************
*/
using IAmdExtD3DDCapCommandListLatest = IAmdExtD3DDCapCommandList2;

/**
***********************************************************************************************************************
* @brief DirectCapture device extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("7b2ab727-336c-446a-80d0-f7bc0cd06b9f"))
IAmdExtD3DDCapDevice : public IUnknown
{
public:
    // Create a private primary surface without going through DXGI fullscreen swapchain
    virtual HRESULT CreatePrimarySurface(const D3D12_HEAP_PROPERTIES* pHeapProperties,
                                         D3D12_HEAP_FLAGS             HeapFlags,
                                         const D3D12_RESOURCE_DESC*   pDesc,
                                         D3D12_RESOURCE_STATES        InitialResourceState,
                                         const D3D12_CLEAR_VALUE*     pOptimizedClearValue,
                                         REFIID                       riidResource,
                                         void**                       ppvResource,
                                         AmdExtD3DPrimaryHandle*      pPrimaryHandle) = 0;

    // Create DirectCapture texture and notify KMD to trigger the post-processing
    virtual HRESULT CreateDirectCaptureTexture(AmdExtD3DDCapSurfaceDesc* pDesc,
                                               ID3D12Resource**          ppTexture,
                                               AmdExtD3DDCapResHandle*   pResHandle) = 0;

    // Notify the driver that post-flip access is required
    virtual HRESULT NotifyPostFlipAccess(uint32_t screenId,
                                         HANDLE   hPostFlipEvent) = 0;

    // Check whether the on-screen buffer properties changed
    virtual HRESULT GetFrameInfo(uint32_t                screenId,
                                 AmdExtD3DDCapFrameInfo* pFrameInfo) = 0;

    // Create a DCapCommandList with the pointer to ID3D12GraphicsCommandList
    virtual HRESULT CreateDCapCommandList(ID3D12CommandList*          pD3D12CommandList,
                                          IAmdExtD3DDCapCommandList** ppDCapCommandList) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 1 DirectCapture device extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("691e93eb-e35c-4508-bbed-27314bb6922f"))
IAmdExtD3DDCapDevice1 : public IAmdExtD3DDCapDevice
{
public:
    // Set RSync control
    virtual HRESULT SetRSyncControl(AmdExtD3DDCapRSyncControl* pRSyncControl) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 2 DirectCapture device extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("b05c1950-4bdf-49b6-b990-a5d0583b22dd"))
IAmdExtD3DDCapDevice2 : public IAmdExtD3DDCapDevice1
{
public:
    virtual HRESULT NotifyPostFlipAccess1(uint32_t screenId, AmdExtD3DDCapPostFlipAccessInfo* pPostFlipAccessInfo) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 3 DirectCapture device extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("675b60b1-1a67-4096-b2be-de59ef8c05aa"))
    IAmdExtD3DDCapDevice3 : public IAmdExtD3DDCapDevice2
{
public:
    // Query the supported frame generation ratio
    virtual AmdExtD3DFrameGenRatio QueryFrameGenerationSupport() = 0;
};

/**
***********************************************************************************************************************
* @brief Version 4 DirectCapture device extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("9ce88076-9964-4d3a-8e08-bddf8ecdaef7"))
    IAmdExtD3DDCapDevice4 : public IAmdExtD3DDCapDevice3
{
public:
    // Disable direct capture
    virtual HRESULT DisableCapture(AmdExtD3DDCapDisableCaptureArg* pArg) = 0;
};

/**
***********************************************************************************************************************
* @brief Version 5 DirectCapture device extension API object
***********************************************************************************************************************
*/
interface __declspec(uuid("3a152494-ca0b-4b66-8709-c56955649b27"))
    IAmdExtD3DDCapDevice5 : public IAmdExtD3DDCapDevice4
{
public:
    // Resource Usage notification from RSX to driver
    virtual HRESULT NotifyResUsage(UINT                        screenId,
                                   ID3D12Resource*             pResource,
                                   AmdExtD3DDCapResUsageType   type) = 0;
};

/**
***********************************************************************************************************************
* @brief Most recent DirectCapture extension device API object
***********************************************************************************************************************
*/
using IAmdExtD3DDCapDeviceLatest = IAmdExtD3DDCapDevice5;
