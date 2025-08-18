// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
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

#include "../../api/internal/ffx_provider.h"
#include "../../api/internal/ffx_provider_external.h"
#include "../fsr3/include/ffx_provider_fsr2.h"
#include "../fsr3/include/ffx_provider_fsr3upscale.h"
#include "../fsr4/include/ffx_provider_fsr4.h"

#include <array>
#include <optional>
#include <d3d12.h>

static constexpr ffxProvider* providers[] = {
    &ffxProvider_FSR4::Instance,
    &ffxProvider_FSR3Upscale::Instance,
    &ffxProvider_FSR2::Instance,};
static constexpr size_t providerCount = _countof(providers);

static std::array<std::optional<ffxProviderExternal>, 10> externalProviders = {};

MIDL_INTERFACE("b58d6601-7401-4234-8180-6febfc0e484c")
IAmdExtFfxApi : public IUnknown
{
public:
    // Update FFX API provider
    virtual HRESULT UpdateFfxApiProvider(void* pData, uint32_t dataSizeInBytes) = 0;
};

struct ExternalProviderData
{
    uint32_t structVersion = 2;
    uint64_t descType;
    ffxProviderInterface provider;
};
#define FFX_EXTERNAL_PROVIDER_STRUCT_VERSION 2u

void GetExternalProviders(ID3D12Device* device, uint64_t descType)
{
    static IAmdExtFfxApi* apiExtension = nullptr;

    if (nullptr != device)
    {
        static bool ranOnce = false;
        if (!ranOnce)
        {
            ranOnce = true;
            static HMODULE hModule = GetModuleHandleA("amdxc64.dll");

            if (device && hModule && !apiExtension)
            {
                typedef HRESULT(__cdecl * PFNAmdExtD3DCreateInterface)(IUnknown * pOuter, REFIID riid, void** ppvObject);
                PFNAmdExtD3DCreateInterface AmdExtD3DCreateInterface =
                    static_cast<PFNAmdExtD3DCreateInterface>((VOID*)GetProcAddress(hModule, "AmdExtD3DCreateInterface"));
                if (AmdExtD3DCreateInterface)
                {
                    HRESULT hr = AmdExtD3DCreateInterface(device, IID_PPV_ARGS(&apiExtension));

                    if (hr != S_OK)
                    {
                        if (apiExtension)
                            apiExtension->Release();
                        apiExtension = nullptr;
                    }
                }
            }
        }
    }

    if (apiExtension)
    {
        ExternalProviderData data = {0};
        data.structVersion = FFX_EXTERNAL_PROVIDER_STRUCT_VERSION;
        data.descType = descType;
        memset(&data.provider, 0, sizeof(ffxProviderInterface));
        ffxProviderInterface nullProvider;
        memset(&nullProvider, 0, sizeof(ffxProviderInterface));
        HRESULT hr = apiExtension->UpdateFfxApiProvider(&data, sizeof(data));
        if (hr != S_OK)
            return;
        if (memcmp(&data.provider, &nullProvider, sizeof(ffxProviderInterface)) == 0)
            return;

        for (auto& slot : externalProviders)
        {
            if (slot.has_value() && slot->GetId() == data.provider.versionId)
            {
                // we already have this provider saved.
                break;
            }
            if (!slot.has_value())
            {
                // first free slot. slots are filled start to end and never released.
                // we do not have this provider yet, add it to the list.
                slot = ffxProviderExternal{data.provider};
                break;
            }
        }
    }
    
}

