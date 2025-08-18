#include <d3d12.h>

#include "AmdExtD3D/AmdExtD3D.h"
#include "AmdExtD3D/AmdExtD3DDeviceApi.h"
#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsApi.h"

bool EnableAMDExtensions(void* pvDevice)
{
    ID3D12Device* device = reinterpret_cast<ID3D12Device*>(pvDevice);

    IAmdExtD3DFactory* pAmdExtD3DFactory = nullptr;
    HRESULT            hr = S_OK;

    // Create extension factory
    {
        PFNAmdExtD3DCreateInterface pfnAmdExtD3DCreateInterface =
            reinterpret_cast<PFNAmdExtD3DCreateInterface>(GetProcAddress(GetModuleHandleW(L"amdxc64.dll"), "AmdExtD3DCreateInterface"));
        if (pfnAmdExtD3DCreateInterface == nullptr)
        {
            return false;
        }
        hr = pfnAmdExtD3DCreateInterface(device, IID_PPV_ARGS(&pAmdExtD3DFactory));
        if (!SUCCEEDED(hr))
        {
            if (pAmdExtD3DFactory)
            {
                pAmdExtD3DFactory->Release();
            }
            return false;
        }
    }

    // Create extension device
    IAmdExtD3DDevice8* pAmdExtDevice = nullptr;
    {
        hr = pAmdExtD3DFactory->CreateInterface(device, IID_PPV_ARGS(&pAmdExtDevice));
        if (!SUCCEEDED(hr) || !pAmdExtDevice)
        {
            if (pAmdExtDevice)
            {
                pAmdExtDevice->Release();
            }
            if (pAmdExtD3DFactory)
            {
                pAmdExtD3DFactory->Release();
            }
            return false;
        }
    }

    // check wave-matrix support for fp8
    // Driver will currently return 14 properties, of which the last 4 are fp8, if supported.
    // Hopefully this remains the case with future drivers.
    // Note: cannot do this properly because we must not allocate heap memory.
    AmdExtWaveMatrixProperties propertiesArray[14];
    size_t count = _countof(propertiesArray);
    pAmdExtDevice->GetWaveMatrixProperties(&count, propertiesArray);

    constexpr auto f8 = AmdExtWaveMatrixProperties::Type::fp8;
    constexpr auto f32 = AmdExtWaveMatrixProperties::Type::float32;
    AmdExtWaveMatrixProperties target = { 16, 16, 16, f8, f8, f32, f32, false };

    bool fp8Found = false;
    for (size_t i = 0; i < count; i++)
    {
        auto& p = propertiesArray[i];
        if (p.mSize == target.mSize && p.nSize == target.nSize && p.kSize == target.kSize && p.aType == target.aType && p.bType == target.bType && p.cType == target.cType && p.resultType == target.resultType && p.saturatingAccumulation == target.saturatingAccumulation)
        {
            fp8Found = true;
            break;
        }
    }
    
    pAmdExtDevice->Release();

    if (!fp8Found)
    {
        pAmdExtD3DFactory->Release();
        return false;
    }

    //Enable required extensions
    IAmdExtD3DShaderIntrinsics* pAmdExtD3DShaderIntrinsics = nullptr;
    hr = pAmdExtD3DFactory->CreateInterface(device, IID_PPV_ARGS(&pAmdExtD3DShaderIntrinsics));
    if (pAmdExtD3DFactory != nullptr)
    {
        pAmdExtD3DFactory->Release();
    }
    if (SUCCEEDED(hr))
    {
        hr = pAmdExtD3DShaderIntrinsics->CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix);
    }
    if (SUCCEEDED(hr))
    {
        hr = pAmdExtD3DShaderIntrinsics->CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Float8Conversion);
    }
    if (SUCCEEDED(hr))
    {
        hr = pAmdExtD3DShaderIntrinsics->Enable();
    }

    if (pAmdExtD3DShaderIntrinsics)
    {
        pAmdExtD3DShaderIntrinsics->Release();
    }

    return SUCCEEDED(hr);
}
