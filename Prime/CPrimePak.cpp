#include "CPrimePak.h"
#include "../Compression.h"
#include "../MREA.h"
#include "../StringUtil.h"

#include <boost/filesystem.hpp>
#include <iostream>

CPrimePak::CPrimePak()
{
}

CPrimePak::CPrimePak(CInputStream& pak)
{
    version = EPakVersion(pak.ReadLong());
    if (version != PrimePak) return;

    pak.Seek(0x4, SEEK_CUR);
    if (pak.EoF()) return; // Echoes demo has an empty pak...

    u32 NamedResourceCount = pak.ReadLong();
    NamedResources.resize(NamedResourceCount);

    // Is this a regular MP1/2 pak or is it a Corruption proto pak?
    if (NamedResourceCount > 0)
    {
        pak.Seek(0x14, SEEK_SET);
        u32 check = pak.ReadLong();
        if (check & 0xFFFF0000) // There's no reason for a name to be this long.
            version = CorruptionProtoPak;
        pak.Seek(0xC, SEEK_SET);
    }

    // Read Named Resources
    for (u32 n = 0; n < NamedResourceCount; n++)
    {
        NamedResources[n].ResType = pak.ReadLong();

        if (version == PrimePak)
            NamedResources[n].ResID = pak.ReadLong();
        if (version == CorruptionProtoPak)
            NamedResources[n].ResID64 = pak.ReadLongLong();

        u32 NameLength = pak.ReadLong();
        NamedResources[n].Name = pak.ReadString(NameLength);
    }

    // Read ToC
    u32 ResourceCount = pak.ReadLong();
    ToC.resize(ResourceCount);
    ExtractionQueue.resize(ResourceCount);

    for (u32 t = 0; t < ResourceCount; t++)
    {
        ToC[t].Compressed = (pak.ReadLong() != 0);
        ToC[t].ResType = pak.ReadLong();

        if (version == PrimePak)
            ToC[t].ResID = pak.ReadLong();
        if (version == CorruptionProtoPak)
            ToC[t].ResID64 = pak.ReadLongLong();

        ToC[t].ResSize = pak.ReadLong();
        ToC[t].ResOffset = pak.ReadLong();
        ExtractionQueue[t] = &ToC[t];
    }

    if (version == PrimePak) {
        std::sort(ExtractionQueue.begin(), ExtractionQueue.end(), ResSortByID32);
        ExtractionQueue.erase(std::unique(ExtractionQueue.begin(), ExtractionQueue.end(), &ResCompare32), ExtractionQueue.end());
    }
    else if (version == CorruptionProtoPak) {
        std::sort(ExtractionQueue.begin(), ExtractionQueue.end(), ResSortByID64);
        ExtractionQueue.erase(std::unique(ExtractionQueue.begin(), ExtractionQueue.end(), &ResCompare64), ExtractionQueue.end());
    }
    std::sort(ExtractionQueue.begin(), ExtractionQueue.end(), &ResSortByOffset);

    PakFile = &pak;
}

void CPrimePak::Extract(std::string OutputDir, bool decompressMREA)
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
        if (version == PrimePak)
            OutName = StringUtil::resToStr((long) res->ResID) + "." + res->ResType.asString();
        else if (version == CorruptionProtoPak)
            OutName = StringUtil::resToStr((long long) res->ResID64) + "." + res->ResType.asString();

        // Printing output is useful for the user, but we don't want to do it too much,
        // or else it'll have a negative and noticeable impact on performance.
        if (!(r & 0x1F) || (r == ExtractionQueue.size() - 1)) {
            std::cout << "\rExtracting file " << std::dec << r + 1 << " of " << ExtractionQueue.size() << " - " << OutName;
        }

        // Extract file
        PakFile->Seek(res->ResOffset, SEEK_SET);
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

