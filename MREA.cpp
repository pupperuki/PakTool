#include "MREA.h"
#include "SAreaHeader.h"
#include "Compression.h"

bool DecompressMREA(CInputStream& input, u32 size, std::vector<u8>& out)
{
    // This function doesn't assume SEEK_SET and Seek_END will correspond to the beginning/end of the resource
    if (!input.IsValid()) return false;
    u32 MREAStart = input.Tell();

    // Read+write header
    SAreaHeader header(input);
    if (!header.valid) return false;

    if (!header.isSupportedVersion)
    {
        out.resize(size);
        input.Seek(MREAStart, SEEK_SET);
        input.ReadBytes(out.data(), out.size());
        return true;
    }

    out.resize(header.HeaderSize + header.DecmpTotal);
    CMemoryOutStream output(out.data(), out.size(), IOUtil::BigEndian);
    output.WriteBytes(header.HeaderBuffer.data(), header.HeaderBuffer.size());

    // Write compressed blocks - compressed size of 0 denotes uncompressed so we set every block to that
    for (u32 c = 0; c < header.numCompressedBlocks; c++)
    {
        output.WriteLong(header.blocks[c].DecmpSize);
        output.WriteLong(header.blocks[c].DecmpSize);
        output.WriteLong(0);
        output.WriteLong(header.blocks[c].NumSections);
    }
    output.WriteToBoundary(32, 0);

    // Section numbers present starting in MP3
    if (header.version >= 0x1D)
        output.WriteBytes(header.NumbersBuffer.data(), header.NumbersBuffer.size());

    // Decompressing data
    input.Seek(header.BlocksOffset, SEEK_SET);
    for (u32 c = 0; c < header.numCompressedBlocks; c++)
    {
        // Is block uncompressed?
        if (header.blocks[c].CmpSize == 0)
        {
            input.ReadBytes(output.DataAtPosition(), header.blocks[c].DecmpSize);
            output.Seek(header.blocks[c].DecmpSize, SEEK_CUR);
            continue;
        }

        // For some reason in MREA Retro pads the beginning of each compressed section instead of the end
        u32 pad = 32 - (header.blocks[c].CmpSize % 32);
        if (pad == 32) pad = 0;
        input.Seek(pad, SEEK_CUR);

        // Decompress
        bool success = DecompressSegmented(input, header.blocks[c].CmpSize, output, header.blocks[c].DecmpSize);
        if (!success) return false;
    }

    return true;
}

bool CompressMREA(CInputStream& input, COutputStream& output, u32 size)
{
    if (!input.IsValid()) return false;
    u32 MREAStart = input.Tell();

    // Read+write header
    SAreaHeader header(input);
    if (!header.valid) return false;

    // If this isn't a supported version of MREA then we'll just copy the whole file directly
    if (!header.isSupportedVersion)
    {
        std::vector<u8> buf(size);
        input.Seek(MREAStart, SEEK_SET);
        input.ReadBytes(buf.data(), buf.size());
        output.WriteBytes(buf.data(), buf.size());
        return true;
    }

    output.WriteBytes(header.HeaderBuffer.data(), header.HeaderBuffer.size());

    // Write compressed blocks - we need to actually fill this in later but we don't have the values yet
    u32 BlockDefsOffset = output.Tell();

    for (u32 c = 0; c < header.numCompressedBlocks * 4; c++)
        output.WriteLong(0);

    output.WriteToBoundary(32, 0);

    // Section numbers present starting in MP3
    if (header.version >= 0x1D)
        output.WriteBytes(header.NumbersBuffer.data(), header.NumbersBuffer.size());
    output.WriteToBoundary(32, 0);

    // Compressing data
    input.Seek(header.BlocksOffset, SEEK_SET);

    for (u32 c = 0; c < header.numCompressedBlocks; c++)
    {
        if (header.blocks[c].CmpSize == 0)
        {
            u32 DecmpSize = header.blocks[c].DecmpSize;

            u8 *decmp = new u8[DecmpSize];
            u8 *cmp = new u8[DecmpSize * 2];
            input.ReadBytes(decmp, DecmpSize);

            u32 CmpSize;
            if (header.version == 0x20) // DKCR - zlib
                CompressZlibSegmented(decmp, DecmpSize, cmp, DecmpSize * 2, CmpSize, true);
            else                        // MP2/3 - LZO
                CompressLZOSegmented(decmp, DecmpSize, cmp, DecmpSize * 2, CmpSize, true);

            u32 pad = 32 - (CmpSize % 32);
            if (pad == 32) pad = 0;

            if (CmpSize + pad >= DecmpSize) // Compressed data is not smaller - leave uncompressed
            {
                header.blocks[c].BufSize = DecmpSize;
                header.blocks[c].DecmpSize = DecmpSize;
                header.blocks[c].CmpSize = 0;
                output.WriteBytes(decmp, DecmpSize);
            }

            else // Compressed data is smaller
            {
                header.blocks[c].BufSize = DecmpSize + 0x120;
                header.blocks[c].DecmpSize = DecmpSize;
                header.blocks[c].CmpSize = CmpSize;

                for (u32 p = 0; p < pad; p++)
                    output.WriteByte(0);
                output.WriteBytes(cmp, CmpSize);
            }

            delete[] cmp;
            delete[] decmp;
        }

        else // Section is already compressed, don't attempt to recompress it
        {
            u32 size = header.blocks[c].CmpSize;
            u32 pad = 32 - (size % 32);
            if (pad != 32) size += pad;

            std::vector<u8> buf(size);
            input.ReadBytes(buf.data(), buf.size());
            output.WriteBytes(buf.data(), buf.size());
        }
    }

    // Rewrite block definitions
    output.Seek(BlockDefsOffset, SEEK_SET);
    for (u32 c = 0; c < header.numCompressedBlocks; c++)
    {
        output.WriteLong(header.blocks[c].BufSize);
        output.WriteLong(header.blocks[c].DecmpSize);
        output.WriteLong(header.blocks[c].CmpSize);
        output.WriteLong(header.blocks[c].NumSections);
    }

    return true;
}
