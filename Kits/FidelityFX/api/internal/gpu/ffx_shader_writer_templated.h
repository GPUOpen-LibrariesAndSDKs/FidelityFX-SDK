#ifndef SHADERWRITERTEMPLATED_H
#define SHADERWRITERTEMPLATED_H

FFX_STATIC FfxInt32 argIndex = -1;

#define DebugPrint(str, ...) do {                                                                                                                                    \
    const uint strLen = ffx_shader_writer::StrLen(str);                                                                                                                     \
    for(uint i = 0; i < strLen - 1; ++i)    {                                                                                                                                 \
        if (strLen - 1 - i > 3 && ffx_shader_writer::isIntFomatingToken(str[i],str[i+1],str[i+2], ffx_shader_writer::argIndex)) {                                      \
                ffx_shader_writer::PrintArgValue(__VA_ARGS__);                                                                                                        \
                i = i +2;                                                                                                                                               \
                continue;                                                                                                                                               \
        }                                                                                                                                                               \
        int numSigFigs = 0;                                                                                                                                             \
        if (strLen - 1 - i > 5 && ffx_shader_writer::isFloatFomatingToken(str[i],str[i+1],str[i+2],str[i+3],str[i+4], ffx_shader_writer::argIndex, numSigFigs)) {      \
                ffx_shader_writer::SetGlobalNumSigFigs(numSigFigs);                                                                                                 \
                ffx_shader_writer::PrintArgValue(__VA_ARGS__);                                                                                                          \
                ffx_shader_writer::SetGlobalNumSigFigs(ffx_shader_writer::defaultNumSigFigs);                                                                       \
                i = i +4;                                                                                                                                               \
                continue;                                                                                                                                               \
        }                                                                                                                                                              \
        ffx_shader_writer::PrintChar(str[i]);                                                                                                                         \
    }                                                                                                                                                                  \
} while(0)


template<typename Type, uint Num> uint StrLen(Type str[Num])
{
    // Includes the null terminator
    return Num;
}

template<typename charHack> int charToInt(charHack c) {
    int charCode = -1;
    uint charMapLen = StrLen("0123456789");
    for (int i = 0; i < charMapLen; i++) {
        if (c == "0123456789"[i]) {
            return i;
        }
    }

    return -1;
}

template<typename Type> void PrintChar(in Type c)
{
    {
        int charCode = -1;
        uint charMapLen = StrLen("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        for (int i = 0; i < charMapLen; i++) {
            if (c == "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i]) {
                charCode = LettersStart + i;
                WriteOneChar(charCode);
                return;
            }
        }
    }

        {
        int charCode = -1;
        uint charMapLen = StrLen("abcdefghijklmnopqrstuvwxyz");
        for (int i = 0; i < charMapLen; i++) {
            if (c == "abcdefghijklmnopqrstuvwxyz"[i]) {
                charCode = LettersStart + i;
                WriteOneChar(charCode);
                return;
            }
        }
    }

    {
        int charCode = charToInt(c);
        if (charCode != -1) {
            WriteOneChar(charCode);
            return;
        }
    }

    {
        int charCode = -1;
        if(c == ':')
            charCode = SpecialCharColon;
        if(c == '-')
            charCode = SpecialCharDash;
        if(c == '.')
            charCode = SpecialCharDot;

        if (charCode != -1) {
            WriteOneChar(charCode);
            return;
        }
    }


    if (c == '\n') {
        NewLine();
        return;
    }

    if (c == ' ') {
        AdvanceCursor(1);
        return;
    }

    // advance if an unknown char?
    AdvanceCursor(1);
    return;
}

template<typename charHack> bool isIntFomatingToken(charHack char0, charHack char1, charHack char2, out int argIdx)
{
    if (char0 == '{' && char2 == '}') {
        argIdx = charToInt(char1);
        return true;
    }

    return false;
}

template<typename charHack> bool isFloatFomatingToken(charHack char0, charHack char1, charHack char2, charHack char3, charHack char4, out int argIdx, out int numSigFigs)
{
    if (char0 == '{' && char2 == '.' && char4 == '}') {
        argIdx = charToInt(char1);
        numSigFigs = charToInt(char3);
        return true;
    }

    return false;
}

void PrintArgValue()
{
    // empty base case for __VA_ARGS with no args
}

template<typename argType> void PrintArgValue(argType value)
{
    WriteValue(value);
}

template<typename argType0, typename argType1> void PrintArgValue(argType0 value0, argType1 value1)
{
    switch(ffx_shader_writer::argIndex)
    {
        case 0:
            WriteValue(value0);
            break;
        case 1:
            WriteValue(value1);
            break;
    }
}

template<typename argType0, typename argType1, typename argType2> void PrintArgValue(argType0 value0, argType1 value1, argType2 value2)
{
    switch(ffx_shader_writer::argIndex)
    {
        case 0:
            WriteValue(value0);
            break;
        case 1:
            WriteValue(value1);
            break;
        case 2:
            WriteValue(value2);
            break;
    }
}

template<typename argType0, typename argType1, typename argType2, typename argType3> void PrintArgValue(argType0 value0, argType1 value1, argType2 value2, argType3 value3)
{
    switch(ffx_shader_writer::argIndex)
    {
        case 0:
            WriteValue(value0);
            break;
        case 1:
            WriteValue(value1);
            break;
        case 2:
            WriteValue(value2);
            break;
        case 3:
            WriteValue(value3);
            break;
    }
}

#endif /* SHADERWRITERTEMPLATED_H */