void CPrimePak::DumpList(std::string OutputFile)
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

        if (version == PrimePak)
            name = StringUtil::resToStr((long) res->ResID);
        else if (version == CorruptionProtoPak)
            name = StringUtil::resToStr((long long)res->ResID64);
        name += "." + res->ResType.asString();

        fprintf(f, "%s %s\n", name.c_str(), res->Name.c_str());
    }
    fprintf(f, "\n");

    // Dump file list
    for (u32 r = 0; r < ToC.size(); r++)
    {
        std::string name;
        SResource *res = &ToC[r];

        if (version == PrimePak)
            name = StringUtil::resToStr((long) res->ResID);
        else if (version == CorruptionProtoPak)
            name = StringUtil::resToStr((long long) res->ResID64);
        name += "." + res->ResType.asString();

        fprintf(f, "%s\n", name.c_str());
    }
}

void CPrimePak::BuildTOC(FILE *list, EWhichGame game)
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

        if (game == MP3Proto)
        {
            sscanf(line, "%16c.%4c %s\n", resID, resType, name);
            res.ResID64 = StringUtil::strToRes64(resID);
        }
        else
        {
            sscanf(line, "%8c.%4c %s\n", resID, resType, name);
            res.ResID = StringUtil::strToRes32(resID);
        }
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

        if (game == MP3Proto)
        {
            sscanf(line, "%16c.%4c\n", resID, resType);
            res.ResID64 = StringUtil::strToRes64(resID);
        }
        else
        {
            sscanf(line, "%8c.%4c\n", resID, resType);
            res.ResID = StringUtil::strToRes32(resID);
        }
        res.ResType = resType;

        ToC.push_back(res);
    }
}

