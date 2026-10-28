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

#include "material.h"
#include "../misc/assert.h"

namespace cauldron
{
    const Vec4 Material::s_BlackTrans = Vec4(0, 0, 0, 0);
    const Vec4 Material::s_WhiteOpaque = Vec4(1, 1, 1, 1);

    Material::Material()
    {
        m_TextureMappings.resize(static_cast<uint32_t>(TextureClass::Count));
    }

    Material::~Material()
    {
        // Clear all texture mappings
        for (auto textureIter = m_TextureMappings.begin(); textureIter != m_TextureMappings.end(); ++textureIter)
            delete (*textureIter);
        m_TextureMappings.clear();
    }

    const TextureInfo* Material::GetTextureInfo(TextureClass tableEntry) const
    {
        return m_TextureMappings[static_cast<uint32_t>(tableEntry)];
    }

    static TextureInfo::SourceInfo GetTextureSourceFromGLTFData(const json& textureData, bool isSRGB, const std::vector<GLTFImageInfo>& imageInfos)
    {
        TextureInfo::SourceInfo source;

        if (textureData.find("extensions") != textureData.end())
        {
            const json& extensions = textureData["extensions"];
            if (extensions.find("MSFT_texture_dds") != extensions.end())
            {
                const json& msft_texture_dds = extensions["MSFT_texture_dds"];
                if (msft_texture_dds.find("source") != msft_texture_dds.end())
                {
                    int ddsIdx = msft_texture_dds["source"];
                    // Validate separately so we can retry with normal source
                    if (ddsIdx >= 0 && ddsIdx < static_cast<int>(imageInfos.size()) && imageInfos[ddsIdx].Exists)
                        source.Index = ddsIdx;
                }
            }
        }

        if (source.Index == TextureInfo::SourceInfo::INDEX_INVALID && textureData.find("source") != textureData.end())
            source.Index = textureData["source"];

        CauldronAssert(ASSERT_CRITICAL, source.Index >= 0 && source.Index < static_cast<int>(imageInfos.size()),
            L"Invalid or missing image source for texture: %d", source.Index);

        source.IsSRGB = isSRGB;
        return source;
    }

