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

#pragma once

#include "misc/helpers.h"
#include "misc/assert.h"

#include <functional>
#include <vector>

namespace cauldron
{
    /**
     * @enum UIElementType
     *
     * UI element types to pick from, add more as needed.
     *
     * @ingroup CauldronCore
     */
    enum class UIElementType
    {
        Text = 0,           ///< Text UI element
        Button,             ///< Button UI element
        Checkbox,           ///< Checkbox UI element
        RadioButton,        ///< Radio button UI element

        Combo,              ///< Combo box UI element
        Slider,             ///< Slider UI element

        Separator,          ///< Separator UI element

        Count               ///< UI element count
    };

    /**
     * @enum UIElement
     *
     * Describes an individual UI element (text, butotn, checkbox, etc.).
     *
     * @ingroup CauldronCore
     */
    struct UIElement
    {
        UIElementType               Type = UIElementType::Text;     // Type of ui element
        std::string                 Description = "";              // Description of ui element

        union                                                       // Data sources (when needed)
        {
            // For direct modification of source data
            void*   pSourceData = nullptr;
            bool*   pBoolSourceData;
            int*    pIntSourceData;
            float*  pFloatSourceData;
        };
        std::string Format = "";                                    // Only FloatSlider and IntSlider require format, with default values of "%.3f" and "%d"

        bool*                       pEnableControl = nullptr;       // To allow conditional enabling of UI element
        std::function<void(void*)>  pCallbackFunction = nullptr;    // Callback (when needed)
        std::vector<std::string>    SelectionOptions = {};
        union
        {
            float   MinFloatValue = 0.f;
            int32_t MinIntValue;
        };

        union
        {
            float   MaxFloatValue = std::numeric_limits<float>::max();
            int32_t MaxIntValue;
        };

        bool        IntRepresentation = false;
        bool        SameLineAsPreviousElement = false;

        UIElement() {}

        bool operator==(const UIElement& rhs)
        {
            // This should be enough to validate an == for now
            return Type == rhs.Type && Description == rhs.Description;
        }
    };

    /**
     * @enum UISectionType
     *
     * UI section classifier. Used for internal ordering in the general UI tab
     *
     * @ingroup CauldronCore
     */
    enum class UISectionType
    {
        Framework = 0,
        Sample,
    };

    /**
     * @enum UISection
     *
     * A UISection is composed of 1 or more <c><i>UIElement</i></c>s. Represents a 
     * section of UI in the general UI tab.
     *
     * @ingroup CauldronCore
     */
    struct UISection
    {
        std::string     SectionName = "";                           ///< Section name
        UISectionType   SectionType = UISectionType::Framework;     ///< <c><i>UISectionType</i></c>
        bool            defaultOpen = true;

        void AddText(const char* pText, bool* pEnabler = nullptr, bool sameLine = false)
        {
            UIElement element;
            element.Type = UIElementType::Text;
            element.Description = pText;
            element.pEnableControl       = pEnabler;
            element.SameLineAsPreviousElement = sameLine;
            SectionElements.push_back(std::move(element));
        }

        void AddButton(const char* pText, std::function<void(void*)> pCallback = nullptr, bool* pEnabler = nullptr, bool sameLine = false)
        {
            UIElement element;
            element.Type = UIElementType::Button;
            element.Description = pText;
            element.pCallbackFunction = pCallback;
            element.pEnableControl = pEnabler;
            element.SameLineAsPreviousElement = sameLine;
            SectionElements.push_back(std::move(element));
        }

        void AddCheckBox(const char* pText, bool* pData, std::function<void(void*)> pCallback = nullptr, bool* pEnabler = nullptr, bool sameLine = false)
        {
            UIElement element;
            element.Type = UIElementType::Checkbox;
            element.Description = pText;
            element.pBoolSourceData = pData;
            element.pCallbackFunction         = pCallback;
            element.pEnableControl = pEnabler;
            element.SameLineAsPreviousElement = sameLine;
            SectionElements.push_back(std::move(element));
        }

        void AddRadioButton(const char* pText, void* pData, const std::vector<std::string>* pOptions, bool* pEnabler = nullptr, bool sameLine = false)
        {
            UIElement element;
            element.Type = UIElementType::RadioButton;
            element.Description = pText;
            element.pSourceData = pData;
            if (pOptions)
                element.SelectionOptions = *pOptions;
            element.pEnableControl = pEnabler;
            element.SameLineAsPreviousElement = sameLine;
            SectionElements.push_back(std::move(element));
        }