const ffxProvider* GetffxProvider(ffxStructType_t descType, uint64_t overrideId, void* device)
{
    // check driver-side providers
    GetExternalProviders(reinterpret_cast<ID3D12Device*>(device), descType);

    // If we are overriding, do not make the best provider choice decision.
    // Also assume that the version id comes from a previous version enumeration call, meaning assume IsSupported returns true on this device.
    // This is neccessary to support queries with a null-context, where device will be null (none of those currently have a device feature requirement).
    if (overrideId)
    {
        for (const auto& provider : externalProviders)
        {
            if (provider.has_value())
            {
                if (provider->GetId() == overrideId && provider->CanProvide(descType))
                    return &*provider;
            }
        }

        for (size_t i = 0; i < providerCount; ++i)
        {
            if (providers[i]->GetId() == overrideId && providers[i]->CanProvide(descType))
                return providers[i];
        }
    }
    else
    {
        const ffxProvider* bestExternalProvider = nullptr;
        const ffxProvider* bestInternalProvider = nullptr;

        // we assume that external providers are vetted for hardware support by the driver, so no call to IsSupported.
        for (const auto& provider : externalProviders)
        {
            if (provider.has_value())
            {
                if (provider->CanProvide(descType))
                {
                    bestExternalProvider = &*provider;
                    break;
                }
            }
        }

        for (size_t i = 0; i < providerCount; ++i)
        {
            if (providers[i]->CanProvide(descType) && providers[i]->IsSupported(device))
            {
                bestInternalProvider = providers[i];
                break;
            }
        }

        if ((bestExternalProvider != nullptr) && (bestInternalProvider != nullptr))
        {
            // Do a version check, and take the newest version.
            // Id()s have lowest 31b valid - bit32 is reserved for "driver override" indication
            uint64_t extId = bestExternalProvider->GetId() & (0x7FFFFFFFu);
            uint64_t intId = bestInternalProvider->GetId() & (0x7FFFFFFFu);

            if (extId > intId)
            {
                return bestExternalProvider;
            }
        }

        return bestInternalProvider;
    }

    return nullptr;
}

const ffxProvider* GetAssociatedProvider(ffxContext* context)
{
    const InternalContextHeader* hdr = (const InternalContextHeader*)(*context);
    const ffxProvider*        provider = hdr->provider;
    return provider;
}

uint64_t GetProviderCount(ffxStructType_t descType, void* device)
{
    return GetProviderVersions(descType, device, UINT64_MAX, nullptr, nullptr);
}

uint64_t GetProviderVersions(ffxStructType_t descType, void* device, uint64_t capacity, uint64_t* versionIds, const char** versionNames)
{
    uint64_t count = 0;

    // check driver-side providers
    GetExternalProviders(reinterpret_cast<ID3D12Device*>(device), descType);

    for (const auto& provider : externalProviders)
    {
        if (count >= capacity) break;
        if (provider.has_value() && provider->CanProvide(descType))
        {
            auto index = count;
            count++;
            if (versionIds)
                versionIds[index] = provider->GetId();
            if (versionNames)
                versionNames[index] = provider->GetVersionName();
        }
    }

    for (size_t i = 0; i < providerCount; ++i)
    {
        if (count >= capacity) break;
        if (providers[i]->CanProvide(descType) && providers[i]->IsSupported(device))
        {
            auto index = count;
            count++;
            if (versionIds)
                versionIds[index] = providers[i]->GetId();
            if (versionNames)
                versionNames[index] = providers[i]->GetVersionName();
        }
    }

    // Sort the returned versions by version ids so newest version is at beginning of array.
    // If same ID, show the internal provider before driver provider.
    // Can't use std::sort to sort two arrays, so implemented as insertion sort
    // (most std implementations use this anyway for small arrays, also should be fast since versions are likely to be almost sorted).
    // Only sort if version ids array is requested. This can result in a different version order when called with only names than with only ids or with both.
    if (versionIds)
    {
        for (uint64_t i = 1; i < count; i++)
        {
            uint64_t j = i;
            while (j > 0 && (versionIds[j] & 0x7FFFFFFF) >= (versionIds[j - 1] & 0x7FFFFFFF))
            {
                if (((versionIds[j] & 0x7FFFFFFF) == (versionIds[j - 1] & 0x7FFFFFFF)) && (versionIds[j - 1] & 0x80000000u))
                {
                    //if the ids are equal, but the previous one is an driver provider, show the internal provider first
                    std::swap(versionIds[j], versionIds[j - 1]);
                    if (versionNames)
                        std::swap(versionNames[j], versionNames[j - 1]);
                }
                else
                {
                    // if the current version is newer than the previous one, swap them
                    std::swap(versionIds[j], versionIds[j - 1]);
                    if (versionNames)
                        std::swap(versionNames[j], versionNames[j - 1]);
                }
                j--;
            }
        }
    }

    return count;
}
