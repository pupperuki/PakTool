#ifndef SAREAHEADER_H
#define SAREAHEADER_H

#include <FileIO/FileIO.h>
#include "types.h"

struct SAreaHeader
{
    bool isSupportedVersion;
    bool valid;
    u32 version;
    u32 numSections;
    u32 numCompressedBlocks;
    u32 numSectionNumbers;
    u32 HeaderSize;
    u32 BlocksOffset;

    struct SCompressedBlock { u32 BufSize, DecmpSize, CmpSize, NumSections; };
    std::vector<SCompressedBlock> blocks;
    u32 DecmpTotal;

    std::vector<u8> HeaderBuffer;
    std::vector<u8> NumbersBuffer;

    SAreaHeader() { valid = false; }

    SAreaHeader(CInputStream& input)
    {
        valid = false;
        isSupportedVersion = false;
        DecmpTotal = 0;

        if (!input.IsValid()) return;
        u32 start = input.Tell();

        u32 deadbeef = input.ReadLong();
        if (deadbeef != 0xdeadbeef) return;
        valid = true;

        version = input.ReadLong();
        if (version < 0x19) return;
        isSupportedVersion = true;

        if (version == 0x19) // Echoes
        {
            input.Seek(start + 0x40, SEEK_SET);
            numSections = input.ReadLong();
            input.Seek(start + 0x70, SEEK_SET);
            numCompressedBlocks = input.ReadLong();
            input.Seek(start + 0x80, SEEK_SET);
        }

        else if (version >= 0x1D) // Corruption Proto and up
        {
            input.Seek(start + 0x40, SEEK_SET);
            numSections = input.ReadLong();
            numCompressedBlocks = input.ReadLong();
            numSectionNumbers = input.ReadLong();
            input.Seek(start + 0x60, SEEK_SET);
        }
        input.Seek(0x4 * numSections, SEEK_CUR);
        input.SeekToBoundary(32);

        HeaderBuffer.resize(input.Tell() - start);
        input.Seek(start, SEEK_SET);
        input.ReadBytes(HeaderBuffer.data(), HeaderBuffer.size());

        // Compressed Blocks
        blocks.resize(numCompressedBlocks);
        for (u32 b = 0; b < blocks.size(); b++)
        {
            blocks[b].BufSize = input.ReadLong();
            blocks[b].DecmpSize = input.ReadLong();
            blocks[b].CmpSize = input.ReadLong();
            blocks[b].NumSections = input.ReadLong();
            DecmpTotal += blocks[b].DecmpSize;
        }
        input.SeekToBoundary(32);

        // Numbers
        if (version >= 0x1D)
        {
            u32 NumbersSize = (((numSectionNumbers + 3) / 4) * 32); // Round to next multiple of 4, multiply by size of 4 numbers
            NumbersBuffer.resize(NumbersSize);
            input.ReadBytes(NumbersBuffer.data(), NumbersBuffer.size());
        }

        HeaderSize = input.Tell() - start;
        BlocksOffset = input.Tell();
    }
};

#endif // SAREAHEADER_H
