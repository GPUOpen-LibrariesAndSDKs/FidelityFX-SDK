# DX12 Agility SDK

## Current Version
1.608.2

## How to update
1. Download the latest version (as a .nupkg) from https://devblogs.microsoft.com/directx/directx12agility/
1. Rename extension (.nupkg) to .zip
1. Unzip it
1. Copy contents of /build/native/bin/x64 to bin/x64 in this folder
1. Copy /build/native/include folder to this folder
1. Update D3D12SDKVersion extern defined at the top of hlsl_compiler.cpp with the subversion number (i.e. 1.608.2 -> 608)
1. Force a rebuild of the entire solution to ensure proper linking occurs.
1. Update this `FFX_SDK_README.md` with the new version number!
