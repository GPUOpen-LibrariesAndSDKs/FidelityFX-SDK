// AMD Cauldron code
//
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#if defined(_VK)

#include "core/win/framework_win.h" // VK builds imply _WIN is defined
#include "misc/assert.h"

#include "render/vk/commandlist_vk.h"
#include "render/vk/device_vk.h"
#include "render/vk/profiler_vk.h"
#include "render/vk/sampler_vk.h"
#include "render/vk/swapchain_vk.h"
#include "render/vk/texture_vk.h"
#include "render/vk/uploadheap_vk.h"

#include <vulkan/vulkan_win32.h>
#include <map>

// macro to get the procedure address of vulkan extensions
#define GET_INSTANCE_PROC_ADDR(name) m_##name = (PFN_##name)vkGetInstanceProcAddr(m_Instance, #name);
#define GET_DEVICE_PROC_ADDR(name) m_##name = (PFN_##name)vkGetDeviceProcAddr(m_Device, #name);
#define SET_FEATURE_IF_SUPPORTED(name) physicalDeviceFeatures.##name = supportedPhysicalDeviceFeatures.##name;\
                                       CauldronAssert(ASSERT_WARNING, physicalDeviceFeatures.##name == VK_TRUE, L"" #name " physical device feature requested but not supported");

#define CHECK_FEATURE_SUPPORT(name) CauldronAssert(ASSERT_WARNING, physicalDeviceFeatures.##name == VK_TRUE, L"" #name " physical device feature requested but not supported");
#define CHECK_FEATURE_SUPPORT_11(name) CauldronAssert(ASSERT_WARNING, vulkan11Features.##name == VK_TRUE, L"" #name " physical device feature for Vulkan 1.1 requested but not supported");
#define CHECK_FEATURE_SUPPORT_12(name) CauldronAssert(ASSERT_WARNING, vulkan12Features.##name == VK_TRUE, L"" #name " physical device feature for Vulkan 1.2 requested but not supported");

namespace cauldron
{
    uint32_t GetScore(VkPhysicalDevice physicalDevice, uint32_t requestedVulkanVersion)
    {
        uint32_t score = 0;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        // Use the features for a more precise way to select the GPU
        //VkPhysicalDeviceFeatures deviceFeatures;
        //vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

        // if the api version isn't enough, reject the device
        if (deviceProperties.apiVersion < requestedVulkanVersion)
            return 0;

        switch (deviceProperties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 10000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 100;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 10;
            break;
        default:
            break;
        }

        // TODO: add other constraints

        return score;
    }

    //
    // Select the best physical device.
    // For now, the code just gets the first discrete GPU.
    // If none is found, it default to an integrated then virtual then cpu one
    //
    VkPhysicalDevice SelectPhysicalDevice(std::vector<VkPhysicalDevice>& physicalDevices, uint32_t requestedVulkanVersion)
    {
        CauldronAssert(AssertLevel::ASSERT_CRITICAL, physicalDevices.size() > 0, L"No GPU found");

        std::multimap<uint32_t, VkPhysicalDevice> ratings;

        for (auto it = physicalDevices.begin(); it != physicalDevices.end(); ++it)
        {
            uint32_t score = GetScore(*it, requestedVulkanVersion);
            if (score > 0)
                ratings.insert(std::make_pair(score, *it));
        }

        CauldronAssert(AssertLevel::ASSERT_CRITICAL, ratings.size() > 0, L"No GPU satisfying the conditions found");

        return ratings.rbegin()->second;
    }

    //
    // Helper class that manages extensions
    //
    class CreatorBase
    {
    public:
        CreatorBase()
            : m_ExtensionProperties()
            , m_ExtensionNames()
        {
        }

        bool TryAddExtension(const char* pExtensionName)
        {
            if (IsExtensionPresent(pExtensionName))
            {
                m_ExtensionNames.push_back(pExtensionName);
                return true;
            }
            else
            {
                std::wstring extensionName = StringToWString(pExtensionName);
                CauldronWarning(L"Extension %ls not found", extensionName.c_str());
                return false;
            }
        }

    private:
        bool IsExtensionPresent(const char* pExtensionName)
        {
            return std::find_if(
                       m_ExtensionProperties.begin(), m_ExtensionProperties.end(),
                [pExtensionName](const VkExtensionProperties& extensionProps) -> bool {
                    return strcmp(extensionProps.extensionName, pExtensionName) == 0;
                       }) != m_ExtensionProperties.end();
        }

    protected:
        std::vector<VkExtensionProperties> m_ExtensionProperties;
        std::vector<const char*> m_ExtensionNames;
    };

    class Appender
    {
    public:
        Appender()
            : m_pNext{nullptr}
        {
        }

        template <typename T>
        void AppendNext(T* newNext, VkStructureType structureType)
        {
            newNext->sType = structureType;
            newNext->pNext = m_pNext;
            m_pNext        = newNext;
        }

        template <typename T>
        void AppendNext(T* newNext)
        {
            newNext->pNext = m_pNext;
            m_pNext        = newNext;
        }

        void* GetNext() const
        {
            return m_pNext;
        }

        void Clear()
        {
            m_pNext = nullptr;
        }

    protected:
        void* m_pNext;
    };

    //
    // Simple helper class that check that checks and manages instance extensions
    //
    class InstanceCreator : public CreatorBase
    {
    public:
        InstanceCreator()
            : CreatorBase()
            , m_LayerProperties()
            , m_LayerNames()
        {
            // Query instance layers
            uint32_t instanceLayerPropertyCount = 0;
            VkResult res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, nullptr);
            m_LayerProperties.resize(instanceLayerPropertyCount);
            CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Unable to enumerate instance layer properties");
            if (instanceLayerPropertyCount > 0)
            {
                res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, m_LayerProperties.data());
                CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Unable to enumerate instance layer properties");
            }

            // Query instance extensions
            uint32_t instanceExtensionPropertyCount = 0;
            res = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, nullptr);
            CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Unable to enumerate instance extension properties");
            m_ExtensionProperties.resize(instanceExtensionPropertyCount);
            if (instanceExtensionPropertyCount > 0)
            {
                res = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, m_ExtensionProperties.data());
                CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Unable to enumerate instance extension properties");
            }
        }

        ~InstanceCreator()
        {
        }

        template <typename T>
        void AppendNext(T* newNext, VkStructureType structureType)
        {
            m_Appender.AppendNext(newNext, structureType);
        }

        bool TryAddLayer(const char* pLayerName)
        {
            if (IsLayerPresent(pLayerName))
            {
                m_LayerNames.push_back(pLayerName);
                return true;
            }
            else
            {
                CauldronWarning(L"Instance layer not found");
                return false;
            }
        }

        VkInstance Create(VkApplicationInfo app_info)
        {
            VkInstanceCreateInfo inst_info = {};
            inst_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            inst_info.pNext                   = m_Appender.GetNext();
            inst_info.flags                   = 0;
            inst_info.pApplicationInfo        = &app_info;
            inst_info.enabledLayerCount       = (uint32_t)m_LayerNames.size();
            inst_info.ppEnabledLayerNames     = (uint32_t)m_LayerNames.size() ? m_LayerNames.data() : nullptr;
            inst_info.enabledExtensionCount   = (uint32_t)m_ExtensionNames.size();
            inst_info.ppEnabledExtensionNames = m_ExtensionNames.data();

            VkInstance instance = VK_NULL_HANDLE;
            VkResult   res      = vkCreateInstance(&inst_info, nullptr, &instance);
            CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Unable to create instance");

            return instance;
        }

    private:
        bool IsLayerPresent(const char* pLayerName)
        {
            return std::find_if(
                m_LayerProperties.begin(),
                m_LayerProperties.end(),
                [pLayerName](const VkLayerProperties& layerProps) -> bool {
                    return strcmp(layerProps.layerName, pLayerName) == 0;
                }) != m_LayerProperties.end();
        }

    private:
        std::vector<VkLayerProperties> m_LayerProperties;
        std::vector<const char*>       m_LayerNames;
        Appender                       m_Appender;
    };

    //
    // Simple helper class that checks and manages device extensions
    //
    class DeviceCreator : public CreatorBase
    {
    public:
        DeviceCreator(VkPhysicalDevice physicalDevice)
            : CreatorBase()
            , m_physicalDevice{ physicalDevice }
        {
            // Query device extensions
            uint32_t extensionCount;
            VkResult res = vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
            m_ExtensionProperties.resize(extensionCount);
            res = vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, m_ExtensionProperties.data());
        }

        ~DeviceCreator()
        {
        }

        template <typename T>
        void AppendNextFeature(T* newNext, VkStructureType structureType)
        {
            m_FeaturesAppender.AppendNext(newNext, structureType);
        }

        template <typename T>
        void AppendNextFeature(T* newNext)
        {
            m_FeaturesAppender.AppendNext(newNext);
        }

        template <typename T>
        void AppendNextProperty(T* newNext, VkStructureType structureType)
        {
            m_PropertiesAppender.AppendNext(newNext, structureType);
        }

        template <typename T>
        bool TryAddExtensionFeature(const char* pExtensionName, T* newNext, VkStructureType structureType)
        {
            if (TryAddExtension(pExtensionName))
            {
                AppendNextFeature(newNext, structureType);
                return true;
            }
            return false;
        }

        VkPhysicalDeviceFeatures QueryDeviceFeatures() const
        {
            VkPhysicalDeviceFeatures2 features = {};
            features.sType                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext                     = m_FeaturesAppender.GetNext();
            vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features);
            return features.features;
        }

        void ClearFeatures()
        {
            m_FeaturesAppender.Clear();
        }

        VkPhysicalDeviceProperties QueryDeviceProperties() const
        {
            VkPhysicalDeviceProperties2 properties = {};
            properties.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            properties.pNext                       = m_PropertiesAppender.GetNext();
            vkGetPhysicalDeviceProperties2(m_physicalDevice, &properties);
            return properties.properties;
        }

        VkDevice Create(const VkDeviceQueueCreateInfo* queueInfos, uint32_t queueCount)
        {
            VkDeviceCreateInfo device_info = {};
            device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_info.pNext                   = m_FeaturesAppender.GetNext();
            device_info.queueCreateInfoCount    = queueCount;
            device_info.pQueueCreateInfos       = queueInfos;
            device_info.enabledExtensionCount   = (uint32_t)m_ExtensionNames.size();
            device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? m_ExtensionNames.data() : nullptr;
            device_info.pEnabledFeatures        = nullptr;

            VkDevice device;
            VkResult res = vkCreateDevice(m_physicalDevice, &device_info, nullptr, &device);
            CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Unable to create device");

            return device;
        }

    private:
        VkPhysicalDevice m_physicalDevice;
        Appender         m_FeaturesAppender;
        Appender         m_PropertiesAppender;
    };

    QueueFamilies GetQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        // Get queue/memory/device properties
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        CauldronAssert(ASSERT_CRITICAL, queueFamilyCount >= 1, L"Unable to get physical device queue family properties");

        std::vector<VkQueueFamilyProperties> queueProps;
        queueProps.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProps.data());
        CauldronAssert(ASSERT_CRITICAL, queueFamilyCount >= 1, L"Unable to get physical device queue family properties");

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        QueueFamilies families;

        // Find a graphics device and a queue that can present to the above surface
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                if (families.GraphicsQueueFamilyIndex == UINT32_MAX)
                {
                    families.GraphicsQueueFamilyIndex = i;
                    families.GraphicsQueueProperties = queueProps[i];
                }

                VkBool32 supportsPresent;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent);
                if (supportsPresent == VK_TRUE)
                {
                    families.GraphicsQueueFamilyIndex = i;
                    families.PresentQueueFamilyIndex = i;
                    families.PresentQueueProperties = queueProps[i];
                    families.presentQueueType = CommandQueue::Graphics;
                    break;
                }
            }
        }

        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        if (families.PresentQueueFamilyIndex == UINT32_MAX)
        {
            for (uint32_t i = 0; i < queueFamilyCount; ++i)
            {
                VkBool32 supportsPresent;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent);
                if (supportsPresent == VK_TRUE)
                {
                    families.PresentQueueFamilyIndex = (uint32_t)i;
                    families.PresentQueueProperties = queueProps[i];
                    break;
                }
            }
        }

        // Get a compute queue that isn't the graphics one if possible
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if ((queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
            {
                if (families.ComputeQueueFamilyIndex == UINT32_MAX)
                {
                    families.ComputeQueueFamilyIndex = i;
                    families.ComputeQueueProperties = queueProps[i];
                }
                if (i != families.GraphicsQueueFamilyIndex)
                {
                    families.ComputeQueueFamilyIndex = i;
                    families.ComputeQueueProperties = queueProps[i];
                    break;
                }
            }
        }

        // Get a copy queue that isn't the graphics or compute one if possible
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if ((queueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
            {
                if (families.CopyQueueFamilyIndex == UINT32_MAX)
                {
                    families.CopyQueueFamilyIndex = i;
                    families.CopyQueueProperties = queueProps[i];
                }
                if (i != families.GraphicsQueueFamilyIndex && i != families.ComputeQueueFamilyIndex)
                {
                    families.CopyQueueFamilyIndex = i;
                    families.CopyQueueProperties = queueProps[i];
                    break;
                }
            }
        }

        // check which queue family the present queue belongs to
        if (families.presentQueueType == CommandQueue::Count)
        {
            if (families.PresentQueueFamilyIndex == families.ComputeQueueFamilyIndex)
                families.presentQueueType = CommandQueue::Compute;
            else if (families.PresentQueueFamilyIndex == families.CopyQueueFamilyIndex)
                families.presentQueueType = CommandQueue::Copy;
        }

        // NOTE: we might want to choose the queue index in a given family to present overlapping queues.
        // for example, if we only have one queue family with graphics, compute and copy, we should take the index 0, 1, 2 from this family for each queue.
        // (providing this family has at least 3 queues)

        return families;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::string s = pCallbackData->pMessage;

        // Vulkan messages contain '%', so we need to escape it
        size_t pos = s.find('%', 0);
        while (pos != std::string::npos)
        {
            s.insert(pos, 1, '%');
            pos = s.find('%', pos + 2);
        }

        CauldronWarning(L"validation layer: %ls\n", StringToWString(s).c_str());

        return VK_FALSE;
    }

    Device* Device::CreateDevice()
    {
        return new DeviceInternal();
    }

    DeviceInternal::DeviceInternal()
        : Device()
    {
        // Will need config settings to initialize the device
        const CauldronConfig* pConfig = GetConfig();

        // object to help create the instance
        InstanceCreator instanceCreator;

        VkApplicationInfo app_info  = {};
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext              = nullptr;
        app_info.pApplicationName   = "Cauldron";
        app_info.applicationVersion = 1;
        app_info.pEngineName        = "Cauldron";
        app_info.engineVersion      = 2;
        app_info.apiVersion         = VK_API_VERSION_1_2;

        // add default extensions
        instanceCreator.TryAddExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        instanceCreator.TryAddExtension(VK_KHR_SURFACE_EXTENSION_NAME);
        instanceCreator.TryAddExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instanceCreator.TryAddExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

        const VkValidationFeatureEnableEXT validationFeaturesRequested[] = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
                                                                            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
                                                                            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
        VkValidationFeaturesEXT            validationFeatures            = {};
        if (pConfig->CPUValidationEnabled)
        {
            if (instanceCreator.TryAddLayer("VK_LAYER_KHRONOS_validation") && instanceCreator.TryAddExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME) &&
                pConfig->GPUValidationEnabled)
            {
                validationFeatures.enabledValidationFeatureCount = _countof(validationFeaturesRequested);
                validationFeatures.pEnabledValidationFeatures    = validationFeaturesRequested;

                instanceCreator.AppendNext(&validationFeatures, VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT);
            }
        }

        // Create the instance
        m_Instance = instanceCreator.Create(app_info);

        VkResult res = VK_SUCCESS;

        // create the debug messenger if needed
        if (pConfig->CPUValidationEnabled)
        {
            VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
            createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;

            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                res = func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
                CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS && m_DebugMessenger != VK_NULL_HANDLE, L"Failed to set up debug messenger.");
            }
            else
            {
                res = VK_ERROR_EXTENSION_NOT_PRESENT;
                CauldronWarning(L"Debug extension not present.");
            }
        }

        // Enumerate physical devices
        uint32_t       physicalDeviceCount = 1;
        uint32_t const req_count           = physicalDeviceCount;
        res                                = vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr);
        CauldronAssert(ASSERT_CRITICAL, physicalDeviceCount > 0, L"No GPU found");

        std::vector<VkPhysicalDevice> physicalDevices;
        physicalDevices.resize(physicalDeviceCount);

        res = vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, physicalDevices.data());
        CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS && physicalDeviceCount >= req_count, L"Unable to enumerate physical devices");

        // get the best available gpu
        m_PhysicalDevice = SelectPhysicalDevice(physicalDevices, app_info.apiVersion);

        // Create a Win32 Surface
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext                       = nullptr;
        createInfo.hwnd                        = GetFramework()->GetInternal()->GetHWND();
        res                                    = vkCreateWin32SurfaceKHR(m_Instance, &createInfo, nullptr, &m_Surface);

        // Use device creator to collect the extensions
        DeviceCreator deviceCreator(m_PhysicalDevice);

        // Add necessary extensions
        deviceCreator.TryAddExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceCreator.TryAddExtension(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
        deviceCreator.TryAddExtension(VK_EXT_HDR_METADATA_EXTENSION_NAME);
        deviceCreator.TryAddExtension(VK_AMD_DISPLAY_NATIVE_HDR_EXTENSION_NAME);
        deviceCreator.TryAddExtension(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME);

        // general features
        VkPhysicalDeviceVulkan11Features vulkan11Features = {};
        VkPhysicalDeviceVulkan12Features vulkan12Features = {};
        deviceCreator.AppendNextFeature(&vulkan11Features, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
        deviceCreator.AppendNextFeature(&vulkan12Features, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);

        // for SM 6.1
        VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragmentShaderBarycentricFeatures = {};
        deviceCreator.TryAddExtensionFeature(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME, &fragmentShaderBarycentricFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR);

        // for 6.2
        // vulkan11Features.storageBuffer16BitAccess is enough

        // for SM 6.3
        // raytracing extensions dependencies
        VkPhysicalDeviceAccelerationStructureFeaturesKHR   accelerationStructureFeatures   = {};
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties = {};
        if (deviceCreator.TryAddExtensionFeature(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            &accelerationStructureFeatures,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR))
        {
            deviceCreator.AppendNextProperty(&accelerationStructureProperties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR);
        }

        VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
        deviceCreator.TryAddExtensionFeature(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, &bufferDeviceAddressFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES);
        
        bool hasDeferredHostExtension = deviceCreator.TryAddExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        
        // RT 1.0
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelinesFeatures     = {};
        deviceCreator.TryAddExtensionFeature(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, &rayTracingPipelinesFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);
        
        // for SM 6.4
        VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeatures = {};
        deviceCreator.TryAddExtensionFeature(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, &fragmentShadingRateFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR);

        VkPhysicalDeviceShaderIntegerDotProductFeatures shaderIntegerDotProductFeatures = {};
        deviceCreator.TryAddExtensionFeature(VK_KHR_SHADER_INTEGER_DOT_PRODUCT_EXTENSION_NAME, &shaderIntegerDotProductFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES);

        // for SM 6.5
        VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {};
        deviceCreator.TryAddExtensionFeature(VK_KHR_RAY_QUERY_EXTENSION_NAME, &rayQueryFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR);

        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {};
        deviceCreator.TryAddExtensionFeature(VK_EXT_MESH_SHADER_EXTENSION_NAME, &meshShaderFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT);

        deviceCreator.TryAddExtension(VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME);

        // for SM 6.6
        VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT shaderDemoteToHelperInvocationFeatures = {};  // promoted to core Vulkan 1.3
        deviceCreator.TryAddExtensionFeature(VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME, &shaderDemoteToHelperInvocationFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT);

        VkPhysicalDeviceSubgroupSizeControlFeaturesEXT subgroupSizeControlFeatures = {};  // promoted to core Vulkan 1.3
        VkPhysicalDeviceSubgroupSizeControlPropertiesEXT subgroupSizeControlProperties = {};
        if (deviceCreator.TryAddExtensionFeature(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME, &subgroupSizeControlFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT))
        {
            deviceCreator.AppendNextProperty(&subgroupSizeControlProperties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT);
        }

        // for SM 6.7

        // extra requested features
        
        // Add support for more dynamic states
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {};  // promoted to Vulkan 1.3
        deviceCreator.TryAddExtensionFeature(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, &extendedDynamicStateFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT);
        
        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {};  // promoted to Vulkan 1.3
        deviceCreator.TryAddExtensionFeature(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, &dynamicRenderingFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES);

        // VK_KHR_timeline_semaphore was promoted to 1.2, so no need to query the extension
        // this is needed for timeline semaphore
        VkPhysicalDeviceTimelineSemaphoreFeaturesKHR semaphoreFeatures = {};
        deviceCreator.AppendNextFeature(&semaphoreFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR);

        // VK_EXT_descriptor_indexing was promoted to 1.2, so no need to query the extension
        // this is needed to allow partially bound descriptors
        VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {};
        deviceCreator.AppendNextFeature(&descriptorIndexingFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES);

        // query all features
        VkPhysicalDeviceFeatures physicalDeviceFeatures = deviceCreator.QueryDeviceFeatures();

        // query properties
        VkPhysicalDeviceVulkan11Properties               vulkan11Properties            = {};
        VkPhysicalDeviceVulkan12Properties               vulkan12Properties            = {};
        VkPhysicalDeviceDriverProperties                 driverProperties              = {};  // query the driver information
        VkPhysicalDeviceSubgroupProperties               subgroupProperties            = {};
        deviceCreator.AppendNextProperty(&vulkan11Properties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES);
        deviceCreator.AppendNextProperty(&vulkan12Properties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES);
        deviceCreator.AppendNextProperty(&driverProperties,   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES);
        deviceCreator.AppendNextProperty(&subgroupProperties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES);
        VkPhysicalDeviceProperties physicalDeviceProperties = deviceCreator.QueryDeviceProperties();

        // list of the features to check
        struct ShaderModelCheckList
        {
            // 6.0
            bool subgroupBallot_6_0 = false;
            // 6.1
            bool multiView          = false;
            bool barycentric        = false;
            // 6.2
            bool float16            = false;
            bool denormMode         = false;
            // 6.3
            bool raytracing_1_0     = false;
            // 6.4
            bool vrsTier1           = false;
            bool vrsTier2           = false;
            bool integerDotProduct  = false;
            // 6.5
            bool raytracing_1_1     = false;
            bool meshShader         = false;
            bool samplerFeedback    = false;  // optional
            bool subgroupBallot_6_5 = false;
            // 6.6
            bool helperLane         = false;
            bool waveSize           = false;
            // 6.7
        } checkList;
        
        // query has been made, we will now add the features that are available/enabled
        deviceCreator.ClearFeatures();

        // for SM 6.0
        // VK_EXT_shader_subgroup_ballot extension deprecated by Vulkan 1.2
        VkSubgroupFeatureFlags allSubgroupFeatures =
            VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_VOTE_BIT | VK_SUBGROUP_FEATURE_ARITHMETIC_BIT | VK_SUBGROUP_FEATURE_BALLOT_BIT |
            VK_SUBGROUP_FEATURE_SHUFFLE_BIT | VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT | VK_SUBGROUP_FEATURE_CLUSTERED_BIT | VK_SUBGROUP_FEATURE_QUAD_BIT;
        checkList.subgroupBallot_6_0 = (((subgroupProperties.supportedOperations & allSubgroupFeatures) == allSubgroupFeatures) &&
                                        (subgroupProperties.quadOperationsInAllStages == VK_TRUE) && (vulkan12Features.subgroupBroadcastDynamicId == VK_TRUE));

        // for SM 6.1
        // VK_KHR_multiview extension promoted to Vulkan 1.1 Core
        checkList.multiView = (vulkan11Features.multiview == VK_TRUE);
        if (fragmentShaderBarycentricFeatures.fragmentShaderBarycentric == VK_TRUE)
        {
            checkList.barycentric = true;
            deviceCreator.AppendNextFeature(&fragmentShaderBarycentricFeatures);
        }

        // for SM 6.2
        // VK_KHR_shader_float16_int8 extension promoted to Vulkan 1.2 Core
        if ((vulkan11Features.storageBuffer16BitAccess == VK_TRUE) && (vulkan12Features.shaderFloat16 == VK_TRUE))
        {
            checkList.float16 = true;
            m_SupportedFeatures |= DeviceFeature::FP16;
        }
        // VK_KHR_shader_float_controls extension promoted to Vulkan 1.2 Core
        checkList.denormMode = (vulkan12Properties.shaderDenormFlushToZeroFloat32 == VK_TRUE) && (vulkan12Properties.shaderDenormPreserveFloat32 == VK_TRUE);

        // for SM 6.3
        if (accelerationStructureFeatures.accelerationStructure == VK_TRUE)
            deviceCreator.AppendNextFeature(&accelerationStructureFeatures);
        if (bufferDeviceAddressFeatures.bufferDeviceAddress == VK_TRUE)
            deviceCreator.AppendNextFeature(&bufferDeviceAddressFeatures);

        if (rayTracingPipelinesFeatures.rayTracingPipeline == VK_TRUE)
        {
            checkList.raytracing_1_0 = true;

            m_MinAccelerationStructureScratchOffsetAlignment = accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;

            deviceCreator.AppendNextFeature(&rayTracingPipelinesFeatures);
            m_SupportedFeatures |= DeviceFeature::RT_1_0;
        }
        CauldronAssert(ASSERT_WARNING,
                       !pConfig->RT_1_0 || FeatureSupported(DeviceFeature::RT_1_0),
                       L"[VK_KHR_ray_tracing_pipeline] DXR 1.0 support requested but unsupported on this device.");
        
        // for SM 6.4
        if (fragmentShadingRateFeatures.pipelineFragmentShadingRate == VK_TRUE)
        {
            checkList.vrsTier1 = true;
            m_SupportedFeatures |= DeviceFeature::VRSTier1;
        }
        if (fragmentShadingRateFeatures.attachmentFragmentShadingRate == VK_TRUE && fragmentShadingRateFeatures.primitiveFragmentShadingRate == VK_TRUE)
        {
            checkList.vrsTier2 = true;
            m_SupportedFeatures |= DeviceFeature::VRSTier2;
        }
        if (FeatureSupported(DeviceFeature::VRSTier1 | DeviceFeature::VRSTier2))
        {
            deviceCreator.AppendNextFeature(&fragmentShadingRateFeatures);
        }
        
        if (shaderIntegerDotProductFeatures.shaderIntegerDotProduct == VK_TRUE)
        {
            checkList.integerDotProduct = true;
            deviceCreator.AppendNextFeature(&shaderIntegerDotProductFeatures);
        }

        // for SM 6.5
        if (rayQueryFeatures.rayQuery == VK_TRUE)
        {
            checkList.raytracing_1_1 = true;
            deviceCreator.AppendNextFeature(&rayQueryFeatures);
            m_SupportedFeatures |= DeviceFeature::RT_1_1;
        }
        CauldronAssert(ASSERT_WARNING,
                       !pConfig->RT_1_1 || FeatureSupported(DeviceFeature::RT_1_1),
                       L"[VK_KHR_ray_query] DXR 1.1 support requested but unsupported on this device.");

        if ((meshShaderFeatures.meshShader == VK_TRUE) && (meshShaderFeatures.taskShader == VK_TRUE))
        {
            checkList.meshShader = true;
            deviceCreator.AppendNextFeature(&meshShaderFeatures);
        }
        checkList.samplerFeedback    = false;  // optional - no Vulkan extension yet
        checkList.subgroupBallot_6_5 = ((subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV) != 0);

        // for SM 6.6
        // VK_EXT_shader_subgroup_ballot extension deprecated by Vulkan 1.3
        if (shaderDemoteToHelperInvocationFeatures.shaderDemoteToHelperInvocation == VK_TRUE)
        {
            checkList.helperLane = true;
            deviceCreator.AppendNextFeature(&shaderDemoteToHelperInvocationFeatures);
        }
        
        m_MinWaveLaneCount = vulkan11Properties.subgroupSize;
        m_MaxWaveLaneCount = vulkan11Properties.subgroupSize;
        if (subgroupSizeControlFeatures.subgroupSizeControl == VK_TRUE)
        {
            m_MinWaveLaneCount = subgroupSizeControlProperties.minSubgroupSize;
            m_MaxWaveLaneCount = subgroupSizeControlProperties.maxSubgroupSize;

            if (((subgroupSizeControlProperties.requiredSubgroupSizeStages | VK_SHADER_STAGE_COMPUTE_BIT) != 0) &&
                (subgroupSizeControlProperties.minSubgroupSize <= 32) && (subgroupSizeControlProperties.maxSubgroupSize >= 64))
            {
                checkList.waveSize = true;
                m_SupportedFeatures |= DeviceFeature::WaveSize;
            }

            deviceCreator.AppendNextFeature(&subgroupSizeControlFeatures);
        }

        // for SM 6.7
        // nothing yet

        // verify shader model
        ShaderModel maxShaderModel = ShaderModel::SM5_1;
        if (maxShaderModel == ShaderModel::SM5_1 && checkList.subgroupBallot_6_0)
            maxShaderModel = ShaderModel::SM6_0;
        if (maxShaderModel == ShaderModel::SM6_0 && checkList.multiView && checkList.barycentric)
            maxShaderModel = ShaderModel::SM6_1;
        if (maxShaderModel == ShaderModel::SM6_1 && checkList.float16 && checkList.denormMode)
            maxShaderModel = ShaderModel::SM6_2;
        if (maxShaderModel == ShaderModel::SM6_2 && checkList.raytracing_1_0)
            maxShaderModel = ShaderModel::SM6_3;
        if (maxShaderModel == ShaderModel::SM6_3 && (checkList.vrsTier1 || checkList.vrsTier2) && checkList.integerDotProduct)
            maxShaderModel = ShaderModel::SM6_4;
        if (maxShaderModel == ShaderModel::SM6_4 && checkList.raytracing_1_1 && checkList.meshShader && checkList.subgroupBallot_6_5)  // sampler feeback is optional
            maxShaderModel = ShaderModel::SM6_5;
        if (maxShaderModel == ShaderModel::SM6_5 && checkList.helperLane && checkList.waveSize)
            maxShaderModel = ShaderModel::SM6_6;
        // 6.7 not yet supported

        m_MaxSupportedShaderModel = maxShaderModel;

        CHECK_FEATURE_SUPPORT(fillModeNonSolid)
        CHECK_FEATURE_SUPPORT(pipelineStatisticsQuery)
        CHECK_FEATURE_SUPPORT(fragmentStoresAndAtomics)
        CHECK_FEATURE_SUPPORT(vertexPipelineStoresAndAtomics)
        CHECK_FEATURE_SUPPORT(shaderImageGatherExtended)
        CHECK_FEATURE_SUPPORT(wideLines)         // needed for drawing lines with a specific width.
        CHECK_FEATURE_SUPPORT(independentBlend)  // needed for having different blend for each render target
        CHECK_FEATURE_SUPPORT(depthClamp)
        CHECK_FEATURE_SUPPORT(depthBiasClamp)
        CHECK_FEATURE_SUPPORT(shaderFloat64)
        CHECK_FEATURE_SUPPORT(shaderInt16)
        CHECK_FEATURE_SUPPORT(robustBufferAccess)  // needed for VK_EXT_robustness2
        CHECK_FEATURE_SUPPORT(samplerAnisotropy)   // for anisotropic filtering
        CHECK_FEATURE_SUPPORT(shaderStorageImageWriteWithoutFormat)
        CHECK_FEATURE_SUPPORT_11(storageBuffer16BitAccess)  // for FP16 support
        CHECK_FEATURE_SUPPORT_12(shaderFloat16)             // for FP16 support

        // Add all the features for device creation
        VkPhysicalDeviceFeatures2 features = {};
        features.features                  = physicalDeviceFeatures;
        deviceCreator.AppendNextFeature(&features, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);
        deviceCreator.AppendNextFeature(&vulkan11Features);
        deviceCreator.AppendNextFeature(&vulkan12Features);

        CauldronAssert(ASSERT_WARNING,
                       extendedDynamicStateFeatures.extendedDynamicState == VK_TRUE,
                       L"[VK_EXT_extended_dynamic_state] Extended dynamic states (primitive topology etc.) requested but unsupported on this device");
        if (extendedDynamicStateFeatures.extendedDynamicState == VK_TRUE)
            deviceCreator.AppendNextFeature(&extendedDynamicStateFeatures);

        CauldronAssert(ASSERT_CRITICAL,
                       dynamicRenderingFeatures.dynamicRendering == VK_TRUE,
                       L"[VK_KHR_dynamic_rendering] Dynamic rendering requested but unsupported on this device");
        if (dynamicRenderingFeatures.dynamicRendering == VK_TRUE)
            deviceCreator.AppendNextFeature(&dynamicRenderingFeatures);
        
        CauldronAssert(ASSERT_WARNING, semaphoreFeatures.timelineSemaphore == VK_TRUE, L"timelineSemaphore feature requested but unsupported on this device");
        if (semaphoreFeatures.timelineSemaphore == VK_TRUE)
            deviceCreator.AppendNextFeature(&semaphoreFeatures);

        CauldronAssert(ASSERT_WARNING,
                       descriptorIndexingFeatures.descriptorBindingPartiallyBound == VK_TRUE,
                       L"descriptorBindingPartiallyBound feature requested but unsupported on this device");
        if (descriptorIndexingFeatures.descriptorBindingPartiallyBound == VK_TRUE)
            deviceCreator.AppendNextFeature(&descriptorIndexingFeatures);

        // Warning for FP16 support
        CauldronAssert(ASSERT_WARNING, !pConfig->FP16 || FeatureSupported(DeviceFeature::FP16), L"FP16 support requested but unsupported on this device.");
        if (FeatureSupported(DeviceFeature::FP16))
        {
            CHECK_FEATURE_SUPPORT_12(shaderSubgroupExtendedTypes);
        }

        // Warning for VRS support request
        CauldronAssert(ASSERT_WARNING,
                       !pConfig->VRSTier1 || FeatureSupported(DeviceFeature::VRSTier1),
                       L"[VK_KHR_fragment_shading_rate] VRS Tier1 support requested but unsupported on this device.");
        CauldronAssert(ASSERT_WARNING,
                       !pConfig->VRSTier2 || FeatureSupported(DeviceFeature::VRSTier2),
                       L"[VK_KHR_fragment_shading_rate] VRS Tier2 support requested but unsupported on this device.");

        // Warning for Wave64 support request
        CauldronAssert(ASSERT_WARNING,
                       FeatureSupported(DeviceFeature::WaveSize),
                       L"[VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME] Wave size control unsupported on this device.");

        // general warning
        if (FeatureSupported(DeviceFeature::RT_1_0) || FeatureSupported(DeviceFeature::RT_1_1))
        {
            CauldronAssert(ASSERT_WARNING,
                           hasDeferredHostExtension && (accelerationStructureFeatures.accelerationStructure == VK_TRUE) &&
                               (bufferDeviceAddressFeatures.bufferDeviceAddress == VK_TRUE),
                           L"Device supports VK_KHR_ray_tracing_pipeline or VK_KHR_ray_query extensions but doesn't support VK_KHR_deferred_host_operations, "
                           L"VK_KHR_acceleration_structure or VK_KHR_buffer_device_address extensions.");
        }

        // Get all the queues we need
        m_QueueFamilies = GetQueues(m_PhysicalDevice, m_Surface);

        // Create device
        constexpr uint32_t c_QueueCount = 3;
        float graphicsQueuePriority = 1.0f;
        float computeQueuePriority = 1.0f;
        float copyQueuePriorities[] = { 0.5f, 0.5f };
        VkDeviceQueueCreateInfo queueInfos[c_QueueCount] = {};
        queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[0].pNext = nullptr;
        queueInfos[0].queueCount = 1;
        queueInfos[0].pQueuePriorities = &graphicsQueuePriority;
        queueInfos[0].queueFamilyIndex = m_QueueFamilies.GraphicsQueueFamilyIndex;
        queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[1].pNext = nullptr;
        queueInfos[1].queueCount = 1;
        queueInfos[1].pQueuePriorities = &computeQueuePriority;
        queueInfos[1].queueFamilyIndex = m_QueueFamilies.ComputeQueueFamilyIndex;
        queueInfos[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[2].pNext = nullptr;
        queueInfos[2].queueCount = 1;
        queueInfos[2].pQueuePriorities = copyQueuePriorities;
        queueInfos[2].queueFamilyIndex = m_QueueFamilies.CopyQueueFamilyIndex;

        m_Device = deviceCreator.Create(queueInfos, c_QueueCount);

        // create the allocator
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;  // for acceleration structures
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorInfo.physicalDevice = m_PhysicalDevice;
        allocatorInfo.device = m_Device;
        allocatorInfo.instance = m_Instance;

        vmaCreateAllocator(&allocatorInfo, &m_VmaAllocator);

        // Get debug procedures
        GET_DEVICE_PROC_ADDR(vkSetDebugUtilsObjectNameEXT);
        GET_DEVICE_PROC_ADDR(vkCmdSetPrimitiveTopologyEXT);
        GET_DEVICE_PROC_ADDR(vkCmdBeginDebugUtilsLabelEXT);
        GET_DEVICE_PROC_ADDR(vkCmdEndDebugUtilsLabelEXT);
        GET_DEVICE_PROC_ADDR(vkCmdBeginRenderingKHR); // TODO: delete once we use Vulkan 1.3
        GET_DEVICE_PROC_ADDR(vkCmdEndRenderingKHR); // TODO: delete once we use Vulkan 1.3
        GET_DEVICE_PROC_ADDR(vkCmdSetFragmentShadingRateKHR);
        GET_DEVICE_PROC_ADDR(vkGetAccelerationStructureBuildSizesKHR);
        GET_DEVICE_PROC_ADDR(vkCreateAccelerationStructureKHR);
        GET_DEVICE_PROC_ADDR(vkDestroyAccelerationStructureKHR);
        GET_DEVICE_PROC_ADDR(vkGetAccelerationStructureDeviceAddressKHR);
        GET_DEVICE_PROC_ADDR(vkCmdBuildAccelerationStructuresKHR);
        GET_DEVICE_PROC_ADDR(vkGetAccelerationStructureBuildSizesKHR);


        // Get hdr procedures
        GET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfaceCapabilities2KHR);
        GET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfaceFormats2KHR);
        GET_DEVICE_PROC_ADDR(vkSetHdrMetadataEXT);
        GET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceFragmentShadingRatesKHR);

        // set device name
        SetResourceName(VK_OBJECT_TYPE_DEVICE, (uint64_t)m_Device, "CauldronDevice");

        // create the queues
        auto queueBuilder = [this](CommandQueue queueType, uint32_t familyIndex, uint32_t queueIndex, uint32_t numFramesInFlight, const char* name)
        {
            m_QueueSyncPrims[static_cast<uint32_t>(queueType)].Init(this, queueType, familyIndex, queueIndex, numFramesInFlight, name);
        };
        queueBuilder(CommandQueue::Graphics,   m_QueueFamilies.GraphicsQueueFamilyIndex, 0, pConfig->BackBufferCount, "CauldronGraphicsQueue"   );
        queueBuilder(CommandQueue::Compute,    m_QueueFamilies.ComputeQueueFamilyIndex,  0, pConfig->BackBufferCount, "CauldronComputeQueue"    );
        queueBuilder(CommandQueue::Copy,       m_QueueFamilies.CopyQueueFamilyIndex,     0, pConfig->BackBufferCount, "CauldronCopyQueue"       );

        // Special case for the present queue
        // For now, we don't accept devices where graphics queue and present queue are separate. We don;t want to deal with swapchain queue ownership transfer.
        CauldronAssert(ASSERT_CRITICAL, m_QueueFamilies.presentQueueType == CommandQueue::Graphics, L"Devices where the present queue isn't the graphics queue aren't supported.");


        m_DeviceName = StringToWString(physicalDeviceProperties.deviceName);
        switch (driverProperties.driverID)
        {
            case VK_DRIVER_ID_AMD_PROPRIETARY:
                m_DriverVersion = L"Adrenalin ";
                break;
            case VK_DRIVER_ID_NVIDIA_PROPRIETARY:
                m_DriverVersion = L"Nvidia ";
                break;
            case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS:
                m_DriverVersion = L"Intel ";
                break;
            case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA:
                m_DriverVersion = L"Intel Mesa ";
                break;
            default:
                m_DriverVersion = L"Unknown ";
                break;
        }
        m_DriverVersion += StringToWString(driverProperties.driverInfo);

        uint32_t vulkanMajorVersion = VK_VERSION_MAJOR(app_info.apiVersion);
        uint32_t vulkanMinorVersion = VK_VERSION_MINOR(app_info.apiVersion);
        m_GraphicsAPIShort          = L"VK";
        m_GraphicsAPIPretty         = L"Vulkan";
        m_GraphicsAPIVersion        = std::to_wstring(vulkanMajorVersion) + L"." + std::to_wstring(vulkanMinorVersion);
        m_GraphicsAPI               = m_GraphicsAPIPretty + L" " + m_GraphicsAPIVersion;

        // create default objects
        const SamplerDesc defaultSamplerDesc = {};
        VkSamplerCreateInfo info = Convert(defaultSamplerDesc); // value is irrelevant
        vkCreateSampler(m_Device, &info, nullptr, &m_DefaultSampler);
    }

    DeviceInternal::~DeviceInternal()
    {
        FlushAllCommandQueues();

        // destroy default objects
        vkDestroySampler(m_Device, m_DefaultSampler, nullptr);

        // Release all the queues
        for (int i = 0; i < static_cast<uint32_t>(CommandQueue::Count); ++i)
            m_QueueSyncPrims[i].Release(m_Device);

        vmaDestroyAllocator(m_VmaAllocator);

        vkDestroyDevice(m_Device, nullptr);
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        if (m_DebugMessenger != VK_NULL_HANDLE)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
            CauldronAssert(ASSERT_CRITICAL, func != nullptr, L"Couldn't find vkDestroyDebugUtilsMessengerEXT proc.");
            func(m_Instance, m_DebugMessenger, nullptr);
        }
        vkDestroyInstance(m_Instance, nullptr);
    }

    static ShadingRate GetShadingRateFromVKFragmentExtent(const VkExtent2D& size)
    {
        int32_t shift = static_cast<int32_t>(log2(size.width));
        return static_cast<ShadingRate>(((1u << shift) << SHADING_RATE_SHIFT) | size.height);
    }

    void DeviceInternal::GetFeatureInfo(DeviceFeature feature, void* pFeatureInfo)
    {
        switch (feature)
        {
        case cauldron::DeviceFeature::FP16:
            break;
        case cauldron::DeviceFeature::VRSTier1:
        case cauldron::DeviceFeature::VRSTier2:
        {
            VkPhysicalDeviceFragmentShadingRatePropertiesKHR physicalDeviceFragmentShadingRateProperties = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR};
            {
                VkPhysicalDeviceProperties2KHR deviceProperties = {};
                {
                    deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                    deviceProperties.pNext = &physicalDeviceFragmentShadingRateProperties;
                }
                vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &deviceProperties);
            }

            ((FeatureInfo_VRS*)pFeatureInfo)->Combiners =
                ShadingRateCombiner::ShadingRateCombiner_Passthrough | ShadingRateCombiner::ShadingRateCombiner_Override;
            if (physicalDeviceFragmentShadingRateProperties.fragmentShadingRateNonTrivialCombinerOps != VK_FALSE)
            {
                ((FeatureInfo_VRS*)pFeatureInfo)->Combiners |= ShadingRateCombiner::ShadingRateCombiner_Min | ShadingRateCombiner::ShadingRateCombiner_Max;
                ((FeatureInfo_VRS*)pFeatureInfo)->Combiners |=
                    (physicalDeviceFragmentShadingRateProperties.fragmentShadingRateStrictMultiplyCombiner != VK_FALSE) ? ShadingRateCombiner::ShadingRateCombiner_Mul
                                                                                                                        : ShadingRateCombiner::ShadingRateCombiner_Sum;
            }

            if (static_cast<bool>(feature & cauldron::DeviceFeature::VRSTier2))
            {
                ((FeatureInfo_VRS*)pFeatureInfo)->MinTileSize[0] = physicalDeviceFragmentShadingRateProperties.minFragmentShadingRateAttachmentTexelSize.width;
                ((FeatureInfo_VRS*)pFeatureInfo)->MinTileSize[1] = physicalDeviceFragmentShadingRateProperties.minFragmentShadingRateAttachmentTexelSize.height;
                ((FeatureInfo_VRS*)pFeatureInfo)->MaxTileSize[0] = physicalDeviceFragmentShadingRateProperties.maxFragmentShadingRateAttachmentTexelSize.width;
                ((FeatureInfo_VRS*)pFeatureInfo)->MaxTileSize[1] = physicalDeviceFragmentShadingRateProperties.maxFragmentShadingRateAttachmentTexelSize.height;
            }

            std::vector<VkPhysicalDeviceFragmentShadingRateKHR> ShadingRates;
            {
                uint32_t ShadingRateCount = 0;
                DeviceInternal* pDevice = GetDevice()->GetImpl();
                pDevice->GetPhysicalDeviceFragmentShadingRatesKHR()(m_PhysicalDevice, &ShadingRateCount, nullptr);
                // Spec says that implementation must support at least 3 predefined modes.
                CauldronAssert(ASSERT_CRITICAL, ShadingRateCount >= 3, L"Must support at least 3 predefined shading rate modes.");

                ShadingRates.resize(ShadingRateCount);
                for (auto& shadingRate : ShadingRates)
                {
                    shadingRate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR;
                }
                pDevice->GetPhysicalDeviceFragmentShadingRatesKHR()(m_PhysicalDevice, &ShadingRateCount, ShadingRates.data());
            }

            ((FeatureInfo_VRS*)pFeatureInfo)->NumShadingRates = static_cast<uint32_t>(std::min(ShadingRates.size(), size_t{MAX_SHADING_RATES}));
            for (uint32_t i = 0; i < ((FeatureInfo_VRS*)pFeatureInfo)->NumShadingRates; ++i)
            {
                ((FeatureInfo_VRS*)pFeatureInfo)->ShadingRates[i] = GetShadingRateFromVKFragmentExtent(ShadingRates[i].fragmentSize);
                if (ShadingRates[i].fragmentSize.width > 2 || ShadingRates[i].fragmentSize.height > 2)
                    ((FeatureInfo_VRS*)pFeatureInfo)->AdditionalShadingRatesSupported = true;
            }
            break;
        }
        case cauldron::DeviceFeature::RT_1_0:
            break;
        case cauldron::DeviceFeature::RT_1_1:
            break;
        case cauldron::DeviceFeature::WaveSize:
            break;
        default:
            break;
        }
    }

    void DeviceInternal::FlushQueue(CommandQueue queueType)
    {
        m_QueueSyncPrims[static_cast<uint32_t>(queueType)].Flush();
    }

    uint64_t DeviceInternal::QueryPerformanceFrequency(CommandQueue queueType)
    {
        uint64_t frequency = 0;
        CauldronAssert(ASSERT_ERROR, queueType == CommandQueue::Compute || queueType == CommandQueue::Graphics, L"Querying performance frequency on invalid device queue. Crash likely.");

        // Get the Physical device properties
        VkPhysicalDeviceProperties2 deviceProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &deviceProperties);

        if ((queueType == CommandQueue::Compute || queueType == CommandQueue::Graphics) && deviceProperties.properties.limits.timestampComputeAndGraphics)
            frequency = static_cast<uint64_t>(deviceProperties.properties.limits.timestampPeriod);

        // Return the number of ticks per second (which is generally 1 tick every 10 nanoseconds)
        return g_NanosecondsPerSecond / frequency;
    }


    CommandList* DeviceInternal::CreateCommandList(const wchar_t* name, CommandQueue queueType)
    {
        CommandListInitParams initParams = { this, m_QueueSyncPrims[static_cast<uint32_t>(queueType)].GetCommandPool() };
        CommandList* pCmdList = CommandList::CreateCommandList(name, queueType, &initParams);
        return pCmdList;
    }

    void DeviceInternal::CreateSwapChain(SwapChain*& pSwapChain, const SwapChainCreationParams& params, CommandQueue queueType)
    {
        CauldronAssert(ASSERT_CRITICAL, m_QueueFamilies.presentQueueType == CommandQueue::Graphics, L"Devices where the present queue isn't the graphics queue aren't supported.");
        VkResult res = vkCreateSwapchainKHR(m_Device, &params.swapchainCreateInfo, nullptr, pSwapChain->GetImpl()->VKSwapChainPtr());
        CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Unable to create swapchain");
    }

    // For swapchain present and signaling (for synchronization)
    uint64_t DeviceInternal::PresentSwapChain(SwapChain* pSwapChain)
    {
        CauldronAssert(ASSERT_CRITICAL, m_QueueFamilies.presentQueueType == CommandQueue::Graphics, L"Devices where the present queue isn't the graphics queue aren't supported.");
        uint32_t imageIndex = static_cast<uint32_t>(pSwapChain->GetBackBufferIndex());
        return m_QueueSyncPrims[static_cast<uint32_t>(m_QueueFamilies.presentQueueType)].Present(pSwapChain->GetImpl()->VKSwapChain(), imageIndex);
    }

    void DeviceInternal::WaitOnQueue(uint64_t waitValue, CommandQueue queueType) const
    {
        m_QueueSyncPrims[static_cast<uint32_t>(queueType)].Wait(m_Device, waitValue);
    }

    uint64_t DeviceInternal::ExecuteCommandLists(std::vector<CommandList*>& cmdLists, CommandQueue queueType, bool isLastSubmissionOfFrame)
    {
        return m_QueueSyncPrims[static_cast<uint32_t>(queueType)].Submit(cmdLists, VK_NULL_HANDLE, VK_NULL_HANDLE, isLastSubmissionOfFrame);
    }

    uint64_t DeviceInternal::ExecuteCommandLists(std::vector<CommandList*>& cmdLists, CommandQueue queueType, VkSemaphore waitSemaphore)
    {
        return m_QueueSyncPrims[static_cast<uint32_t>(queueType)].Submit(cmdLists, VK_NULL_HANDLE, waitSemaphore, false);
    }

    VkSemaphore DeviceInternal::ExecuteCommandListsWithSignalSemaphore(std::vector<CommandList*>& cmdLists, CommandQueue queueType)
    {
        VkSemaphore signalSemaphore = m_QueueSyncPrims[static_cast<uint32_t>(queueType)].GetOwnershipTransferSemaphore();
        m_QueueSyncPrims[static_cast<uint32_t>(queueType)].Submit(cmdLists, signalSemaphore, VK_NULL_HANDLE, false);
        return signalSemaphore;
    }

    void DeviceInternal::ExecuteCommandListsImmediate(std::vector<CommandList*>& cmdLists, CommandQueue queueType)
    {
        uint64_t waitValue = ExecuteCommandLists(cmdLists, queueType, false);
        m_QueueSyncPrims[static_cast<uint32_t>(queueType)].Wait(m_Device, waitValue);
    }

    void DeviceInternal::ExecuteCommandListsImmediate(std::vector<CommandList*>& cmdLists, CommandQueue queueType, VkSemaphore waitSemaphore, CommandQueue waitQueueType)
    {
        uint64_t waitValue = ExecuteCommandLists(cmdLists, queueType, waitSemaphore);
        m_QueueSyncPrims[static_cast<uint32_t>(queueType)].Wait(m_Device, waitValue);
        m_QueueSyncPrims[static_cast<uint32_t>(waitQueueType)].ReleaseOwnershipTransferSemaphore(waitSemaphore);
    }

    void DeviceInternal::ExecuteResourceTransitionImmediate(uint32_t barrierCount, const Barrier* pBarriers)
    {
        // should be executed on the transition queue
        // need to check if the resource is in a state meaning it was on the copy queue

        // Make sure any copying is being done on secondary threads
        CauldronAssert(ASSERT_ERROR, std::this_thread::get_id() != GetFramework()->MainThreadID() || !GetFramework()->IsRunning(), L"Do not issue immediate resource transition commands on the main thread after initialization is complete as this will be a blocking operation.");

        // TODO: for now, immediate transition are only called after loading data, so it should be done on the graphics queue as resources have been transfered there.
        constexpr CommandQueue c_QueueType = CommandQueue::Graphics;

        CommandList* pImmediateCmdList = CreateCommandList(L"TransitionCmdList", c_QueueType);

        ResourceBarrier(pImmediateCmdList, barrierCount, pBarriers);
        CloseCmdList(pImmediateCmdList);

        // Execute and sync
        std::vector<CommandList*> cmdLists;
        cmdLists.push_back(pImmediateCmdList);
        ExecuteCommandListsImmediate(cmdLists, c_QueueType);

        // No longer needed, will release allocator on destruction
        delete pImmediateCmdList;
    }

    void DeviceInternal::ExecuteTextureResourceCopyImmediate(uint32_t resourceCopyCount, const TextureCopyDesc* pCopyDescs)
    {
        // NOTE: This code doesn't handle the queue ownership transfer
        // Assume the texture is in a CopyDest state

        // Make sure any copying is being done on secondary threads
        CauldronAssert(ASSERT_ERROR, std::this_thread::get_id() != GetFramework()->MainThreadID() || !GetFramework()->IsRunning(), L"Do not issue loaded resource copy commands on the main thread as this will be a blocking operation.");

        constexpr CommandQueue c_QueueType = CommandQueue::Copy;

        CommandList* pImmediateCmdList = CreateCommandList(L"TextureCopyCmdList", c_QueueType);

        // Enqueue the barriers and close the command list
        for (uint32_t i = 0; i < resourceCopyCount; ++i)
            CopyTextureRegion(pImmediateCmdList, &pCopyDescs[i]);
        CloseCmdList(pImmediateCmdList);

        // Execute and sync
        std::vector<CommandList*> cmdLists;
        cmdLists.push_back(pImmediateCmdList);
        ExecuteCommandListsImmediate(cmdLists, c_QueueType);

        // No longer needed, will release allocator on destruction
        delete pImmediateCmdList;
    }

    void DeviceInternal::ExecuteResourceTransitionImmediate(CommandQueue queueType, uint32_t barrierCount, const Barrier* pBarriers)
    {
        // NOTE: This code doesn't handle the queue ownership transfer

        CommandList* pImmediateCmdList = CreateCommandList(L"TransitionCmdList", queueType);

        ResourceBarrier(pImmediateCmdList, barrierCount, pBarriers);
        CloseCmdList(pImmediateCmdList);

        // Execute and sync
        std::vector<CommandList*> cmdLists;
        cmdLists.push_back(pImmediateCmdList);
        ExecuteCommandListsImmediate(cmdLists, queueType);

        // No longer needed, will release allocator on destruction
        delete pImmediateCmdList;
    }

    void DeviceInternal::ReleaseCommandPool(CommandList* pCmdList)
    {
        m_QueueSyncPrims[static_cast<int32_t>(pCmdList->GetQueueType())].ReleaseCommandPool(pCmdList->GetImpl()->VKCmdPool());
    }

    void DeviceInternal::SetResourceName(VkObjectType objectType, uint64_t handle, const char* name)
    {
        if (m_vkSetDebugUtilsObjectNameEXT && handle && name)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = handle;
            nameInfo.pObjectName = name;
            m_vkSetDebugUtilsObjectNameEXT(m_Device, &nameInfo);
        }
    }

    void DeviceInternal::SetResourceName(VkObjectType objectType, uint64_t handle, const wchar_t* name)
    {
        if (m_vkSetDebugUtilsObjectNameEXT && handle && name)
        {
            std::string objectName = WStringToString(name);
            SetResourceName(objectType, handle, objectName.c_str());
        }
    }

    void DeviceInternal::QueueSyncPrimitive::Init(Device* pDevice, CommandQueue queueType, uint32_t queueFamilyIndex, uint32_t queueIndex, uint32_t numFramesInFlight, const char* name)
    {
        DeviceInternal* pDeviceInternal = static_cast<DeviceInternal*>(pDevice);
        m_QueueType = queueType;
        m_FamilyIndex = queueFamilyIndex;
        vkGetDeviceQueue(pDeviceInternal->VKDevice(), queueFamilyIndex, queueIndex, &m_Queue);
        pDeviceInternal->SetResourceName(VK_OBJECT_TYPE_QUEUE, (uint64_t)m_Queue, name);

        // create timeline semaphore
        VkSemaphoreTypeCreateInfo typeCreateInfo = {};
        typeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeCreateInfo.pNext = nullptr;
        typeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        typeCreateInfo.initialValue = m_LatestSemaphoreValue;

        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = &typeCreateInfo;
        createInfo.flags = 0;
        VkResult res = vkCreateSemaphore(pDeviceInternal->VKDevice(), &createInfo, nullptr, &m_Semaphore);
        CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Failed to create queue semaphore!");

        pDeviceInternal->SetResourceName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_Semaphore, "CauldronTimelineSemaphore");

        // create the frame semaphores semaphores
        createInfo.pNext = nullptr;
        createInfo.flags = 0; // not signaled
        m_FrameSemaphores.reserve(numFramesInFlight);
        for (uint32_t i = 0; i < numFramesInFlight; ++i)
        {
            VkSemaphore semaphore = VK_NULL_HANDLE;
            res = vkCreateSemaphore(pDeviceInternal->VKDevice(), &createInfo, nullptr, &semaphore);
            CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS && semaphore != VK_NULL_HANDLE, L"Failed to create queue semaphore!");

            pDeviceInternal->SetResourceName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, "CauldronSemaphore");
            m_FrameSemaphores.push_back(semaphore);
        }
    }

    void DeviceInternal::QueueSyncPrimitive::Release(VkDevice device)
    {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        while (m_AvailableCommandPools.PopFront(commandPool))
        {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }

        vkDestroySemaphore(device, m_Semaphore, nullptr);

        for (size_t i = 0; i < m_FrameSemaphores.size(); ++i)
        {
            vkDestroySemaphore(device, m_FrameSemaphores[i], nullptr);
        }
        m_FrameSemaphores.clear();
    }

    VkCommandPool DeviceInternal::QueueSyncPrimitive::GetCommandPool()
    {
        DeviceInternal* pDevice = GetDevice()->GetImpl();

        VkCommandPool pool = VK_NULL_HANDLE;

        // Check if there are any available allocators we can use
        if (m_AvailableCommandPools.PopFront(pool))
        {
            // reset the pool before using it
            vkResetCommandPool(pDevice->VKDevice(), pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

            return pool;
        }
        // No available allocators, so create a new one
        else
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.pNext = nullptr;
            poolInfo.flags = 0; // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = m_FamilyIndex;

            VkCommandPool commandPool;
            VkResult res = vkCreateCommandPool(pDevice->VKDevice(), &poolInfo, nullptr, &commandPool);
            CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Failed to create queue command pool!");
            pDevice->SetResourceName(VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)commandPool, "CauldronCommandPool");

            return commandPool;
        }
    }

    void DeviceInternal::QueueSyncPrimitive::ReleaseCommandPool(VkCommandPool commandPool)
    {
        m_AvailableCommandPools.PushBack(commandPool);
    }

    uint64_t DeviceInternal::QueueSyncPrimitive::Submit(const std::vector<CommandList*>& cmdLists, const VkSemaphore signalSemaphore, const VkSemaphore waitSemaphore, bool useEndOfFrameSemaphore)
    {
        std::lock_guard<std::mutex> lock(m_SubmitMutex);

        std::vector<VkCommandBuffer> commandBuffers;
        commandBuffers.reserve(cmdLists.size());
        for (auto& list : cmdLists)
        {
            CauldronAssert(ASSERT_CRITICAL, m_QueueType == list->GetQueueType(), L"Command list is submitted on the wrong queue.");
            commandBuffers.push_back(list->GetImpl()->VKCmdBuffer());
        }

        VkPipelineStageFlags waitStageFlags[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        VkSemaphore waitSemaphores[] = { m_Semaphore, waitSemaphore };

        uint32_t signalSemaphoreCount = 0;
        VkSemaphore signalSemaphores[3];
        signalSemaphores[signalSemaphoreCount++] = m_Semaphore;
        if (signalSemaphore != VK_NULL_HANDLE)
            signalSemaphores[signalSemaphoreCount++] = signalSemaphore;
        if (useEndOfFrameSemaphore)
            signalSemaphores[signalSemaphoreCount++] = m_FrameSemaphores[GetSwapChain()->GetBackBufferIndex()];

        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pNext = nullptr;
        info.waitSemaphoreCount = (waitSemaphore == VK_NULL_HANDLE) ? 1 : 2;
        info.pWaitSemaphores = waitSemaphores;
        info.pWaitDstStageMask = waitStageFlags;
        info.pCommandBuffers = commandBuffers.data();
        info.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        info.signalSemaphoreCount = signalSemaphoreCount;
        info.pSignalSemaphores = signalSemaphores;

        uint64_t semaphoreWaitValues[] = { m_LatestSemaphoreValue, 0 };
        ++m_LatestSemaphoreValue;

        // the second value is useless
        uint64_t semaphoreSignalValues[] = { m_LatestSemaphoreValue, 0, 0 };

        VkTimelineSemaphoreSubmitInfo semaphoreSubmitInfo = {};
        semaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        semaphoreSubmitInfo.pNext = nullptr;
        semaphoreSubmitInfo.waitSemaphoreValueCount = info.waitSemaphoreCount;
        semaphoreSubmitInfo.pWaitSemaphoreValues = semaphoreWaitValues;
        semaphoreSubmitInfo.signalSemaphoreValueCount = info.signalSemaphoreCount;
        semaphoreSubmitInfo.pSignalSemaphoreValues = semaphoreSignalValues;

        info.pNext = &semaphoreSubmitInfo;

        vkQueueSubmit(m_Queue, 1, &info, VK_NULL_HANDLE);

        return m_LatestSemaphoreValue;
    }

    uint64_t DeviceInternal::QueueSyncPrimitive::Present(VkSwapchainKHR swapchain, uint32_t imageIndex) // only valid on the present queue
    {
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_FrameSemaphores[imageIndex]; // NOTE: imageIndex is technically different from the frame in flight index but we are using the same in Cauldron.
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        std::lock_guard<std::mutex> lock(m_SubmitMutex);

        VkResult res = vkQueuePresentKHR(m_Queue, &presentInfo);
        CauldronAssert(ASSERT_ERROR, res == VK_SUCCESS, L"Failed to present");

        return m_LatestSemaphoreValue;
    }

    void DeviceInternal::QueueSyncPrimitive::Wait(VkDevice device, uint64_t waitValue) const
    {
        VkSemaphoreWaitInfo waitInfo = {};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.pNext = nullptr;
        waitInfo.flags = 0;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_Semaphore;
        waitInfo.pValues = &waitValue;
        VkResult res = vkWaitSemaphores(device, &waitInfo, std::numeric_limits<uint64_t>::max());
        CauldronAssert(ASSERT_WARNING, res == VK_SUCCESS, L"Failed to wait on the queue semaphore.");
    }

    void DeviceInternal::QueueSyncPrimitive::Flush()
    {
        std::lock_guard<std::mutex> lock(m_SubmitMutex);
        vkQueueWaitIdle(m_Queue);
    }

    VkSemaphore DeviceInternal::QueueSyncPrimitive::GetOwnershipTransferSemaphore()
    {
        std::lock_guard<std::mutex> lock(m_SubmitMutex);

        DeviceInternal* pDevice = GetDevice()->GetImpl();

        VkSemaphore semaphore;

        if (m_AvailableOwnershipTransferSemaphores.size() > 0)
        {
            semaphore = m_AvailableOwnershipTransferSemaphores[m_AvailableOwnershipTransferSemaphores.size() - 1];
            m_AvailableOwnershipTransferSemaphores.pop_back();
        }
        else
        {
            VkSemaphoreCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;

            VkResult res = vkCreateSemaphore(pDevice->VKDevice(), &createInfo, nullptr, &semaphore);
            CauldronAssert(ASSERT_CRITICAL, res == VK_SUCCESS, L"Failed to create queue ownership transfer semaphore!");
            pDevice->SetResourceName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, "CauldronOwnershipTransferSemaphore");
        }

        m_UsedOwnershipTransferSemaphores.push_back(semaphore);

        return semaphore;
    }

    void DeviceInternal::QueueSyncPrimitive::ReleaseOwnershipTransferSemaphore(VkSemaphore semaphore)
    {
        std::lock_guard<std::mutex> lock(m_SubmitMutex);

        for (auto it = m_UsedOwnershipTransferSemaphores.begin(); it != m_UsedOwnershipTransferSemaphores.end(); ++it)
        {
            if (*it == semaphore)
            {
                m_UsedOwnershipTransferSemaphores.erase(it);
                m_AvailableOwnershipTransferSemaphores.push_back(semaphore);
                return;
            }
        }

        CauldronCritical(L"Queue ownership transfer semaphore to release wasn't found.");
    }

} // namespace cauldron

#endif // #if defined(_VK)
