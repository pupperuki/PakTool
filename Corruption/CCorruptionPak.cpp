#include "CCorruptionPak.h"
#include "../Compression.h"
#include "../MREA.h"
#include "../StringUtil.h"

#include <boost/filesystem.hpp>
#include <hl_md5.h>
#include <iostream>

CCorruptionPak::CCorruptionPak()
{
}

CCorruptionPak::CCorruptionPak(CInputStream& pak)
{
    u32 check = pak.ReadLong();
    if (check != 2) return;
    pak.SeekToBoundary(64);

    u32 numPakSections = pak.ReadLong();
    u32 StringSize, TocSize, DataSize;

    for (u32 s = 0; s < numPakSections; s++)
    {
        CFourCC type = pak.ReadLong();
        if (type == "STRG") StringSize = pak.ReadLong();
        else if (type == "RSHD") TocSize = pak.ReadLong();
        else if (type == "DATA") DataSize = pak.ReadLong();
    }
    pak.SeekToBoundary(64);

    StringOffset = pak.Tell();
    TocOffset = StringOffset + StringSize;
    DataOffset = TocOffset + TocSize;

    // Names
    pak.Seek(StringOffset, SEEK_SET);
    u32 NamedResCount = pak.ReadLong();
    NamedResources.resize(NamedResCount);

    for (u32 n = 0; n < NamedResCount; n++)
    {
        NamedResources[n].Name = pak.ReadString();
        NamedResources[n].ResType = pak.ReadLong();
        NamedResources[n].ResID = pak.ReadLongLong();
    }

    // Resources
    pak.Seek(TocOffset, SEEK_SET);
    u32 numResources = pak.ReadLong();
    ToC.resize(numResources);
    ExtractionQueue.resize(numResources);

    for (u32 r = 0; r < numResources; r++)
    {
        ToC[r].Compressed = (pak.ReadLong() != 0);
        ToC[r].ResType = pak.ReadLong();
        ToC[r].ResID = pak.ReadLongLong();
        ToC[r].ResSize = pak.ReadLong();
        ToC[r].ResOffset = pak.ReadLong();
        ExtractionQueue[r] = &ToC[r];
    }

    // Remove duplicates - sort by ID, erase sequential duplicates, then sort again by offset to restore original order
    std::sort(ExtractionQueue.begin(), ExtractionQueue.end(), &ResSortByID);
    ExtractionQueue.erase(std::unique(ExtractionQueue.begin(), ExtractionQueue.end(), &ResCompare), ExtractionQueue.end());
    std::sort(ExtractionQueue.begin(), ExtractionQueue.end(), &ResSortByOffset);

    PakFile = &pak;
}

void CCorruptionPak::Extract(std::string OutputDir, bool decompressMREA)
{
    if (ToC.empty())
    {
        std::cout << "Unable to unpack; pak is empty\n";
        return;
    }
    if (!PakFile->IsValid())
    {
        std::cout << "Unable to unpack; invalid input file\n";
        return;
    }

    StringUtil::ensureEndsWithSlash(OutputDir);

    for (u32 r = 0; r < ExtractionQueue.size(); r++)
    {
        SResource *res = ExtractionQueue[r];

        std::string OutName;
        OutName = StringUtil::resToStr((long long) res->ResID) + "." + res->ResType.asString();

        // Printing output is useful for the user, but we don't want to do it too much,
        // or else it'll have a negative and noticeable impact on performance.
        if (!(r & 0x1F) || (r == ExtractionQueue.size() - 1)) {
            std::cout << "\rExtracting file " << std::dec << r + 1 << " of " << ExtractionQueue.size() << " - " << OutName;
        }

        // Extract file
        PakFile->Seek(DataOffset + res->ResOffset, SEEK_SET);

        std::vector<u8> buf;

        if (res->Compressed)
        {
            bool success = Decompress(*PakFile, *res, buf);
            if (!success)
            {
                std::cout << "\rError: Unable to decompress resource: " << OutName << "\n";
                continue;
            }
        }

        else if ((res->ResType == "MREA") && (decompressMREA))
        {
            bool success = DecompressMREA(*PakFile, res->ResSize, buf);
            if (!success)
            {
                std::cout << "\rError: Unable to decompress MREA: " << OutName << "\n";
                PakFile->Seek(res->ResOffset, SEEK_SET);
                buf.resize(res->ResSize);
                PakFile->ReadBytes(buf.data(), buf.size());
            }
        }

        else
        {
            buf.resize(res->ResSize);
            PakFile->ReadBytes(buf.data(), buf.size());
        }

        CFileOutStream out(OutputDir + OutName, IOUtil::BigEndian);
        out.WriteBytes(buf.data(), buf.size());
    }
    std::cout << "\n";
}

