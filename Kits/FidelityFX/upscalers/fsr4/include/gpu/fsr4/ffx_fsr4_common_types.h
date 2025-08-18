
enum InputUpscaleMethod : int32_t {
    InputUpscaleMethod_Default,
    InputUpscaleMethod_Bilinear,
    InputUpscaleMethod_Smoothstep,
    InputUpscaleMethod_Gaussian,
    InputUpscaleMethod_Gaussian16tap,
    InputUpscaleMethod_Nearest,
    InputUpscaleMethod_Count,
};

enum VelocitySampleMethod : int32_t {
    VelocitySampleMethod_Default,
    VelocitySampleMethod_ClosestDepth,
    VelocitySampleMethod_ClosestDepthDisocclusion,
    VelocitySampleMethod_Count,
};

enum DisocclusionMethod : int32_t {
    DisocclusionMethod_Default,
    DisocclusionMethod_None,
    DisocclusionMethod_SIE,
    DisocclusionMethod_Count,
};

enum VelocityAttenuateMethod : int32_t {
    VelocityAttenuateMethod_Default,
    VelocityAttenuateMethod_None,
    VelocityAttenuateMethod_FlushToZero,
    VelocityAttenuateMethod_SIE_MvShake,
    VelocityAttenuateMethod_Count,
};

enum Colorspace : int32_t {
    Colorspace_Default,
    Colorspace_Linear,
    Colorspace_PQ,
    Colorspace_sRGB,
    Colorspace_Reinhard,
    Colorspace_ReinhardSq,
    Colorspace_ACES,
    Colorspace_OKlab,
    Colorspace_MuLaw,
};

enum DilationMethod : int32_t {
    DilationMethod_Default,
    DilationMethod_BeforeUpscale,
    DilationMethod_AfterUpscale,
};

#define    RfmActivation_None 7
#define    RfmActivation_Sigmoid 8
#define    RfmActivation_Relu 9

struct DilationResult {
    float center_depth;
    float chosen_depth;
    uint2  chosen_depth_pos;
    uint2  chosen_mv_pos;
    bool is_no_edge;
};
