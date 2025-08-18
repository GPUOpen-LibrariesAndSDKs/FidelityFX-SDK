//Exposure logic ref, page 85: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
//Average log-luminance ref: https://www.sciencedirect.com/topics/computer-science/average-luminance
float computeEV100FromAvgLuminance(float avgLuminance)
{
	// We later use the middle gray at 12.7% in order to have
	// a middle gray at 18% with a sqrt (2) room for specular highlights
	// But here we deal with the spot meter measuring the middle gray
	// which is fixed at 12.5 for matching standard camera
	// constructor settings (i.e. calibration constant K = 12.5)
	// Reference : http :// en. wikipedia . org / wiki / Film_speed
	return log2(avgLuminance * 100.0f / 12.5f);
}

float convertEV100ToExposure(float EV100)
{
	// Compute the maximum luminance possible with H_sbs sensitivity
	// maxLum = 78 / ( S * q ) * N^2 / t
	// = 78 / ( S * q ) * 2^ EV_100
	// = 78 / (100 * 0.65) * 2^ EV_100
	// = 1.2 * 2^ EV
	// Reference : http :// en. wikipedia . org / wiki / Film_speed
	float maxLuminance = 1.2f * pow(2.0f, EV100);
	return 1.0f / maxLuminance;
}

float GetExposure(float avgLuminance)
{
	// ??? FIXME: Tonemap should be prepareinputcolor, dealing with tonemap and/or exposure
	float Lavg = exp(avgLuminance);
	//float Lavg = avgLuminance;

	// usage with auto settings
	float currentEV = computeEV100FromAvgLuminance(Lavg);
	float exposure = convertEV100ToExposure(currentEV);

	return exposure;
}

Texture2D<float3>                   r_input_color             : register(t0);

globallycoherent RWTexture2D<uint>  rw_spd_global_atomic      : register(u0);
globallycoherent RWTexture2D<float> rw_autoexp_mip_5          : register(u1);
RWTexture2D<float>                  rw_auto_exposure_texture  : register(u2);



#include "ffx_core.h"
#include "ffx_common_types.h"

cbuffer AutoExposureSPDConstants : register(b0)
{
    FfxUInt32       mips;
    FfxUInt32       numWorkGroups;
    FfxUInt32x2     workGroupOffset;
    FfxFloat32x2    invInputSize;       // Only used for linear sampling mode
    FfxFloat32      preExposure;
    FfxFloat32      padding;

    #define FFX_SPD_CONSTANT_BUFFER_1_SIZE 8  // Number of 32-bit values. This must be kept in sync with the cbSPD size.
};

FfxUInt32 MipCount()
{
    return mips;
}

FfxUInt32 NumWorkGroups()
{
    return numWorkGroups;
}

FfxUInt32x2  WorkGroupOffset()
{
    return workGroupOffset;
}

FfxFloat32x2 InvInputSize()
{
    return invInputSize;
}



FFX_GROUPSHARED FfxUInt32 spdCounter;

void SpdIncreaseAtomicCounter(FfxUInt32 slice)
{
    InterlockedAdd(rw_spd_global_atomic[FfxInt32x2(0, 0)], 1, spdCounter);
}

FfxUInt32 SpdGetAtomicCounter()
{
    return spdCounter;
}

void SpdResetAtomicCounter(FfxUInt32 slice)
{
    rw_spd_global_atomic[FfxInt32x2(0, 0)] = 0;
}

FFX_GROUPSHARED FfxFloat32 spdIntermediateR[16][16];
FFX_GROUPSHARED FfxFloat32 spdIntermediateG[16][16];
FFX_GROUPSHARED FfxFloat32 spdIntermediateB[16][16];
FFX_GROUPSHARED FfxFloat32 spdIntermediateA[16][16];

FfxFloat32x4 SpdLoadSourceImage(FfxFloat32x2 iPxPos, FfxUInt32 slice)
{
    //We assume linear data. if non-linear input (sRGB, ...),
    //then we should convert to linear first and back to sRGB on output.
    const float3 rgb2y = float3(0.2126, 0.7152, 0.0722);
    const float3 color = r_input_color[iPxPos].rgb / preExposure;
	float Y = dot(rgb2y, color);
	const float epsilon = 1e-06f;
	Y = log(max(epsilon, Y));

    return float4(Y,0,0,0);
}

FfxFloat32x4 SpdLoad(FfxInt32x2 tex, FfxUInt32 slice)
{
    return FfxFloat32x4(rw_autoexp_mip_5[tex], 0, 0, 0);
}

FfxFloat32x4 SpdReduce4(FfxFloat32x4 v0, FfxFloat32x4 v1, FfxFloat32x4 v2, FfxFloat32x4 v3)
{
    return (v0 + v1 + v2 + v3) * 0.25f;
}

void SpdStore(FfxInt32x2 pix, FfxFloat32x4 outValue, FfxUInt32 index, FfxUInt32 slice)
{
    if (index == 5) {
        rw_autoexp_mip_5[pix] = outValue.x;
    }
    else if (index == MipCount() - 1) { //accumulate on 1x1 level

        if (all(FFX_EQUAL(pix, FfxInt32x2(0, 0))))
        {
            float prev_exposre = rw_auto_exposure_texture[int2(0,0)];

			rw_auto_exposure_texture[int2(0,0)] = GetExposure(outValue.x);
            rw_auto_exposure_texture[int2(1,0)] = prev_exposre;

        }
    }
}

FfxFloat32x4 SpdLoadIntermediate(FfxUInt32 x, FfxUInt32 y)
{
    return FfxFloat32x4(
        spdIntermediateR[x][y],
        spdIntermediateG[x][y],
        spdIntermediateB[x][y],
        spdIntermediateA[x][y]);
}
void SpdStoreIntermediate(FfxUInt32 x, FfxUInt32 y, FfxFloat32x4 value)
{
    spdIntermediateR[x][y] = value.x;
    spdIntermediateG[x][y] = value.y;
    spdIntermediateB[x][y] = value.z;
    spdIntermediateA[x][y] = value.w;
}

#include "spd/ffx_spd.h"


[numthreads(256, 1, 1)]
void main(uint3 WorkGroupId : SV_GroupID, uint LocalThreadIndex : SV_GroupIndex)
{
SpdDownsample(
	FfxUInt32x2(WorkGroupId.xy),
	FfxUInt32(LocalThreadIndex),
	FfxUInt32(MipCount()),
	FfxUInt32(NumWorkGroups()),
	FfxUInt32(WorkGroupId.z),
	FfxUInt32x2(WorkGroupOffset()));


}