void CCorruptionPak::DumpList(std::string OutputFile)
{
    // todo - use FileIO CTextOutStream instead of a FILE*
    if (NamedResources.empty() && ToC.empty())
    {
        std::cout << "Unable to dump resource list; pak is empty.\n";
        return;
    }

    FILE *f = fopen(OutputFile.c_str(), "w");
    if (!f)
    {
        std::cout << "Unable to dump resource list; couldn't open output file.\n";
        return;
    }

    // Dump names
    for (u32 n = 0; n < NamedResources.size(); n++)
    {
        std::string name;
        SNamedResource *res = &NamedResources[n];

        name = StringUtil::resToStr((long long)res->ResID) + "." + res->ResType.asString();
        fprintf(f, "%s %s\n", name.c_str(), res->Name.c_str());
    }
    fprintf(f, "\n");

    // Dump file list
    for (u32 r = 0; r < ToC.size(); r++)
    {
        std::string name;
        SResource *res = &ToC[r];

        name = StringUtil::resToStr((long long) res->ResID) + "." + res->ResType.asString();
        fprintf(f, "%s\n", name.c_str());
    }
}

void CCorruptionPak::BuildTOC(FILE *list, EWhichGame game)
{
    // todo: make CTextInStream suck less and use that instead of a FILE*
    NamedResources.clear();
    ToC.clear();
    ExtractionQueue.clear();

    // named resources
    while (!feof(list))
    {
        char line[276];
        memset(line, 0, 276);
        fgets(line, 276, list);

        // blank line indicates end of named resource section of the list
        if (strcmp(line, "\n") == 0) break;

        SNamedResource res;
        char resID[17];
        char resType[5];
        char name[256];
        memset(resID, 0, 17);
        memset(resType, 0, 5);
        memset(name, 0, 256);

        sscanf(line, "%16c.%4c %s\n", resID, resType, name);
        res.ResID = StringUtil::strToRes64(resID);
        res.ResType = resType;
        res.Name = name;

        if (res.Name.empty())
        {
            std::cout << "\rError: List file is not formatted correctly!\n";
            return;
        }

        NamedResources.push_back(res);
    }

    // resource table
    while (true)
    {
        char line[276];
        memset(line, 0, 276);
        fgets(line, 276, list);
        if (!line[0]) break;

        SResource res;
        char resID[17];
        char resType[5];
        memset(resID, 0, 17);
        memset(resType, 0, 5);

        sscanf(line, "%16c.%4c\n", resID, resType);
        res.ResID = StringUtil::strToRes64(resID);
        res.ResType = resType;

        ToC.push_back(res);
    }
}

