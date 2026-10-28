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

//------------------------------------------------------------------------------
// Sample Includes
//------------------------------------------------------------------------------

// rr::DenoiserRenderModule
#include "denoiserrendermodule.h"

//------------------------------------------------------------------------------
// Cauldron Includes
//------------------------------------------------------------------------------

// json
#include "Cauldron2/framework/core/framework.h"
// cauldron::CommandList
// cauldron::RenderModule
#include "Cauldron2/framework/render/rendermodule.h"

//------------------------------------------------------------------------------
// Cauldron Forward Declarations
//------------------------------------------------------------------------------

namespace cauldron
{
    class Texture;
    class ParameterSet;
    class RootSignature;
    class PipelineObject;
}  // namespace cauldron

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    //--------------------------------------------------------------------------
    // DebugOverlayRenderModule
    //--------------------------------------------------------------------------
    
    class DebugOverlayRenderModule final : public cauldron::RenderModule
    {

    public:

        //----------------------------------------------------------------------
        // Member Methods: Constructors and Destructors
        //----------------------------------------------------------------------
        
        [[nodiscard]]
        DebugOverlayRenderModule();

        ~DebugOverlayRenderModule() override;

        //----------------------------------------------------------------------
        // Member Methods
        //----------------------------------------------------------------------
        
        void Init(const json& initData);

        void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;

    private:

        //----------------------------------------------------------------------
        // Member Variables
        //----------------------------------------------------------------------
        
        const cauldron::Texture* m_pColorTarget = {};

        cauldron::RootSignature* m_pDebugOverlayRootSignature = {};
        cauldron::PipelineObject* m_pDebugOverlayPipeline = {};
        cauldron::ParameterSet* m_pDebugOverlayParameterSet = {};

        const cauldron::Texture* m_pDenoiserDebugView = {};
        DenoiserRenderModule* m_pDenoiserRenderModule = {};
    };
} // namespace rr
