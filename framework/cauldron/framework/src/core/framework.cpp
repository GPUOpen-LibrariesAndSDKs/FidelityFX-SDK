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

// Pull in platform versions for actual cauldron type
#if defined(_WINDOWS)
    #include "core/win/framework_win.h"
#else
    #error Unsupported API or Platform!
#endif // defined(_WINDOWS)

#include "core/components/cameracomponent.h"
#include "core/components/lightcomponent.h"
#include "core/components/meshcomponent.h"
#include "core/components/animationcomponent.h"
#include "core/components/particlespawnercomponent.h"
#include "core/contentmanager.h"
#include "core/inputmanager.h"
#include "core/uimanager.h"
#include "core/scene.h"
#include "misc/corecounts.h"
#include "misc/fileio.h"
#include "misc/log.h"
#include "render/device.h"
#include "render/dynamicbufferpool.h"
#include "render/dynamicresourcepool.h"
#include "render/profiler.h"
#include "render/rasterview.h"
#include "render/resourceresizedlistener.h"
#include "render/resourceviewallocator.h"
#include "render/shadowmapresourcepool.h"
#include "render/swapchain.h"
#include "render/uploadheap.h"
#include "render/commandlist.h"
#include "render/rendermodule.h"
#include "render/rendermodules/fpslimiter/fpslimiterrendermodule.h"
#include "render/rendermodules/swapchain/swapchainrendermodule.h"
#include "render/rendermodules/tonemapping/tonemappingrendermodule.h"
#include "render/rendermodules/ui/uirendermodule.h"
#include "render/shaderbuilder.h"

#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <cctype>
#include <time.h>

#include "renderdoc/include/renderdoc_app.h"
#include "pix/pix3.h"
#define PIX_CAPTURE_PATH L"tempPix.wpix"

using namespace std::experimental;

// map DisplayMode values to JSON as strings
NLOHMANN_JSON_SERIALIZE_ENUM(DisplayMode, {
    {DisplayMode::DISPLAYMODE_LDR,           "DISPLAYMODE_LDR"          },
    {DisplayMode::DISPLAYMODE_HDR10_2084,    "DISPLAYMODE_HDR10_2084"   },
    {DisplayMode::DISPLAYMODE_HDR10_SCRGB,   "DISPLAYMODE_HDR10_SCRGB"  },
    {DisplayMode::DISPLAYMODE_FSHDR_2084,    "DISPLAYMODE_FSHDR_2084"   },
    {DisplayMode::DISPLAYMODE_FSHDR_SCRGB,   "DISPLAYMODE_FSHDR_SCRGB"  },
})

namespace cauldron
{
    // map ResourceFormat values to JSON as strings
    NLOHMANN_JSON_SERIALIZE_ENUM(cauldron::ResourceFormat,
    {
        {ResourceFormat::Unknown, "Unknown"},
        {ResourceFormat::R8_UNORM, "R8_UNORM"},
        {ResourceFormat::R8_UINT, "R8_UINT"},
        {ResourceFormat::R16_FLOAT, "R16_FLOAT"},
        {ResourceFormat::RGBA8_UNORM, "RGBA8_UNORM"},
        {ResourceFormat::RGBA8_SNORM, "RGBA8_SNORM"},
        {ResourceFormat::RGBA8_SRGB, "RGBA8_SRGB"},
        {ResourceFormat::RGB10A2_UNORM, "RGB10A2_UNORM"},
        {ResourceFormat::RG11B10_FLOAT, "RG11B10_FLOAT"},
        {ResourceFormat::RG16_FLOAT, "RG16_FLOAT"},
        {ResourceFormat::R32_FLOAT, "R32_FLOAT"},
        {ResourceFormat::RGBA16_UNORM, "RGBA16_UNORM"},
        {ResourceFormat::RGBA16_SNORM, "RGBA16_SNORM"},
        {ResourceFormat::RGBA16_FLOAT, "RGBA16_FLOAT"},
        {ResourceFormat::RG32_FLOAT, "RG32_FLOAT"},
        {ResourceFormat::RGBA32_FLOAT, "RGBA32_FLOAT"},
        {ResourceFormat::D16_UNORM, "D16_UNORM"},
        {ResourceFormat::D32_FLOAT, "D32_FLOAT"},
    })

    // map ShaderModel values to JSON as strings
    NLOHMANN_JSON_SERIALIZE_ENUM(cauldron::ShaderModel, {
        {ShaderModel::SM5_1,      "SM5_1"},
        {ShaderModel::SM6_0,      "SM6_0"},
        {ShaderModel::SM6_1,      "SM6_1"},
        {ShaderModel::SM6_2,      "SM6_2"},
        {ShaderModel::SM6_3,      "SM6_3"},
        {ShaderModel::SM6_4,      "SM6_4"},
        {ShaderModel::SM6_5,      "SM6_5"},
        {ShaderModel::SM6_6,      "SM6_6"},
        {ShaderModel::SM6_7,      "SM6_7"},
    })

    ///////////////////////////////////////////////////////////////////
    // CauldronConfig

    void CauldronConfig::Validate() const
    {
        // detect cycles in RenderResources
        const size_t maxMappingLoops = RenderResourceMappings.size();
        for (auto it = RenderResourceMappings.begin(); it != RenderResourceMappings.end(); ++it)
        {
            size_t i = 0;
            const wchar_t* name = it->second.c_str(); // first mapping
            // find last alias
            for (; i < maxMappingLoops; ++i)
            {
                CauldronAssert(ASSERT_ERROR, std::this_thread::get_id() != GetFramework()->MainThreadID() || !GetFramework()->IsRunning(), L"Performance Warning: Using std::map::find on the main thread while app is running.");
                auto mapping = RenderResourceMappings.find(name);
                if (mapping != RenderResourceMappings.end())
                {
                    name = mapping->second.c_str();
                }
                else
                {
                    CauldronAssert(ASSERT_CRITICAL, RenderResources.find(name) != RenderResources.end(), L"Resource %ls isn't defined.", name);
                    break;
                }
            }
            CauldronAssert(ASSERT_CRITICAL, i < maxMappingLoops, L"A cyclic render resource definition has been detected in the config file starting at %ls.", it->first.c_str());
        }
    }

    // Search for the final name of the resource if there are some mappings/aliases
    const wchar_t* CauldronConfig::GetAliasedResourceName(const wchar_t* resourceName) const
    {
        CauldronAssert(ASSERT_ERROR, std::this_thread::get_id() != GetFramework()->MainThreadID() || !GetFramework()->IsRunning(), L"Performance Warning: Using std::map::find on the main thread while app is running.");
        // detect loops
        const size_t maxLoops = RenderResourceMappings.size();

        // find last alias
        for (size_t i = 0; i <= maxLoops; ++i)
        {
            auto it = RenderResourceMappings.find(resourceName);
            if (it != RenderResourceMappings.end())
            {
                resourceName = it->second.c_str();
            }
            else
            {
                return resourceName;
            }
        }

        CauldronCritical(L"There is a loop in Config RenderResourceMappings");

        return nullptr;
    }

    RenderResourceInformation CauldronConfig::GetRenderResourceInformation(const wchar_t* resourceName) const
    {
        CauldronAssert(ASSERT_ERROR, std::this_thread::get_id() != GetFramework()->MainThreadID() || !GetFramework()->IsRunning(), L"Performance Warning: Using std::map::find on the main thread while app is running.");
        return RenderResources.find(GetAliasedResourceName(resourceName))->second;
    }

    ///////////////////////////////////////////////////////////////////
    // CommonFramework
    Framework* g_pFrameworkInstance = nullptr;

    Framework::Framework(const FrameworkInitParams* pInitParams) :
        m_pImpl(new FrameworkInternal(this, pInitParams)),
        m_Name(pInitParams->Name),
        m_ConfigFileName(L"configs/cauldronconfig.json"),
        m_CmdLine(pInitParams->CmdLine)
    {
        CauldronAssert(ASSERT_ERROR, !g_pFrameworkInstance, L"Multiple framework instances being created. Resources will leak.");
        g_pFrameworkInstance = static_cast<Framework*>(this);

        // Initialize the logger (crucial to pickup any init errors)
        CauldronAssert(ASSERT_CRITICAL, Log::InitLogSystem(L"Cauldron.log") >= 0, L"Failed to initialize log system. Make sure folder is not write-protected or drive is full.");

        // Initialize the task manager (necessary to do any background loading we might request)
        m_pTaskManager = new TaskManager();
        uint32_t numThreads = GetRecommendedThreadCount() - 1;  // Remove one thread to account for the main thread, which we won't count
        CauldronAssert(ASSERT_CRITICAL, !m_pTaskManager->Init(numThreads), L"Failed to initialize the task manager.");

        // Also set the CPU name while we are at it
        GetCPUDescription(m_CPUName);
    }

    Framework::~Framework()
    {
        delete m_pContentManager;
        delete m_pScene;
        delete m_pUIManager;
        delete m_pInputManager;
        delete m_pDynamicBufferPool;
        delete m_pUploadHeap;
        delete m_pProfiler;
        delete m_pSwapChain;
        delete m_pShadowMapResourcePool;
        delete m_pDynamicResourcePool;
        delete m_pResourceViewAllocator;
        delete m_pRasterViewAllocator;
        delete m_pDevice;
        delete m_pTaskManager;
        delete m_pImpl;
    }