    void Material::InitFromGLTFData(const json& materialData, const json& textureData, std::vector<SamplerDesc>& textureSamplers, const std::vector<GLTFImageInfo>& imageInfos)
    {
        static const SamplerDesc GLTF_DEFAULT_SAMPLER
        {
             FilterFunc::MinMagMipLinear,
             AddressMode::Wrap,
             AddressMode::Wrap,
             AddressMode::Wrap,
             ComparisonFunc::Never,
             0.0f,
             (std::numeric_limits<float>::max)(),
             0.0f,
             16
        };

        m_Name = L"Unspecified";
        if (materialData.find("name") != materialData.end())
        {
            auto iter = materialData["name"];
            m_Name = StringToWString(iter.get<std::string>());
        }

        if (materialData.find("doubleSided") != materialData.end())
            m_DoubleSided = true;

        if (materialData.find("alphaMode") != materialData.end())
        {
            // Setup alpha mode information (default is opaque)
            std::string blendMode = materialData["alphaMode"].get<std::string>();
            if (blendMode == "MASK")
                m_BlendMode = MaterialBlend::Mask;
            else if (blendMode == "BLEND")
                m_BlendMode = MaterialBlend::AlphaBlend;

            // Check if we have an alpha cutoff
            if (materialData.find("alphaCutoff") != materialData.end())
                m_AlphaCutoff = materialData["alphaCutoff"];
        }

        // KHR_materials_transmission: transmissionFactor is orthogonal to alphaMode and describes
        // physical light transmission (e.g. glass). A value of 0 means fully opaque, 1 means fully transmissive.
        if (materialData.find("extensions") != materialData.end())
        {
            const json& extensions = materialData["extensions"];
            if (extensions.find("KHR_materials_transmission") != extensions.end())
            {
                const json& transmission = extensions["KHR_materials_transmission"];
                if (transmission.find("transmissionFactor") != transmission.end())
                    m_TransmissionFactor = transmission["transmissionFactor"].get<float>();
            }
        }

        // Load data we are tracking

        // Emissive value
        if (materialData.find("emissiveFactor") != materialData.end())
            m_Emmissive = Vec4(materialData["emissiveFactor"][0], materialData["emissiveFactor"][1], materialData["emissiveFactor"][2], 0);

        // Normal texture
        if (materialData.find("normalTexture") != materialData.end())
        {
            TextureInfo* pTexInfo = m_TextureMappings[static_cast<uint32_t>(TextureClass::Normal)] = new TextureInfo();
            CauldronAssert(ASSERT_ERROR, pTexInfo, L"Normal Texture info could not be created");
            if (pTexInfo)
            {
                if (materialData["normalTexture"].find("scale") != materialData["normalTexture"].end())
                    pTexInfo->Multiplier = materialData["normalTexture"]["scale"];
                
                // Store the index of the texture this will point to for easy mapping later once textures are loaded
                int32_t index = materialData["normalTexture"]["index"];
                pTexInfo->Source = GetTextureSourceFromGLTFData(textureData[index], false, imageInfos);

                // Store sampler information (or default) if we have it
                pTexInfo->TexSamplerDesc = GLTF_DEFAULT_SAMPLER;
                if (textureData[index].find("sampler") != textureData[index].end())
                    pTexInfo->TexSamplerDesc = textureSamplers[textureData[index]["sampler"]];

                // Add uv set to use if available
                if (materialData["normalTexture"].find("texCoord") != materialData["normalTexture"].end())
                    pTexInfo->UVSet = materialData["normalTexture"]["texCoord"];
                else
                    pTexInfo->UVSet = 0;
            }
        }

        // Occlusion texture
        if (materialData.find("occlusionTexture") != materialData.end())
        {
            TextureInfo* pTexInfo = m_TextureMappings[static_cast<uint32_t>(TextureClass::Occlusion)] = new TextureInfo();
            CauldronAssert(ASSERT_ERROR, pTexInfo, L"Occlusion Texture info could not be created");
            if (pTexInfo)
            {
                if (materialData["occlusionTexture"].find("strength") != materialData["occlusionTexture"].end())
                    pTexInfo->Multiplier = materialData["occlusionTexture"]["strength"];

                // Store the index of the texture this will point to for easy mapping later once textures are loaded
                int32_t index = materialData["occlusionTexture"]["index"];
                pTexInfo->Source = GetTextureSourceFromGLTFData(textureData[index], false, imageInfos);

                // Store sampler information (or default) if we have it
                pTexInfo->TexSamplerDesc = GLTF_DEFAULT_SAMPLER;
                if (textureData[index].find("sampler") != textureData[index].end())
                    pTexInfo->TexSamplerDesc = textureSamplers[textureData[index]["sampler"]];

                // Add uv set to use if available
                if (materialData["occlusionTexture"].find("texCoord") != materialData["occlusionTexture"].end())
                    pTexInfo->UVSet = materialData["occlusionTexture"]["texCoord"];
                else
                    pTexInfo->UVSet = 0;
            }
        }

        // Emmissive texture
        if (materialData.find("emissiveTexture") != materialData.end())
        {
            TextureInfo* pTexInfo = m_TextureMappings[static_cast<uint32_t>(TextureClass::Emissive)] = new TextureInfo();
            CauldronAssert(ASSERT_ERROR, pTexInfo, L"Emmissive Texture info could not be created");
            if (pTexInfo)
            {
                // Store the index of the texture this will point to for easy mapping later once textures are loaded
                int32_t index = materialData["emissiveTexture"]["index"];
                pTexInfo->Source = GetTextureSourceFromGLTFData(textureData[index], true, imageInfos); // Flag this texture for needing SRGB

                // Store sampler information (or default) if we have it
                pTexInfo->TexSamplerDesc = GLTF_DEFAULT_SAMPLER;
                if (textureData[index].find("sampler") != textureData[index].end())
                    pTexInfo->TexSamplerDesc = textureSamplers[textureData[index]["sampler"]];

                // Add uv set to use if available
                if (materialData["emissiveTexture"].find("texCoord") != materialData["emissiveTexture"].end())
                    pTexInfo->UVSet = materialData["emissiveTexture"]["texCoord"];
                else
                    pTexInfo->UVSet = 0;
            }
        }

        // PBR Metal-Roughness
        if (materialData.find("pbrMetallicRoughness") != materialData.end())
        {
            const json& metalRough = materialData["pbrMetallicRoughness"];
            m_MetalRough = true;
                
            if (metalRough.find("metallicFactor") != metalRough.end())
                m_PBRInfo.m_MetalRough.setX(metalRough["metallicFactor"]);
            else
                m_PBRInfo.m_MetalRough.setX(1.f);

            if (metalRough.find("roughnessFactor") != metalRough.end())
                m_PBRInfo.m_MetalRough.setY(metalRough["roughnessFactor"]);

            if (metalRough.find("baseColorFactor") != metalRough.end())
                m_Albedo = Vec4(metalRough["baseColorFactor"][0], metalRough["baseColorFactor"][1],
                                         metalRough["baseColorFactor"][2], metalRough["baseColorFactor"][3]);

            // Albedo texture
            if (metalRough.find("baseColorTexture") != metalRough.end())
            {
                TextureInfo* pTexInfo = m_TextureMappings[static_cast<uint32_t>(TextureClass::Albedo)] = new TextureInfo();
                CauldronAssert(ASSERT_ERROR, pTexInfo, L"Albedo Texture info could not be created");
                if (pTexInfo)
                {
                    const json& baseColorTexture = metalRough["baseColorTexture"];

                    // Store the index of the texture this will point to for easy mapping later once textures are loaded
                    int32_t index = baseColorTexture["index"];
                    pTexInfo->Source = GetTextureSourceFromGLTFData(textureData[index], true, imageInfos); // Flag this texture for needing SRGB

                    // Store sampler information (or default) if we have it
                    pTexInfo->TexSamplerDesc = GLTF_DEFAULT_SAMPLER;
                    if (textureData[index].find("sampler") != textureData[index].end())
                        pTexInfo->TexSamplerDesc = textureSamplers[textureData[index]["sampler"]];

                    // Add uv set to use if available
                    if (baseColorTexture.find("texCoord") != baseColorTexture.end())
                        pTexInfo->UVSet = baseColorTexture["texCoord"];
                    else
                        pTexInfo->UVSet = 0;
                }
            }

            // MetalRough texture
            if (metalRough.find("metallicRoughnessTexture") != metalRough.end())
            {
                TextureInfo* pTexInfo = m_TextureMappings[static_cast<uint32_t>(TextureClass::MetalRough)] = new TextureInfo();
                CauldronAssert(ASSERT_ERROR, pTexInfo, L"Metal-Rough Texture info could not be created");
                if (pTexInfo)
                {
                    const json& metallicRoughness = metalRough["metallicRoughnessTexture"];

                    // Store the index of the texture this will point to for easy mapping later once textures are loaded
                    int32_t index = metallicRoughness["index"];
                    pTexInfo->Source = GetTextureSourceFromGLTFData(textureData[index], false, imageInfos);

                    // Store sampler information (or default) if we have it
                    pTexInfo->TexSamplerDesc = GLTF_DEFAULT_SAMPLER;
                    if (textureData[index].find("sampler") != textureData[index].end())
                        pTexInfo->TexSamplerDesc = textureSamplers[textureData[index]["sampler"]];

                    // Add uv set to use if available
                    if (metallicRoughness.find("texCoord") != metallicRoughness.end())
                        pTexInfo->UVSet = metallicRoughness["texCoord"];
                    else
                        pTexInfo->UVSet = 0;
                }
            }
        }

        // PBR Spec-Glossiness
        else if (materialData.find("extensions") != materialData.end() && materialData["extensions"].find("KHR_materials_pbrSpecularGlossiness") != materialData["extensions"].end())
        {
            const json& specGloss = materialData["extensions"]["KHR_materials_pbrSpecularGlossiness"];
            m_SpecGloss = true;

            if (specGloss.find("diffuseFactor") != specGloss.end())
                m_Albedo.setXYZ(Vec3(specGloss["diffuseFactor"][0], specGloss["diffuseFactor"][1], specGloss["diffuseFactor"][2]));

            Vec4 specGlossValue = s_WhiteOpaque;
            if (specGloss.find("glossinessFactor") != specGloss.end())
                specGlossValue.setW(specGloss["glossinessFactor"]);

            if (specGloss.find("specularFactor") != specGloss.end())
                specGlossValue.setXYZ(Vec3(specGloss["specularFactor"][0], specGloss["specularFactor"][1], specGloss["specularFactor"][2]));
            m_PBRInfo.m_SpecGloss = specGlossValue;

            // Albedo texture
            if (specGloss.find("diffuseTexture") != specGloss.end())
            {
                TextureInfo* pTexInfo = m_TextureMappings[static_cast<uint32_t>(TextureClass::Albedo)] = new TextureInfo();
                CauldronAssert(ASSERT_ERROR, pTexInfo, L"Albedo Texture info could not be created");
                if (pTexInfo)
                {
                    // Store the index of the texture this will point to for easy mapping later once textures are loaded
                    int32_t index = specGloss["diffuseTexture"]["index"];
                    pTexInfo->Source = GetTextureSourceFromGLTFData(textureData[index], true, imageInfos); // Flag this texture for needing SRGB

                    // Store sampler information (or default) if we have it
                    pTexInfo->TexSamplerDesc = GLTF_DEFAULT_SAMPLER;
                    if (textureData[index].find("sampler") != textureData[index].end())
                        pTexInfo->TexSamplerDesc = textureSamplers[textureData[index]["sampler"]];

                    // Add uv set to use if available
                    if (specGloss["diffuseTexture"].find("texCoord") != specGloss["diffuseTexture"].end())
                        pTexInfo->UVSet = specGloss["diffuseTexture"]["texCoord"];
                    else
                        pTexInfo->UVSet = 0;
                }
            }

            // SpecGloss texture
            if (specGloss.find("specularGlossinessTexture") != specGloss.end())
            {
                TextureInfo* pTexInfo = m_TextureMappings[static_cast<uint32_t>(TextureClass::SpecGloss)] = new TextureInfo();
                CauldronAssert(ASSERT_ERROR, pTexInfo, L"SpecGloss Texture info could not be created");
                if (pTexInfo)
                {
                    // Store the index of the texture this will point to for easy mapping later once textures are loaded
                    int32_t index = specGloss["specularGlossinessTexture"]["index"];
                    pTexInfo->Source = GetTextureSourceFromGLTFData(textureData[index], true, imageInfos); // Flag this texture for needing SRGB

                    // Store sampler information (or default) if we have it
                    pTexInfo->TexSamplerDesc = GLTF_DEFAULT_SAMPLER;
                    if (textureData[index].find("sampler") != textureData[index].end())
                        pTexInfo->TexSamplerDesc = textureSamplers[textureData[index]["sampler"]];

                    // Add uv set to use if available
                    if (specGloss["specularGlossinessTexture"].find("texCoord") != specGloss["specularGlossinessTexture"].end())
                        pTexInfo->UVSet = specGloss["specularGlossinessTexture"]["texCoord"];
                    else
                        pTexInfo->UVSet = 0;
                }
            }
        }
    }
}
