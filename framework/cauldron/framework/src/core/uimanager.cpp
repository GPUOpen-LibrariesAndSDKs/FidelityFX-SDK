// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2023 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "core/uimanager.h"
#include "core/framework.h"
#include "core/uibackend.h"

namespace cauldron
{
    UIManager::UIManager()
    {
        // Create the UI back end
        m_pUIBackEnd = UIBackend::CreateUIBackend();
    }

    UIManager::~UIManager()
    {
        delete m_pUIBackEnd;
    }

    void UIManager::Update(double deltaTime)
    {
        if (!m_pUIBackEnd->Ready())
            return;

        m_ProcessingUI = true;

        // Does back end updates, sets up input for the platform
        // and calls all UI building blocks
        m_pUIBackEnd->Update(deltaTime);

        m_ProcessingUI = false;
    }

    bool UIManager::UIBackendMessageHandler(void* pMessage)
    {
        return m_pUIBackEnd->MessageHandler(pMessage);
    }

    void UIManager::RegisterUIElements(UISection& uiSection)
    {
        CauldronAssert(ASSERT_CRITICAL, !m_ProcessingUI, L"UI element stack cannot be updated during UI update cycle.");

        // First, check if there is an existing section for this element
        // If so, add to it
        std::vector<UISection>::iterator sectionItr = m_UIGeneralLayout.begin();
        bool foundSection = false;
        while (sectionItr != m_UIGeneralLayout.end())
        {
            if (sectionItr->SectionName == uiSection.SectionName)
            {
                foundSection = true;
                break;
            }

            ++sectionItr;
        }

        // If we didn't find an existing section, add a new one
        if (!foundSection)
        {
            UISection newSection;
            newSection.SectionName = uiSection.SectionName;
            newSection.SectionType = uiSection.SectionType;
            
            // If this is a sample UI, push to the front
            if (newSection.SectionType == UISectionType::Sample)
            {
                sectionItr = m_UIGeneralLayout.insert(m_UIGeneralLayout.begin(), newSection);
            }
            // Otherwise, add it to the back
            else
            {
                m_UIGeneralLayout.push_back(newSection);
                sectionItr = m_UIGeneralLayout.end() - 1;
            }
        }

        // Add all of the elements to the existing/new section
        for (auto& elementItr = uiSection.SectionElements.begin(); elementItr != uiSection.SectionElements.end(); ++elementItr)
            sectionItr->SectionElements.push_back(*elementItr);
    }

    void UIManager::UnRegisterUIElements(UISection& uiSection)
    {
        CauldronAssert(ASSERT_CRITICAL, !m_ProcessingUI, L"UI element stack cannot be updated during UI update cycle.");

        // Find the section
        std::vector<UISection>::iterator sectionItr   = m_UIGeneralLayout.begin();
        bool                             foundSection = false;
        while (sectionItr != m_UIGeneralLayout.end())
        {
            if (sectionItr->SectionName == uiSection.SectionName)
            {
                foundSection = true;
                break;
            }

            ++sectionItr;
        }
        CauldronAssert(ASSERT_CRITICAL, foundSection, L"Could not find UI section %hs for element unregistration", uiSection.SectionName.c_str());

        // If this section has the same number of elements we are trying to remove, remove the entire section
        if (sectionItr->SectionElements.size() == uiSection.SectionElements.size())
        {
            m_UIGeneralLayout.erase(sectionItr);
            return;
        }

        // If this section has elements from multiple entries, find the correct first element in the list we want to remove
        auto& sectionStartItr = uiSection.SectionElements.begin();
        for (auto elementItr = sectionItr->SectionElements.begin(); elementItr != sectionItr->SectionElements.end(); ++elementItr)
        {
            if ((*elementItr) == (*sectionStartItr))
            {
                sectionItr->SectionElements.erase(elementItr, elementItr + uiSection.SectionElements.size());
                return;
            }
        }
    }

} // namespace cauldron
