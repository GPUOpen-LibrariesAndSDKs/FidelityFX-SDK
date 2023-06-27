#pragma once

#ifdef __cplusplus
    #include <stdint.h>
    #include "misc/math.h"
#endif

#ifdef __cplusplus
struct ApplyReflectionsConstants
{
    Vec4 viewDirection;
    uint32_t showReflectionTarget;
    uint32_t drawReflections;
    float    reflectionsIntensity;
};
#else
cbuffer Constants : register(b0) {
    float4 viewDirection;
    uint showReflectionTarget;
    uint drawReflections;
    float reflectionsIntensity;
};
#endif
