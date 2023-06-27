RWStructuredBuffer<float> DataBuffer : register(u0);

cbuffer cb : register(b0) 
{
    uint NumLoops;
}

[numthreads(32,1,1)]
void CSMain(uint dtID : SV_DispatchThreadID)
{
    float tmp = DataBuffer[dtID];
    for (uint i = 0; i < NumLoops; i++)
    {
        tmp = sin(tmp) + 1.5f;
    }
    DataBuffer[dtID] = tmp;
}