    int32_t Framework::Init()
    {
        // The main thread ID
        m_MainThreadID = std::this_thread::get_id();

        if (0 != m_pImpl->Init()) {
            return -1;
        }

        // Set width and height accordingly to what's been specified in config/command line
        m_ResolutionInfo = { m_Config.Width, m_Config.Height, m_Config.Width, m_Config.Height };

        // Init RenderDoc
        if(m_Config.EnableRenderDocCapture)
        {
            Log::Write(LOGLEVEL_TRACE, L"Initializing RenderDoc.");
            HMODULE mod = ::LoadLibraryW(L"renderdoc.dll");

            if(mod)
            {
                pRENDERDOC_GetAPI RENDERDOC_GetAPI =
                    (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
                int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&m_RenderDocApi);
                assert(ret == 1);
            }
        }

        // Init Pix
        if(m_Config.EnablePixCapture)
        {
            Log::Write(LOGLEVEL_TRACE, L"Initializing WinPixGpuCapturer.");
            HMODULE mod = ::LoadLibraryW(L"WinPixGpuCapturer.dll");
            assert(mod);
        }

        // Initialize the device, resource allocator, and swap chain
        Log::Write(LOGLEVEL_TRACE, L"Initializing graphics device.");
        m_pDevice = Device::CreateDevice();
        if (!m_pDevice) return -1;

        Log::Write(LOGLEVEL_TRACE, L"Initializing graphics resource view allocator.");
        m_pResourceViewAllocator = ResourceViewAllocator::CreateResourceViewAllocator();
        if (!m_pResourceViewAllocator) return -1;

        Log::Write(LOGLEVEL_TRACE, L"Initializing raster view allocator.");
        m_pRasterViewAllocator = new RasterViewAllocator();
        if (!m_pRasterViewAllocator) return -1;

        Log::Write(LOGLEVEL_TRACE, L"Initializing graphics dynamic resource pool.");
        m_pDynamicResourcePool = new DynamicResourcePool();
        if (!m_pDynamicResourcePool) return -1;

        Log::Write(LOGLEVEL_TRACE, L"Initializing graphics shadow map resource pool.");
        m_pShadowMapResourcePool = new ShadowMapResourcePool();
        if (!m_pShadowMapResourcePool) return -1;

        Log::Write(LOGLEVEL_TRACE, L"Initializing graphics swap chain.");
        m_pSwapChain = SwapChain::CreateSwapchain();
        if (!m_pSwapChain) return -1;

        Log::Write(LOGLEVEL_TRACE, L"Initializing requested render resources.");
        if (-1 == CreateRenderResources()) return -1;

        Log::Write(LOGLEVEL_TRACE, L"Initializing profiler.");
        m_pProfiler = Profiler::CreateProfiler();
        if (!m_pProfiler) return -1;

        // Initialize upload heap and constant buffer pool
        Log::Write(LOGLEVEL_TRACE, L"Initializing graphics upload heap.");
        m_pUploadHeap = UploadHeap::CreateUploadHeap();
        if (!m_pUploadHeap) return -1;

        Log::Write(LOGLEVEL_TRACE, L"Initializing graphics constant buffer pool.");
        m_pDynamicBufferPool = DynamicBufferPool::CreateDynamicBufferPool();
        if (!m_pDynamicBufferPool) return -1;

        // Initialize shader compile system
        Log::Write(LOGLEVEL_TRACE, L"Initializing shader compiler.");
        if (InitShaderCompileSystem() < 0) return -1;

        // Initialize input manager
        Log::Write(LOGLEVEL_TRACE, L"Initializing input manager.");
        m_pInputManager = InputManager::CreateInputManager();
        if (!m_pInputManager) return -1;

        // Initialize UI manager
        Log::Write(LOGLEVEL_TRACE, L"Initializing UI manager.");
        m_pUIManager = new UIManager();
        if (!m_pUIManager) return -1;

        // Create the scene
        Log::Write(LOGLEVEL_TRACE, L"Initializing scene");
        m_pScene = new Scene();
        if (!m_pScene) return -1;

        // Initialize the ContentManager (which is partially dependent on everything above)
        Log::Write(LOGLEVEL_TRACE, L"Initializing content manager.");
        m_pContentManager = new ContentManager();
        if (!m_pContentManager) return -1;

        // Do all necessary registrations of software modules
        RegisterComponentsAndModules();

        // Initialize the scene (default entity's for camera, etc)
        m_pScene->InitScene();

        // Initializing render modules (note we can start getting asset loads at this point, so everything needs to be in place prior)
        Log::Write(LOGLEVEL_TRACE, L"Creating RenderModules");
        for (auto iter = m_Config.RenderModules.begin(); iter != m_Config.RenderModules.end(); ++iter)
        {
            // Create render module instance, and add it's default execution callback to our list of callbacks for the runtime
            RenderModule* pRMInstance = RenderModuleFactory::CreateInstance(iter->Name);

            // Push the default execution callback
            std::pair<RenderModule*, ExecuteCallback> defaultCallback = std::make_pair(pRMInstance, [pRMInstance](double deltaTime, CommandList* pCmdList) {
                pRMInstance->Execute(deltaTime, pCmdList);
                });
            m_ExecutionCallbacks.push_back(std::make_pair(pRMInstance->GetName(), defaultCallback));

            // Complete initialization after all instances have been created
            m_RenderModules.push_back(pRMInstance);
        }

        // RM's must be initialized after all of them are instanced in the event we need to register additional callbacks
        // woven into the rm execution callbacks
        Log::Write(LOGLEVEL_TRACE, L"Initializing RenderModules");
        auto iterRM = m_RenderModules.begin();
        for (auto iter = m_Config.RenderModules.begin(); iter != m_Config.RenderModules.end(); ++iter, ++iterRM)
        {
            // Fetch the RM instance (and do a sanity check)
            CauldronAssert(ASSERT_CRITICAL, StringToWString(iter->Name) == (*iterRM)->GetName(), L"Mismatch in RenderModule order and Config RenderModule order. Cannot properly initialize. Abort!");
            (*iterRM)->Init(iter->InitOptions);
        }

        // Initialize the sample side (and quit now if something goes wrong)
        Log::Write(LOGLEVEL_TRACE, L"Initializing sample.");
        if ( DoSampleInit() < 0 ) return -1;

        return 0;
    }

    int32_t Framework::PreRun()
    {
        // Initialize internal scene content (which may get tracked by various RMs)
        m_pScene->InitSceneContent();

        // Track sample loading time and log once complete
        m_LoadingStartTime = std::chrono::system_clock::now();

        // Kick off any content creation tasks as well
        for (auto& task : m_Config.ContentCreationTasks)
            GetTaskManager()->AddTask(task);

        // Request startup content
        for (const auto& scene : m_Config.StartupContent.Scenes)
        {
            filesystem::path contentPath = scene.c_str();
            // Only GLTF is supported now
            if (contentPath.extension() == L".gltf")
                GetContentManager()->LoadGLTFToScene(contentPath);

            else
                CauldronError(L"File %ls is not a currently supported file type and will be ignored", contentPath.c_str());
        }

        if (!m_Config.StartupContent.ParticleSpawners.empty())
            GetContentManager()->LoadParticlesToScene(m_Config.StartupContent.ParticleSpawners);

        // Initialize time last right before running so we don't hit a spike on the first frame
        m_LastFrameTime = std::chrono::system_clock::now();

        return 0;
    }

    // In case we need it later
    void Framework::PostRun()
    {
        // Exiting the run state
        m_Running.store(false);

        // If we are benchmarking and need to take a screen shot, dump the last frame out to file
        if (m_Config.TakeScreenshot)
        {
            // If we are bencharmking, use the benchmarking path
            filesystem::path outputPath;
            if (m_Config.EnableBenchmark)
            {
                outputPath = m_Config.BenchmarkPath;
            }
            else
            {
                outputPath = L"screenshots\\";
            }

            // Defensive, in case path doesn't exist
            if (!outputPath.empty())
                filesystem::create_directory(outputPath);

            // Make a file name that is unique (sample name exe + permutations of interest + time stamp to seconds)
            std::wstringstream fileName;
            fileName << m_Config.AppName << '_' << GetDevice()->GetDeviceName();

            // Add resolution
            const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
            fileName << '_' << resInfo.DisplayWidth << 'x' << resInfo.DisplayHeight;

            // Add permutations to file name for further identification
            for (auto& permutation : m_Config.BenchmarkPermutationOptions) {
                fileName << '_' << permutation.second;
            }

#pragma warning(push)
#pragma warning(disable:4996) // Avoid deprecation warning on std::localtime
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            fileName << '_' << std::put_time(std::localtime(&in_time_t), L"%Y-%m-%d-%H-%M-%S") << L".jpg";
            outputPath.append(fileName.str());
#pragma warning(pop)

            if (m_Config.EnableBenchmark)
                m_Config.ScreenShotFileName = outputPath;

            // Dump it out
            m_pSwapChain->DumpSwapChainToFile(outputPath);
        }
    }

