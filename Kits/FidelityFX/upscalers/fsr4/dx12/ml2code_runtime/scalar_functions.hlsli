// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.

template<typename T>
T Relu(const T v)
{
    return max(v, 0);
}

template<typename T>
T Sigmoid(const T v)
{
    return 1 / (1 + exp(-v));
}

int4 ReluClampS8(const int4 v)
{
    return clamp(v, 0, 127);
}

template<typename T>
T LeakyRelu(const float alpha, const T v)
{
    return v >= 0 ? v : alpha * v;
}

template<typename T>
T SqrSwish(const T v)
{
    return 0.5 * v * (1. + v / sqrt(v * v + 1.));
}