#ifndef SHADERWRITER_H
#define SHADERWRITER_H

#if defined(FFX_HLSL) && (__HLSL_VERSION >= 2021)
namespace ffx_shader_writer {
#endif //  defined(FFX_HLSL) && (__HLSL_VERSION >= 2021)

FFX_STATIC const FfxUInt32 char_map[80] = {
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

// constants
FFX_STATIC const FfxInt32 NumbersStart = 0;
FFX_STATIC const FfxInt32 LettersStart = 10;
FFX_STATIC const FfxInt32 SpecialCharsStart = 36;

FFX_STATIC const FfxInt32 SpecialCharDot = SpecialCharsStart;
FFX_STATIC const FfxInt32 SpecialCharDash = SpecialCharsStart + 1;
FFX_STATIC const FfxInt32 SpecialCharExponent = SpecialCharsStart + 2;
FFX_STATIC const FfxInt32 SpecialCharColon = SpecialCharsStart + 3;

FFX_STATIC const FfxInt32x2 fontSizeInPixels = FfxInt32x2(6,8);
FFX_STATIC const FfxInt32 uintsPerChar = 2;
FFX_STATIC const FfxInt32 columnsPerUint = 3;
FFX_STATIC const FfxInt32 kerning = 1;
FFX_STATIC const FfxInt32 defaultNumSigFigs = 4;
 
// globals 
FFX_STATIC FfxInt32 numCharInCurrentLine = 0;
FFX_STATIC FfxInt32 fontSize = 3;
FFX_STATIC FfxInt32x2 cursorPos = FfxInt32x2(0, 0);
FFX_STATIC FfxInt32x2 threadPos = FfxInt32x2(0, 0);
FFX_STATIC FfxFloat32x3 backgroundColor = FfxFloat32x3(0.0f, 0.0f, 0.0f);
FFX_STATIC FfxFloat32x3 fontColor = FfxFloat32x3(1.0f, 1.0f, 1.0f);
FFX_STATIC FfxBoolean transparentBackground = true;
FFX_STATIC FfxInt32 globalNumSigFigs = 4;

#ifdef USE_LETTERS_MACROS
#define A WriteOneChar(LettersStart + 0);
#define B WriteOneChar(LettersStart + 1);
#define C WriteOneChar(LettersStart + 2);
#define D WriteOneChar(LettersStart + 3);
#define E WriteOneChar(LettersStart + 4);
#define F WriteOneChar(LettersStart + 5);
#define G WriteOneChar(LettersStart + 6);
#define H WriteOneChar(LettersStart + 7);
#define I WriteOneChar(LettersStart + 8);
#define J WriteOneChar(LettersStart + 9);
#define K WriteOneChar(LettersStart + 10);
#define L WriteOneChar(LettersStart + 11);
#define M WriteOneChar(LettersStart + 12);
#define N WriteOneChar(LettersStart + 13);
#define O WriteOneChar(LettersStart + 14);
#define P WriteOneChar(LettersStart + 15);
#define Q WriteOneChar(LettersStart + 16);
#define R WriteOneChar(LettersStart + 17);
#define S WriteOneChar(LettersStart + 18);
#define T WriteOneChar(LettersStart + 19);
#define U WriteOneChar(LettersStart + 20);
#define V WriteOneChar(LettersStart + 21);
#define W WriteOneChar(LettersStart + 22);
#define X WriteOneChar(LettersStart + 23);
#define Y WriteOneChar(LettersStart + 24);
#define Z WriteOneChar(LettersStart + 25);
#define CL WriteOneChar(SpecialCharColon);
#define NL NewLine();
#define SP AdvanceCursor(1);
#endif // USE_LETTERS_MACROS

// declare this for output
void StoreWriterOutput(FfxInt32x2 pos, FfxFloat32x4 value);
FfxInt32x2 InitThreadPos();

void SetFontSize(FfxInt32 newSize)
{
    fontSize = newSize;
}

void SetCursorPos(FfxInt32x2 newPos)
{
    cursorPos = newPos;
    numCharInCurrentLine = 0;
}

void SetFontColor(FfxFloat32x3 newColor)
{
    fontColor = newColor;
}

void SetBackgroundColor(FfxFloat32x3 newColor)
{
    backgroundColor = newColor;
}

void SetTransparentBackgroundState(FfxBoolean newState)
{
    transparentBackground = newState;
}

void SetGlobalNumSigFigs(FfxInt32 newGlobalNumSigFigs) {
    globalNumSigFigs = newGlobalNumSigFigs;
}


void AdvanceCursor(FfxInt32 advanceBy) {
    cursorPos.x += advanceBy * ((fontSizeInPixels.x + kerning) * fontSize);
    numCharInCurrentLine += advanceBy;
}

void WriteOneChar(FfxInt32 valueToWrite) {
    threadPos = InitThreadPos();
    FfxInt32 value_to_print = valueToWrite;

    FfxInt32x2 pos_coord = threadPos - cursorPos;
    FfxUInt32 top = (pos_coord.x / fontSize % fontSizeInPixels.x) / columnsPerUint;
    FfxUInt32 char_to_print = char_map[value_to_print * uintsPerChar + top];

    FfxUInt32 column = (pos_coord.x / fontSize) % columnsPerUint;
    FfxUInt32 row = pos_coord.y / fontSize;

    FfxUInt32 bit_value;
    bit_value = 1 << (column * fontSizeInPixels.y + row);

    
    if (all(FFX_GREATER_THAN_EQUAL(threadPos - cursorPos, FfxInt32x2(0, 0))) && 
        all(FFX_LESS_THAN(threadPos - cursorPos, fontSizeInPixels * fontSize)))
    {
        FfxFloat32x3 outColor = (char_to_print & bit_value) > 0 ? fontColor : backgroundColor;
        if (transparentBackground )
        {
            if ((char_to_print & bit_value) > 0) {
                StoreWriterOutput(threadPos, FfxFloat32x4(outColor, 1.0));
            }
        }
        else {
            StoreWriterOutput(threadPos, FfxFloat32x4(outColor, 1.0));
        }
        
    }
    
    AdvanceCursor(1);    
}

void WriteOneChar(FfxUInt32 valueToWrite)
{
    WriteOneChar(FfxInt32(valueToWrite));
}


void WriteValue(FfxInt32 intValueToWrite) {
    
    FfxInt32 value_to_print = intValueToWrite;

    
    FfxInt32 digit_array[10];
    digit_array[0] = 0; //base case

    // Extract the sign
    if (value_to_print < 0) {
        WriteOneChar(SpecialCharDash);
    }

    // Take the absolute value, we already extracted the sign
    value_to_print = abs(value_to_print);

    // Convert to a digit array
    FfxInt32 i = 0;
    while (value_to_print > 0 && i < 10) {
        digit_array[i] = value_to_print % 10;
        value_to_print /= 10;
        i++;
    }

    // Reverse the digit array
    FfxInt32 len = max(i, 1);
    for (FfxInt32 j = 0; j < len / 2; j++) {
        FfxInt32 temp = digit_array[j];
        digit_array[j] = digit_array[len - j - 1];
        digit_array[len - j - 1] = temp;
    }
        
    for (i = 0; i < len; i++) {
        WriteOneChar(digit_array[i]);
    }

}

void WriteValue(FfxUInt32 uintValueToWrite) {

    FfxUInt32 value_to_print = uintValueToWrite;

    FfxUInt32 digit_array[10];
    digit_array[0] = 0; // base case

    // Convert to a digit array
    FfxInt32 i = 0;
    while (value_to_print > 0 && i < 10) {
        digit_array[i] = value_to_print % 10;
        value_to_print /= 10;
        i++;
    }

    // Reverse the digit array
    FfxInt32 len = max(i, 1);
    for (FfxInt32 j = 0; j < len / 2; j++) {
        FfxUInt32 temp            = digit_array[j];
        digit_array[j]           = digit_array[len - j - 1];
        digit_array[len - j - 1] = temp;
    }

    for (i = 0; i < len; i++) {
        WriteOneChar(digit_array[i]);
    }
}


void WriteValue(FfxFloat32 floatValueToWrite) {

    FfxFloat32 value_to_print = floatValueToWrite;

    FfxInt32 digit_array[10];
        digit_array[0] = 0; //base case

    // Extract the sign    
    if (value_to_print < 0) {
        WriteOneChar(SpecialCharDash);    
    }

    // Take the absolute value of the float, we already extracted the sign
    value_to_print = abs(value_to_print);

    if (isnan(value_to_print)) {
        // write out NAN instead
        WriteOneChar(LettersStart + 13);
        WriteOneChar(LettersStart + 0);
        WriteOneChar(LettersStart + 13);        
        return;
    }

    if (isinf(value_to_print)) {
        // write out INF instead
        WriteOneChar(LettersStart + 8);
        WriteOneChar(LettersStart + 13);
        WriteOneChar(LettersStart + 5);
        return;
    }
    // if the value is large, switch to scientific notation
    FfxBoolean print_exponent = false;
    FfxInt32 exponent;
    if (value_to_print > 1e10){
        print_exponent = true;
        exponent = FfxInt32(log(value_to_print) / log(10.0));
        
        // Remove the exponent, now that we saved it
        value_to_print /= ffxPow(10.0f, exponent);
    }
    
    // Extract the integer part and fractional part
    FfxFloat32 fractional_part = ffxFract(value_to_print);
    FfxInt32 integer_part = FfxInt32(value_to_print-fractional_part);
    

    WriteValue(integer_part);
    WriteOneChar(SpecialCharDot);

    // Convert the fractional part to a digit array
    FfxInt32 i = 0;
    while (i < globalNumSigFigs && fractional_part > 0.0) {
        fractional_part *= 10.0;
        digit_array[i] = FfxInt32(fractional_part);
        fractional_part -= FfxFloat32(FfxInt32(fractional_part));
        i++;
    }
    FfxInt32 len = max(i,1);

    for (i = 0; i < len; i++) {
        WriteOneChar(digit_array[i]);
    }

    if (print_exponent) {
        WriteOneChar(SpecialCharExponent);
        WriteValue(exponent);
    }
}

void WriteValue(FfxFloat32x2 floatValueToWrite) {
    WriteValue(floatValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(floatValueToWrite.y);
}

void WriteValue(FfxFloat32x3 floatValueToWrite) {
    WriteValue(floatValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(floatValueToWrite.y);
    AdvanceCursor(1);
    WriteValue(floatValueToWrite.z);
}

void WriteValue(FfxFloat32x4 floatValueToWrite) {
    WriteValue(floatValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(floatValueToWrite.y);
    AdvanceCursor(1);
    WriteValue(floatValueToWrite.z);
    AdvanceCursor(1);
    WriteValue(floatValueToWrite.w);
}

void WriteValue(FfxInt32x2 intValueToWrite) {
    WriteValue(intValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(intValueToWrite.y);
}

void WriteValue(FfxInt32x3 intValueToWrite) {
    WriteValue(intValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(intValueToWrite.y);
    AdvanceCursor(1);
    WriteValue(intValueToWrite.z);
}

void WriteValue(FfxInt32x4 intValueToWrite) {
    WriteValue(intValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(intValueToWrite.y);
    AdvanceCursor(1);
    WriteValue(intValueToWrite.z);
    AdvanceCursor(1);
    WriteValue(intValueToWrite.w);
}

void WriteValue(FfxUInt32x2 uintValueToWrite) {
    WriteValue(uintValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(uintValueToWrite.y);
}

void WriteValue(FfxUInt32x3 uintValueToWrite) {
    WriteValue(uintValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(uintValueToWrite.y);
    AdvanceCursor(1);
    WriteValue(uintValueToWrite.z);
}

void WriteValue(FfxUInt32x4 uintValueToWrite) {
    WriteValue(uintValueToWrite.x);
    AdvanceCursor(1);
    WriteValue(uintValueToWrite.y);
    AdvanceCursor(1);
    WriteValue(uintValueToWrite.z);
    AdvanceCursor(1);
    WriteValue(uintValueToWrite.w);
}


void NewLine() {
    cursorPos.y += (fontSizeInPixels.y + kerning) * fontSize;
    cursorPos.x -= (numCharInCurrentLine) * (fontSizeInPixels.x + kerning) * fontSize;
    numCharInCurrentLine = 0;
}


#if defined(FFX_HLSL) && (__HLSL_VERSION >= 2021)
#include "ffx_shader_writer_templated.h"
#endif //  defined(FFX_HLSL) && (__HLSL_VERSION >= 2021)

#if defined(FFX_HLSL) && (__HLSL_VERSION >= 2021)
} // namespace ffx_shader_writer
#endif //  defined(FFX_HLSL) && (__HLSL_VERSION >= 2021)

#endif /* SHADERWRITER_H */
