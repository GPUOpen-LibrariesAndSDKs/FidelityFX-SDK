// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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

#include <FidelityFX/host/ffx_fsr1.h>
#include <FidelityFX/host/ffx_fsr3.h>
#include <FidelityFX/host/backends/dx12/d3dx12.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <codecvt>
#include <mutex>

#include "ShaderCompiler.h"
#include "HlslShaderCompiler.h"

namespace HlslShaderCompiler
{
    #define FFX_MAX_RESOURCE_IDENTIFIER_COUNT (128)

    std::vector<std::wstring> additionalIncludeDirs = {L"ffx_sdk\\fidelityfx-sdk-gpu\\"};

    const std::vector<std::wstring> getShaderPathFromEffectPassID(FfxEffect effectId, FfxPass passId)
    {
        if (effectId == FFX_EFFECT_FSR3UPSCALER)
        {
            const std::vector<std::wstring> shaderFilenames[FFX_FSR3UPSCALER_PASS_COUNT] = {
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_depth_clip_pass.hlsl"},
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_reconstruct_previous_depth_pass.hlsl"},
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_lock_pass.hlsl"},
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_accumulate_pass.hlsl"},
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_accumulate_pass.hlsl"},
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_rcas_pass.hlsl"},
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_compute_luminance_pyramid_pass.hlsl"},
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_autogen_reactive_pass.hlsl"},
                { L"ffx_sdk\\fsr3upscaler\\ffx_fsr3upscaler_tcr_autogen_pass.hlsl"},
            };

            return shaderFilenames[passId];
        }
        else if (effectId == FFX_EFFECT_OPTICALFLOW)
        {
            const std::vector<std::wstring> shaderFilenames[FFX_OPTICALFLOW_PASS_COUNT] = {
                { L"ffx_sdk\\opticalflow\\ffx_opticalflow_prepare_luma_pass.hlsl"},
                { L"ffx_sdk\\opticalflow\\ffx_opticalflow_compute_luminance_pyramid_pass.hlsl"},
                { L"ffx_sdk\\opticalflow\\ffx_opticalflow_generate_scd_histogram_pass.hlsl"},
                { L"ffx_sdk\\opticalflow\\ffx_opticalflow_compute_scd_divergence_pass.hlsl"},
                { L"ffx_sdk\\opticalflow\\ffx_opticalflow_compute_optical_flow_advanced_pass_v5.hlsl"},
                { L"ffx_sdk\\opticalflow\\ffx_opticalflow_filter_optical_flow_pass_v5.hlsl"},
                { L"ffx_sdk\\opticalflow\\ffx_opticalflow_scale_optical_flow_advanced_pass_v5.hlsl"},
            };

            return shaderFilenames[passId];
        }
        else if (effectId == FFX_EFFECT_FRAMEINTERPOLATION)
        {
            const std::vector<std::wstring> shaderFilenames[FFX_FRAMEINTERPOLATION_PASS_COUNT] = {
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_setup.hlsl"},
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_reconstruct_previous_depth.hlsl"},
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_game_motion_vector_field.hlsl"},
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_optical_flow_vector_field.hlsl"},
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_disocclusion_mask.hlsl"},
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_pass.hlsl"},
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_compute_inpainting_pyramid_pass.hlsl"},
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_inpainting_pass.hlsl"},
                { L"ffx_sdk\\frameinterpolation\\ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass.hlsl"},
            };

            return shaderFilenames[passId];
        }
        return {};
    }


    static std::map<std::wstring, std::wstring> s_shaderDefines;

    struct ShaderCompilationResult
    {
        IDxcBlob*                shaderBlob = nullptr;
        std::vector<uint32_t>    boundSamplers;
        std::vector<std::string> boundSamplersNames;

        std::vector<uint32_t>    boundRegistersUAVs;
        std::vector<std::string> boundRegistersUAVNames;
        std::vector<uint32_t>    boundRegistersUAVsCount;
        std::vector<uint32_t>    boundRegistersSRVs;
        std::vector<std::string> boundRegistersSRVNames;
        std::vector<uint32_t>    boundRegistersCBs;
        std::vector<std::string> boundRegistersCBNames;

        std::vector<uint32_t>    boundRegistersUAVBuffers;
        std::vector<std::string> boundRegistersUAVBuffersNames;
        std::vector<uint32_t>    boundRegistersSRVBuffers;
        std::vector<std::string> boundRegistersSRVBuffersNames;
        std::vector<uint32_t>    boundRegistersRTASs;
        std::vector<std::string> boundRegistersRTASsNames;
    };

    struct D3D12DeviceProperties
    {
        D3D_SHADER_MODEL HighestShaderModel;
        uint32_t         WaveLaneCountMin;
        uint32_t         WaveLaneCountMax;
        bool             HLSLWaveSizeSupported;
        bool             FP16Supported;
    };

    static FidelityFx::ShaderCompiler   s_dxilCompiler             = {};
    static wchar_t                      s_fullshaderCompilerPath[] = L"./dxcompiler.dll";
    static wchar_t                      s_fullshaderDXILPath[]     = L"./dxil.dll";
    static wchar_t                      s_fullShaderOutputPath[]   = L"./ShaderLibDX/FSR2.0/";
    static D3D12DeviceProperties        s_deviceProperties;
    static bool                         s_sharpenEnabled;
    static bool                         s_bUseHLSLShaders = true;
    static ID3D12Device*                s_dx12Device      = nullptr;

// Dev options
// !NOT FOR SHIPPING!
#define DECL_DEV_OPTION(VarName, ShaderDef, DefaultVal)           \
    static bool           s_##VarName               = DefaultVal; \
    static const wchar_t* s_##szShaderDef_##VarName = L#ShaderDef;

#define DECL_DEV_OPTION_TYPED(Type, VarName, ShaderDef, DefaultVal) \
    static Type           s_##VarName               = DefaultVal;   \
    static const wchar_t* s_##szShaderDef_##VarName = L#ShaderDef;

    DECL_DEV_OPTION(bOptionUpsampleSamplersUseDataHalf, FFX_FSR3UPSCALER_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF, false)
    DECL_DEV_OPTION(bOptionAccumulateSamplersUseDataHalf, FFX_FSR3UPSCALER_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF, false)
    DECL_DEV_OPTION(bOptionReprojectSamplersUseDataHalf, FFX_FSR3UPSCALER_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF, true)
    DECL_DEV_OPTION(bOptionPostprocessLockstatusSamplersUseDataHalf, FFX_FSR3UPSCALER_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF, false)

    enum LanczosType
    {
        LANCZOS_TYPE_REFERENCE,
        LANCZOS_TYPE_LUT,
        LANCZOS_TYPE_APPROXIMATE,
    };
    DECL_DEV_OPTION_TYPED(LanczosType, bOptionUpsampleUseLanczosType, FFX_FSR3UPSCALER_OPTION_UPSAMPLE_USE_LANCZOS_TYPE, LANCZOS_TYPE_APPROXIMATE);
    DECL_DEV_OPTION_TYPED(LanczosType, bOptionReprojectUseLanczosType, FFX_FSR3UPSCALER_OPTION_REPROJECT_USE_LANCZOS_TYPE, LANCZOS_TYPE_LUT);