    void Framework::Shutdown()
    {
        // Flush for good measure one last time before shutting down to make sure nothing is still in the pipes
        m_pDevice->FlushAllCommandQueues();

        // Let the sample know we are shutting down in case there is some cleanup it needs to do
        Log::Write(LOGLEVEL_TRACE, L"Shutting down sample.");
        DoSampleShutdown();

        // Shutdown the framework
        Log::Write(LOGLEVEL_TRACE, L"Shutting down cauldron framework.");

        // Output Perf Stats
        if (m_Config.EnableBenchmark && m_PerfFrameCount > 0)
        {
            filesystem::path outputPath(m_Config.BenchmarkPath);

            // Defensive, in case path doesn't exist
            if (!m_Config.BenchmarkPath.empty())
                filesystem::create_directory(m_Config.BenchmarkPath);

            std::wstring fileName;
            if (m_Config.BenchmarkAppend)
            {
                // no timestamp, write everything into one file.
                fileName = m_Name + L"-perf" + (m_Config.BenchmarkJson ? L".json" : L".csv");
            }
            else
            {
#pragma warning(push)
#pragma warning(disable:4996) // Avoid deprecation warning on std::localtime
                // put timestamp in filename to avoid overwriting
                // have to use C-style time formatting because C++11 does not have any
                std::time_t  now = std::time(nullptr);
                std::wstring timeString(64, ' ');
                timeString.resize(std::wcsftime(&timeString[0], timeString.size(), L"%FT%H-%M-%S", std::localtime(&now)));
                fileName = m_Name + L"-perf-" + timeString + (m_Config.BenchmarkJson ? L".json" : L".csv");
#pragma warning(pop)
            }

            // Open for writing
            filesystem::path outputFile = outputPath / fileName;
            // create the file if it does not exist
            std::wofstream   file(outputFile.c_str(), m_Config.BenchmarkAppend ? std::ios_base::app : std::ios_base::out);
            if (m_Config.BenchmarkAppend)
            {
                // we have to reopen with in|out, so that we can overwrite things, and to make tellp() work
                file.close();
                file.open(outputFile.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::ate);
            }
            if (!file)
            {
                char errmsg[256];
                strerror_s(errmsg, 256, errno);
                Log::Write(LOGLEVEL_FATAL, L"Opening benchmark file failed: %ls", StringToWString(errmsg).c_str());
            }
            bool hasHeader = file.tellp() > std::wofstream::pos_type(0);

            double runtime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - m_StartTime).count();
            const auto GetMs = [](std::chrono::nanoseconds ns) -> double {
                return (double)ns.count() / 1000000.0;
            };
            if (m_Config.BenchmarkJson)
            {
                if (m_Config.BenchmarkAppend && !hasHeader)
                {
                    // Json append output is array.
                    file << L"[\n";
                }
                if (m_Config.BenchmarkAppend && hasHeader)
                {
                    // Override array close ']' with a comma.
                    file.seekp(-1, std::ios_base::end);
                    file << L",\n";
                }
                json outputData = json::object();
                outputData["AppID"] = WStringToString(m_Config.AppName);
                outputData["GPU"] = WStringToString(m_pDevice->GetDeviceName());
                outputData["DriverVersion"] = WStringToString(m_pDevice->GetDriverVersion());
                outputData["API"] = WStringToString(m_pDevice->GetGraphicsAPIShort());
                outputData["CPU"] = WStringToString(m_CPUName);
                outputData["CmdLine"] = WStringToString(m_CmdLine);
                outputData["Permutations"] = json::object({});
                for (const auto& permutation : m_Config.BenchmarkPermutationOptions)
                {
                    outputData["Permutations"][WStringToString(permutation.first)] = WStringToString(permutation.second);
                }
                outputData["DisplayResolution"] = json::array({m_BenchmarkResolutionInfo.DisplayWidth, m_BenchmarkResolutionInfo.DisplayHeight});
                outputData["RenderResolution"] = json::array({m_BenchmarkResolutionInfo.RenderWidth, m_BenchmarkResolutionInfo.RenderHeight});
                outputData["Runtime"] = runtime;
                outputData["AvgFPS"] = (double)m_PerfFrameCount / runtime;

                auto buildLabelJson = [GetMs, this](const PerfStats& ps) -> json {
                    return json::object({
                        {"min_ms", GetMs(ps.min)},
                        {"min_ns", ps.min.count()},
                        {"max_ms", GetMs(ps.max)},
                        {"max_ns", ps.max.count()},
                        {"avg_ms", GetMs(ps.total) / (double)m_PerfFrameCount},
                        {"total_ns", ps.total.count()},
                    });
                };
                outputData["GPUTime"] = buildLabelJson(m_GpuPerfStats[0]);
                outputData["CPUTime"] = buildLabelJson(m_CpuPerfStats[0]);
                outputData["GPULabels"] = json::object({});
                for (const auto& ps : m_GpuPerfStats)
                {
                    outputData["GPULabels"][WStringToString(ps.Label)] = buildLabelJson(ps);
                }
                outputData["CPULabels"] = json::object({});
                for (const auto& ps : m_CpuPerfStats)
                {
                    outputData["CPULabels"][WStringToString(ps.Label)] = buildLabelJson(ps);
                }

                // Dump the screenshot name associate with this benchmark if we've got one
                outputData["ScreenshotName"] = m_Config.ScreenShotFileName.empty() ? "" : WStringToString(m_Config.ScreenShotFileName.c_str());

                file << StringToWString(outputData.dump());
                if (m_Config.BenchmarkAppend)
                {
                    file << ']';
                }
            }
            else
            {
                if (m_Config.BenchmarkAppend)
                {
                    if (!hasHeader)
                    {
                        file << L"AppID,GPU,DriverVersion,API,CPU,Display Resolution,Render Resolution,Runtime [s],Avg FPS,Min GPU [ms],Max GPU [ms],Avg GPU [ms],Min CPU [ms],Max CPU [ms],Avg CPU [ms]";
                        // Lay out all of the counters (will just output meantime)
                        for (const auto& ps : m_GpuPerfStats)
                            file << ',' << ps.Label ;

                        for (const auto& ps : m_CpuPerfStats)
                            file << ',' << ps.Label;

                        file << L",ScreenshotName";
                        file << L",CmdLine\n";
                    }

                    // Write out the numbers
                    file << m_Config.AppName.c_str() << ',';
                    file << m_pDevice->GetDeviceName() << ',';
                    file << m_pDevice->GetDriverVersion() << ',';
                    file << m_pDevice->GetGraphicsAPIShort() << ',';
                    file << m_CPUName << ',';
                    file << m_BenchmarkResolutionInfo.DisplayWidth << 'x' << m_BenchmarkResolutionInfo.DisplayHeight << ',';
                    file << m_BenchmarkResolutionInfo.RenderWidth << 'x' << m_BenchmarkResolutionInfo.RenderHeight << ',';
                    file << runtime << ',' << (double)m_PerfFrameCount / runtime << ',';

                    // get min/max/avg from first label
                    file << GetMs(m_GpuPerfStats[0].min) << ',' << GetMs(m_GpuPerfStats[0].max) << ',' << GetMs(m_GpuPerfStats[0].total) / (double)m_PerfFrameCount << ',';
                    file << GetMs(m_CpuPerfStats[0].min) << ',' << GetMs(m_CpuPerfStats[0].max) << ',' << GetMs(m_CpuPerfStats[0].total) / (double)m_PerfFrameCount;

                    // go through all labels
                    for (const auto& ps : m_GpuPerfStats)
                        file << ',' << GetMs(ps.total) / (double)m_PerfFrameCount;

                    for (const auto& ps : m_CpuPerfStats)
                        file << ',' << GetMs(ps.total) / (double)m_PerfFrameCount;

                    // Dump the screenshot name associate with this benchmark if we've got one
                    file << ',' << (m_Config.ScreenShotFileName.empty() ? "" : WStringToString(m_Config.ScreenShotFileName.c_str()).c_str());

                    file << ',' << m_CmdLine.c_str() << '\n';
                }
                else
                {
                    // write info header
                    file << L"Info,Value\n";
                    file << L"CmdLine," << m_CmdLine.c_str() << '\n';
                    file << L"AppId," << m_Config.AppName.c_str() << '\n';
                    file << L"GPU," << m_pDevice->GetDeviceName() << '\n';
                    file << L"DriverVersion," << m_pDevice->GetDriverVersion() << '\n';
                    file << L"API," << m_pDevice->GetGraphicsAPIShort() << '\n';
                    file << L"CPU," << m_CPUName << '\n';
                    file << L"Display Resolution," << m_BenchmarkResolutionInfo.DisplayWidth << 'x' << m_BenchmarkResolutionInfo.DisplayHeight << '\n';
                    file << L"Render Resolution," << m_BenchmarkResolutionInfo.RenderWidth << 'x' << m_BenchmarkResolutionInfo.RenderHeight << '\n';
                    file << L"Runtime [s]," << runtime << '\n';
                    file << L"Avg FPS," << (double)m_PerfFrameCount / runtime << '\n';
                    // non-append mode has per-marker details. First marker in CPU and GPU sections is whole frame.
                    file << L"CPU/GPU,Label,Min [ms],Max [ms],Mean [ms]\n";
                    for (const auto& ps : m_CpuPerfStats)
                    {
                        file << "CPU," << ps.Label << ',' << GetMs(ps.min) << ',' << GetMs(ps.max) << ',' << GetMs(ps.total) / (double)m_PerfFrameCount << '\n';
                    }
                    for (const auto& ps : m_GpuPerfStats)
                    {
                        file << "GPU," << ps.Label << ',' << GetMs(ps.min) << ',' << GetMs(ps.max) << ',' << GetMs(ps.total) / (double)m_PerfFrameCount << '\n';
                    }

                    // Dump the screenshot name associate with this benchmark if we've got one
                    file << "ScreenshotName," << (m_Config.ScreenShotFileName.empty() ? "" : WStringToString(m_Config.ScreenShotFileName.c_str())).c_str() << '\n';
                }
            }
        }

        // Terminate the task manager
        m_pTaskManager->Shutdown();

        // delete all loaded content
        m_pContentManager->Shutdown();

        // terminate the scene
        m_pScene->TerminateScene();

        // Unregister all component managers
        UnRegisterComponentsAndRenderModules();

        // Terminate shader compiler
        TerminateShaderCompileSystem();