void CCorruptionPak::Repack(CFileOutStream& pak, std::string FileDir, EWhichGame game, ECompressMode CompressMode)
{
    if (!pak.IsValid()) return;
    StringUtil::ensureEndsWithSlash(FileDir);

    // Header
    pak.WriteLong(2);
    pak.WriteLong(0x40);
    pak.WriteToBoundary(64, 0);

    // Table of Contents - write later
    u32 TocOffset = pak.Tell();
    pak.WriteLong(0x3);
    pak.WriteToBoundary(64, 0);

    // Named Resources
    u32 STRGStart = pak.Tell();
    pak.WriteLong(NamedResources.size());
    for (u32 n = 0; n < NamedResources.size(); n++)
    {
        pak.WriteString(NamedResources[n].Name);
        pak.WriteLong(NamedResources[n].ResType.asLong());
        pak.WriteLongLong(NamedResources[n].ResID);
    }
    pak.WriteToBoundary(64, 0);
    u32 STRGSize = pak.Tell() - STRGStart;

    // Resource Table - write later. We don't know the file sizes/offsets yet.
    u32 RSHDStart = pak.Tell();
    pak.WriteLong(ToC.size());
    for (u32 r = 0; r < ToC.size(); r++)
    {
        for (u32 v = 0; v < 6; v++)
            pak.WriteLong(0);
    }
    pak.WriteToBoundary(64, 0);
    u32 RSHDSize = pak.Tell() - RSHDStart;

    // Resource Data
    u32 DATAStart = pak.Tell();
    for (u32 r = 0; r < ToC.size(); r++)
    {
        SResource *res = &ToC[r];
        res->ResOffset = pak.Tell() - DATAStart;
        std::string filename = StringUtil::resToStr((long long) res->ResID) + "." + res->ResType.asString();

        if (!(r & 0x1F) || (r == ToC.size() - 1))
            std::cout << "\rRepacking file " << std::dec << r + 1 << " of " << ToC.size() << " - " << filename;

        CFileInStream file(FileDir + filename);

        if (!file.IsValid())
        {
            std::cout << "\rError: Unable to open resource: " << filename << "\n";
            continue;
        }

        std::vector<u8> buf;

        // Compression
        if ((CompressMode != NoCompression) && (ShouldCompress(res->ResType, file.Size())))
        {
            buf.resize(file.Size() * 2);
            CMemoryOutStream mem(buf.data(), buf.size(), IOUtil::BigEndian);
            bool success = Compress(file, mem, *res, game);

            if (!success)
                std::cout << "\rError: Unable to compress resource: " << filename << "\n";

            u32 padded = (mem.SpaceUsed() + 63) & ~63;

            if ((success) && (padded < file.Size()))
            {
                res->Compressed = true;
                buf.resize(mem.SpaceUsed());
            }

            else
            {
                res->Compressed = false;
                buf.resize(file.Size());
                file.Seek(0x0, SEEK_SET);
                file.ReadBytes(buf.data(), buf.size());
            }
        }

        else if ((res->ResType == "MREA") && (CompressMode == CompressEmbedded))
        {
            res->Compressed = false;
            buf.resize(file.Size() * 2);
            CMemoryOutStream mem(buf.data(), buf.size(), IOUtil::BigEndian);
            bool success = CompressMREA(file, mem, file.Size());

            if (!success)
            {
                std::cout << "\rError: Unable to compress area: " << filename << "\n";
                buf.resize(file.Size());
                file.Seek(0x0, SEEK_SET);
                file.ReadBytes(buf.data(), buf.size());
            }

            else
                buf.resize(mem.SpaceUsed());
        }

        else
        {
            res->Compressed = false;
            buf.resize(file.Size());
            file.Seek(0x0, SEEK_SET);
            file.ReadBytes(buf.data(), buf.size());
        }
        file.Close();

        pak.WriteBytes(buf.data(), buf.size());
        pak.WriteToBoundary(64, 0xFF);
        res->ResSize = pak.Tell() - res->ResOffset - DATAStart;
    }
    std::cout << "\n";

    u32 DATASize = pak.Tell() - DATAStart;

    // Writing the ToC
    pak.Seek(TocOffset, SEEK_SET);
    pak.WriteLong(3);
    pak.WriteLong(0x53545247); // "STRG"
    pak.WriteLong(STRGSize);
    pak.WriteLong(0x52534844); // "RSHD"
    pak.WriteLong(RSHDSize);
    pak.WriteLong(0x44415441); // "DATA"
    pak.WriteLong(DATASize);

    // Writing the resource table, then we're done
    pak.Seek(RSHDStart, SEEK_SET);
    pak.WriteLong(ToC.size());
    for (u32 r = 0; r < ToC.size(); r++)
    {
        SResource *res = &ToC[r];
        pak.WriteLong(res->Compressed ? 1 : 0);
        pak.WriteLong(res->ResType.asLong());
        pak.WriteLongLong(res->ResID);
        pak.WriteLong(res->ResSize);
        pak.WriteLong(res->ResOffset);
    }

    // Calculate MD5
    std::cout << "Generating MD5 hash...\n";
    std::string pakname = pak.FileName();
    pak.Close();

    u8 md5[16];
    HashMD5(CFileInStream(pakname, IOUtil::BigEndian), md5);

    pak.Update(pakname, IOUtil::BigEndian);
    pak.Seek(0x8, SEEK_SET);
    pak.WriteBytes(md5, 16);
}