    static void queryDeviceProperties(ID3D12Device* device)
    {
        // setup what we need for HLSL compilation
        memset(&s_deviceProperties, 0, sizeof(D3D12DeviceProperties));
        s_deviceProperties.HighestShaderModel       = D3D_SHADER_MODEL_6_2;
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {D3D_SHADER_MODEL_6_6};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL))))
        {
            s_deviceProperties.HighestShaderModel = shaderModel.HighestShaderModel;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS1 d3d12Options1 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &d3d12Options1, sizeof(d3d12Options1))))
        {
            s_deviceProperties.HLSLWaveSizeSupported = (s_deviceProperties.HighestShaderModel >= D3D_SHADER_MODEL_6_6) && d3d12Options1.WaveOps;

            s_deviceProperties.WaveLaneCountMin = d3d12Options1.WaveLaneCountMin;
            s_deviceProperties.WaveLaneCountMax = d3d12Options1.WaveLaneCountMax;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Options = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Options, sizeof(d3d12Options))))
        {
            s_deviceProperties.FP16Supported = !!(d3d12Options.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT);
        }
    }

    static void setupShaderCompiler(ID3D12Device* device)
    {
        s_dxilCompiler.init(s_fullshaderCompilerPath, s_fullshaderDXILPath);
    }

    typedef enum Fs3UpscalerShaderPermutationOptions
    {
        FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE       = (1 << 0),  ///< Off means reference, On means LUT
        FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT        = (1 << 1),  ///< Enables the HDR code path
        FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS = (1 << 2),  ///< Indicates low resolution motion vectors provided
        FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS  = (1 << 3),  ///< Indicates motion vectors were generated with jitter
        FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED         = (1 << 4),  ///< Indicates input resources were generated with inverted depth
        FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING      = (1 << 5),  ///< Enables a supplementary sharpening pass
        FSR3UPSCALER_SHADER_PERMUTATION_FORCE_WAVE64           = (1 << 6),  ///< doesn't map to a define, selects different table
        FSR3UPSCALER_SHADER_PERMUTATION_ALLOW_FP16             = (1 << 7),  ///< Enables fast math computations where possible
    } Fs3UpscalerShaderPermutationOptions;

    typedef enum FrameinterpolationShaderPermutationOptions
    {
        FRAMEINTERPOLATION_SHADER_PERMUTATION_USE_LANCZOS_TYPE       = (1 << 0),  ///< Off means reference, On means LUT
        FRAMEINTERPOLATION_SHADER_PERMUTATION_HDR_COLOR_INPUT        = (1 << 1),  ///< Enables the HDR code path
        FRAMEINTERPOLATION_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS = (1 << 2),  ///< Indicates low resolution motion vectors provided
        FRAMEINTERPOLATION_SHADER_PERMUTATION_JITTER_MOTION_VECTORS  = (1 << 3),  ///< Indicates motion vectors were generated with jitter
        FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED         = (1 << 4),  ///< Indicates input resources were generated with inverted depth
        FRAMEINTERPOLATION_SHADER_PERMUTATION_ENABLE_SHARPENING      = (1 << 5),  ///< Enables a supplementary sharpening pass
        FRAMEINTERPOLATION_SHADER_PERMUTATION_FORCE_WAVE64           = (1 << 6),  ///< doesn't map to a define, selects different table
        FRAMEINTERPOLATION_SHADER_PERMUTATION_ALLOW_FP16             = (1 << 7),  ///< Enables fast math computations where possible
    } FrameinterpolationShaderPermutationOptions;

    typedef enum OpticalflowShaderPermutationOptions
    {
        OPTICALFLOW_SHADER_PERMUTATION_FORCE_WAVE64 = (1 << 0),  ///< doesn't map to a define, selects different table
        OPTICALFLOW_SHADER_PERMUTATION_ALLOW_FP16   = (1 << 1),  ///< Enables fast math computations where possible
        OPTICALFLOW_HDR_COLOR_INPUT                 = (1 << 2),
    } OpticalflowShaderPermutationOptions;

    static void getShaderDefinesFromEffectPassID(FfxEffect effect, FfxPass pass, uint32_t permutationOptions)
    {
        s_deviceProperties.FP16Supported = true;

        // clear the list of defines.
        s_shaderDefines.clear();

        s_shaderDefines[L"FFX_GPU"]      = L"1";
        s_shaderDefines[L"FFX_HLSL"]     = L"1";
        s_shaderDefines[L"FFX_HLSL_6_2"] = L"1";
        
#define REG_OPT(x)      x ? L"1" : L"0";

        if (effect == FFX_EFFECT_FSR3UPSCALER)
        {
            bool enable_fp16 = s_deviceProperties.FP16Supported && (permutationOptions & FSR3UPSCALER_SHADER_PERMUTATION_ALLOW_FP16);
            switch (pass)
            {
            case FFX_FSR3UPSCALER_PASS_COMPUTE_LUMINANCE_PYRAMID:
            case FFX_FSR3UPSCALER_PASS_RCAS:
                enable_fp16 = false;
                break;
            }

            s_shaderDefines[L"FFX_HALF"]          = REG_OPT(enable_fp16);
            s_shaderDefines[L"FFX_PREFER_WAVE64"] = (permutationOptions & FSR3UPSCALER_SHADER_PERMUTATION_FORCE_WAVE64) ? L"[WaveSize(64)]" : L"";
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF"]              = L"0";
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF"]            = L"0";
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF"]             = L"1";
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF"] = L"0";
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_UPSAMPLE_USE_LANCZOS_TYPE"]                    = L"2";

            // Creation params
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_REPROJECT_USE_LANCZOS_TYPE"]    = REG_OPT(permutationOptions & FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_HDR_COLOR_INPUT"]               = REG_OPT(permutationOptions & FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_LOW_RESOLUTION_MOTION_VECTORS"] = REG_OPT(permutationOptions & FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_JITTERED_MOTION_VECTORS"]       = REG_OPT(permutationOptions & FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_INVERTED_DEPTH"]                = REG_OPT(permutationOptions & FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
            s_shaderDefines[L"FFX_FSR3UPSCALER_OPTION_APPLY_SHARPENING"]              = (pass == FFX_FSR3UPSCALER_PASS_ACCUMULATE_SHARPEN) ? L"1" : L"0";
          }
        else if (effect == FFX_EFFECT_FRAMEINTERPOLATION)
        {
            bool enable_fp16             = s_deviceProperties.FP16Supported && (permutationOptions & FRAMEINTERPOLATION_SHADER_PERMUTATION_ALLOW_FP16);
            s_shaderDefines[L"FFX_HALF"] = REG_OPT(enable_fp16);

            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF"]       = L"0";
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF"]     = L"0";
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF"]      = L"1";
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF"] = L"0";
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_UPSAMPLE_USE_LANCZOS_TYPE"]             = REG_OPT(permutationOptions & FRAMEINTERPOLATION_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_REPROJECT_USE_LANCZOS_TYPE"]            = REG_OPT(permutationOptions & FRAMEINTERPOLATION_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_HDR_COLOR_INPUT"]                       = REG_OPT(permutationOptions & FRAMEINTERPOLATION_SHADER_PERMUTATION_HDR_COLOR_INPUT);
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_LOW_RESOLUTION_MOTION_VECTORS"]         = REG_OPT(permutationOptions & FRAMEINTERPOLATION_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_JITTERED_MOTION_VECTORS"]               = REG_OPT(permutationOptions & FRAMEINTERPOLATION_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH"]                        = REG_OPT(permutationOptions & FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);
            s_shaderDefines[L"FFX_FRAMEINTERPOLATION_OPTION_APPLY_SHARPENING"]                      = REG_OPT(permutationOptions & FRAMEINTERPOLATION_SHADER_PERMUTATION_ENABLE_SHARPENING);
        }
        else if (effect == FFX_EFFECT_OPTICALFLOW)
        {
            bool enable_fp16 = s_deviceProperties.FP16Supported && (permutationOptions & OPTICALFLOW_SHADER_PERMUTATION_ALLOW_FP16);
            bool s_forceWave64 = (permutationOptions & OPTICALFLOW_SHADER_PERMUTATION_FORCE_WAVE64);
            if (s_forceWave64)
            {
                s_shaderDefines[L"FFX_PREFER_WAVE64"]             = L"[WaveSize(64)]";
                s_shaderDefines[L"FFX_OPTICALFLOW_PREFER_WAVE64"] = L"[WaveSize(64)]";
            }

            s_shaderDefines[L"FFX_HALF"]          = REG_OPT(enable_fp16);
        }
    }

    static ShaderCompilationResult compileShader(std::wstring const&                         path,
                                                 std::wstring const&                         entryPoint,
                                                 const std::map<std::wstring, std::wstring>& defines,
                                                 const std::vector<std::wstring>&            additionalIncludeDirs,
                                                 std::wstring const&                         profile = L"")
    {
        ShaderCompilationResult     compilationResult;
        FidelityFx::ShaderCompilationDesc shaderDesc;

        std::wstring fullshaderCompilerPath = L"dxcompiler.dll";
        std::wstring fullshaderDXILPath     = L"dxil.dll";
        s_dxilCompiler.init(fullshaderCompilerPath.c_str(), fullshaderDXILPath.c_str());

        // https://simoncoenen.com/blog/programming/graphics/DxcCompiling.html
        //shaderDesc.CompileArguments.push_back(L"-Gis");							// Force IEEE strictness
        shaderDesc.CompileArguments.push_back(L"-Wno-for-redefinition");  // Disable "for redefinition" warning
        shaderDesc.CompileArguments.push_back(L"-Wno-ambig-lit-shift");
        // shaderDesc.CompileArguments.push_back(L"-Wconversion");

        // Workaround: We want to keep an eye on warn_hlsl_implicit_vector_truncation, but it share the same toggle as the warn_hlsl_sema_minprecision_promotion and others.
        // For debug build, enabling -Wconversion but disabling "-enable-16bit-types" as it causes flooding minprecision_promotion warnings.
        //#ifndef _DEBUG
        shaderDesc.CompileArguments.push_back(L"-enable-16bit-types");  // Enable 16bit types and disable min precision types. Available in HLSL 2018 and shader model 6.2
        //#endif

        // need to hold ref to the strings
        std::vector<std::wstring> additionalIncludeDirsStrings;
        for (const auto& includeDir : additionalIncludeDirs)
        {
            additionalIncludeDirsStrings.push_back(std::wstring(L"-I") + includeDir);
        }

        for (const auto& includeDir : additionalIncludeDirsStrings)
        {
            shaderDesc.CompileArguments.push_back(includeDir.c_str());
        }

#if defined(_DEBUG) || defined(FFX_DEBUG_SHADERS)
        shaderDesc.CompileArguments.push_back(L"-Qembed_debug");  // Embed PDB in shader container (must be used with /Zi)
        shaderDesc.CompileArguments.push_back(L"-Zi");            // Enable debug information
#endif

        shaderDesc.EntryPoint    = entryPoint.c_str();
        shaderDesc.TargetProfile = profile.empty() ? L"cs_6_2" : profile.c_str();  // Shader model 6.2
        std::vector<DxcDefine> definesv;
        for (const auto& def : defines)
        {
            definesv.push_back({def.first.c_str(), def.second.c_str()});
        }
        shaderDesc.Defines = definesv;

        IDxcBlob* pCS       = nullptr;
        IDxcBlob* pSignedCS = nullptr;
        //OutputDebugString(path.c_str());
        if (SUCCEEDED(s_dxilCompiler.compileFromFile(path.c_str(), &shaderDesc, &pCS)))
        {
            IDxcOperationResult* result = nullptr;
            if (SUCCEEDED(s_dxilCompiler.validate(pCS, &result, DxcValidatorFlags_Default)))
            {
                HRESULT validateStatus;
                result->GetStatus(&validateStatus);
                if (SUCCEEDED(validateStatus) && SUCCEEDED(result->GetResult(&pSignedCS)))
                {
                    compilationResult.shaderBlob = pSignedCS;

                    auto dgbReflection = s_dxilCompiler.dgbReflection();
                    dgbReflection->Load(pSignedCS);

                    UINT shaderIdx;
                    dgbReflection->FindFirstPartKind(FidelityFx::DFCC_DXIL, &shaderIdx);

                    ID3D12ShaderReflection* pReflection = nullptr;
                    dgbReflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&pReflection));

                    D3D12_SHADER_DESC desc;
                    if (SUCCEEDED(pReflection->GetDesc(&desc)))
                    {
                        for (UINT i = 0; i < desc.BoundResources; i++)
                        {
                            D3D12_SHADER_INPUT_BIND_DESC bindDesc;
                            if (SUCCEEDED(pReflection->GetResourceBindingDesc(i, &bindDesc)))
                            {
                                bindDesc = bindDesc;
                                if (bindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER)
                                {
                                    compilationResult.boundSamplers.push_back(bindDesc.BindPoint);
                                    compilationResult.boundSamplersNames.push_back(bindDesc.Name);
                                }
                                else if (bindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE)
                                {
                                    compilationResult.boundRegistersSRVs.push_back(bindDesc.BindPoint);
                                    compilationResult.boundRegistersSRVNames.push_back(bindDesc.Name);
                                }
                                else if (bindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWTYPED)
                                {
                                    compilationResult.boundRegistersUAVs.push_back(bindDesc.BindPoint);
                                    compilationResult.boundRegistersUAVNames.push_back(bindDesc.Name);
                                    compilationResult.boundRegistersUAVsCount.push_back(bindDesc.BindCount);
                                }
                                else if (bindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
                                {
                                    compilationResult.boundRegistersCBs.push_back(bindDesc.BindPoint);
                                    compilationResult.boundRegistersCBNames.push_back(bindDesc.Name);
                                }
                                else if (bindDesc.Type == D3D_SIT_STRUCTURED)
                                {
                                    compilationResult.boundRegistersSRVBuffers.push_back(bindDesc.BindPoint);
                                    compilationResult.boundRegistersSRVBuffersNames.push_back(bindDesc.Name);
                                }
                                else if (bindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
                                {
                                    compilationResult.boundRegistersUAVBuffers.push_back(bindDesc.BindPoint);
                                    compilationResult.boundRegistersUAVBuffersNames.push_back(bindDesc.Name);
                                }
                                else if (bindDesc.Type == D3D_SIT_RTACCELERATIONSTRUCTURE)
                                {
                                    compilationResult.boundRegistersRTASs.push_back(bindDesc.BindPoint);
                                    compilationResult.boundRegistersRTASsNames.push_back(bindDesc.Name);
                                }
                            }
                        }
                    }
                }

                FidelityFx::SafeRelease(result);
            }

            FidelityFx::SafeRelease(pCS);
        }

        return compilationResult;
    }

    D3D12_TEXTURE_ADDRESS_MODE FfxGetAddressModeDX12(const FfxAddressMode& addressMode)
    {
        switch (addressMode)
        {
        case FFX_ADDRESS_MODE_WRAP:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case FFX_ADDRESS_MODE_MIRROR:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case FFX_ADDRESS_MODE_CLAMP:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case FFX_ADDRESS_MODE_BORDER:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case FFX_ADDRESS_MODE_MIRROR_ONCE:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:
            FFX_ASSERT_MESSAGE(false, "Unsupported addressing mode requested. Please implement");
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            break;
        }
    }

    // !!!!!!!
    // BAD HACK: needs to match BackendContext_DX12 in ffx_dx12.cpp
    typedef struct BackendContext_DX12
    {
        // store for resources and resourceViews
        typedef struct Resource
        {
#ifdef _DEBUG
            wchar_t resourceName[64] = {};
#endif
            ID3D12Resource*        resourcePtr;
            FfxResourceDescription resourceDescription;
            FfxResourceStates      initialState;
            FfxResourceStates      currentState;
            uint32_t               srvDescIndex;
            uint32_t               uavDescIndex;
            uint32_t               uavDescCount;
        } Resource;

        ID3D12Device* device = nullptr;

        FfxGpuJobDescription* pGpuJobs;
        uint32_t              gpuJobCount;

        uint32_t              nextRtvDescriptor;
        ID3D12DescriptorHeap* descHeapRtvCpu;

        ID3D12DescriptorHeap* descHeapSrvCpu;
        ID3D12DescriptorHeap* descHeapUavCpu;
        ID3D12DescriptorHeap* descHeapUavGpu;

        uint32_t              descRingBufferSize;
        uint32_t              descRingBufferBase;
        ID3D12DescriptorHeap* descRingBuffer;

        void*           constantBufferMem;
        ID3D12Resource* constantBufferResource;
        uint32_t        constantBufferSize;
        uint32_t        constantBufferOffset;
        std::mutex      constantBufferMutex;

        D3D12_RESOURCE_BARRIER barriers[FFX_MAX_BARRIERS];
        uint32_t               barrierCount;

        typedef struct alignas(32) EffectContext
        {
            // Resource allocation
            uint32_t nextStaticResource;
            uint32_t nextDynamicResource;

            // UAV offsets
            uint32_t nextStaticUavDescriptor;
            uint32_t nextDynamicUavDescriptor;

            // Usage
            bool active;

        } EffectContext;

        // Resource holder
        Resource*      pResources;
        EffectContext* pEffectContexts;

    } BackendContext_DX12;
    // !!!!!!!
    // End: BAD HACK: needs to match BackendContext_DX12 in ffx_dx12.cpp

    //
    DXGI_FORMAT ffxGetDX12FormatFromSurfaceFormat(FfxSurfaceFormat surfaceFormat)
    {
        switch (surfaceFormat)
        {
        case (FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case (FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case (FFX_SURFACE_FORMAT_R32G32_FLOAT):
            return DXGI_FORMAT_R32G32_FLOAT;
        case (FFX_SURFACE_FORMAT_R32_UINT):
            return DXGI_FORMAT_R32_UINT;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case (FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16_FLOAT):
            return DXGI_FORMAT_R16G16_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16_UINT):
            return DXGI_FORMAT_R16G16_UINT;
        case (FFX_SURFACE_FORMAT_R16G16_SINT):
            return DXGI_FORMAT_R16G16_SINT;
        case (FFX_SURFACE_FORMAT_R16_FLOAT):
            return DXGI_FORMAT_R16_FLOAT;
        case (FFX_SURFACE_FORMAT_R16_UINT):
            return DXGI_FORMAT_R16_UINT;
        case (FFX_SURFACE_FORMAT_R16_UNORM):
            return DXGI_FORMAT_R16_UNORM;
        case (FFX_SURFACE_FORMAT_R16_SNORM):
            return DXGI_FORMAT_R16_SNORM;
        case (FFX_SURFACE_FORMAT_R8_UNORM):
            return DXGI_FORMAT_R8_UNORM;
        case (FFX_SURFACE_FORMAT_R8_UINT):
            return DXGI_FORMAT_R8_UINT;
        case (FFX_SURFACE_FORMAT_R8G8_UNORM):
            return DXGI_FORMAT_R8G8_UNORM;
        case (FFX_SURFACE_FORMAT_R32_FLOAT):
            return DXGI_FORMAT_R32_FLOAT;
        case (FFX_SURFACE_FORMAT_UNKNOWN):
            return DXGI_FORMAT_UNKNOWN;

        default:
            FFX_ASSERT_MESSAGE(false, "Format not yet supported");
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    FfxErrorCode ffxGetPermutationBlobByIndexFromSource(FfxEffect effectId, FfxPass passId, FfxBindStage stageId, uint32_t permutationOptions, FfxShaderBlob* outBlob)
    {
        // CompileShader FromSource
        std::vector<std::wstring> shaderPath = getShaderPathFromEffectPassID(effectId, passId);
        getShaderDefinesFromEffectPassID(effectId, passId, permutationOptions);

        ShaderCompilationResult compileResult;
        if (stageId == FFX_BIND_COMPUTE_SHADER_STAGE)
        {
            compileResult = compileShader(shaderPath.back(), L"CS", s_shaderDefines, additionalIncludeDirs, L"cs_6_6");
        }
        else if (stageId == FFX_BIND_VERTEX_SHADER_STAGE)
        {
            compileResult = compileShader(shaderPath[0], L"mainVS", s_shaderDefines, additionalIncludeDirs, L"vs_6_6");
        }
        else if (stageId == FFX_BIND_PIXEL_SHADER_STAGE)
        {
            compileResult = compileShader(shaderPath[1], L"mainPS", s_shaderDefines, additionalIncludeDirs, L"ps_6_6");
        }

        // Copy Reflection Data to blob
        char** CBNames = nullptr;
        uint32_t* CBBindings = nullptr;
        uint32_t* CBCounts   = nullptr;
        {
            // TODO: deallocate this memory after use
            CBNames = new char*[compileResult.boundRegistersCBs.size()];
            CBBindings = new uint32_t[compileResult.boundRegistersCBs.size()];
            CBCounts   = new uint32_t[compileResult.boundRegistersCBs.size()];

            for (int i = 0; i < compileResult.boundRegistersCBs.size(); ++i)
            {
                // TODO: probably replace this with strdup(), below as well
                size_t strlen   = compileResult.boundRegistersCBNames[i].length() + 1;
                CBNames[i]      = new char[strlen];
                strncpy_s(CBNames[i], strlen, compileResult.boundRegistersCBNames[i].c_str(), strlen);

                CBBindings[i] = compileResult.boundRegistersCBs[i];
                CBCounts[i]   = 1;
            }
        }

        char**    SrvTexNames    = nullptr;
        uint32_t* SrvTexBindings = nullptr;
        uint32_t* SrvTexCounts   = nullptr;
        {
            SrvTexNames = new char*[compileResult.boundRegistersSRVs.size()];
            SrvTexBindings = new uint32_t[compileResult.boundRegistersSRVs.size()];
            SrvTexCounts   = new uint32_t[compileResult.boundRegistersSRVs.size()];

            for (int i = 0; i < compileResult.boundRegistersSRVs.size(); ++i)
            {
                size_t strlen = compileResult.boundRegistersSRVNames[i].length() + 1;
                SrvTexNames[i]  = new char[strlen];
                strncpy_s(SrvTexNames[i], strlen, compileResult.boundRegistersSRVNames[i].c_str(), strlen);

                SrvTexBindings[i] = compileResult.boundRegistersSRVs[i];
                SrvTexCounts[i]   = 1;
            }
        }

        char**    UavTexNames    = nullptr;
        uint32_t* UavTexBindings = nullptr;
        uint32_t* UavTexCounts   = nullptr;
        {
            UavTexNames    = new char*[compileResult.boundRegistersUAVs.size()];
            UavTexBindings = new uint32_t[compileResult.boundRegistersUAVs.size()];
            UavTexCounts   = new uint32_t[compileResult.boundRegistersUAVs.size()];

            for (int i = 0; i < compileResult.boundRegistersUAVs.size(); ++i)
            {
                size_t strlen  = compileResult.boundRegistersUAVNames[i].length() + 1;
                UavTexNames[i]  = new char[strlen];
                strncpy_s(UavTexNames[i], strlen, compileResult.boundRegistersUAVNames[i].c_str(), strlen);

                UavTexBindings[i] = compileResult.boundRegistersUAVs[i];
                UavTexCounts[i]   = compileResult.boundRegistersUAVsCount[i] ;
            }
        }

        char**    SrvBufferNames    = nullptr;
        uint32_t* SrvBufferBindings = nullptr;
        uint32_t* SrvBufferCounts   = nullptr;
        {
            SrvBufferNames    = new char*[compileResult.boundRegistersSRVBuffers.size()];
            SrvBufferBindings = new uint32_t[compileResult.boundRegistersSRVBuffers.size()];
            SrvBufferCounts   = new uint32_t[compileResult.boundRegistersSRVBuffers.size()];

            for (int i = 0; i < compileResult.boundRegistersSRVBuffers.size(); ++i)
            {
                size_t strlen     = compileResult.boundRegistersSRVBuffersNames[i].length() + 1;
                SrvBufferNames[i] = new char[strlen];
                strncpy_s(SrvBufferNames[i], strlen, compileResult.boundRegistersSRVBuffersNames[i].c_str(), strlen);

                SrvBufferBindings[i] = compileResult.boundRegistersSRVBuffers[i];
                SrvBufferCounts[i]   = 1;
            }
        }

        char**    UavBufferNames    = nullptr;
        uint32_t* UavBufferBindings = nullptr;
        uint32_t* UavBufferCounts   = nullptr;
        {
            UavBufferNames    = new char*[compileResult.boundRegistersUAVBuffers.size()];
            UavBufferBindings = new uint32_t[compileResult.boundRegistersUAVBuffers.size()];
            UavBufferCounts   = new uint32_t[compileResult.boundRegistersUAVBuffers.size()];

            for (int i = 0; i < compileResult.boundRegistersUAVBuffers.size(); ++i)
            {
                size_t strlen     = compileResult.boundRegistersUAVBuffersNames[i].length() + 1;
                UavBufferNames[i] = new char[strlen];
                strncpy_s(UavBufferNames[i], strlen, compileResult.boundRegistersUAVBuffersNames[i].c_str(), strlen);

                UavBufferBindings[i] = compileResult.boundRegistersUAVBuffers[i];
                UavBufferCounts[i]   = 1;
            }
        }

        char**    SamplerNames    = nullptr;
        uint32_t* SamplerBindings = nullptr;
        uint32_t* SamplerCounts   = nullptr;
        {
            SamplerNames    = new char*[compileResult.boundSamplers.size()];
            SamplerBindings = new uint32_t[compileResult.boundSamplers.size()];
            SamplerCounts   = new uint32_t[compileResult.boundSamplers.size()];

            for (int i = 0; i < compileResult.boundSamplers.size(); ++i)
            {
                size_t strlen   = compileResult.boundSamplersNames[i].length() + 1;
                SamplerNames[i] = new char[strlen];
                strncpy_s(SamplerNames[i], strlen, compileResult.boundSamplersNames[i].c_str(), strlen);

                SamplerBindings[i] = compileResult.boundSamplers[i];
                SamplerCounts[i]   = 1;
            }
        }

        char**    RTASNames    = nullptr;
        uint32_t* RTASBindings = nullptr;
        uint32_t* RTASCounts   = nullptr;
        {
            RTASNames    = new char*[compileResult.boundRegistersRTASs.size()];
            RTASBindings = new uint32_t[compileResult.boundRegistersRTASs.size()];
            RTASCounts   = new uint32_t[compileResult.boundRegistersRTASs.size()];

            for (int i = 0; i < compileResult.boundRegistersRTASs.size(); ++i)
            {
                size_t strlen = compileResult.boundRegistersRTASsNames[i].length() + 1;
                RTASNames[i]    = new char[strlen];
                strncpy_s(RTASNames[i], strlen, compileResult.boundRegistersRTASsNames[i].c_str(), strlen);

                RTASBindings[i] = compileResult.boundRegistersRTASs[i];
                RTASCounts[i]   = 1;
            }
        }

        FfxShaderBlob blob = {
            compileResult.shaderBlob != nullptr ? (uint8_t*) compileResult.shaderBlob->GetBufferPointer() : nullptr,
            compileResult.shaderBlob != nullptr ? (uint32_t) compileResult.shaderBlob->GetBufferSize() : 0,

            (uint32_t)compileResult.boundRegistersCBs.size(),
            (uint32_t)compileResult.boundRegistersSRVs.size(),
            (uint32_t)compileResult.boundRegistersUAVs.size(),
            (uint32_t)compileResult.boundRegistersSRVBuffers.size(),
            (uint32_t)compileResult.boundRegistersUAVBuffers.size(),
            (uint32_t)compileResult.boundSamplers.size(),
            (uint32_t)compileResult.boundRegistersRTASs.size(),

            const_cast<const char**>(CBNames), CBBindings, CBCounts,
            const_cast<const char**>(SrvTexNames), SrvTexBindings, SrvTexCounts,
            const_cast<const char**>(UavTexNames), UavTexBindings, UavTexCounts,
            const_cast<const char**>(SrvBufferNames), SrvBufferBindings, SrvBufferCounts,
            const_cast<const char**>(UavBufferNames), UavBufferBindings, UavBufferCounts,
            const_cast<const char**>(SamplerNames), SamplerBindings, SamplerCounts,
            const_cast<const char**>(RTASNames), RTASBindings, RTASCounts
        };

        memcpy(outBlob, &blob, sizeof(FfxShaderBlob));

        return compileResult.shaderBlob == nullptr ? FFX_ERROR_BACKEND_API_ERROR : FFX_OK;
    }

    FfxErrorCode CreatePipelineFromSourceInternal(  FfxInterface*                 backendInterface,
                                            FfxEffect                     effect,
                                            FfxPass                       pass,
                                            uint32_t                      permutationOptions,
                                            const FfxPipelineDescription* pipelineDescription,
                                            FfxUInt32                     effectContextId,
                                            FfxPipelineState*             outPipeline)
    {
        FFX_ASSERT(NULL != backendInterface);
        FFX_ASSERT(NULL != pipelineDescription);

        BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
        ID3D12Device*        dx12Device     = backendContext->device;

        if (pipelineDescription->stage == FfxBindStage(FFX_BIND_VERTEX_SHADER_STAGE | FFX_BIND_PIXEL_SHADER_STAGE))
        {
            // create rasterPipeline
            // ToDo: use compileShaderFromFile instead
            FfxShaderBlob shaderBlobVS = {};
            ffxGetPermutationBlobByIndexFromSource(effect, pass, FFX_BIND_VERTEX_SHADER_STAGE, permutationOptions, &shaderBlobVS);
            FFX_ASSERT(shaderBlobVS.data && shaderBlobVS.size);

            FfxShaderBlob shaderBlobPS = {};
            ffxGetPermutationBlobByIndexFromSource(effect, pass, FFX_BIND_PIXEL_SHADER_STAGE, permutationOptions, &shaderBlobPS);
            FFX_ASSERT(shaderBlobPS.data && shaderBlobPS.size);

            {
                // set up root signature
                // easiest implementation: simply create one root signature per pipeline
                // should add some management later on to avoid unnecessarily re-binding the root signature
                {
                    FFX_ASSERT(pipelineDescription->samplerCount <= FFX_MAX_SAMPLERS);
                    const size_t              samplerCount = pipelineDescription->samplerCount;
                    D3D12_STATIC_SAMPLER_DESC dx12SamplerDescriptions[FFX_MAX_SAMPLERS];
                    for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
                    {
                        D3D12_STATIC_SAMPLER_DESC dx12SamplerDesc = {};

                        dx12SamplerDesc.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
                        dx12SamplerDesc.MinLOD           = 0.f;
                        dx12SamplerDesc.MaxLOD           = D3D12_FLOAT32_MAX;
                        dx12SamplerDesc.MipLODBias       = 0.f;
                        dx12SamplerDesc.MaxAnisotropy    = 16;
                        dx12SamplerDesc.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
                        dx12SamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                        dx12SamplerDesc.AddressU         = FfxGetAddressModeDX12(pipelineDescription->samplers[currentSamplerIndex].addressModeU);
                        dx12SamplerDesc.AddressV         = FfxGetAddressModeDX12(pipelineDescription->samplers[currentSamplerIndex].addressModeV);
                        dx12SamplerDesc.AddressW         = FfxGetAddressModeDX12(pipelineDescription->samplers[currentSamplerIndex].addressModeW);

                        switch (pipelineDescription->samplers[currentSamplerIndex].filter)
                        {
                        case FFX_FILTER_TYPE_MINMAGMIP_POINT:
                            dx12SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                            break;
                        case FFX_FILTER_TYPE_MINMAGMIP_LINEAR:
                            dx12SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                            break;
                        case FFX_FILTER_TYPE_MINMAGLINEARMIP_POINT:
                            dx12SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                            break;

                        default:
                            FFX_ASSERT_MESSAGE(false, "Unsupported filter type requested. Please implement");
                            break;
                        }

                        dx12SamplerDescriptions[currentSamplerIndex]                = dx12SamplerDesc;
                        dx12SamplerDescriptions[currentSamplerIndex].ShaderRegister = (UINT)currentSamplerIndex;
                    }

                    // storage for maximum number of descriptor ranges.
                    const int32_t          maximumDescriptorRangeSize             = 2;
                    D3D12_DESCRIPTOR_RANGE dx12Ranges[maximumDescriptorRangeSize] = {};
                    int32_t                currentDescriptorRangeIndex            = 0;

                    // storage for maximum number of root parameters.
                    const int32_t        maximumRootParameters                     = 10;
                    D3D12_ROOT_PARAMETER dx12RootParameters[maximumRootParameters] = {};
                    int32_t              currentRootParameterIndex                 = 0;

                    // Allocate a max of binding slots
                    int32_t uavCount =
                        shaderBlobVS.uavBufferCount || shaderBlobVS.uavTextureCount || shaderBlobVS.uavBufferCount || shaderBlobVS.uavTextureCount
                            ? FFX_MAX_RESOURCE_IDENTIFIER_COUNT
                            : 0;
                    int32_t srvCount =
                        shaderBlobVS.srvBufferCount || shaderBlobVS.srvTextureCount || shaderBlobPS.srvBufferCount || shaderBlobPS.srvTextureCount
                            ? FFX_MAX_RESOURCE_IDENTIFIER_COUNT
                            : 0;
                    if (uavCount > 0)
                    {
                        FFX_ASSERT(currentDescriptorRangeIndex < maximumDescriptorRangeSize);
                        D3D12_DESCRIPTOR_RANGE* dx12DescriptorRange = &dx12Ranges[currentDescriptorRangeIndex];
                        memset(dx12DescriptorRange, 0, sizeof(D3D12_DESCRIPTOR_RANGE));
                        dx12DescriptorRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                        dx12DescriptorRange->RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                        dx12DescriptorRange->BaseShaderRegister                = 0;
                        dx12DescriptorRange->NumDescriptors                    = uavCount;
                        currentDescriptorRangeIndex++;

                        FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
                        D3D12_ROOT_PARAMETER* dx12RootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
                        memset(dx12RootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
                        dx12RootParameterSlot->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                        dx12RootParameterSlot->DescriptorTable.NumDescriptorRanges = 1;
                        currentRootParameterIndex++;
                    }

                    if (srvCount > 0)
                    {
                        FFX_ASSERT(currentDescriptorRangeIndex < maximumDescriptorRangeSize);
                        D3D12_DESCRIPTOR_RANGE* dx12DescriptorRange = &dx12Ranges[currentDescriptorRangeIndex];
                        memset(dx12DescriptorRange, 0, sizeof(D3D12_DESCRIPTOR_RANGE));
                        dx12DescriptorRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                        dx12DescriptorRange->RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        dx12DescriptorRange->BaseShaderRegister                = 0;
                        dx12DescriptorRange->NumDescriptors                    = srvCount;
                        currentDescriptorRangeIndex++;

                        FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
                        D3D12_ROOT_PARAMETER* dx12RootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
                        memset(dx12RootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
                        dx12RootParameterSlot->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                        dx12RootParameterSlot->DescriptorTable.NumDescriptorRanges = 1;
                        currentRootParameterIndex++;
                    }

                    // Setup the descriptor table bindings for the above
                    for (int32_t currentRangeIndex = 0; currentRangeIndex < currentDescriptorRangeIndex; currentRangeIndex++)
                    {
                        dx12RootParameters[currentRangeIndex].DescriptorTable.pDescriptorRanges = &dx12Ranges[currentRangeIndex];
                    }

                    // Setup root constants as push constants for dx12
                    FFX_ASSERT(pipelineDescription->rootConstantBufferCount <= FFX_MAX_NUM_CONST_BUFFERS);
                    size_t   rootConstantsSize = pipelineDescription->rootConstantBufferCount;
                    uint32_t rootConstants[FFX_MAX_NUM_CONST_BUFFERS];

                    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipelineDescription->rootConstantBufferCount;
                         ++currentRootConstantIndex)
                    {
                        rootConstants[currentRootConstantIndex] = pipelineDescription->rootConstants[currentRootConstantIndex].size;
                    }

                    for (int32_t currentRootConstantIndex = 0; currentRootConstantIndex < (int32_t)pipelineDescription->rootConstantBufferCount;
                         currentRootConstantIndex++)
                    {
                        FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
                        D3D12_ROOT_PARAMETER* rootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
                        memset(rootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
                        rootParameterSlot->ParameterType            = D3D12_ROOT_PARAMETER_TYPE_CBV;
                        rootParameterSlot->Constants.ShaderRegister = currentRootConstantIndex;
                        currentRootParameterIndex++;
                    }

                    D3D12_ROOT_SIGNATURE_DESC dx12RootSignatureDescription = {};
                    dx12RootSignatureDescription.NumParameters             = currentRootParameterIndex;
                    dx12RootSignatureDescription.pParameters               = dx12RootParameters;
                    dx12RootSignatureDescription.NumStaticSamplers         = (UINT)samplerCount;
                    dx12RootSignatureDescription.pStaticSamplers           = dx12SamplerDescriptions;

                    ID3DBlob* outBlob   = nullptr;
                    ID3DBlob* errorBlob = nullptr;

                    //Query D3D12SerializeRootSignature from d3d12.dll handle
                    typedef HRESULT(__stdcall * D3D12SerializeRootSignatureType)(
                        const D3D12_ROOT_SIGNATURE_DESC*, D3D_ROOT_SIGNATURE_VERSION, ID3DBlob**, ID3DBlob**);

                    //Do not pass hD3D12 handle to the FreeLibrary function, as GetModuleHandle will not increment refcount
                    HMODULE d3d12ModuleHandle = GetModuleHandleW(L"D3D12.dll");

                    if (NULL != d3d12ModuleHandle)
                    {
                        D3D12SerializeRootSignatureType dx12SerializeRootSignatureType =
                            (D3D12SerializeRootSignatureType)GetProcAddress(d3d12ModuleHandle, "D3D12SerializeRootSignature");

                        if (nullptr != dx12SerializeRootSignatureType)
                        {
                            HRESULT result = dx12SerializeRootSignatureType(&dx12RootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &outBlob, &errorBlob);
                            if (FAILED(result))
                            {
                                return FFX_ERROR_BACKEND_API_ERROR;
                            }

                            size_t   blobSize = outBlob->GetBufferSize();
                            int64_t* blobData = (int64_t*)outBlob->GetBufferPointer();

                            result = dx12Device->CreateRootSignature(0,
                                                                     outBlob->GetBufferPointer(),
                                                                     outBlob->GetBufferSize(),
                                                                     IID_PPV_ARGS(reinterpret_cast<ID3D12RootSignature**>(&outPipeline->rootSignature)));
                            if (FAILED(result))
                            {
                                return FFX_ERROR_BACKEND_API_ERROR;
                            }
                        }
                        else
                        {
                            return FFX_ERROR_BACKEND_API_ERROR;
                        }
                    }
                    else
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }
                }

                ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(outPipeline->rootSignature);

                // Only set the command signature if this is setup as an indirect workload
                if (pipelineDescription->indirectWorkload)
                {
                    D3D12_INDIRECT_ARGUMENT_DESC argumentDescs        = {D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH};
                    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
                    commandSignatureDesc.pArgumentDescs               = &argumentDescs;
                    commandSignatureDesc.NumArgumentDescs             = 1;
                    commandSignatureDesc.ByteStride                   = sizeof(D3D12_DISPATCH_ARGUMENTS);

                    HRESULT result = dx12Device->CreateCommandSignature(
                        &commandSignatureDesc, nullptr, IID_PPV_ARGS(reinterpret_cast<ID3D12CommandSignature**>(&outPipeline->cmdSignature)));
                    if (FAILED(result))
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }
                }
                else
                {
                    outPipeline->cmdSignature = nullptr;
                }

                FFX_ASSERT(shaderBlobVS.srvTextureCount + shaderBlobVS.srvBufferCount + shaderBlobPS.srvTextureCount + shaderBlobPS.srvBufferCount <=
                           FFX_MAX_NUM_SRVS);
                FFX_ASSERT(shaderBlobVS.uavTextureCount + shaderBlobVS.uavBufferCount + shaderBlobPS.uavTextureCount + shaderBlobPS.uavBufferCount <=
                           FFX_MAX_NUM_UAVS);
                FFX_ASSERT(shaderBlobVS.cbvCount + shaderBlobPS.cbvCount <= FFX_MAX_NUM_CONST_BUFFERS);

                // populate the pass.
                outPipeline->srvTextureCount = shaderBlobVS.srvTextureCount + shaderBlobPS.srvTextureCount;
                outPipeline->uavTextureCount = shaderBlobVS.uavTextureCount + shaderBlobPS.uavTextureCount;
                outPipeline->srvBufferCount  = shaderBlobVS.srvBufferCount + shaderBlobPS.srvBufferCount;
                outPipeline->uavBufferCount  = shaderBlobVS.uavBufferCount + shaderBlobPS.uavBufferCount;
                outPipeline->constCount      = shaderBlobVS.cbvCount + shaderBlobPS.cbvCount;

                // Todo when needed
                //outPipeline->samplerCount      = shaderBlob.samplerCount;
                //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                uint32_t                                               srvIndex = 0;
                for (uint32_t vsSrvIndex = 0; vsSrvIndex < shaderBlobVS.srvTextureCount; ++vsSrvIndex)
                {
                    outPipeline->srvTextureBindings[srvIndex].slotIndex = shaderBlobVS.boundSRVTextures[vsSrvIndex];
                    outPipeline->srvTextureBindings[srvIndex].bindCount = shaderBlobVS.boundSRVTextureCounts[vsSrvIndex];
                    wcscpy_s(outPipeline->srvTextureBindings[srvIndex].name, converter.from_bytes(shaderBlobVS.boundSRVTextureNames[vsSrvIndex]).c_str());
                    ++srvIndex;
                }
                for (uint32_t psSrvIndex = 0; psSrvIndex < shaderBlobPS.srvTextureCount; ++psSrvIndex)
                {
                    outPipeline->srvTextureBindings[srvIndex].slotIndex = shaderBlobPS.boundSRVTextures[psSrvIndex];
                    outPipeline->srvTextureBindings[srvIndex].bindCount = shaderBlobPS.boundSRVTextureCounts[psSrvIndex];
                    wcscpy_s(outPipeline->srvTextureBindings[srvIndex].name, converter.from_bytes(shaderBlobPS.boundSRVTextureNames[psSrvIndex]).c_str());
                    ++srvIndex;
                }

                uint32_t uavIndex = 0;
                for (uint32_t vsUavIndex = 0; vsUavIndex < shaderBlobVS.uavTextureCount; ++vsUavIndex)
                {
                    outPipeline->uavTextureBindings[uavIndex].slotIndex = shaderBlobVS.boundUAVTextures[vsUavIndex];
                    outPipeline->uavTextureBindings[uavIndex].bindCount = shaderBlobVS.boundUAVTextureCounts[vsUavIndex];
                    wcscpy_s(outPipeline->uavTextureBindings[uavIndex].name, converter.from_bytes(shaderBlobVS.boundUAVTextureNames[vsUavIndex]).c_str());
                    ++uavIndex;
                }
                for (uint32_t psUavIndex = 0; psUavIndex < shaderBlobPS.uavTextureCount; ++psUavIndex)
                {
                    outPipeline->uavTextureBindings[uavIndex].slotIndex = shaderBlobPS.boundUAVTextures[psUavIndex];
                    outPipeline->uavTextureBindings[uavIndex].bindCount = shaderBlobPS.boundUAVTextureCounts[psUavIndex];
                    wcscpy_s(outPipeline->uavTextureBindings[uavIndex].name, converter.from_bytes(shaderBlobPS.boundUAVTextureNames[psUavIndex]).c_str());
                    ++uavIndex;
                }

                uint32_t srvBufferIndex = 0;
                for (uint32_t vsSrvIndex = 0; vsSrvIndex < shaderBlobVS.srvBufferCount; ++vsSrvIndex)
                {
                    outPipeline->srvBufferBindings[srvBufferIndex].slotIndex = shaderBlobVS.boundSRVBuffers[vsSrvIndex];
                    outPipeline->srvBufferBindings[srvBufferIndex].bindCount = shaderBlobVS.boundSRVBufferCounts[vsSrvIndex];
                    wcscpy_s(outPipeline->srvBufferBindings[srvBufferIndex].name, converter.from_bytes(shaderBlobVS.boundSRVBufferNames[vsSrvIndex]).c_str());
                    ++srvBufferIndex;
                }
                for (uint32_t psSrvIndex = 0; psSrvIndex < shaderBlobPS.srvBufferCount; ++psSrvIndex)
                {
                    outPipeline->srvBufferBindings[srvBufferIndex].slotIndex = shaderBlobPS.boundSRVBuffers[psSrvIndex];
                    outPipeline->srvBufferBindings[srvBufferIndex].bindCount = shaderBlobPS.boundSRVBufferCounts[psSrvIndex];
                    wcscpy_s(outPipeline->srvBufferBindings[srvBufferIndex].name, converter.from_bytes(shaderBlobPS.boundSRVBufferNames[psSrvIndex]).c_str());
                    ++srvBufferIndex;
                }

                uint32_t uavBufferIndex = 0;
                for (uint32_t vsUavIndex = 0; vsUavIndex < shaderBlobVS.uavBufferCount; ++vsUavIndex)
                {
                    outPipeline->uavBufferBindings[uavBufferIndex].slotIndex = shaderBlobVS.boundUAVBuffers[vsUavIndex];
                    outPipeline->uavBufferBindings[uavBufferIndex].bindCount = shaderBlobVS.boundUAVBufferCounts[vsUavIndex];
                    wcscpy_s(outPipeline->uavBufferBindings[uavBufferIndex].name, converter.from_bytes(shaderBlobVS.boundUAVBufferNames[vsUavIndex]).c_str());
                    ++uavBufferIndex;
                }
                for (uint32_t psUavIndex = 0; psUavIndex < shaderBlobPS.uavBufferCount; ++psUavIndex)
                {
                    outPipeline->uavBufferBindings[uavBufferIndex].slotIndex = shaderBlobPS.boundUAVBuffers[psUavIndex];
                    outPipeline->uavBufferBindings[uavBufferIndex].bindCount = shaderBlobPS.boundUAVBufferCounts[psUavIndex];
                    wcscpy_s(outPipeline->uavBufferBindings[uavBufferIndex].name, converter.from_bytes(shaderBlobPS.boundUAVBufferNames[psUavIndex]).c_str());
                    ++uavBufferIndex;
                }

                uint32_t cbIndex = 0;
                for (uint32_t vsCbIndex = 0; vsCbIndex < shaderBlobVS.cbvCount; ++vsCbIndex)
                {
                    outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlobVS.boundConstantBuffers[vsCbIndex];
                    outPipeline->constantBufferBindings[cbIndex].bindCount = shaderBlobVS.boundConstantBufferCounts[vsCbIndex];
                    wcscpy_s(outPipeline->constantBufferBindings[cbIndex].name, converter.from_bytes(shaderBlobVS.boundConstantBufferNames[vsCbIndex]).c_str());
                    ++cbIndex;
                }
                for (uint32_t psCbIndex = 0; psCbIndex < shaderBlobPS.cbvCount; ++psCbIndex)
                {
                    outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlobPS.boundConstantBuffers[psCbIndex];
                    outPipeline->constantBufferBindings[cbIndex].bindCount = shaderBlobPS.boundConstantBufferCounts[psCbIndex];
                    wcscpy_s(outPipeline->constantBufferBindings[cbIndex].name, converter.from_bytes(shaderBlobPS.boundConstantBufferNames[psCbIndex]).c_str());
                    ++cbIndex;
                }
            }

            ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(outPipeline->rootSignature);

            FfxSurfaceFormat ffxBackbufferFmt = pipelineDescription->backbufferFormat;

            // create the PSO
            D3D12_GRAPHICS_PIPELINE_STATE_DESC dx12PipelineStateDescription = {};
            dx12PipelineStateDescription.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            dx12PipelineStateDescription.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            dx12PipelineStateDescription.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            dx12PipelineStateDescription.DepthStencilState.DepthEnable      = FALSE;
            dx12PipelineStateDescription.SampleMask                         = UINT_MAX;
            dx12PipelineStateDescription.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            dx12PipelineStateDescription.SampleDesc                         = {1, 0};
            dx12PipelineStateDescription.NumRenderTargets                   = 1;
            dx12PipelineStateDescription.RTVFormats[0]                      = ffxGetDX12FormatFromSurfaceFormat(ffxBackbufferFmt);

            dx12PipelineStateDescription.Flags              = D3D12_PIPELINE_STATE_FLAG_NONE;
            dx12PipelineStateDescription.pRootSignature     = dx12RootSignature;
            dx12PipelineStateDescription.VS.pShaderBytecode = shaderBlobVS.data;
            dx12PipelineStateDescription.VS.BytecodeLength  = shaderBlobVS.size;
            dx12PipelineStateDescription.PS.pShaderBytecode = shaderBlobPS.data;
            dx12PipelineStateDescription.PS.BytecodeLength  = shaderBlobPS.size;

            if (FAILED(dx12Device->CreateGraphicsPipelineState(&dx12PipelineStateDescription,
                                                               IID_PPV_ARGS(reinterpret_cast<ID3D12PipelineState**>(&outPipeline->pipeline)))))
                return FFX_ERROR_BACKEND_API_ERROR;
        }
        else
        {
            FfxShaderBlob shaderBlob = {};
            ffxGetPermutationBlobByIndexFromSource(effect, pass, FFX_BIND_COMPUTE_SHADER_STAGE, permutationOptions, &shaderBlob);
            FFX_ASSERT(shaderBlob.data && shaderBlob.size);

            // set up root signature
            // easiest implementation: simply create one root signature per pipeline
            // should add some management later on to avoid unnecessarily re-binding the root signature
            {
                FFX_ASSERT(pipelineDescription->samplerCount <= FFX_MAX_SAMPLERS);
                const size_t              samplerCount = pipelineDescription->samplerCount;
                D3D12_STATIC_SAMPLER_DESC dx12SamplerDescriptions[FFX_MAX_SAMPLERS];
                for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
                {
                    D3D12_STATIC_SAMPLER_DESC dx12SamplerDesc = {};

                    dx12SamplerDesc.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
                    dx12SamplerDesc.MinLOD           = 0.f;
                    dx12SamplerDesc.MaxLOD           = D3D12_FLOAT32_MAX;
                    dx12SamplerDesc.MipLODBias       = 0.f;
                    dx12SamplerDesc.MaxAnisotropy    = 16;
                    dx12SamplerDesc.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
                    dx12SamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                    dx12SamplerDesc.AddressU         = FfxGetAddressModeDX12(pipelineDescription->samplers[currentSamplerIndex].addressModeU);
                    dx12SamplerDesc.AddressV         = FfxGetAddressModeDX12(pipelineDescription->samplers[currentSamplerIndex].addressModeV);
                    dx12SamplerDesc.AddressW         = FfxGetAddressModeDX12(pipelineDescription->samplers[currentSamplerIndex].addressModeW);

                    switch (pipelineDescription->samplers[currentSamplerIndex].filter)
                    {
                    case FFX_FILTER_TYPE_MINMAGMIP_POINT:
                        dx12SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                        break;
                    case FFX_FILTER_TYPE_MINMAGMIP_LINEAR:
                        dx12SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                        break;
                    case FFX_FILTER_TYPE_MINMAGLINEARMIP_POINT:
                        dx12SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                        break;

                    default:
                        FFX_ASSERT_MESSAGE(false, "Unsupported filter type requested. Please implement");
                        break;
                    }

                    dx12SamplerDescriptions[currentSamplerIndex]                = dx12SamplerDesc;
                    dx12SamplerDescriptions[currentSamplerIndex].ShaderRegister = (UINT)currentSamplerIndex;
                }

                // storage for maximum number of descriptor ranges.
                const int32_t          maximumDescriptorRangeSize             = 2;
                D3D12_DESCRIPTOR_RANGE dx12Ranges[maximumDescriptorRangeSize] = {};
                int32_t                currentDescriptorRangeIndex            = 0;

                // storage for maximum number of root parameters.
                const int32_t        maximumRootParameters                     = 10;
                D3D12_ROOT_PARAMETER dx12RootParameters[maximumRootParameters] = {};
                int32_t              currentRootParameterIndex                 = 0;

                // Allocate a max of binding slots
                int32_t uavCount = shaderBlob.uavBufferCount || shaderBlob.uavTextureCount ? FFX_MAX_RESOURCE_IDENTIFIER_COUNT : 0;
                int32_t srvCount = shaderBlob.srvBufferCount || shaderBlob.srvTextureCount ? FFX_MAX_RESOURCE_IDENTIFIER_COUNT : 0;
                if (uavCount > 0)
                {
                    FFX_ASSERT(currentDescriptorRangeIndex < maximumDescriptorRangeSize);
                    D3D12_DESCRIPTOR_RANGE* dx12DescriptorRange = &dx12Ranges[currentDescriptorRangeIndex];
                    memset(dx12DescriptorRange, 0, sizeof(D3D12_DESCRIPTOR_RANGE));
                    dx12DescriptorRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    dx12DescriptorRange->RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    dx12DescriptorRange->BaseShaderRegister                = 0;
                    dx12DescriptorRange->NumDescriptors                    = uavCount;
                    currentDescriptorRangeIndex++;

                    FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
                    D3D12_ROOT_PARAMETER* dx12RootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
                    memset(dx12RootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
                    dx12RootParameterSlot->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    dx12RootParameterSlot->DescriptorTable.NumDescriptorRanges = 1;
                    currentRootParameterIndex++;
                }

                if (srvCount > 0)
                {
                    FFX_ASSERT(currentDescriptorRangeIndex < maximumDescriptorRangeSize);
                    D3D12_DESCRIPTOR_RANGE* dx12DescriptorRange = &dx12Ranges[currentDescriptorRangeIndex];
                    memset(dx12DescriptorRange, 0, sizeof(D3D12_DESCRIPTOR_RANGE));
                    dx12DescriptorRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    dx12DescriptorRange->RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    dx12DescriptorRange->BaseShaderRegister                = 0;
                    dx12DescriptorRange->NumDescriptors                    = srvCount;
                    currentDescriptorRangeIndex++;

                    FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
                    D3D12_ROOT_PARAMETER* dx12RootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
                    memset(dx12RootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
                    dx12RootParameterSlot->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    dx12RootParameterSlot->DescriptorTable.NumDescriptorRanges = 1;
                    currentRootParameterIndex++;
                }

                // Setup the descriptor table bindings for the above
                for (int32_t currentRangeIndex = 0; currentRangeIndex < currentDescriptorRangeIndex; currentRangeIndex++)
                {
                    dx12RootParameters[currentRangeIndex].DescriptorTable.pDescriptorRanges = &dx12Ranges[currentRangeIndex];
                }

                // Setup root constants as push constants for dx12
                FFX_ASSERT(pipelineDescription->rootConstantBufferCount <= FFX_MAX_NUM_CONST_BUFFERS);
                size_t   rootConstantsSize = pipelineDescription->rootConstantBufferCount;
                uint32_t rootConstants[FFX_MAX_NUM_CONST_BUFFERS];

                for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipelineDescription->rootConstantBufferCount; ++currentRootConstantIndex)
                {
                    rootConstants[currentRootConstantIndex] = pipelineDescription->rootConstants[currentRootConstantIndex].size;
                }

                for (int32_t currentRootConstantIndex = 0; currentRootConstantIndex < (int32_t)pipelineDescription->rootConstantBufferCount;
                     currentRootConstantIndex++)
                {
                    FFX_ASSERT(currentRootParameterIndex < maximumRootParameters);
                    D3D12_ROOT_PARAMETER* rootParameterSlot = &dx12RootParameters[currentRootParameterIndex];
                    memset(rootParameterSlot, 0, sizeof(D3D12_ROOT_PARAMETER));
                    rootParameterSlot->ParameterType            = D3D12_ROOT_PARAMETER_TYPE_CBV;
                    rootParameterSlot->Constants.ShaderRegister = currentRootConstantIndex;
                    currentRootParameterIndex++;
                }

                D3D12_ROOT_SIGNATURE_DESC dx12RootSignatureDescription = {};
                dx12RootSignatureDescription.NumParameters             = currentRootParameterIndex;
                dx12RootSignatureDescription.pParameters               = dx12RootParameters;
                dx12RootSignatureDescription.NumStaticSamplers         = (UINT)samplerCount;
                dx12RootSignatureDescription.pStaticSamplers           = dx12SamplerDescriptions;

                ID3DBlob* outBlob   = nullptr;
                ID3DBlob* errorBlob = nullptr;

                //Query D3D12SerializeRootSignature from d3d12.dll handle
                typedef HRESULT(__stdcall * D3D12SerializeRootSignatureType)(
                    const D3D12_ROOT_SIGNATURE_DESC*, D3D_ROOT_SIGNATURE_VERSION, ID3DBlob**, ID3DBlob**);

                //Do not pass hD3D12 handle to the FreeLibrary function, as GetModuleHandle will not increment refcount
                HMODULE d3d12ModuleHandle = GetModuleHandleW(L"D3D12.dll");

                if (NULL != d3d12ModuleHandle)
                {
                    D3D12SerializeRootSignatureType dx12SerializeRootSignatureType =
                        (D3D12SerializeRootSignatureType)GetProcAddress(d3d12ModuleHandle, "D3D12SerializeRootSignature");

                    if (nullptr != dx12SerializeRootSignatureType)
                    {
                        HRESULT result = dx12SerializeRootSignatureType(&dx12RootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &outBlob, &errorBlob);
                        if (FAILED(result))
                        {
                            return FFX_ERROR_BACKEND_API_ERROR;
                        }

                        size_t   blobSize = outBlob->GetBufferSize();
                        int64_t* blobData = (int64_t*)outBlob->GetBufferPointer();

                        result = dx12Device->CreateRootSignature(0,
                                                                 outBlob->GetBufferPointer(),
                                                                 outBlob->GetBufferSize(),
                                                                 IID_PPV_ARGS(reinterpret_cast<ID3D12RootSignature**>(&outPipeline->rootSignature)));
                        
                        SafeRelease(outBlob);
                        SafeRelease(errorBlob);

                        if (FAILED(result))
                        {
                            return FFX_ERROR_BACKEND_API_ERROR;
                        }
                    }
                    else
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }
                }
                else
                {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }
            

            ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(outPipeline->rootSignature);

            // Only set the command signature if this is setup as an indirect workload
            if (pipelineDescription->indirectWorkload)
            {
                D3D12_INDIRECT_ARGUMENT_DESC argumentDescs        = {D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH};
                D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
                commandSignatureDesc.pArgumentDescs               = &argumentDescs;
                commandSignatureDesc.NumArgumentDescs             = 1;
                commandSignatureDesc.ByteStride                   = sizeof(D3D12_DISPATCH_ARGUMENTS);

                HRESULT result = dx12Device->CreateCommandSignature(
                    &commandSignatureDesc, nullptr, IID_PPV_ARGS(reinterpret_cast<ID3D12CommandSignature**>(&outPipeline->cmdSignature)));
                if (FAILED(result))
                {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }
            else
            {
                outPipeline->cmdSignature = nullptr;
            }

            FFX_ASSERT(shaderBlob.srvTextureCount + shaderBlob.srvBufferCount <= FFX_MAX_NUM_SRVS);
            FFX_ASSERT(shaderBlob.uavTextureCount + shaderBlob.uavBufferCount <= FFX_MAX_NUM_UAVS);
            FFX_ASSERT(shaderBlob.cbvCount <= FFX_MAX_NUM_CONST_BUFFERS);

            // populate the pass.
            outPipeline->srvTextureCount = shaderBlob.srvTextureCount;
            outPipeline->uavTextureCount = shaderBlob.uavTextureCount;
            outPipeline->srvBufferCount  = shaderBlob.srvBufferCount;
            outPipeline->uavBufferCount  = shaderBlob.uavBufferCount;

            // Todo when needed
            //outPipeline->samplerCount      = shaderBlob.samplerCount;
            //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

            outPipeline->constCount = shaderBlob.cbvCount;
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvTextureCount; ++srvIndex)
            {
                outPipeline->srvTextureBindings[srvIndex].slotIndex = shaderBlob.boundSRVTextures[srvIndex];
                outPipeline->srvTextureBindings[srvIndex].bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];
                wcscpy_s(outPipeline->srvTextureBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVTextureNames[srvIndex]).c_str());
            }
            for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavTextureCount; ++uavIndex)
            {
                outPipeline->uavTextureBindings[uavIndex].slotIndex = shaderBlob.boundUAVTextures[uavIndex];
                outPipeline->uavTextureBindings[uavIndex].bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];
                wcscpy_s(outPipeline->uavTextureBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVTextureNames[uavIndex]).c_str());
            }
            for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvBufferCount; ++srvIndex)
            {
                outPipeline->srvBufferBindings[srvIndex].slotIndex = shaderBlob.boundSRVBuffers[srvIndex];
                outPipeline->srvBufferBindings[srvIndex].bindCount = shaderBlob.boundSRVBufferCounts[srvIndex];
                wcscpy_s(outPipeline->srvBufferBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVBufferNames[srvIndex]).c_str());
            }
            for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavBufferCount; ++uavIndex)
            {
                outPipeline->uavBufferBindings[uavIndex].slotIndex = shaderBlob.boundUAVBuffers[uavIndex];
                outPipeline->uavBufferBindings[uavIndex].bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];
                wcscpy_s(outPipeline->uavBufferBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVBufferNames[uavIndex]).c_str());
            }
            for (uint32_t cbIndex = 0; cbIndex < outPipeline->constCount; ++cbIndex)
            {
                outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlob.boundConstantBuffers[cbIndex];
                outPipeline->constantBufferBindings[cbIndex].bindCount = shaderBlob.boundConstantBufferCounts[cbIndex];
                wcscpy_s(outPipeline->constantBufferBindings[cbIndex].name, converter.from_bytes(shaderBlob.boundConstantBufferNames[cbIndex]).c_str());
            }

            // create the PSO
            D3D12_COMPUTE_PIPELINE_STATE_DESC dx12PipelineStateDescription = {};
            dx12PipelineStateDescription.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
            dx12PipelineStateDescription.pRootSignature                    = dx12RootSignature;
            dx12PipelineStateDescription.CS.pShaderBytecode                = shaderBlob.data;
            dx12PipelineStateDescription.CS.BytecodeLength                 = shaderBlob.size;

            if (FAILED(dx12Device->CreateComputePipelineState(&dx12PipelineStateDescription,
                                                              IID_PPV_ARGS(reinterpret_cast<ID3D12PipelineState**>(&outPipeline->pipeline)))))
                return FFX_ERROR_BACKEND_API_ERROR;

            // Set the pipeline name
            reinterpret_cast<ID3D12PipelineState*>(outPipeline->pipeline)->SetName(pipelineDescription->name);
        }

        return FFX_OK;
    }

    FfxCreatePipelineFunc fpFallback = nullptr;

    FfxErrorCode CreatePipelineFromSource(FfxInterface* backendInterface,
        FfxEffect                     effect,
        FfxPass                       pass,
        uint32_t                      permutationOptions,
        const FfxPipelineDescription* pipelineDescription,
        FfxUInt32                     effectContextId,
        FfxPipelineState* outPipeline)
    {
        FfxErrorCode result   = FFX_ERROR_BACKEND_API_ERROR;

        static bool         bUseHLSL = true;
        if (bUseHLSL)
        {
            result = CreatePipelineFromSourceInternal(backendInterface, effect, pass, permutationOptions, pipelineDescription, effectContextId, outPipeline);
        }
        // if we run into troubles compiling from source, try fallback to precompiled shader
        if (result != FFX_OK)
        {
            FFX_ASSERT(false && "HLSL compile failed, reverting to pre-built pipeline");
            result = fpFallback(backendInterface, effect, pass, permutationOptions, pipelineDescription, effectContextId, outPipeline);
        }
        return result;
    }

    void SetupFallback(FfxCreatePipelineFunc func)
    {
        fpFallback = func;
    }
}