        // Terminate log system
        Log::TerminateLogSystem();
    }

    // Utility function to parse all known options from config data
    void Framework::ParseConfigData(const json& jsonConfigData)
    {
        // Get the configuration data passed in
        json configData = jsonConfigData;

        // Initialize validation configuration
        if (configData.find("Validation") != configData.end())
        {
            json validationConfig         = configData["Validation"];
            m_Config.CPUValidationEnabled = validationConfig.value("CpuValidationLayerEnabled", m_Config.CPUValidationEnabled);
            m_Config.GPUValidationEnabled = validationConfig.value("GpuValidationLayerEnabled", m_Config.GPUValidationEnabled);
        }

        // Initialize debug configuration
        if (configData.find("DebugOptions") != configData.end())
        {
            json debugOptionsConfig         = configData["DebugOptions"];
            m_Config.DeveloperMode          = debugOptionsConfig.value("DevelopmentMode", m_Config.DeveloperMode);
            m_Config.DebugShaders           = debugOptionsConfig.value("DebugShaders", m_Config.DebugShaders);
            m_Config.EnableRenderDocCapture = debugOptionsConfig.value("EnableRenderDocCapture", m_Config.EnableRenderDocCapture);
            m_Config.EnablePixCapture       = debugOptionsConfig.value("EnablePixCapture", m_Config.EnablePixCapture);
        }

        // Initialize feature support configuration
        if (configData.find("FeatureSupport") != configData.end())
        {
            json featuresConfig     = configData["FeatureSupport"];
            m_Config.VRSTier1       = featuresConfig.value("VRSTier1", m_Config.VRSTier1);
            m_Config.VRSTier2       = featuresConfig.value("VRSTier2", m_Config.VRSTier2);
            m_Config.RT_1_0         = featuresConfig.value("RT1.0", m_Config.RT_1_0);
            m_Config.RT_1_1         = featuresConfig.value("RT1.1", m_Config.RT_1_1);
            m_Config.FP16           = featuresConfig.value("FP16", m_Config.FP16);
            m_Config.MinShaderModel = featuresConfig.value<ShaderModel>("ShaderModel", m_Config.MinShaderModel);
        }

        // Initialize presentation configuration
        if (configData.find("Presentation") != configData.end())
        {
            json presentationConfig     = configData["Presentation"];
            m_Config.BackBufferCount    = presentationConfig.value<uint8_t>("BackBufferCount", m_Config.BackBufferCount);
            m_Config.Vsync              = presentationConfig.value("Vsync", m_Config.Vsync);
            m_Config.Fullscreen         = presentationConfig.value("Fullscreen", m_Config.Fullscreen);
            m_Config.Width              = presentationConfig.value<uint32_t>("Width", m_Config.Width);
            m_Config.Height             = presentationConfig.value<uint32_t>("Height", m_Config.Height);
            m_Config.CurrentDisplayMode = presentationConfig.value<DisplayMode>("Mode", m_Config.CurrentDisplayMode);
        }

        // Initialize allocation configuration
        if (configData.find("Allocations") != configData.end())
        {
            json allocationsConfig         = configData["Allocations"];
            m_Config.UploadHeapSize        = allocationsConfig.value("UploadHeapSize", m_Config.UploadHeapSize);  // Default to 100 MB
            m_Config.DynamicBufferPoolSize = allocationsConfig.value("DynamicBufferPoolSize", m_Config.DynamicBufferPoolSize);
            m_Config.GPUSamplerViewCount   = allocationsConfig.value("GPUSamplerViewCount", m_Config.GPUSamplerViewCount);
            m_Config.GPUResourceViewCount  = allocationsConfig.value("GPUResourceViewCount", m_Config.GPUResourceViewCount);
            m_Config.CPUResourceViewCount  = allocationsConfig.value("CPUResourceViewCount", m_Config.CPUResourceViewCount);
            m_Config.CPURenderViewCount    = allocationsConfig.value("CPURenderViewCount", m_Config.CPURenderViewCount);
            m_Config.CPUDepthViewCount     = allocationsConfig.value("CPUDepthViewCount", m_Config.CPUDepthViewCount);
        }

        // Initialize render resources
        if (configData.find("RenderResources") != configData.end())
        {
            // Initialize render resources
            CauldronAssert(ASSERT_ERROR,
                           std::this_thread::get_id() != GetFramework()->MainThreadID() || !GetFramework()->IsRunning(),
                           L"Performance Warning: Using std::map::emplace on the main thread while app is running.");

            json renderResources = configData["RenderResources"];
            if (renderResources.is_object())
            {
                for (auto it = renderResources.begin(); it != renderResources.end(); ++it)
                {
                    if (it->is_string())
                    {
                        auto emplaceResults = m_Config.RenderResourceMappings.emplace(std::make_pair(StringToWString(it.key()), StringToWString(it->get<std::string>())));
                        // If we didn't emplace the alias, that means we are trying to re-direct it so fix it up
                        if (!emplaceResults.second)
                            emplaceResults.first->second = StringToWString(it->get<std::string>());
                    }

                    else if (it.value().is_object())
                    {
                        RenderResourceInformation info;
                        info.Format   = it.value().value<ResourceFormat>("Format", ResourceFormat::RGBA8_UNORM);
                        info.AllowUAV = it.value().value<bool>("AllowUAV", false);
                        info.RenderResolution = it.value().value<bool>("RenderResolution", false);
                        auto emplaceResults = m_Config.RenderResources.emplace(std::make_pair(StringToWString(it.key()), info));
                        // If we didn't emplace the resource information, that means we are trying to re-define it, so fix up the existing definition
                        if (!emplaceResults.second)
                            emplaceResults.first->second = info;
                    }
                }
            }
        }

        // Initialize other settings
        m_Config.DeveloperMode         = configData.value("DevelopmentMode", m_Config.DeveloperMode);
        m_Config.DebugShaders          = configData.value("DebugShaders", m_Config.DebugShaders);
        m_Config.FontSize              = configData.value("FontSize", m_Config.FontSize);
        m_Config.AGSEnabled            = configData.value("AGSEnabled", m_Config.AGSEnabled);
        m_Config.StablePowerState      = configData.value("StablePowerState", m_Config.StablePowerState);
        m_Config.InvertedDepth         = configData.value("InvertedDepth", m_Config.InvertedDepth);
        m_Config.OverrideSceneSamplers = configData.value("OverrideSceneSamplers", m_Config.OverrideSceneSamplers);
        m_Config.TakeScreenshot        = configData.value("Screenshot", m_Config.TakeScreenshot);
        m_Config.BuildRayTracingAccelerationStructure = configData.value("BuildRayTracingAccelerationStructure", m_Config.BuildRayTracingAccelerationStructure);

        // Content initialization
        if (configData.find("Content") != configData.end())
        {
            json loadingContent = configData["Content"];

            // Check if we requested any scenes load
            if (loadingContent.find("Scenes") != loadingContent.end())
            {
                for (uint32_t sceneId = 0; sceneId < loadingContent["Scenes"].size(); ++sceneId)
                {
                    // If we have a valid path to a scene file, queue it up
                    // (Note these scenes can be overridden by passing (a) scene(s) to load on the command line)
                    filesystem::path sceneFile = filesystem::path(StringToWString(loadingContent["Scenes"][sceneId]));
                    if (filesystem::exists(sceneFile)) {
                        m_Config.StartupContent.Scenes.push_back(sceneFile);
                    }
                }
            }

            // Check if we requested any camera to be set as default
            if (loadingContent.find("Camera") != loadingContent.end())
            {
                m_Config.StartupContent.Camera = StringToWString(loadingContent["Camera"]);
            }

            // Check if we requested a specific exposure
            if (loadingContent.find("SceneExposure") != loadingContent.end())
            {
                m_Config.StartupContent.SceneExposure = loadingContent.value("SceneExposure", m_Config.StartupContent.SceneExposure);
            }

            // Check if we requested any IBL maps
            if (loadingContent.find("DiffuseIBL") != loadingContent.end())
            {
                m_Config.StartupContent.DiffuseIBL = StringToWString(loadingContent["DiffuseIBL"]);
            }
            if (loadingContent.find("SpecularIBL") != loadingContent.end())
            {
                m_Config.StartupContent.SpecularIBL = StringToWString(loadingContent["SpecularIBL"]);
            }
            if (loadingContent.find("SkyMap") != loadingContent.end())
            {
                m_Config.StartupContent.SkyMap = StringToWString(loadingContent["SkyMap"]);
            }

            // Check for particle spawners
            json particleSpawners = loadingContent["ParticleSpawners"];
            if (particleSpawners.is_array())
            {
                for (auto it = particleSpawners.begin(); it != particleSpawners.end(); ++it)
                {
                    json spawner = it.value();
                    ParticleSpawnerDesc spawnDesc = {};

                    spawnDesc.Name = StringToWString(spawner.value("Name", ""));
                    spawnDesc.AtlasPath = filesystem::path(StringToWString(spawner.value("AtlasPath", "")));
                    spawnDesc.Position = Vec3(spawner["Position"][0], spawner["Position"][1], spawner["Position"][2]);
                    spawnDesc.Sort = spawner.value("Sort", true);

                    // Go through emitters
                    json emitters = spawner["Emitters"];
                    if (emitters.is_array())
                    {
                        for (auto emitteritr = emitters.begin(); emitteritr != emitters.end(); ++emitteritr)
                        {
                            json emitter = emitteritr.value();
                            EmitterDesc emitterDesc = {};

                            emitterDesc.EmitterName = StringToWString(emitter.value("Name", ""));
                            emitterDesc.SpawnOffset = Vec3(emitter["SpawnOffset"][0], emitter["SpawnOffset"][1], emitter["SpawnOffset"][2]);
                            emitterDesc.SpawnOffsetVariance = Vec3(emitter["SpawnOffsetVariance"][0], emitter["SpawnOffsetVariance"][1], emitter["SpawnOffsetVariance"][2]);
                            emitterDesc.SpawnVelocity = Vec3(emitter["SpawnVelocity"][0], emitter["SpawnVelocity"][1], emitter["SpawnVelocity"][2]);
                            emitterDesc.SpawnVelocityVariance = emitter.value("SpawnVelocityVariance", 0.f);
                            emitterDesc.ParticlesPerSecond = emitter.value("ParticlesPerSecond", 0);
                            emitterDesc.Lifespan = emitter.value("Lifespan", 0.f);
                            emitterDesc.SpawnSize = emitter.value("SpawnSize", 0.f);
                            emitterDesc.KillSize = emitter.value("KillSize", 0.f);
                            emitterDesc.Mass = emitter.value("Mass", 0.f);
                            emitterDesc.AtlasIndex = emitter.value("AtlasIndex", -1);

                            json flags = emitter["Flags"];
                            emitterDesc.Flags |= flags.value("Streaks", false) ? EmitterDesc::EF_Streaks : 0;
                            emitterDesc.Flags |= flags.value("Reactive", true) ? EmitterDesc::EF_Reactive : 0;

                            // Add it to the list
                            spawnDesc.Emitters.push_back(emitterDesc);
                        }
                    }

                    // Add it to the list
                    m_Config.StartupContent.ParticleSpawners.push_back(spawnDesc);
                }
            }
        }

        if (configData.find("RenderModuleOptions") != configData.end())
        {
            m_Config.RenderModules.back().InitOptions = configData["RenderModuleOptions"];
        }

        // Check if dependencies of the current render module are satisfied
        if (configData.find("Dependencies") != configData.end())
        {
            json dependenciesList = configData["Dependencies"];
            if (dependenciesList.is_object())
            {
                std::set<std::string> rmDependencies;
                for (auto it = dependenciesList.begin(); it != dependenciesList.end(); ++it)
                {
                    rmDependencies.insert(it.value());
                }

                std::set<std::string> rmAvailable;
                for (auto it = m_Config.RenderModules.begin(); it != m_Config.RenderModules.end(); ++it)
                {
                    rmAvailable.insert(it->Name);
                }

                const std::string logMessage("Could not parse dependencies for " + m_Config.RenderModules.back().Name);
                CauldronAssert(ASSERT_CRITICAL, AreDependeciesPresent(rmDependencies, rmAvailable), std::wstring(logMessage.begin(), logMessage.end()).c_str());
            }
        }

        // Get all the render module information for initialization later
        if (configData.find("RenderModules") != configData.end())
        {
            json renderModuleList = configData["RenderModules"];
            {

                for (auto it = renderModuleList.begin(); it != renderModuleList.end(); ++it)
                {
                    const json& renderModuleData = *it;

                    RenderModuleInfo rmInfo;
                    rmInfo.Name = it.value();

                    // Store for initialization later (or override from the calling sample if needed)
                    m_Config.RenderModules.push_back(rmInfo);

                    // Check if the render module has a config file
                    // If there is one, it will be in configs\rm_configs\[rendermodulename].json"
                    // where [rendermodulename] is the name of the rendermodule in lowercase letters
                    // NOTE: This is only valid for render modules that live within the rendermodule directory.
                    // Specialized rendermodules will need to pull in their configs (if any) through the sample config it (see default sample.cpp)

                    // Lowercase rendermodule name
                    std::string lowerCaseRMConfigName = rmInfo.Name;
                    std::transform(rmInfo.Name.begin(), rmInfo.Name.end(), lowerCaseRMConfigName.begin(), [](unsigned char c) { return std::tolower(c); });

                    const auto configPath = filesystem::path("configs\\rm_configs\\" + lowerCaseRMConfigName + ".json");
                    if (filesystem::exists(configPath))
                    {
                        json rmConfigData;
                        CauldronAssert(ASSERT_CRITICAL, ParseJsonFile(configPath.c_str(), rmConfigData), L"Could not parse JSON file %ls", rmInfo.Name);

                        // Get the sample configuration
                        json configData = rmConfigData[rmInfo.Name];

                        // Let the framework parse all the known options for us
                        const std::string logMessage("Parsing config file for " + rmInfo.Name);
                        Log::Write(LOGLEVEL_TRACE, std::wstring(logMessage.begin(), logMessage.end()).c_str());
                        ParseConfigData(configData);
                    }
                    else
                    {
                        const std::string logMessage("Could not find config file for " + rmInfo.Name + ", skipping...");
                        Log::Write(LOGLEVEL_TRACE, std::wstring(logMessage.begin(), logMessage.end()).c_str());
                    }

                }
            }
        }

        if (configData.find("RenderModuleOverrides") != configData.end())
        {
            json renderModuleList = configData["RenderModuleOverrides"];
            if (renderModuleList.is_object())
            {
                for (auto it = renderModuleList.begin(); it != renderModuleList.end(); ++it)
                {
                    const json& renderModuleOverride = *it;
#if _DEBUG
                    bool found = false;
#endif // _DEBUG

                    for (auto& iter : m_Config.RenderModules)
                    {
                        if (iter.Name == it.key().c_str())
                        {
                            iter.InitOptions = renderModuleOverride;
#if _DEBUG
                            found = true;
#endif // _DEBUG
                            break;
                        }
                    }

#if _DEBUG
                    CauldronAssert(ASSERT_ERROR, found, L"Could not find render module %hs to override options", it.key().c_str());
#endif // _DEBUG
                }
            }
        }
        // After getting render module configurations, e.g. TAA writes to this value
        m_Config.MotionVectorGeneration = configData.value("MotionVectorGeneration", m_Config.MotionVectorGeneration);

        // Initialize Benchmark config
        if (configData.find("Benchmark") != configData.end())
        {
            json benchmarkConfig     = configData["Benchmark"];
            m_Config.EnableBenchmark = benchmarkConfig.value("Enabled", m_Config.EnableBenchmark);
            m_Config.BenchmarkFrameDuration = benchmarkConfig.value("FrameDuration", m_Config.BenchmarkFrameDuration);
            m_Config.BenchmarkPath = benchmarkConfig.value("Path", m_Config.BenchmarkPath);
        }

        // Validate that the information are correct
        m_Config.Validate();
    }

    void Framework::InitConfig()
    {
        // Parse config file
        ParseConfigFile(m_ConfigFileName.c_str());

        // Parse the command line parameters (these can be used to override config params)
        ParseCmdLine(m_CmdLine.c_str());

        // GPU timing info is synched to the swapchain and reported with a delay equal to the number of back buffers
        // so we need to set up that delay at the start
        m_PerfFrameCount = -static_cast<int64_t>(m_Config.BackBufferCount);
    }

    void Framework::EnableUpscaling(bool enabled, ResolutionUpdateFunc func/*=nullptr*/)
    {
        m_UpscalerEnabled = enabled;
        CauldronAssert(ASSERT_WARNING, !enabled || func, L"Upscaler enabled without resolution update function, there may be some unintended side effects");

        // Set or clear the resolution updater
        m_ResolutionUpdaterFn = (m_UpscalerEnabled) ? func : nullptr;

        // Need to update the resolution info
        if (m_ResolutionUpdaterFn)
            m_ResolutionInfo = m_ResolutionUpdaterFn(m_ResolutionInfo.DisplayWidth, m_ResolutionInfo.DisplayHeight);
        else
            m_ResolutionInfo = { m_ResolutionInfo.DisplayWidth, m_ResolutionInfo.DisplayHeight, m_ResolutionInfo.DisplayWidth, m_ResolutionInfo.DisplayHeight };

        // Flush the GPU as this may have implications on resource creations
        ResizeEvent();
    }

    void Framework::EnableFrameInterpolation(bool enabled)
    {
        m_FrameInterpolationEnabled = true;
    }

    void Framework::ResizeEvent()
    {
        // Flush everything before resizing resources (can't have anything in the pipes)
        GetDevice()->FlushAllCommandQueues();

        // resize all resolution-dependent resources
        m_pDynamicResourcePool->OnResolutionChanged(m_ResolutionInfo);

        // Notify that the swapchain has been recreated and other resources have been resized
        {
            std::lock_guard<std::mutex> lock(m_ResourceResizeMutex);
            for (auto pListener : m_ResourceResizedListeners)
                pListener->OnResourceResized();
        }

        // Handle any render module resize callbacks
        for (auto& rm : m_RenderModules)
            rm->OnResize(m_ResolutionInfo);

        // Call the sample resize
        DoSampleResize(m_ResolutionInfo);
    }

    void Framework::ParseConfigFile(const wchar_t* configFileName)
    {
        Log::Write(LOGLEVEL_TRACE, L"Parsing cauldron config file.");

        json jsonConfigFile;
        CauldronAssert(ASSERT_CRITICAL, ParseJsonFile(configFileName, jsonConfigFile), L"Could not parse JSON file %ls", configFileName);

        // Setup default bools prior to parse
        m_Config.CPUValidationEnabled  = false;
        m_Config.GPUValidationEnabled  = false;
        m_Config.VRSTier1              = false;
        m_Config.VRSTier2              = false;
        m_Config.RT_1_0                = false;
        m_Config.RT_1_1                = true;
        m_Config.FP16                  = true;
        m_Config.Vsync                 = false;
        m_Config.Fullscreen            = false;
        m_Config.DeveloperMode         = false;
        m_Config.DebugShaders          = false;
        m_Config.AGSEnabled            = false;
        m_Config.StablePowerState      = false;
        m_Config.TakeScreenshot        = false;
        m_Config.LimitFPS              = false;
        m_Config.GPULimitFPS           = false;
        m_Config.InvertedDepth         = true;
        m_Config.OverrideSceneSamplers = true;
        m_Config.BuildRayTracingAccelerationStructure = false;

        // Perf defaults
        m_Config.BenchmarkAppend       = false;
        m_Config.EnableBenchmark       = false;
        m_Config.BenchmarkJson         = false;

        // Get the Cauldron configuration
        json cauldronConfig = jsonConfigFile["Cauldron"];

        // Parse the data for cauldron
        ParseConfigData(cauldronConfig);

        // Do sample-side configuration loading
        ParseSampleConfig();

        // Append UI, FPSLimiter, and Swap chain render modules which are integral to cauldron's functionality
        // Defining here instead of thorugh config file to make use of numeric_limits to get largest priorities
        // Cauldron's render resources are defined in the config file
        RenderModuleInfo uiRMInfo        = {"UIRenderModule", {}};
        RenderModuleInfo fpsLimitRMInfo  = {"FPSLimiterRenderModule", {}};
        RenderModuleInfo swapChainRMInfo = {"SwapChainRenderModule", {}};
        m_Config.RenderModules.push_back(uiRMInfo);
        m_Config.RenderModules.push_back(fpsLimitRMInfo);
        m_Config.RenderModules.push_back(swapChainRMInfo);
    }

    void Framework::ParseCmdLine(const wchar_t* cmdLine)
    {
        Log::Write(LOGLEVEL_TRACE, L"Parsing command line parameters.");

        // Process the command line settings
        LPWSTR* pArgList;
        int argCount;
        pArgList = CommandLineToArgvW(cmdLine, &argCount);

        std::wstring command;
        for (int currentArg = 0; currentArg < argCount; ++currentArg)
        {
            command = pArgList[currentArg];

            // Development mode
            if (command == L"-devmode")
            {
                m_Config.DeveloperMode = true;
                m_Config.DebugShaders = true;           // Enable debug shaders
                m_Config.CPUValidationEnabled = true;   // Enable CPU validation layers in devmode
                continue;
            }

            // Frame limiter (gpu & cpu variants)
            if (command == L"-cpulimiter")
            {
                // We require 1 argument to limit fps
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"No target frame rate provided when  -cpulimiter requested!");
                m_Config.LimitFPS = true;
                m_Config.GPULimitFPS = false;
                try
                {
                    uint32_t targetFPS = std::stoi(pArgList[currentArg + 1]);
                    m_Config.LimitedFrameRate = targetFPS;
                }
                catch (std::invalid_argument const&)
                {
                    CauldronCritical(L"Could not convert provided command line target frame rate to numerical value.");
                }
                ++currentArg;
                continue;
            }
            else if (command == L"-gpulimiter")
            {
                // We require 1 argument to limit fps
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"No target frame rate provided when  -gpulimiter requested!");
                m_Config.LimitFPS = true;
                m_Config.GPULimitFPS = true;
                try
                {
                    uint32_t targetFPS = std::stoi(pArgList[currentArg + 1]);
                    m_Config.LimitedFrameRate = targetFPS;
                }
                catch (std::invalid_argument const&)
                {
                    CauldronCritical(L"Could not convert provided command line target frame rate to numerical value.");
                }
                ++currentArg;
                continue;
            }

            // Override depth type
            if (command == L"-inverteddepth")
            {
                // We require 1 argument to override depth
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"-inverteddepth usage: -inverteddepth 1/0");
                bool invertedDepth = !(std::wstring(pArgList[currentArg + 1]) == L"0"); // disable with zero, anything else means "on"
                m_Config.InvertedDepth = invertedDepth;
                ++currentArg;
                continue;
            }

            // Force full screen
            if (command == L"-fullscreen")
            {
                m_Config.Fullscreen = true;
                continue;
            }

            // Specify a display resolution
            if (command == L"-resolution")
            {
                // We require 2 arguments to set resolution
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 2 && pArgList[currentArg + 1][0] != L'-' && pArgList[currentArg + 2][0] != L'-', L"-resolution requires a width and height be provided (usage: -resolution <width> <height>");
                try
                {
                    uint32_t width = std::stoi(pArgList[currentArg + 1]);
                    m_Config.Width = width;

                    uint32_t height = std::stoi(pArgList[currentArg + 2]);
                    m_Config.Height = height;
                }
                catch (std::invalid_argument const&)
                {
                    CauldronCritical(L"Could not convert provided command line width or height to numerical value.");
                }
                currentArg += 2;
                continue;
            }

            // Load content at startup
            if (command == L"-loadcontent")
            {
                // We require at least 1 argument to load content
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"No content provided for loading when -loadcontent requested!");

                // Clear any queud up content (from config) as this is an override
                m_Config.StartupContent.Scenes.clear();

                int contentArgId = 1;
                while (currentArg + contentArgId < argCount)
                {
                    // If we've not encountered a new command, enqueue content to load
                    if (pArgList[currentArg + contentArgId][0] != L'-')
                        m_Config.StartupContent.Scenes.push_back(pArgList[currentArg + contentArgId++]);
                    else
                        break;
                }

                // Update location of current arg
                currentArg += contentArgId - 1;
                continue;
            }

            // Override diffuse IBL
            if (command == L"-diffuseibl")
            {
                // We require at least 1 argument to load content
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"No content provided for loading when -diffuseibl requested!");

                // If we've not encountered a new command, enqueue content to load
                m_Config.StartupContent.DiffuseIBL = pArgList[currentArg + 1];

                // Update location of current arg
                ++currentArg;
                continue;
            }

            // Override specular IBL
            if (command == L"-specularibl")
            {
                // We require at least 1 argument to load content
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"No content provided for loading when -specularibl requested!");

                // If we've not encountered a new command, enqueue content to load
                m_Config.StartupContent.SpecularIBL = pArgList[currentArg + 1];

                // Update location of current arg
                ++currentArg;
                continue;
            }

            // Override skydome environment map
            if (command == L"-skymap")
            {
                // We require at least 1 argument to load content
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"No content provided for loading when -skymap requested!");

                // If we've not encountered a new command, enqueue content to load
                m_Config.StartupContent.SkyMap = pArgList[currentArg + 1];

                // Update location of current arg
                ++currentArg;
                continue;
            }

            // Override camera
            if (command == L"-camera")
            {
                // We require at least 1 argument to set camera
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"No camera name provided when -camera requested!");

                // If we've not encountered a new command, enqueue content to load
                m_Config.StartupContent.Camera = pArgList[currentArg + 1];

                // Update location of current arg
                ++currentArg;
                continue;
            }

            // Override scene exposure
            if (command == L"-exposure")
            {
                CauldronAssert(ASSERT_CRITICAL, argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-', L"-exposure requires a floating point exposure be provided (usage: -exposure <value>");
                try
                {
                    float exposure = std::stof(pArgList[currentArg + 1]);
                    m_Config.StartupContent.SceneExposure = exposure;
                }

                catch (std::invalid_argument const&)
                {
                    CauldronCritical(L"Could not convert provided command line exposure to numerical value.");
                }

                currentArg++;
                continue;
            }

            if(command == L"-renderdoc")
            {
                m_Config.EnableRenderDocCapture = true;
                continue;
            }

            if(command == L"-pix")
            {
                m_Config.EnablePixCapture = true;
                continue;
            }

            if (command == L"-screenshot")
            {
                m_Config.TakeScreenshot = true;
                continue;
            }

            // perf dump
            if (command == L"-benchmark")
            {
                m_Config.EnableBenchmark = true;
                if (currentArg + 1 >= argCount)
                {
                    CauldronError(L"No arguments given to -benchmark");
                    m_Config.EnableBenchmark = false;
                    continue;
                }

                // Force FPS limiting on (GPU) when doing benchmarking
                m_Config.LimitFPS = true;
                m_Config.GPULimitFPS = true;
                m_Config.LimitedFrameRate = 60;

                // cycle through all the options until we hit the next core command line option or end of arguments
                int localArg = currentArg + 1;
                while (localArg < argCount && pArgList[localArg][0] != L'-')
                {
                    std::wstring argument = pArgList[localArg];

                    if (argument == L"append")
                        m_Config.BenchmarkAppend = true;

                    else if (argument == L"json")
                        m_Config.BenchmarkJson = true;

                    else if (!argument.compare(0, 9, L"duration="))
                    {
                        argument = argument.substr(9);
                        CauldronAssert(ASSERT_WARNING, argument.length() <= 10, L"Benchmark duration exceeds number of digits that will fit into a uint32. Value will be truncated.");
                        wchar_t* wstr;
                        uint32_t frameDuration = wcstol(argument.c_str(), &wstr, 10);
                        if (*wstr)
                        {
                            CauldronWarning(L"Benchmark duration does not convert to a numerical value");
                        }
                        else
                            m_Config.BenchmarkFrameDuration = frameDuration;
                    }

                    else if (!argument.compare(0, 5, L"path="))
                        m_Config.BenchmarkPath = argument.substr(5);

                    ++localArg;
                }

                // Setup to properly continue parsing
                currentArg = localArg - 1;
                continue;
            }

            // Display Mode
            if (command == L"-displaymode")
            {
                // We require at least 1 argument to load content
                CauldronAssert(ASSERT_CRITICAL,
                               argCount - currentArg > 1 && pArgList[currentArg + 1][0] != L'-',
                               L"-displaymode requires a input to be provided (usage: -displaymode <input>");

                command = pArgList[currentArg + 1];
                if (command == L"DISPLAYMODE_LDR")
                    m_Config.CurrentDisplayMode = DisplayMode::DISPLAYMODE_LDR;
                else if (command == L"DISPLAYMODE_HDR10_2084")
                    m_Config.CurrentDisplayMode = DisplayMode::DISPLAYMODE_HDR10_2084;
                else if (command == L"DISPLAYMODE_HDR10_SCRGB")
                    m_Config.CurrentDisplayMode = DisplayMode::DISPLAYMODE_HDR10_SCRGB;
                else if (command == L"DISPLAYMODE_FSHDR_2084")
                    m_Config.CurrentDisplayMode = DisplayMode::DISPLAYMODE_FSHDR_2084;
                else if (command == L"DISPLAYMODE_FSHDR_SCRGB")
                    m_Config.CurrentDisplayMode = DisplayMode::DISPLAYMODE_FSHDR_SCRGB;
                else
                    CauldronCritical(L"Could not convert provided command line displaymode to enum value.");

                m_Config.BenchmarkPermutationOptions.push_back(std::make_pair(L"displaymode", command));

                currentArg += 1;
                continue;
            }
        }

        // Pass on the command line string to the sample in the event they are overriding our parsing
        ParseSampleCmdLine(cmdLine);
    }

    void Framework::RegisterComponentsAndModules()
    {
        // Register framework's render modules
        RenderModuleFactory::RegisterModule<ToneMappingRenderModule>("ToneMappingRenderModule");
        RenderModuleFactory::RegisterModule<UIRenderModule>("UIRenderModule");
        RenderModuleFactory::RegisterModule<FPSLimiterRenderModule>("FPSLimiterRenderModule");
        RenderModuleFactory::RegisterModule<SwapChainRenderModule>("SwapChainRenderModule");

        // Register all Component Managers we know about
        RegisterComponentManager<CameraComponentMgr>();
        RegisterComponentManager<LightComponentMgr>();
        RegisterComponentManager<MeshComponentMgr>();
        RegisterComponentManager<AnimationComponentMgr>();
        RegisterComponentManager<ParticleSpawnerComponentMgr>();

        // Call sample registrations
        RegisterSampleModules();
    }

    bool Framework::AreDependeciesPresent(const std::set<std::string>& dependencies, const std::set<std::string>& available)
    {
        uint32_t count = 0;

        for (const auto& rm : dependencies)
        {
            if(std::find(available.begin(), available.end(), rm) != available.end())
            {
                ++count;
            }
        }

        return count == dependencies.size();
    }

    int32_t Framework::Run()
    {
        return m_pImpl->Run();
    }

    // Handles updating things outside the scope of the calling sample, and calls the sample's main loop function that controls render flow
    void Framework::MainLoop()
    {
        // Before doing component/render module updates, offer samples the chance to do any updates
        {
            CPUScopedProfileCapture marker(L"SampleUpdates");
            DoSampleUpdates(m_DeltaTime);
        }

        // Begin Frame
        BeginFrame();

        // Update UI manager (can impact other items in the frame)
        // NOTE: UI updates before input as we need to let our ui back end (ImGui) hijack inputs to work well
        {
            CPUScopedProfileCapture marker(L"UI Update");
            m_pUIManager->Update(m_DeltaTime);
        }

        // Update our input state
        {
            CPUScopedProfileCapture marker(L"Input Update");
            m_pInputManager->Update();
        }

        // Delete all the contents that have been marked for deletion
        {
            CPUScopedProfileCapture marker(L"UpdateContent");
            m_pContentManager->UpdateContent(m_FrameID);
        }

        // Update all registered component managers
        {
            CPUScopedProfileCapture marker(L"ComponentUpdates");
            for (auto compMgrIter = m_ComponentManagers.begin(); compMgrIter != m_ComponentManagers.end(); ++compMgrIter)
                compMgrIter->second->UpdateComponents(m_DeltaTime);
        }

        // If the scene is not yet ready, skip to end frame
        if (m_pScene->IsReady())
        {
            // Do any scene updates (setup scene info for the frame, etc.)
            {
                CPUScopedProfileCapture marker(L"UpdateScene");
                m_pScene->UpdateScene(m_DeltaTime, m_pCmdListForFrame);
            }

            // Call all registered render modules
            {
                CloseCmdList(m_pCmdListForFrame);
                m_vecCmdListsForFrame.push_back(m_pCmdListForFrame);

                CPUScopedProfileCapture marker(L"RM Executes");
                for (auto& callback : m_ExecutionCallbacks)
                {
                    if (callback.second.first->ModuleEnabled() && callback.second.first->ModuleReady())
                    {
                        m_pCmdListForFrame = m_pDevice->CreateCommandList(L"RenderModuleGraphicsCmdList", CommandQueue::Graphics);
                        SetAllResourceViewHeaps(m_pCmdListForFrame);

                        callback.second.second(m_DeltaTime, m_pCmdListForFrame);
                        CloseCmdList(m_pCmdListForFrame);
                        m_vecCmdListsForFrame.push_back(m_pCmdListForFrame);
                    }
                }
            }
        }

        m_pCmdListForFrame = m_pDeviceCmdListForFrame;

        // EndFrame will close and submit all active command lists and
        // kick off the work for execution before performing a Present()
        EndFrame();
    }

    // Handles start of frame logic (like frame count update and delta time calculations)
    void Framework::BeginFrame()
    {
        // Update frame count
        ++m_FrameID;

        // Start updating the CPU counters first to catch any waiting on swapchain
        m_pProfiler->BeginCPUFrame();

        static bool loggedLoadingTime = false;
        if (!loggedLoadingTime && !m_pContentManager->IsCurrentlyLoading())
        {
            // Log the time it took to load
            double loadDelta = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now() - m_LoadingStartTime).count() / 1000.0;
            Log::Write(LOGLEVEL_TRACE, L"Content loading took %f seconds", loadDelta);
            loggedLoadingTime = true;
        }

        if (m_Config.EnableBenchmark)
        {
            if (!m_pContentManager->IsCurrentlyLoading()
                && std::all_of(m_RenderModules.begin(), m_RenderModules.end(), [](const auto& it) { return it->ModuleReady(); }))
            {
                if (m_PerfFrameCount == 0)
                {
                    Log::Write(LOGLEVEL_TRACE, L"All modules ready, commencing benchmark.");
                    // First frame with all modules ready. Wait one frame to start gathering information (from this next frame).
                    // Set start (and possible stop) time now.
                    m_StartTime = std::chrono::steady_clock::now();
                    if (m_Config.BenchmarkFrameDuration < UINT32_MAX)
                        Log::Write(LOGLEVEL_TRACE, L"Benchmarking for %u frames.", m_Config.BenchmarkFrameDuration);
                    m_PerfFrameCount++;
                    // store resolution info at beginning of benchmark, since it will change when upscaling modules are disabled,
                    // which will happen just before shutdown (before output is printed)
                    m_BenchmarkResolutionInfo = m_ResolutionInfo;
                }
                else if (m_PerfFrameCount < 0)
                {
                    // wait two more frames until GPU timings are available and reliable (non-zero)
                    m_PerfFrameCount++;
                }
                else
                {
                    // Get timings from last frame
                    const std::vector<TimingInfo>& cpuTimings = m_pProfiler->GetCPUTimings();
                    const std::vector<TimingInfo>& gpuTimings = m_pProfiler->GetGPUTimings();

                    // aggregate stats
                    if (m_CpuPerfStats.size() != cpuTimings.size() || m_GpuPerfStats.size() != gpuTimings.size())
                    {
                        if (!m_CpuPerfStats.empty() || !m_GpuPerfStats.empty())
                        {
                            Log::Write(LOGLEVEL_INFO, L"Timing markers changed during benchmark. Resetting stats.");
                            if (m_Config.BenchmarkFrameDuration < UINT32_MAX)
                                Log::Write(LOGLEVEL_TRACE, L"Benchmarking for another %u frames.", m_Config.BenchmarkFrameDuration);
                        }
                        // initialize with current info
                        m_CpuPerfStats.clear();
                        m_CpuPerfStats.reserve(cpuTimings.size());
                        for (const auto& ti : cpuTimings)
                        {
                            std::chrono::nanoseconds dur = ti.GetDuration();
                            m_CpuPerfStats.emplace_back(PerfStats{ ti.Label, dur, dur, dur });
                        }
                        m_GpuPerfStats.clear();
                        m_GpuPerfStats.reserve(gpuTimings.size());
                        for (const auto& ti : gpuTimings)
                        {
                            std::chrono::nanoseconds dur = ti.GetDuration();
                            m_GpuPerfStats.emplace_back(PerfStats{ ti.Label, dur, dur, dur });
                        }
                        m_PerfFrameCount = 1;
                    }
                    else
                    {
                        // update stats
                        for (size_t i = 0; i < cpuTimings.size(); i++)
                        {
                            std::chrono::nanoseconds dur = cpuTimings[i].GetDuration();
                            m_CpuPerfStats[i].min = std::min(dur, m_CpuPerfStats[i].min);
                            m_CpuPerfStats[i].max = std::max(dur, m_CpuPerfStats[i].max);
                            m_CpuPerfStats[i].total += dur;
                        }
                        for (size_t i = 0; i < gpuTimings.size(); i++)
                        {
                            std::chrono::nanoseconds dur = gpuTimings[i].GetDuration();
                            m_GpuPerfStats[i].min = std::min(dur, m_GpuPerfStats[i].min);
                            m_GpuPerfStats[i].max = std::max(dur, m_GpuPerfStats[i].max);
                            m_GpuPerfStats[i].total += dur;
                        }
                        m_PerfFrameCount++;
                    }
                }
            }
        }

        // Need to exclude begin CPUFrame from our timings due to switch over (maybe should move CPU timing collection to end of frame?)
        CPUScopedProfileCapture marker(L"Begin Frame");

        // Make sure the swapchain is ready for this frame
        m_pSwapChain->WaitForSwapChain();

        // Refresh the command list pool
        m_pDeviceCmdListForFrame = m_pDevice->BeginFrame();

        // create a commandlist to use
        m_pCmdListForFrame = m_pDevice->CreateCommandList(L"EndFrameGraphicsCmdList", CommandQueue::Graphics);
        SetAllResourceViewHeaps(m_pCmdListForFrame);

        // Start GPU counters now that we have a cmd list
        m_pProfiler->BeginGPUFrame(m_pCmdListForFrame);

        // If upscaler is enabled, we will be in a pre-upscale state until the upscaler is executed and updates the state
        if (m_UpscalerEnabled)
            m_UpscalingState = UpscalerState::PreUpscale;

        // Transition swapchain to expected state for render module usage
        Barrier presentBarrier = Barrier::Transition(m_pSwapChain->GetBackBufferRT()->GetCurrentResource(), ResourceState::Present, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
        ResourceBarrier(m_pCmdListForFrame, 1, &presentBarrier);

        // Get the Dynamic Buffer pool ready for frame
        m_pDynamicBufferPool->BeginFrame();

        // Update frame time (so that it is in seconds)
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        m_DeltaTime = std::chrono::duration<double, std::milli>(now - m_LastFrameTime).count() / 1000.0;
        m_LastFrameTime = now;

        // Capture RenderDoc
        if(m_RenderDocApi && (m_RenderDocCaptureState == FrameCaptureState::CaptureRequested))
        {
            // Kicked-off the capture for this frame.
            static_cast<RENDERDOC_API_1_1_2*>(m_RenderDocApi)->StartFrameCapture(NULL, NULL);
            m_RenderDocCaptureState = FrameCaptureState::CaptureStarted;
        }

        // Capture Pix
        if(m_PixCaptureState == FrameCaptureState::CaptureRequested)
        {
            PIXCaptureParameters params = {};
            params.GpuCaptureParameters.FileName = PIX_CAPTURE_PATH;

            // Kicked-off the capture for this frame.
            PIXBeginCapture(PIX_CAPTURE_GPU, &params);
            m_PixCaptureState = FrameCaptureState::CaptureStarted;
        }
    }

    struct GPUExecutionPacket
    {
        std::vector<CommandList*> CmdLists     = {};
        uint64_t                  CompletionID = 0;

        GPUExecutionPacket(std::vector<CommandList*>& cmdLists, uint64_t completionID)
            : CmdLists(std::move(cmdLists))
            , CompletionID(completionID)
        {
        }
        GPUExecutionPacket() = delete;
    };
    void Framework::DeleteCommandListAsync(void* pInFlightGPUInfo)
    {
        GPUExecutionPacket* pInflightPacket = static_cast<GPUExecutionPacket*>(pInFlightGPUInfo);

        // Wait until the command lists are processed
        m_pDevice->WaitOnQueue(pInflightPacket->CompletionID, CommandQueue::Graphics);

        // Delete them to release the allocators
        for (auto cmdListIter = pInflightPacket->CmdLists.begin(); cmdListIter != pInflightPacket->CmdLists.end(); ++cmdListIter)
            delete *cmdListIter;
        pInflightPacket->CmdLists.clear();
        delete pInflightPacket;
    }

    // Handles all end of frame logic (like present)
    void Framework::EndFrame()
    {
        CPUScopedProfileCapture marker(L"EndFrame");

        // Transition swapchain from expected state for render module usage to present
        Barrier presentBarrier = Barrier::Transition(m_pSwapChain->GetBackBufferRT()->GetCurrentResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::Present);
        ResourceBarrier(m_pCmdListForFrame, 1, &presentBarrier);

        {
            // Asynchronously delete the active command list in the background once it's cleared the graphics queue
            uint64_t                      signalValue     = m_pDevice->ExecuteCommandLists(m_vecCmdListsForFrame, CommandQueue::Graphics, false);
            cauldron::GPUExecutionPacket* pInflightPacket = new GPUExecutionPacket(m_vecCmdListsForFrame, signalValue);
            GetTaskManager()->AddTask(Task(std::bind(&Framework::DeleteCommandListAsync, this, std::placeholders::_1), reinterpret_cast<void*>(pInflightPacket)));
        }

        // End the frame of the profiler
        m_pProfiler->EndFrame(m_pCmdListForFrame);

        // Can't be referenced until next time BeginFrame is called
        m_pDeviceCmdListForFrame = nullptr;
        m_pCmdListForFrame = nullptr;
        m_vecCmdListsForFrame.clear();

        // Closes all command lists
        m_pDevice->EndFrame();

        // Present
        m_pSwapChain->Present();

        // If we are doing GPU Validation, flush every frame
        if (m_Config.GPUValidationEnabled)
            m_pDevice->FlushAllCommandQueues();

        // Reset the RenderDoc capture
        if(m_RenderDocApi && (m_RenderDocCaptureState == FrameCaptureState::CaptureStarted))
        {
            static_cast<RENDERDOC_API_1_1_2*>(m_RenderDocApi)->EndFrameCapture(NULL, NULL);
            static_cast<RENDERDOC_API_1_1_2*>(m_RenderDocApi)->LaunchReplayUI(1, nullptr);
            static_cast<RENDERDOC_API_1_1_2*>(m_RenderDocApi)->ShowReplayUI();

            // Done with the RenderDoc for the frame
            m_RenderDocCaptureState = FrameCaptureState::None;
        }

        // Reset the Pix capture
        if(m_PixCaptureState == FrameCaptureState::CaptureStarted)
        {
            while(PIXEndCapture(false) == E_PENDING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
            m_PixCaptureState = FrameCaptureState::None;

            // Open the file with the default application
            {
                SHELLEXECUTEINFOW sei;
                memset(&sei, 0, sizeof(SHELLEXECUTEINFOW));

                sei.cbSize = sizeof(SHELLEXECUTEINFOW);
                sei.lpVerb = L"open";
                sei.lpFile = PIX_CAPTURE_PATH;
                sei.nShow = SW_SHOWNORMAL;
                sei.fMask = SEE_MASK_NOCLOSEPROCESS;

                if (!ShellExecuteExW(&sei))
                {
                    MessageBoxW(NULL, L"Could not open the Pix capture - is Pix installed?", L"Err0r", MB_ICONERROR);
                }
            }
        }

        // Stop running if the perf dump timer ran out
        // If no timer is set, the stop time is UINT32_MAX, which should be high enough to not occur with normal operation
        // (it would take 50 days at 1000 FPS)
        if (m_PerfFrameCount == m_Config.BenchmarkFrameDuration)
        {
            m_StopTime = std::chrono::steady_clock::now();

            // imitate user closing the window for graceful shutdown
            PostQuitMessage(0);
        }
    }

    const Texture* Framework::GetColorTargetForCallback(const wchar_t* callbackOrModuleName)
    {
        CauldronAssert(ASSERT_WARNING, !IsRunning(), L"GetColorTargetForCallback is intended to be called during initialization phase. Calling this function at runtime can have performance implications.");
        return GetRenderTexture(L"HDR11Color");
    }

    void Framework::GetUpscaledRenderInfo(uint32_t& width, uint32_t& height, float& renderWidthRatio, float& renderHeightRatio) const
    {
        if (m_UpscalingState == UpscalerState::None || m_UpscalingState == UpscalerState::PostUpscale)
        {
            width  = m_ResolutionInfo.DisplayWidth;
            height = m_ResolutionInfo.DisplayHeight;
            renderWidthRatio = 1.f;
            renderHeightRatio = 1.f;
        }
        else
        {
            width             = m_ResolutionInfo.RenderWidth;
            height            = m_ResolutionInfo.RenderHeight;
            renderWidthRatio  = m_ResolutionInfo.GetRenderWidthScaleRatio();
            renderHeightRatio = m_ResolutionInfo.GetRenderHeightScaleRatio();
        }
    }

    void Framework::RegisterExecutionCallback(std::wstring insertionName, bool preInsertion, ExecutionTuple callbackTuple)
    {
        // Start by trying to find the insertion point
        auto iter = m_ExecutionCallbacks.begin();
        for (iter; iter != m_ExecutionCallbacks.end(); ++iter)
        {
            if (iter->first == insertionName)
                break;
        }

        // Did we find an insertion point?
        if (iter != m_ExecutionCallbacks.end())
        {
            // insert will always insert before a specified iterator (see std::vector::insert() documentation)
            if (preInsertion)
                m_ExecutionCallbacks.insert(iter, callbackTuple);
            else
                m_ExecutionCallbacks.insert(++iter, callbackTuple);
            return;
        }

        CauldronWarning(L"Could not find ExecutionCallback insertionname %ls", insertionName.c_str());
    }

    template <typename Comp>
    void Framework::RegisterComponentManager()
    {
        Comp* pComponentManager = new Comp();

        // Make sure it wasn't already added
        CauldronAssert(ASSERT_ERROR, std::this_thread::get_id() != GetFramework()->MainThreadID() || !GetFramework()->IsRunning(), L"Performance Warning: Using std::map::find on the main thread while app is running.");
        std::map<std::wstring, ComponentMgr*>::iterator iter = m_ComponentManagers.find(pComponentManager->ComponentType());
        CauldronAssert(ASSERT_ERROR, iter == m_ComponentManagers.end(), L"Component manager %ls is being registered multiple times. Ignoring duplicate registration", pComponentManager->ComponentType());
        if (iter != m_ComponentManagers.end())
            return;

        // Add it to the list and initialize it (if needed)
        m_ComponentManagers.emplace(std::make_pair(pComponentManager->ComponentType(), pComponentManager));
        pComponentManager->Initialize();
    }

    void Framework::UnRegisterComponentsAndRenderModules()
    {
        for (auto rmIter = m_RenderModules.begin(); rmIter != m_RenderModules.end(); ++rmIter)
        {
            // Delete the memory
            delete *rmIter;
        }
        m_RenderModules.clear();

        for (auto compIter = m_ComponentManagers.begin(); compIter != m_ComponentManagers.end(); ++compIter)
        {
            // Call it's shutdown first
            compIter->second->Shutdown();
            // Delete the memory
            delete compIter->second;
        }
        m_ComponentManagers.clear();
    }

    void Framework::AddResizableResourceDependence(ResourceResizedListener* pListener)
    {
        std::lock_guard<std::mutex> lock(m_ResourceResizeMutex);
        CauldronAssert(cauldron::ASSERT_CRITICAL, pListener != nullptr, L"Cannot add null resource resized listener.");
        if (m_ResourceResizedListeners.find(pListener) == m_ResourceResizedListeners.end())
            m_ResourceResizedListeners.insert(pListener);
    }

    void Framework::RemoveResizableResourceDependence(ResourceResizedListener* pListener)
    {
        std::lock_guard<std::mutex> lock(m_ResourceResizeMutex);
        CauldronAssert(cauldron::ASSERT_CRITICAL, pListener != nullptr, L"Cannot remove null resource resized listener.");
        auto it = m_ResourceResizedListeners.find(pListener);
        if (it != m_ResourceResizedListeners.end())
            m_ResourceResizedListeners.erase(it);
    }

    int Framework::CreateRenderResources()
    {
        std::map<std::wstring, RenderResourceInformation>::iterator iter = m_Config.RenderResources.begin();
        while (iter != m_Config.RenderResources.end())
        {
            // Get resource creation information
            RenderResourceInformation info = iter->second;

            // Create the render target and validate it was created
            TextureDesc desc;

            if (IsDepth(info.Format))
                desc.Flags = ResourceFlags::AllowDepthStencil;
            else
                desc.Flags = ResourceFlags::AllowRenderTarget;

            if (info.AllowUAV)
                desc.Flags = static_cast<ResourceFlags>(desc.Flags | ResourceFlags::AllowUnorderedAccess);

            desc.Format = info.Format;
            desc.Width = m_ResolutionInfo.DisplayWidth;
            desc.Height = m_ResolutionInfo.DisplayHeight;
            desc.Dimension = TextureDimension::Texture2D;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Name = iter->first;

            if (info.RenderResolution)
            {
                desc.Width =  m_ResolutionInfo.RenderWidth;
                desc.Height = m_ResolutionInfo.RenderHeight;

                const Texture* pRenderTarget = GetDynamicResourcePool()->CreateRenderTexture(
                    &desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
                        desc.Width  = renderingWidth;
                        desc.Height = renderingHeight;
                    });

                CauldronAssert(ASSERT_ERROR, pRenderTarget, L"Could not create render target %ls", iter->first);
                if (!pRenderTarget)
                    return -1;
            }
            else
            {
                // Always use full display width/height for resizing of auto-resources. We can control what viewport to use with the frame work
                const Texture* pRenderTarget = GetDynamicResourcePool()->CreateRenderTexture(
                    &desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
                        desc.Width  = displayWidth;
                        desc.Height = displayHeight;
                    });

                CauldronAssert(ASSERT_ERROR, pRenderTarget, L"Could not create render target %ls", iter->first);
                if (!pRenderTarget)
                    return -1;
            }


            ++iter;
        }

        return 0;
    }

    const Texture* Framework::GetRenderTexture(const wchar_t* name) const
    {
        name = m_Config.GetAliasedResourceName(name);
        return m_pDynamicResourcePool->GetTexture(name);
    }

    RenderModule* Framework::GetRenderModule(uint32_t order)
    {
        auto iter = m_RenderModules.begin() + order;
        return (iter == m_RenderModules.end()) ? nullptr : *iter;
    }

    RenderModule* Framework::GetRenderModule(const char* name)
    {
        for (auto pRM : m_RenderModules)
        {
            if (StringToWString(name) == pRM->GetName())
                return pRM;
        }
        CauldronCritical(L"Could not find render module %ls", StringToWString(name).c_str());
        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    // Global accessors

    // Framework instance accessor
    Framework* GetFramework()
    {
        return g_pFrameworkInstance;
    }

    // Global configuration accessor
    const CauldronConfig* GetConfig()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetConfig();
    }

    // Global task manager accessor
    TaskManager* GetTaskManager()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetTaskManager();
    }

    // Global content manager accessor
    ContentManager* GetContentManager()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetContentManager();
    }

    // Global profiler accessor
    Profiler* GetProfiler()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetProfiler();
    }

    // Global device accessor
    Device* GetDevice()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetDevice();
    }

    // Global resource view allocator accessor
    ResourceViewAllocator* GetResourceViewAllocator()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetResourceViewAllocator();
    }

    // Global raster view allocator accessor
    RasterViewAllocator* GetRasterViewAllocator()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetRasterViewAllocator();
    }

    // Global swap chain accessor
    SwapChain* GetSwapChain()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetSwapChain();
    }

    // Global upload heap accessor
    UploadHeap* GetUploadHeap()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetUploadHeap();
    }

    // Global constant buffer pool accessor
    DynamicBufferPool* GetDynamicBufferPool()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetDynamicBufferPool();
    }

    // Global resizable resource pool accessor
    DynamicResourcePool* GetDynamicResourcePool()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetDynamicResourcePool();
    }

    Scene* GetScene()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetScene();
    }

    InputManager* GetInputManager()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetInputMgr();
    }

    UIManager* GetUIManager()
    {
        CauldronAssert(ASSERT_CRITICAL, g_pFrameworkInstance, L"No framework instance to query. Application will crash.");
        return g_pFrameworkInstance->GetUIManager();
    }

    // Entry point to start everything
    int RunFramework(Framework* pFramework)
    {
        CauldronAssert(ASSERT_ERROR, g_pFrameworkInstance, L"Framework pointer can't be null");
        CauldronAssert(ASSERT_ERROR, g_pFrameworkInstance == pFramework, L"Framework pointer and Framework instance don't match");
        if (!pFramework)
            return 1;

        // Process configuration settings and command line options
        Log::Write(LOGLEVEL_TRACE, L"Initializing configuration");
        pFramework->InitConfig();

        // Initialize the framework (and quit now if something goes wrong)
        Log::Write(LOGLEVEL_TRACE, L"Initializing framework components.");
        if (pFramework->Init() < 0)
            return -1;

        // Do any pre-run setup that needs to happen
        // i.e. things we don't want to do before DoSampleInit because it might take long to happen
        if (pFramework->PreRun() < 0)
            return -1;

        // Run the framework (won't return until we are done
        int32_t result = pFramework->Run();

        // Do any cleanup prior to shut down
        pFramework->PostRun();

        // Shut everything down before deleting and returning
        pFramework->Shutdown();

        // Return the end result back to the sample in case it's needed
        return result;
    }

} // namespace cauldron
