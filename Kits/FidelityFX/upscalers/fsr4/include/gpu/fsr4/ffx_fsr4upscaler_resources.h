// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
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

#ifndef FFX_FSR3UPSCALER_RESOURCES_H
#define FFX_FSR3UPSCALER_RESOURCES_H

#if defined(FFX_CPU) || defined(FFX_GPU)
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_NULL                                           0
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_INPUT_COLOR                                    1
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS                           2
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_INPUT_DEPTH                                    3
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE                                 4
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_AUTO_EXPOSURE                                  5
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT                               6
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_SPD_MIP5                                       7
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_LUMA_0                                         8
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_LUMA_1                                         9
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_MLSR_OUTPUT                                    10
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_RCAS_OUTPUT                                    11
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_FINAL_OUTPUT                                   12
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_RECURRENT                                      13
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_HISTORY                                        14
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_HISTORY_REPROJECTED                            15
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_RCAS_TEMP                                      16
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_DEFAULT_EXPOSURE                      17

#define FFX_MLSR_BIND_SRV_BUFFER_NHWC_INPUTS                                                18
#define FFX_MLSR_BIND_SRV_BUFFER_FUSED_QUANTIZED_NHWC_OUTPUT                                19
#define FFX_MLSR_BIND_SRV_INITIALIZER_BUFFER                                                20
#define FFX_MLSR_BIND_SRV_INITIALIZER_STAGING_BUFFER                                        21
#define FFX_MLSR_BIND_SRV_SCRATCH_BUFFER                                                    22

#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_DEBUG_INFORMATION                              23
#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_RESULT_COLOR                                   24

#define FFX_FSR4UPSCALER_RESOURCE_IDENTIFIER_COUNT                                          25


#define FFX_FSR4UPSCALER_CONSTANTBUFFER_IDENTIFIER_FSR4UPSCALER                             0
#define FFX_FSR4UPSCALER_CONSTANTBUFFER_IDENTIFIER_AUTOEXPOSURE                             1
#define FFX_FSR4UPSCALER_CONSTANTBUFFER_IDENTIFIER_SPD_AUTOEXPOSURE                         2
#define FFX_FSR4UPSCALER_CONSTANTBUFFER_IDENTIFIER_RCAS                                     3
#define FFX_FSR4UPSCALER_CONSTANTBUFFER_IDENTIFIER_PASS_WEIGHTS                             4

#define FFX_FSR4UPSCALER_CONSTANTBUFFER_COUNT                                               5






#endif // #if defined(FFX_CPU) || defined(FFX_GPU)

#endif //!defined( FFX_FSR3UPSCALER_RESOURCES_H )
