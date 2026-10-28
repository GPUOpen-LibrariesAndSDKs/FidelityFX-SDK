// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
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

#pragma once
#include "../include/ffx_api.hpp"
#include "../include/ffx_api_types.h"
#include "ffx_internal_types.h"
#include "ffx_api_helper.h"

#include <optional>
#include <span>

class ffxProvider
{
public:
    ffxProvider(uint64_t i_id, uint64_t i_effect_id, const char* i_version_name)
        : RefCount(1), Id(i_id), EffectId(i_effect_id), VersionName(i_version_name)
    {
    }

    virtual ~ffxProvider() = default;

    size_t GetRefCount() const
    {
        return RefCount;
    }

    uint64_t GetId() const
    {
        return Id;
    }

    const char* GetVersionName() const
    {
        return VersionName;
    }

    virtual bool CanProvide(uint64_t descType) const
    {
        return (descType & FFX_API_EFFECT_MASK) == EffectId;
    }

    virtual bool IsSupported(void* device) const
    {
        (void)device;
        // most providers have no special device requirements
        return true;
    }

    virtual ffxReturnCode_t CreateContext(ffxContext* context, ffxCreateContextDescHeader* desc, Allocator& alloc) = 0;

    virtual ffxReturnCode_t DestroyContext(ffxContext* context, Allocator& alloc) = 0;

    virtual ffxReturnCode_t Configure(ffxContext* context, const ffxConfigureDescHeader* desc) const = 0;

    virtual ffxReturnCode_t Query(ffxContext* context, ffxQueryDescHeader* desc) const = 0;

    virtual ffxReturnCode_t Dispatch(ffxContext* context, const ffxDispatchDescHeader* desc) const = 0;

protected:
    size_t RefCount = 0;
    uint64_t Id;
    uint64_t EffectId;
    const char* VersionName;
};

struct InternalContextHeader
{
    ffxProvider* provider;
};

inline ffxProvider* GetAssociatedProvider(ffxContext context)
{
    return reinterpret_cast<InternalContextHeader*>(context)->provider;
}

ffxProvider* GetProvider(ffxStructType_t descType, uint64_t overrideId, void* device, std::optional<ffxProviderExternal>& extProviderSlot);

inline ffxProvider* GetProvider(ffxStructType_t descType, uint64_t overrideId, void* device, std::optional<ffxProviderExternal>& extProviderSlot, std::span<ffxProvider* const> providers)
{
#if defined(FFX_BACKEND_DX12) || defined(FFX_BACKEND_VK)
    // check driver-side provider
    extProviderSlot.emplace(device, descType);
#else
    // Unreferenced parameter
    (void)extProviderSlot;
#endif // defined(FFX_BACKEND_DX12) || defined(FFX_BACKEND_VK)

    // If we are overriding, do not make the best provider choice decision.
    // Also assume that the version id comes from a previous version enumeration call, meaning assume IsSupported returns true on this device.
    // This is necessary to support queries with a null-context, where device will be null (none of those currently have a device feature requirement).
    if (overrideId)
    {
        if (extProviderSlot && extProviderSlot->Valid())
        {
            if (extProviderSlot->GetId() == overrideId && extProviderSlot->CanProvide(descType) && extProviderSlot->IsSupported(device))
                return &*extProviderSlot;
        }

        for (auto provider : providers)
        {
            if (provider->GetId() == overrideId && provider->CanProvide(descType) && provider->IsSupported(device))
                return provider;
        }
    }
    else
    {
        ffxProvider* bestInternalProvider = nullptr;
        for (auto provider : providers)
        {
            if (provider->CanProvide(descType) && provider->IsSupported(device))
            {
                bestInternalProvider = provider;
                break;
            }
        }

        // we assume that external providers are vetted for hardware support by the driver, so no call to IsSupported.
        if (extProviderSlot && extProviderSlot->Valid() && extProviderSlot->CanProvide(descType) && extProviderSlot->IsSupported(device))
        {
            if (bestInternalProvider)
            {
                // Do a version check, and take the newest version.
                // Ids have lowest 31b valid - bit32 is reserved for "driver override" indication
                uint64_t extId = extProviderSlot->GetId() & (0x7FFFFFFFu);
                uint64_t intId = bestInternalProvider->GetId() & (0x7FFFFFFFu);

                if (extId > intId)
                {
                    return &*extProviderSlot;
                }
            }
            else
            {
                return &*extProviderSlot;
            }
        }

        return bestInternalProvider;
    }

    return nullptr;
}

