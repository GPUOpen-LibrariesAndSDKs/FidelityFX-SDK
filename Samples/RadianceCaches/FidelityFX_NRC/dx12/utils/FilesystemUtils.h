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

#include <string>
#include <vector>

namespace fsr
{
    // Ensures that the input path has a trailing slash at the end
    std::string SlashifyPath(const std::string& input);

    // Removes trailing slashes at the end of a path
    std::string DeslashifyPath(const std::string& input);

    // Check to see whether a file exists
    bool FileExists(const std::string& absolutePath);

    // Check to see whether a directory exists
    bool DirectoryExists(const std::string& absolutePath);

    // Create a directory at the specified path
    bool CreateDirectory(const std::string absolutePath);

    // Replace the existing file extention with a new one
    std::string ReplaceExtension(const std::string& absolutePath, const std::string& newExtension);

    // Get the extension for the path
    std::string GetExtension(const std::string& path);

    // Get the filename for the path
    std::string GetFilename(const std::string& path);

    // Get the file extension for the path
    std::string GetFileExtension(const std::string& absolutePath);

    // Get the file stem for the path
    std::string GetFileStem(const std::string& path);

    // Gets the parent directory for the path
    std::string GetParentDirectory(const std::string& path);

    // Replace with a filename with a new one
    bool ReplaceFilename(std::string& absolutePath, const std::string& newFilename);

    // Get the directory that the executable is situated in
    std::string GetModuleDirectory();

    // Load a text file into a string
    std::string ReadTextFile(const std::string& filePath);

    // Write a string to a text file 
    bool WriteTextFile(const std::string& filePath, const std::string& data);

    // Get a file handle to the specified path
    bool GetInputFileHandle(const std::string& filePath, std::ifstream& file, std::string* actualPath = nullptr);

    // Get a file handle to the specified path
    bool GetOutputFileHandle(const std::string& filePath, std::ofstream& file, std::string* actualPath = nullptr);

    // Returns true if the path is absolute i.e. it's unambiguous on the filesystem
    bool IsAbsolutePath(const std::string& path);

    // Concatenates the root path with the relative path to create an absolute path
    std::string MakeAbsolutePath(const std::string& parentPath, const std::string& relativePath);

    // Enumerates all files in the source directory with the extension (if any). Returns the number of paths
    int EnumerateDirectoryFiles(const std::string& sourceDirectory, const bool recurse, std::vector<std::string>& outputPaths, const std::string& extensionFilter = "");

    // Makes a path absolute e.g. "a/b/../c" -> "a/c"
    std::string MakeAbsolutePath(const std::string& path);
    
    // Converts a path to normal form
    std::string NormalizePath(const std::string& path);
}
