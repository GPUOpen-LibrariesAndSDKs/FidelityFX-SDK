#include "mlsr_optimized_includes.hlsli"

struct DebugViewport
{
    uint2 offset;
    uint2 size;
};

// Macro to cull and draw debug viewport
#define DRAW_VIEWPORT(function, pos, vp)    \
    {                                       \
        if (pointIsInsideViewport(pos, vp)) \
        {                                   \
            function(pos, vp);              \
        }                                   \
    }

float2 getTransformedUv(uint2 pxPos, DebugViewport vp)
{
    return (float2(pxPos - vp.offset) + 0.5f) / vp.size;
}

float2 ClampUv(float2 uv, uint2 textureSize, uint2 resourceSize)
{
    const float2 sampleLocation = uv * textureSize;
    const float2 clampedLocation = max(float2(0.5f, 0.5f), min(sampleLocation, float2(textureSize) - float2(0.5f, 0.5f)));
    const float2 clampedUv = clampedLocation / float2(resourceSize);

    return clampedUv;
}

void WriteOutput(uint2 pxPos, float3 color)
{
    if (rcas_enabled)
        rw_rcas_output[pxPos] = color;
    else
        rw_mlsr_output_color[pxPos] = color;
}

float3 GetCurrentColor(float2 uv)
{
    return r_result_color.SampleLevel(g_history_sampler, uv, 0);
}

float3 HSVtoRGB(in float3 HSV)
{
    float R = abs(HSV.x * 6 - 3) - 1;
    float G = 2 - abs(HSV.x * 6 - 2);
    float B = 2 - abs(HSV.x * 6 - 4);
    return ((saturate(float3(R, G, B)) - 1) * HSV.y + 1) * HSV.z;
}

void drawVelocity(uint2 pxPos, DebugViewport vp)
{
    const float2 displaySize = float2(width, height);
    const float2 renderSize = float2(width_lr, height_lr);

    float2 uv = getTransformedUv(pxPos, vp);
    uv = ClampUv(uv, (uint2)renderSize, max_renderSize);
    float2 mv = SampleInputVelocity(uv) * renderSize;

    // Motion Vector visualization taken from https://github.com/Radeon-Pro/spoofing_wrapper/blob/main/hlsl/debug_viewer_ps.hlsl
    static const float mv_hsv_saturation_distance = 16.0f;
    float r = tanh(length(mv) / mv_hsv_saturation_distance);

    const float PI = 3.14159265359f;
	float theta = smoothstep(-PI, PI, atan2(mv.y, mv.x));
	float3 HSV = float3(theta, 1.0f, r);
	float3 mvColor = HSVtoRGB(HSV);

    const bool drawLines = true;
    if (drawLines)
    {
		// Compute the current and center block coordinates of the destination image.
		int2 blockIndex = pxPos / mv_hsv_saturation_distance;
		float2 dstCenterCoords = blockIndex * mv_hsv_saturation_distance + (mv_hsv_saturation_distance / 2.0f);
        float2 uv = getTransformedUv(dstCenterCoords, vp);
        uv = ClampUv(uv, (uint2)renderSize, max_renderSize);

		// Sample the motion vector using the source center UV coordinates.
		// use UV coordinates to ensure the sample works when rendering over HR image.
        float2 mv = SampleInputVelocity(uv) * displaySize;

		// Highlight lines of motion.
		if (abs(distance(dstCenterCoords, dstCenterCoords + mv) - (distance(dstCenterCoords, pxPos) + distance(dstCenterCoords + mv, pxPos))) < 0.025f)
			mvColor = float3(1.0f, 1.0f, 1.0f);

		// Color the source of the movement in red (to reduce ambiguity WRT which end is which).
		if (all(uint2(dstCenterCoords) == pxPos))
			mvColor = float3(1.0f, 0.0f, 0.0f);
    }

    WriteOutput(pxPos, mvColor);
}

void drawPredictedBlendFactor(uint2 pxPos, DebugViewport vp)
{
    float2 uv = getTransformedUv(pxPos, vp);

    float blendingFactor = DebugSamplePredictedBlendFactor(uv);
    float3 currentColor = GetCurrentColor(uv);

    float grayscale = dot(currentColor, float3(0.2126f, 0.7152f, 0.0722f)); // assuming Bt.709 primaries
    float3 predictedBlendColor = lerp((float3)grayscale, float3(0.9f, 0.0f, 0.9f), blendingFactor);

    WriteOutput(pxPos, predictedBlendColor);
}

bool pointIsInsideViewport(uint2 pxPos, DebugViewport vp)
{
    uint2 extent = vp.offset + vp.size;

    return (pxPos.x >= vp.offset.x && pxPos.x < extent.x) && (pxPos.y >= vp.offset.y && pxPos.y < extent.y);
}

static const uint ViewportGridSizeX = 3;
static const uint ViewportGridSizeY = 3;
static const float2 ViewportScale = float2(1.0f / ViewportGridSizeX, 1.0f / ViewportGridSizeY);

[numthreads(8,8,1)]
void main(uint2 pxPos : SV_DispatchThreadID)
{
    const uint2 displaySize = uint2(width, height);
    const uint2 viewportSize  = uint2(displaySize * ViewportScale);

    DebugViewport vp[ViewportGridSizeY][ViewportGridSizeX];
    for (uint y = 0; y < ViewportGridSizeY; ++y)
    {
        for (uint x = 0; x < ViewportGridSizeX; ++x)
        {
            vp[y][x].offset = viewportSize * uint2(x, y);
            vp[y][x].size   = viewportSize;
        }
    }

    // top row
    DRAW_VIEWPORT(drawVelocity, pxPos, vp[0][0]);
    //DRAW_VIEWPORT(unusedVP, pxPos, vp[0][1]);
    DRAW_VIEWPORT(drawPredictedBlendFactor, pxPos, vp[0][2]);

    // bottom row
    //DRAW_VIEWPORT(unusedVp, pxPos, vp[2][0]);
    //DRAW_VIEWPORT(unusedVp, pxPos, vp[2][1]);
    //DRAW_VIEWPORT(unusedVp, pxPos, vp[2][2]);
}