u32 CCorruptionPak::GetResourceCount()
{
    return ToC.size();
}

u32 CCorruptionPak::GetUniqueResourceCount()
{
    return ExtractionQueue.size();;
}

u32 CCorruptionPak::GetNamedResourceCount()
{
    return NamedResources.size();
}

bool CCorruptionPak::Decompress(CInputStream& input, SResource& res, std::vector<u8>& out)
{
    if (!input.IsValid()) return false;

    CFourCC CMPD = input.ReadLong();
    if (CMPD != "CMPD") return false;

    u32 numBlocks = input.ReadLong();
    struct SCompressedBlock { u32 CmpSize, DecmpSize; };
    std::vector<SCompressedBlock> blocks(numBlocks);

    u32 TotalDecmp = 0;
    for (u32 b = 0; b < numBlocks; b++)
    {
        blocks[b].CmpSize = input.ReadLong() & 0x00FFFFFF;
        blocks[b].DecmpSize = input.ReadLong() & 0x00FFFFFF;
        TotalDecmp += blocks[b].DecmpSize;
    }
    out.resize(TotalDecmp);
    CMemoryOutStream output(out.data(), out.size(), IOUtil::BigEndian);

    for (u32 b = 0; b < numBlocks; b++)
    {
        // Is block compressed?
        if (blocks[b].CmpSize == blocks[b].DecmpSize)
        {
            input.ReadBytes(output.DataAtPosition(), blocks[b].DecmpSize);
            output.Seek(blocks[b].DecmpSize, SEEK_CUR);
            continue;
        }

        u16 type = input.PeekShort();
        u32 total_out;

        if ((type == 0x78DA) || (type == 0x7801) || (type == 0x789C)) // zlib
        {
            std::vector<u8> in(blocks[b].CmpSize);
            input.ReadBytes(in.data(), in.size());

            bool success = DecompressZlib(in.data(), in.size(), (u8*) output.DataAtPosition(), blocks[b].DecmpSize, total_out);
            if (!success) return false;

            output.Seek(total_out, SEEK_CUR);
        }

        else // segmented lzo
        {
            bool success = DecompressSegmented(input, blocks[b].CmpSize, output, blocks[b].DecmpSize);
            if (!success) return false;
        }
    }

    return true;
}


