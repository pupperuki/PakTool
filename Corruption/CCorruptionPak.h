#ifndef CCORRUPTIONPAK_H
#define CCORRUPTIONPAK_H

#include "../CFourCC.h"
#include "../PakEnum.h"
#include "../types.h"
#include <FileIO/FileIO.h>

class CCorruptionPak
{
    struct SNamedResource
    {
        std::string Name;
        CFourCC ResType;
        u64 ResID;
    };

    struct SResource
    {
        bool Compressed;
        CFourCC ResType;
        u64 ResID;
        u32 ResSize;
        u32 ResOffset;
    };

    std::vector<SNamedResource> NamedResources;
    std::vector<SResource> ToC;
    std::vector<SResource*> ExtractionQueue;

    CInputStream *PakFile;
    u32 StringOffset, TocOffset, DataOffset;

    bool Decompress(CInputStream& input, SResource& res, std::vector<u8>& out);
    bool Compress(CInputStream& input, COutputStream& output, SResource& res, EWhichGame game);
    bool ShouldCompress(CFourCC ResType, u32 ResSize);
    friend bool ResSortByOffset(CCorruptionPak::SResource* left, CCorruptionPak::SResource* right);
    friend bool ResSortByID(CCorruptionPak::SResource* left, CCorruptionPak::SResource* right);
    friend bool ResCompare(CCorruptionPak::SResource* left, CCorruptionPak::SResource* right);

public:
    CCorruptionPak();
    CCorruptionPak(CInputStream& pak);
    void Extract(std::string OutputDir, bool decompressMREA);
    void DumpList(std::string OutputFile);
    void BuildTOC(FILE *list, EWhichGame game);
    void Repack(CFileOutStream& pak, std::string FileDir, EWhichGame game, ECompressMode CompressMode);
    u32 GetResourceCount();
    u32 GetUniqueResourceCount();
    u32 GetNamedResourceCount();
    static void HashMD5(CInputStream& pak, unsigned char *out);
};

#endif // CCORRUPTIONPAK_H
