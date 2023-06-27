//--------------------------------------------------------------------------------------
// PLATFORM / PORTABILITY SETUP
//--------------------------------------------------------------------------------------
#define THREADGROUP_DIMENSION_X 8
#define THREADGROUP_DIMENSION_Y 8

//-----------------------------------------------------------------------------------------
// SHADER RESOURCES
//-----------------------------------------------------------------------------------------
cbuffer Parameters : register(b0, space0)
{
    uint2 ImageSize;
    float DiffFactor;
}

Texture2D           imgSrc_Color0          : register(t0, space0);
Texture2D           imgSrc_Color1          : register(t1, space0);
RWTexture2D<float4> imgDst_Output          : register(u0, space0);


//-----------------------------------------------------------------------------------------
// ENTRY POINT
//-----------------------------------------------------------------------------------------
[numthreads(THREADGROUP_DIMENSION_X, THREADGROUP_DIMENSION_Y, 1)]
void MainCS(
	  uint3 GroupThreadID    : SV_GroupThreadID
	, uint3 WorkGroupId      : SV_GroupID
	, uint3 DispatchThreadID : SV_DispatchThreadID
)
{
    int2 pxCoord = DispatchThreadID.xy;

    // early out for out-of-image-bounds
    if (pxCoord.x >= ImageSize.x || pxCoord.y >= ImageSize.y)
        return;
    
    float3 rgb1 = imgSrc_Color0[pxCoord].rgb;
    float3 rgb2 = imgSrc_Color1[pxCoord].rgb;
    float red = DiffFactor * ((rgb1.r + rgb1.g + rgb1.b) - (rgb2.r + rgb2.g + rgb2.b));

	// Store out
    imgDst_Output[pxCoord] = float4(red, 0, 0, 1.0);
}
