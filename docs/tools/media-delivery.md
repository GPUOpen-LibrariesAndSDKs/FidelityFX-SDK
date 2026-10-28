<!-- @page page_tools_media_delivery AMD FSR™ Media Delivery -->

<h1>AMD FSR™ SDK Media Delivery System</h1>

The AMD FSR™ SDK Media Delivery System is a tool distributed as part of the AMD FSR™ SDK package and is used to download, verify and extract the media bundles used to run the SDK samples.

<h2>Using the Media Delivery System</h2>

The current version of the Media Delivery System is kept in the /Tools/media_delivery folder.

Use it from the `UpdateMedia.bat` or `ClearMediaCache.bat` files in the SDK root folder. Those batch files will get a known media bundle or clear the download cache respectively.

The following arguments and options will allow you to use the system outside of those two ways:

<h3>Command line syntax:</h3>

"MediaDelivery.exe \[Options\]"

**Options**

| Option             | Description                                                                                                  |
|--------------------|--------------------------------------------------------------------------------------------------------------|
| **-help**          | Shows the complete usage of the tool.                                                                         |
| **-clear-cache**   | Deletes all cached media bundles.                                                                             |
| **-debug**         | Enables debug mode (only available in internal builds).                                                      |
| **-download-only** | Downloads a media bundle without extracting it.                                                               |
| **-force**         | Skips confirmation when the media directory will be overwritten and always extracts the bundle.                |
| **-host**          | Specifies the hostname to connect to (only available in internal builds, defaults to `ffx-sdk-assets.amdgameeng.com`). |
| **-root-dir**      | Sets the root directory of the AMD FSR™ SDK (defaults to the current working directory).                    |
| **-target-sha256** | Sets the SHA256 hash of the media bundle you want to download.                                                    |
| **-unstable**      | Downloads the most recent unstable media bundle.                                                             |
| **-media-sub-dir** | Specifies the folder in which to place the downloaded media content. |