void CPrimePak::Repack(COutputStream& pak, std::string FileDir, EWhichGame game, ECompressMode CompressMode)
{
    if (!pak.IsValid()) return;
    StringUtil::ensureEndsWithSlash(FileDir);

    // Header
    pak.WriteLong(0x00030005);
    pak.WriteLong(0);

    // Named Resources
    pak.WriteLong(NamedResources.size());
    for (u32 n = 0; n < NamedResources.size(); n++)
    {
        SNamedResource *res = &NamedResources[n];
        pak.WriteLong(res->ResType.asLong());

        if (game == MP3Proto)
            pak.WriteLongLong(res->ResID64);
        else
            pak.WriteLong(res->ResID);

        pak.WriteLong(res->Name.size());
        pak.WriteString(res->Name, res->Name.size());
    }

    // Resource Table
    pak.WriteLong(ToC.size());
    u32 tocStart = pak.Tell();

    // For now we just fill the resource table with 0s.
    // We can't write it yet because we don't know the file sizes and offsets yet.
    for (u32 r = 0; r < ToC.size(); r++)
    {
        if (game == MP3Proto)
            for (u32 v = 0; v < 6; v++)
                pak.WriteLong(0);
        else
            for (u32 v = 0; v < 5; v++)
                pak.WriteLong(0);
    }
    pak.WriteToBoundary(32, 0);

    // Resource Data
    for (u32 r = 0; r < ToC.size(); r++)
    {
        SResource *res = &ToC[r];
        res->ResOffset = pak.Tell();
        std::string filename = (game == MP3Proto ? StringUtil::resToStr((long long) res->ResID64) : StringUtil::resToStr((long) res->ResID));
        filename += "." + res->ResType.asString();

        if (!(r & 0x1F  ) || (r == ToC.size() - 1))
            std::cout << "\rRepacking file " << std::dec << r + 1 << " of " << ToC.size() << " - " << filename;

        CFileInStream file(FileDir + filename);

        if (!file.IsValid())
        {
            std::cout << "\rError: Unable to open resource: " << filename << "\n";
            continue;
        }

        std::vector<u8> buf;

        // Compression
        if ((CompressMode != NoCompression) && (ShouldCompress(res->ResType, file.Size(), game)))
        {
            buf.resize(file.Size() * 2);
            CMemoryOutStream mem(buf.data(), buf.size(), IOUtil::BigEndian);
            bool success = Compress(file, mem, game);

            if (!success)
                std::cout << "\rError: Unable to compress resource: " << filename << "\n";

            if ((success) && (mem.SpaceUsed() < file.Size()))
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
        pak.WriteToBoundary(32, 0xFF);
        res->ResSize = pak.Tell() - res->ResOffset;
    }

    // Now we actually write the resource table, then we're done
    pak.Seek(tocStart, SEEK_SET);
    for (u32 r = 0; r < ToC.size(); r++)
    {
        SResource *res = &ToC[r];
        pak.WriteLong(res->Compressed ? 1 : 0);
        pak.WriteLong(res->ResType.asLong());

        if (game == MP3Proto)
            pak.WriteLongLong(res->ResID64);
        else
            pak.WriteLong(res->ResID);

        pak.WriteLong(res->ResSize);
        pak.WriteLong(res->ResOffset);
    }
}

u32 CPrimePak::GetResourceCount()
{
    return ToC.size();
}

u32 CPrimePak::GetUniqueResourceCount()
{
    return ExtractionQueue.size();
}

u32 CPrimePak::GetNamedResourceCount()
{
    return NamedResources.size();
}

bool CPrimePak::Decompress(CInputStream& input, SResource& res, std::vector<u8>& out)
{
    if (!input.IsValid()) return false;

    u32 decmp = input.ReadLong();
    u16 type = input.PeekShort();
    out.resize(decmp);

    if ((type == 0x78DA) || (type == 0x7801) || (type == 0x789C)) // zlib
    {
        u32 total_out;

        std::vector<u8> in(res.ResSize);
        input.ReadBytes(in.data(), in.size());

        return DecompressZlib(in.data(), in.size(), out.data(), out.size(), total_out);
    }

    else // segmented lzo
    {
        CMemoryOutStream output(out.data(), out.size(), IOUtil::BigEndian);
        return DecompressSegmented(input, res.ResSize, output, output.Size());
    }
}

bool CPrimePak::Compress(CInputStream& src, COutputStream& dst, EWhichGame game)
{
    if (!src.IsValid()) return false;
    if (!dst.IsValid()) return false;

    u32 DecmpSize = src.Size();
    std::vector<u8> decmp(DecmpSize);
    src.ReadBytes(decmp.data(), decmp.size());

    u32 CmpSize;
    std::vector<u8> cmp(src.Size() * 2);

    bool success;
    if (game == MP1 || game == MP2Demo)
        success = CompressZlib(decmp.data(), decmp.size(), cmp.data(), cmp.size(), CmpSize);
    else
        success = CompressLZOSegmented(decmp.data(), decmp.size(), cmp.data(), cmp.size(), CmpSize, false);

    if (!success) return false;

    dst.WriteLong(DecmpSize);
    dst.WriteBytes(cmp.data(), CmpSize);
    return true;
}

bool CPrimePak::ShouldCompress(CFourCC ResType, u32 ResSize, EWhichGame game)
{
  u32 cutoff = (game == MP3Proto) ? 0x60 : 0x400;

  if (ResType == "TXTR") return true;
  if (ResType == "CMDL") return true;
  if (ResType == "CSKR") return true;
  if (ResType == "ANCS") return true;
  if (ResType == "ANIM") return true;
  if (ResType == "FONT") return true;
  if ((ResType == "PART") && (ResSize >= cutoff)) return true;
  if ((ResType == "ELSC") && (ResSize >= cutoff)) return true;
  if ((ResType == "SWHC") && (ResSize >= cutoff)) return true;
  if ((ResType == "DPSC") && (ResSize >= cutoff)) return true;
  if ((ResType == "WPSC") && (ResSize >= cutoff)) return true;
  if ((ResType == "CRSC") && (ResSize >= cutoff)) return true;
  return false;
}

bool ResSortByOffset(CPrimePak::SResource* left, CPrimePak::SResource* right)
{
    return (left->ResOffset < right->ResOffset);
}

bool ResSortByID32(CPrimePak::SResource* left, CPrimePak::SResource* right)
{
    return (left->ResID < right->ResID);
}

bool ResSortByID64(CPrimePak::SResource* left, CPrimePak::SResource* right)
{
    return (left->ResID64 < right->ResID64);
}

bool ResCompare32(CPrimePak::SResource* left, CPrimePak::SResource* right)
{
    return ((left->ResID == right->ResID) && (left->ResType == right->ResType));
}

bool ResCompare64(CPrimePak::SResource* left, CPrimePak::SResource* right)
{
    return ((left->ResID64 == right->ResID64) && (left->ResType == right->ResType));
}
