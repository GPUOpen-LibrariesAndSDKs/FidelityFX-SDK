<!-- @page page_getting-started_sdk-structure FidelityFX SDK Structure -->

<h1>SDK Structure</h1>

The AMD FidelityFX SDK is comprised of several main components, organised to make sure you have a good experience exploring and learning about the different techniques implemented by the SDK. 

The core components which would ultimately ship in your application are separated for convenient integration in your project's source code.

<h2>FidelityFX SDK</h2>

<h3>SDK</h3>

**Includes** 

The SDK contains everything needed to ship all of our FidelityFX effects in your application:

- Host side includes and source for all of our effect components, which represent the pre-built easy to use library for easier effect integration.
- A collection of HLSL and GLSL shaders and associated assets, which would ultimately ship in your application.

**Components**

A collection of pre-built runtime components to automate the usage of the FidelityFX SDK and make integration as simple as possible.

**Tools**

The FidelityFX Shader Compiler tool which enables the pre-generation of all shader variants needed for each FidelityFX effect, plus the media downloader tool.

<h3>Docs</h3>

A collection of detailed markdown and doxygen-generated documents.

<h3>Samples</h3>

The FidelityFX SDK comes complete with numerous examples which demonstrate use of the effects implemented by the SDK. 

The SDK features a collection of sample applications in the effects sub-folder which are dependent on the **SDK** effect components. Each sample application demonstrates how a technique (or techniques) should be integrated into your application. The samples present lots of options to allow you to evaluate and explore the technique before undertaking an integration.

<h4>Dependencies</h4>

The SDK samples are dependent upon our backing graphics framework Cauldron, various common render modules, and a custom SDK backend implementation that wraps our graphics framework. 

The **Samples** location is also where the custom Cauldron SDK backend (**ffx_cauldron**) can be found as well as our updated graphics framework, **Cauldron**, and a number of render modules for commonly used rendering features (**rendermodules**).

The architecture of the AMD FidelityFX SDK implies a set of dependencies between the components. The image below shows which components depend on which other components, and which you might ultimately depend upon in your application.

![alt text](media/component-dependencies.png "A diagram of AMD FidelityFX component dependencies.")

<h3>Media</h3>

Media delivery for the AMD FidelityFX SDK is handled via remote download into the SDK folder.

Simply run the ```UpdateMedia.bat``` file in the root directory to fetch the latest version of the FidelityFX SDK content.

When fetching content for updated code packages from github, please first run ```ClearMediaCache.bat``` to ensure fetching of the latest content.