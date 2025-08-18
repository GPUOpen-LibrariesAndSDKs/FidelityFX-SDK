static const uint char_map[80] = {
// numbers
  9544062
, 8291721
, 8553472
, 8421631
, 10600834
, 8817041
, 9011522
, 7768457
, 1316880
, 1113874
, 9014092
, 7440777
, 9539966
, 6459793
, 1122753
, 197897
, 9539950
, 7246225
, 9539854
, 7246225
// letters
, 1118718
, 16650513
, 9013759
, 7768457
, 8471100
, 4358529
, 8487423
, 3949185
, 9540095
, 9539985
, 592383
, 592137
, 8471100
, 3297681
, 1052927
, 16715792
, 16711680
, 0
, 8404992
, 32640
, 528639
, 8479252
, 8421631
, 8421504
, 262911
, 65282
, 787199
, 16728112
, 8471100
, 3949185
, 1118719
, 3601
, 9519676
, 12337825
, 1118719
, 12986641
, 9537862
, 4366737
, 16711937
, 257
, 8421503
, 8355968
, 3148803
, 473280
, 2113791
, 16728112
, 1599105
, 8480280
, 525825
, 67320
, 9544129
, 8619401
// special chars
, 0
, 64
, 526336
, 526344
, 11053304
, 168
, 0
, 34
};

cbuffer WatermarkCB : register(b0)
{
    uint2 Offset;
    uint FontSize;
    uint _padding;
    uint4 PackedMessage[30];
};

RWTexture2D<float3> rw_target : register(u0);

static const uint2 FontSizeInPixels = uint2(6, 8);
static const uint Kerning = 1;

[numthreads(8, 8, 1)]
void main(uint2 dtid : SV_DispatchThreadID)
{
    uint2 threadPixelPos = Offset + dtid;
    
    uint2 cursor = dtid / ((FontSizeInPixels + Kerning) * FontSize);
    if (cursor.y >= 4 || cursor.x >= 120)
        return;
    
    uint4 PackedWords = PackedMessage[(cursor.y * 30 + cursor.x / 4) / 4];
    uint character4 = PackedWords[(cursor.y * 30 + cursor.x / 4) % 4];
    uint character = (character4 >> (cursor.x % 4 * 8)) & 0xFF;
    if (character == 0 || character > 40) // space or invalid
        return;

    uint2 pixelInChar = dtid % ((FontSizeInPixels + Kerning) * FontSize);
    uint2 pixelInGlyph = pixelInChar / FontSize;

    if (any(pixelInGlyph >= FontSizeInPixels))
        return;

    uint glyphIndex = (character - 1) * 2;
    if (pixelInGlyph.x >= 3)
    {
        // each word stores 3 columns, if we are above column 3, need to get next word in the bitmap table
        glyphIndex++;
        pixelInGlyph.x -= 3;
    }

    uint bitmap = char_map[glyphIndex];
    uint bit = 1 << (pixelInGlyph.x * FontSizeInPixels.y + pixelInGlyph.y);
    if ((bitmap & bit) > 0)
    {
        rw_target[threadPixelPos] = float3(1.0, 1.0, 1.0);
    }
}
