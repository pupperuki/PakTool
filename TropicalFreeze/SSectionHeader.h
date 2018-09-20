#ifndef SSECTIONHEADER_H
#define SSECTIONHEADER_H

#include "../types.h"
#include "../CFourCC.h"
#include <FileIO/FileIO.h>

struct SSectionHeader
{
    CFourCC type;
    u64 size;
    u32 unknown1; // Always 1
    u32 unknown2; // Always 0
    u32 unknown3; // Always 0

    // Although this is a struct, I'm including brief read/write functions to simplify file IO operations.
    SSectionHeader()
    {
        unknown1 = 1;
        unknown2 = 0;
        unknown3 = 0;
    }

    SSectionHeader(CInputStream& input)
    {
        type = CFourCC(input);
        size = input.ReadLongLong();
        unknown1 = input.ReadLong();
        unknown2 = input.ReadLong();
        unknown3 = input.ReadLong();
    }

    void Write(COutputStream& output)
    {
        output.WriteLong(type.asLong());
        output.WriteLongLong(size);
        output.WriteLong(1);
        output.WriteLong(0);
        output.WriteLong(0);
    }
};

#endif // SSECTIONHEADER_H