        void AddCombo(const char* pText, int32_t* pData, const std::vector<std::string>* pOptions, std::function<void(void*)> pCallback = nullptr, bool* pEnabler = nullptr, bool sameLine = false)
        {
            UIElement element;
            element.Type           = UIElementType::Combo;
            element.Description    = pText;
            element.pIntSourceData = pData;
            if (pOptions)
                element.SelectionOptions = *pOptions;
            element.pCallbackFunction         = pCallback;
            element.pEnableControl = pEnabler;
            element.SameLineAsPreviousElement = sameLine;
            SectionElements.push_back(std::move(element));
        }

        void AddFloatSlider(const char* pText, float* pData, float minValue, float maxValue, std::function<void(void*)> pCallback = nullptr, bool* pEnabler = nullptr, bool sameLine = false, const char* format = "%.3f")
        {
            UIElement element;
            element.Type                      = UIElementType::Slider;
            element.Description               = pText;
            element.pFloatSourceData          = pData;
            element.Format                    = format;
            element.MinFloatValue             = minValue;
            element.MaxFloatValue             = maxValue;
            element.IntRepresentation         = false;
            element.pCallbackFunction         = pCallback;
            element.pEnableControl = pEnabler;
            element.SameLineAsPreviousElement = sameLine;
            SectionElements.push_back(std::move(element));
        }

        void AddIntSlider(const char* pText, int32_t* pData, int32_t minValue, int32_t maxValue, std::function<void(void*)> pCallback = nullptr, bool* pEnabler = nullptr, bool sameLine = false, const char* format = "%d")
        {
            UIElement element;
            element.Type = UIElementType::Slider;
            element.Description = pText;
            element.pIntSourceData = pData;
            element.Format = format;
            element.MinIntValue = minValue;
            element.MaxIntValue = maxValue;
            element.IntRepresentation = true;
            element.pCallbackFunction         = pCallback;
            element.pEnableControl = pEnabler;
            element.SameLineAsPreviousElement = sameLine;
            SectionElements.push_back(std::move(element));
        }

        void AddSeparator()
        {
            UIElement element;
            element.Type = UIElementType::Separator;
        }

        std::vector<UIElement>          SectionElements = {};       // Section ui elements
    };



    class UIBackend;

    /**
     * @enum UIManager
     *
     * The UIManager instance is responsible for setting up the individual ui elements for the General UI tab.
     * This is the main tab of the graphics framework and is used to convey the bulk of the sample information.
     *
     * @ingroup CauldronCore
     */
    class UIManager
    {
    public:

        /**
         * @brief   Constructor. Creates the UI backend.
         */
        UIManager();

        /**
         * @brief   Destructor. Destroys the UI backend.
         */
        virtual ~UIManager();

        /**
         * @brief   Updates the UI for the frame, going through the <c><i>UIBackend</i></c>.
         */
        void Update(double deltaTime);

        /**
         * @brief   Message handler for the UIManager. Calls through to the <c><i>UIBackend</i></c>.
         */
        bool UIBackendMessageHandler(void* pMessage);

        /**
         * @brief   Registers a new <c><i>UISection</i></c>. If the <c><i>UISection</i></c> is already present
         *          new <c><i>UIElement/i></c>s will be added to it.
         */
        void RegisterUIElements(UISection& uiSection);

        /**
         * @brief   Unregisters a <c><i>UISection</i></c>. This will make the <c><i>UISection</i></c> stop rendering.
         */
        void UnRegisterUIElements(UISection& uiSection);

        /**
         * @brief   Gets all <c><i>UISection</i></c> elements that make up the general layout tab.
         */
        const std::vector<UISection>& GetGeneralLayout() const { return m_UIGeneralLayout; }
        std::vector<UISection>& GetGeneralLayout() { return m_UIGeneralLayout; }

    private:

        // No Copy, No Move
        NO_COPY(UIManager);
        NO_MOVE(UIManager);

    private:

        UIBackend*              m_pUIBackEnd = nullptr;
        std::vector<UISection>  m_UIGeneralLayout = {};     // Layout for the general tab in the UI (all elements go here under various headings)
        std::atomic_bool        m_ProcessingUI = false;
    };

} // namespace cauldron
