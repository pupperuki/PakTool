#ifndef SRFRMHEADER_H
#define SRFRMHEADER_H

#include "../types.h"
#include "../CFourCC.h"
#include <FileIO/FileIO.h>

struct SRFRMHeader
{
    CFourCC RFRM;
    u64 size;
    u64 zero; // Not sure, but I've never seen this be anything other than 0.
    CFourCC type;
    u32 unknown1; // Seems to vary from type to type.
    u32 unknown2; // Always matches unknown1.

    // Although this is a struct, I'm including brief read/write functions to simplify file IO operations.
    SRFRMHeader() {}

    SRFRMHeader(CInputStream& input)
    {
        RFRM = CFourCC(input);
        size = input.ReadLongLong();
        zero = input.ReadLongLong();
        type = CFourCC(input);
        unknown1 = input.ReadLong();
        unknown2 = input.ReadLong();
    }

    void Write(COutputStream& output)
    {
        output.WriteLong(RFRM.asLong());
        output.WriteLongLong(size);
        output.WriteLongLong(0);
        output.WriteLong(type.asLong());
        output.WriteLong(unknown1);
        output.WriteLong(unknown2);
    }
};
#endif // SRFRMHEADER_H
