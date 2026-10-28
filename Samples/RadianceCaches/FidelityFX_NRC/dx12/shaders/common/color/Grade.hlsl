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

#ifndef SOURCE_COLOR_GRADE
#define SOURCE_COLOR_GRADE

#include "Math.hlsl"

// Neural colour grading. Shout out to UnitZeroOne for sharing this code! :-) 
float3 ApplyRedGrade(float3 inputColor) 
{
    inputColor = saturate(inputColor.zyx);
 
 // Named constants up front
    const float ZERO  = 0.0;
    const float PAD   = 0.0;  // used for padding only
    const float HALF  = 0.5;
    const float ONE   = 1.0;
    const float TWO   = 2.0;
    const float GELU_C1 = 0.7978845608;
    const float GELU_C2 = 0.044715;

   const float4x4 layer0_chunk0_W0 = float4x4(0.237038, 0.002839, -0.613395, 0.023955, 2.190638, 0.681709, -2.263870, -0.567830, 0.501725, 1.600221, -0.383257, 0.890942, PAD, PAD, PAD, PAD);
   const float4 layer0_chunk0_bias = float4(-0.287044, 0.280783, 0.290978, 0.598706);

   const float4x4 layer0_chunk1_W0 = float4x4(-0.137439, -0.103585, -0.010283, -1.756933, -0.938134, 1.964871, -0.696499, 1.249109, 1.416610, -0.654387, -0.856150, 1.315522, PAD, PAD, PAD, PAD);
   const float4 layer0_chunk1_bias = float4(-0.027628, 0.506725, 0.573981, 0.743510);

   const float4x4 layer2_chunk0_W0 = float4x4(3.334981, 0.692745, -2.249792, -1.533133, 0.759040, -0.884428, 0.228476, 1.654801, -1.449445, 0.674627, 1.597890, 0.084069, 1.227800, 0.103707, 2.272768, -3.488576);
   const float4x4 layer2_chunk0_W1 = float4x4(-3.658784, 0.920236, 2.443871, 1.958547, 1.074470, -0.660618, 1.283018, 0.921886, 1.856311, -0.596886, -2.903828, 1.161765, 0.368526, -0.286363, -0.506707, -0.171955);
   const float4 layer2_chunk0_bias = float4(1.218964, 0.054669, 0.264244, 0.295188);

   const float4x4 layer2_chunk1_W0 = float4x4(-0.191983, -0.747375, 0.912338, -1.968686, 0.272265, -0.705990, 0.595160, 1.205564, -0.024412, -0.238474, -0.449195, 0.163486, -1.953892, 1.715119, -0.194429, -2.550383);
   const float4x4 layer2_chunk1_W1 = float4x4(1.502533, -2.311996, -1.078473, 1.977682, -0.300410, 1.483174, 0.281399, 1.433337, -2.760890, -0.152508, -0.236289, 0.070996, 0.006024, -0.143973, 0.195464, -0.261372);
   const float4 layer2_chunk1_bias = float4(1.241102, 0.342907, 0.901451, 0.191256);

   const float4x4 layer4_chunk0_W0 = float4x4(0.208272, -0.118866, 0.661599, PAD, -1.598513, -2.092330, -0.095411, PAD, 0.630788, -0.224832, 0.052688, PAD, -3.094599, -0.439651, -0.093030, PAD);
   const float4x4 layer4_chunk0_W1 = float4x4(3.054257, 0.375831, -0.121975, PAD, 0.069616, 0.252565, 0.532385, PAD, -0.114408, 0.896074, -0.793594, PAD, 3.216065, 0.745465, -0.031920, PAD);
   const float4 layer4_chunk0_bias = float4(0.230162, -0.046474, -1.497791, PAD);

    // Scale inputColor from [0,1] to [-1,1]
    float3 scaledColor = inputColor * TWO - ONE;

    float4 layer0_chunk0_out = mulT(layer0_chunk0_W0, float4(scaledColor, ZERO)) + layer0_chunk0_bias;

    float4 layer0_chunk1_out = mulT(layer0_chunk1_W0, float4(scaledColor, ZERO)) + layer0_chunk1_bias;

    layer0_chunk0_out = layer0_chunk0_out * (ONE + tanh(GELU_C1 * (layer0_chunk0_out + GELU_C2 * layer0_chunk0_out*layer0_chunk0_out*layer0_chunk0_out))) * HALF;
    layer0_chunk1_out = layer0_chunk1_out * (ONE + tanh(GELU_C1 * (layer0_chunk1_out + GELU_C2 * layer0_chunk1_out*layer0_chunk1_out*layer0_chunk1_out))) * HALF;

    float4 layer2_chunk0_out = mulT(layer2_chunk0_W0, layer0_chunk0_out) + mulT(layer2_chunk0_W1, layer0_chunk1_out) + layer2_chunk0_bias;

    float4 layer2_chunk1_out = mulT(layer2_chunk1_W0, layer0_chunk0_out) + mulT(layer2_chunk1_W1, layer0_chunk1_out) + layer2_chunk1_bias;

    layer2_chunk0_out = layer2_chunk0_out * (ONE + tanh(GELU_C1 * (layer2_chunk0_out + GELU_C2 * layer2_chunk0_out*layer2_chunk0_out*layer2_chunk0_out))) * HALF;
    layer2_chunk1_out = layer2_chunk1_out * (ONE + tanh(GELU_C1 * (layer2_chunk1_out + GELU_C2 * layer2_chunk1_out*layer2_chunk1_out*layer2_chunk1_out))) * HALF;

    float4 layer4_chunk0_out = mulT(layer4_chunk0_W0, layer2_chunk0_out) + mulT(layer4_chunk0_W1, layer2_chunk1_out) + layer4_chunk0_bias;

    layer4_chunk0_out = ONE / (ONE + exp(-layer4_chunk0_out));

    return saturate(layer4_chunk0_out.xyz);
}

#endif