bool CCorruptionPak::Compress(CInputStream& input, COutputStream& output, SResource& res, EWhichGame game)
{
    if (!input.IsValid()) return false;
    if (!output.IsValid()) return false;

    struct block { u32 DecmpSize, CmpSize; u8 flag; u8 *buffer; };
    std::vector<block> blocks(1);

    // Force everything to use one block
    blocks[0].DecmpSize = input.Size();

    // Compress each block
    for (u32 b = 0; b < blocks.size(); b++)
    {
        u32 size = blocks[b].DecmpSize;

        u8 *decmp = new u8[size];
        u8 *cmp = new u8[size * 2];
        input.ReadBytes(decmp, size);

        u32 CmpSize;
        bool success;
        if (game == DKCR)
            success = CompressZlib(decmp, size, cmp, size * 2, CmpSize);
        else
            success = CompressLZOSegmented(decmp, size, cmp, size * 2, CmpSize, false);

        if (!success) return false;

        // Is compressed data smaller?
        if (CmpSize < size)
        {
            blocks[b].CmpSize = CmpSize;
            blocks[b].flag = 0xA0;
            blocks[b].buffer = cmp;
            delete[] decmp;
        }
        else
        {
            blocks[b].CmpSize = blocks[b].DecmpSize;
            blocks[b].flag = 0;
            blocks[b].buffer = decmp;
            delete[] cmp;
        }
    }

    // Write to output buffer
    output.WriteLong(0x434D5044); // "CMPD"
    output.WriteLong(blocks.size());
    for (u32 b = 0; b < blocks.size(); b++)
    {
        output.WriteLong(blocks[b].CmpSize | (blocks[b].flag << 24));
        output.WriteLong(blocks[b].DecmpSize);
    }
    for (u32 b = 0; b < blocks.size(); b++)
    {
        output.WriteBytes(blocks[b].buffer, blocks[b].CmpSize);
        delete[] blocks[b].buffer;
    }

    return true;
}

bool CCorruptionPak::ShouldCompress(CFourCC ResType, u32 ResSize)
{
    if (ResType == "TXTR") return true;
    if (ResType == "CMDL") return true;
    if (ResType == "CSKR") return true;
    if (ResType == "CHAR") return true;
    if (ResType == "SAND") return true;
    if (ResType == "ANIM") return true;
    if (ResType == "SCAN") return true;
    if (ResType == "FONT") return true;
    if (ResType == "CSMP") return true;
    if (ResType == "STRG") return true;
    if (ResType == "CAAD") return true;
    if (ResType == "DCLN") return true;
    if (ResType == "USRC") return true;
    if ((ResType == "PART") && (ResSize >= 0x80)) return true;
    if ((ResType == "ELSC") && (ResSize >= 0x80)) return true;
    if ((ResType == "SWHC") && (ResSize >= 0x80)) return true;
    if ((ResType == "WPSC") && (ResSize >= 0x80)) return true;
    if ((ResType == "CRSC") && (ResSize >= 0x80)) return true;
    if ((ResType == "BFRC") && (ResSize >= 0x80)) return true;
    if ((ResType == "SPSC") && (ResSize >= 0x80)) return true;
    if ((ResType == "DPSC") && (ResSize >= 0x80)) return true;
    return false;
}

void CCorruptionPak::HashMD5(CInputStream& pak, unsigned char *out)
{
    u32 size = pak.Size();
    pak.Seek(0x40, SEEK_SET);

    HL_MD5_CTX ctx;
    MD5 md5;
    md5.MD5Init(&ctx);

    u32 left = 1;
    u8 buf[1024];

    while (left)
    {
        left = size - pak.Tell();
        u32 len = (left > 1024) ? 1024 : left;
        pak.ReadBytes(buf, len);
        md5.MD5Update(&ctx, buf, len);
    }

    md5.MD5Final(out, &ctx);
}

bool ResSortByOffset(CCorruptionPak::SResource* left, CCorruptionPak::SResource* right)
{
    return (left->ResOffset < right->ResOffset);
}

bool ResSortByID(CCorruptionPak::SResource* left, CCorruptionPak::SResource* right)
{
    return (left->ResID < right->ResID);
}

bool ResCompare(CCorruptionPak::SResource* left, CCorruptionPak::SResource* right)
{
    return ((left->ResID == right->ResID) && (left->ResType == right->ResType));
}
