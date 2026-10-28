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

#include "../misc/helpers.h"
#include "../misc/math.h"
#include "sampler.h"

#include "../../../OpenSource/nlohmann/json.hpp"
using json = nlohmann::ordered_json;

#include <map>

namespace cauldron
{
    class Texture;

    /// An enumeration of supported texture class types
    ///
    /// @ingroup CauldronRender
    enum class TextureClass : int32_t
    {
        Albedo = 0,     ///< Texture represents albedo data.
        Normal,         ///< Texture represents normal data.
        Emissive,       ///< Texture represents emissive data.
        Occlusion,      ///< Texture represents occlusion data.
        MetalRough,     ///< Texture represents metal-rough data.
        SpecGloss,      ///< Texture represents spec-gloss data.

        Count,          ///< Number of texture classes.
    };

    /// An enumeration of supported surface blend types
    ///
    /// @ingroup CauldronRender
    enum class MaterialBlend : int32_t
    {
        Opaque = 0,     ///< Opaque blend type (no blending).
        Mask,           ///< Mask blend type (alpha to mask).
        AlphaBlend,     ///< Alpha blend type (Cb(1 - alpha) + Ca).

        Count,          ///< Number of blend types.
    };

    /// Helper populated by GLTFLoader when loading glTF image assets.
    struct GLTFImageInfo
    {
        std::wstring Path;
        bool         Exists     = false;
        bool         Referenced = false;  ///< Set after materials are parsed to identify which images are needed.
        bool         IsSRGB     = false;  ///< Set after materials are parsed; true if any material slot uses this image as SRGB.
    };

    /// Structure representing the needed information for the usage of a material texture.
    ///
    /// @ingroup CauldronRender
    struct TextureInfo
    {
        struct SourceInfo
        {
            static const int INDEX_INVALID = -1;
            int  Index  = INDEX_INVALID;    /// Resolved glTF image index (DDS preferred when available and present on disk)
            bool IsSRGB = false;            /// Whether the texture is gonna be sRGB or not
        } Source;
        const Texture*  pTexture = nullptr;     ///< The <c><i>Texture</i></c> resource pointer to the backing resource.
        float           Multiplier = 1.f;       ///< The scale or strength depending on texture type.
        uint32_t        UVSet = 0;              ///< The uv set associated with this texture.
        SamplerDesc     TexSamplerDesc = {};    ///< A sampler description for the needed sampler to read the texture as intended for a given material.
    };

    /**
     * @class Material
     *
     * The <c><i>FidelityFX Cauldron Framework</i></c> material representation.
     *
     * @ingroup CauldronRender
     */
    class Material
    {
    public:

        /**
         * @brief   Construction.
         */
        Material();

        /**
         * @brief   Destruction.
         */
        ~Material();

        /**
         * @brief   Returns the material name. This is the name of the material as defined in the glTF file.
         */

        const std::wstring& GetName() const { return m_Name; }

        /**
         * @brief   Returns the TextureInfo associated with a TextureClass in the material (if present).
         */
        const TextureInfo* GetTextureInfo(TextureClass tableEntry) const;

        /**
         * @brief   Returns true if the material has metal-rough data. A material can not be both metal-rough and spec-gloss simultaneously.
         */
        bool HasPBRMetalRough() const { return m_MetalRough; }

        /**
         * @brief   Returns true if the material has spec-gloss data. A material can not be both metal-rough and spec-gloss simultaneously.
         */
        bool HasPBRSpecGloss() const { return m_SpecGloss; }

        /**
         * @brief   Returns true if the material has PBR information (high-level check).
         */
        bool HasPBRInfo() const { return HasPBRMetalRough() || HasPBRSpecGloss(); }

        /**
         * @brief   Returns true if the material represents double-sided geometry.
         */
        bool HasDoubleSided() const { return m_DoubleSided; }

        /**
         * @brief   Set the double-sided state of a material.
         */
        void SetDoubleSided(bool flag) { m_DoubleSided = flag; }

        /**
         * @brief   Sets the material's PBR workflow to metal-roughness (true) or spec-glossiness (false).
         *          Must be called before SetPBRInfo so the union is interpreted correctly.
         */
        void SetMetalRough(bool flag) { m_MetalRough = flag; m_SpecGloss = !flag; }