uint64_t GetProviderVersions(ffxStructType_t descType, void* device, uint64_t capacity, uint64_t* versionIds, const char** versionNames, std::optional<ffxProviderExternal>& extProviderSlot);

inline uint64_t GetProviderVersions(ffxStructType_t descType, void* device, uint64_t capacity, uint64_t* versionIds, const char** versionNames, std::optional<ffxProviderExternal>& extProviderSlot, std::span<ffxProvider* const> providers)
{
    uint64_t count = 0;

    // check driver-side provider
#if defined(FFX_BACKEND_DX12) || defined(FFX_BACKEND_VK)
    // check driver-side provider
    extProviderSlot.emplace(device, descType);
#else
    // Unreferenced parameter
    (void)extProviderSlot;
#endif // defined(FFX_BACKEND_DX12) || defined(FFX_BACKEND_VK)

    static char extProviderNames[FFX_API_EFFECT_COUNT][64] = {};

    // In order to eliminate redundant driver versions of each effect, first gather what is in the Game SDK
    uint64_t* sdk_versions = (uint64_t*)alloca(providers.size() * sizeof(uint64_t));
    for (uint32_t index = 0; index < providers.size(); index++)
    {
        ffxProvider* provider = providers[index];
        if (provider->CanProvide(descType) && provider->IsSupported(device))
        {
            sdk_versions[index] = provider->GetId();
        }
        else
        {
            sdk_versions[index] = 0;
        }
    }

    if (count < capacity)
    {
        if (extProviderSlot->Valid() && extProviderSlot->CanProvide(descType) && extProviderSlot->IsSupported(device))
        {
            bool add_to_list = true;
            auto index = count;
            uint64_t version = extProviderSlot->GetId();

            // Only show driver versions that are:
            // - A different effect 
            // - A different major version
            // - The same effect & major version, but greater minor.patch.release
            // This reduces confusion for the user.
            for (uint32_t i = 0; i < providers.size(); i++)
            {
                if (sdk_versions[i])
                {
                    uint64_t ext_ident = (version & 0xFFFFFFFF00000000llu);
                    uint64_t sdk_ident = (sdk_versions[i] & 0xFFFFFFFF00000000llu);
                    uint32_t ext_major = ((version & 0x7FFFFFFF) >> 22);
                    uint32_t sdk_major = ((sdk_versions[i] & 0x7FFFFFFF) >> 22);
                    if ((ext_ident == sdk_ident) && (sdk_major == ext_major))
                    {
                        add_to_list = ((version & 0x7FFFFFFF) > (sdk_versions[i] & 0x7FFFFFFF));
                    }
                }
            }

            if (add_to_list)
            {
                count++;
                if (versionIds)
                    versionIds[index] = extProviderSlot->GetId();
                if (versionNames)
                {
                    // this needs to be copied. points into dll, which may not outlive the caller.
                    const uint32_t effectIndex = ((descType & FFX_API_EFFECT_MASK) >> 16u) - 1u;
                    char* extProviderName = extProviderNames[effectIndex];
                    strncpy_s(extProviderName, sizeof(extProviderNames[effectIndex]), extProviderSlot->GetVersionName(), _TRUNCATE);
                    versionNames[index] = extProviderName;
                }
            }
        }
    }

    for (auto provider : providers)
    {
        if (count >= capacity) break;
        if (provider->CanProvide(descType) && provider->IsSupported(device))
        {
            auto index = count;
            count++;
            if (versionIds)
                versionIds[index] = provider->GetId();
            if (versionNames)
                versionNames[index] = provider->GetVersionName();
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

inline uint64_t GetProviderCount(ffxStructType_t descType, void* device, std::optional<ffxProviderExternal>& extProviderSlot)
{
    return GetProviderVersions(descType, device, UINT64_MAX, nullptr, nullptr, extProviderSlot);
}