        /**
         * @brief   Sets the material name.
         */
        void SetName(std::wstring name) { m_Name = std::move(name); }

        /**
         * @brief   Gets the material's albedo color.
         */
        Vec4 GetAlbedoColor() const { return m_Albedo; }

        /**
         * @brief   Sets the material's albedo color.
         */
        void SetAlbedoColor(const Vec4& albedo) { m_Albedo = albedo; }

        /**
         * @brief   Gets the material's emissive color.
         */
        Vec4 GetEmissiveColor() const { return m_Emmissive; }

        /**
         * @brief   Sets the material's emissive color.
         */
        void SetEmissiveColor(const Vec4& emissive) { m_Emmissive = emissive; }

        /**
         * @brief   Gets the material's PBR information. Values depend on if material is spec-gloss or metal-rough.
         */
        Vec4 GetPBRInfo() const { return HasPBRMetalRough() ? m_PBRInfo.m_MetalRough : m_PBRInfo.m_SpecGloss; }

        /**
         * @brief   Sets the material's PBR information.
         *          For metal-rough: x=metalness, y=roughness.
         *          For spec-gloss:  xyz=specular, w=glossiness.
         */
        void SetPBRInfo(const Vec4& pbrInfo) { HasPBRMetalRough() ? m_PBRInfo.m_MetalRough = pbrInfo : m_PBRInfo.m_SpecGloss = pbrInfo; }

        /**
         * @brief   Gets the multiplier factor for material's texture
         */
        float GetTextureMultiplier(TextureClass textureClass, float defaultValue = 1.0f) const
        {
            const TextureInfo* pTI = m_TextureMappings[static_cast<uint32_t>(textureClass)];
            return pTI ? pTI->Multiplier : defaultValue;
        }

        /**
         * @brief   Sets the multiplier factor for material's texture
         */
        void SetTextureMultiplier(TextureClass textureClass, float factor)
        {
            TextureInfo* pTI = m_TextureMappings[static_cast<uint32_t>(textureClass)];
            if (pTI)
                pTI->Multiplier = factor;
        }

        /**
         * @brief   Gets the material's blend mode.
         */
        MaterialBlend GetBlendMode() const { return m_BlendMode; }

        /**
         * @brief   Gets the material's alpha cutoff value.
         */
        float GetAlphaCutOff() const { return m_AlphaCutoff; }

        /**
         * @brief   Gets the material's transmission factor (KHR_materials_transmission). 0 = opaque, 1 = fully transmissive.
         */
        float GetTransmissionFactor() const { return m_TransmissionFactor; }

        /**
         * @brief   Sets the material's transmission factor (KHR_materials_transmission). 0 = opaque, 1 = fully transmissive.
         */
        void SetTransmissionFactor(float factor) { m_TransmissionFactor = factor; }

        /**
         * @brief   Initializes a material from loaded json glTF data.
         */
        void InitFromGLTFData(const json& materialData, const json& textureData, std::vector<SamplerDesc>& textureSamplers, const std::vector<GLTFImageInfo>& imageInfos);

    private:
        NO_COPY(Material);
        NO_MOVE(Material);

        std::wstring m_Name;

        std::vector<TextureInfo*>  m_TextureMappings = {};

        static const Vec4 s_BlackTrans;
        static const Vec4 s_WhiteOpaque;

        // Constant values if not provided via texture input
        Vec4            m_Albedo = s_WhiteOpaque;
        Vec4            m_Emmissive = s_BlackTrans;
        MaterialBlend   m_BlendMode = MaterialBlend::Opaque;
        float           m_AlphaCutoff = 0.5f;
        float           m_TransmissionFactor = 0.f;  ///< KHR_materials_transmission: fraction of light transmitted through the surface (0 = opaque, 1 = fully transmissive).

        union PBRInfo {
            Vec4 m_MetalRough = s_WhiteOpaque;
            Vec4 m_SpecGloss;
        } m_PBRInfo;

        // Material properties
        bool m_MetalRough = false;
        bool m_SpecGloss = false;
        bool m_DoubleSided = false;
    };
}
