#ifndef CPRIMEPAK_H
#define CPRIMEPAK_H

#include "../PakEnum.h"
#include "../CFourCC.h"
#include "../types.h"
#include <FileIO/FileIO.h>

class CPrimePak
{
    struct SNamedResource
    {
        CFourCC ResType;
        union {
            u32 ResID;
            u64 ResID64;
        };
        std::string Name;
    };
    struct SResource
    {
        bool Compressed;
        CFourCC ResType;
        union {
            u32 ResID;
            u64 ResID64;
        };
        u32 ResSize;
        u32 ResOffset;
    };
    std::vector<SNamedResource> NamedResources;
    std::vector<SResource> ToC;
    std::vector<SResource*> ExtractionQueue;

    CInputStream *PakFile;
    EPakVersion version;

    bool Decompress(CInputStream& input, SResource& res, std::vector<u8>& out);
    bool Compress(CInputStream& src, COutputStream& dst, EWhichGame game);
    bool ShouldCompress(CFourCC ResType, u32 ResSize, EWhichGame game);

    friend bool ResSortByOffset(CPrimePak::SResource* left, CPrimePak::SResource* right);
    friend bool ResSortByID32(CPrimePak::SResource* left, CPrimePak::SResource* right);
    friend bool ResSortByID64(CPrimePak::SResource* left, CPrimePak::SResource* right);
    friend bool ResCompare32(CPrimePak::SResource* left, CPrimePak::SResource* right);
    friend bool ResCompare64(CPrimePak::SResource* left, CPrimePak::SResource* right);

public:
    CPrimePak();
    CPrimePak(CInputStream& pak);
    void Extract(std::string OutputDir, bool decompressMREA);
    void DumpList(std::string OutputFile);
    void BuildTOC(FILE *list, EWhichGame game);
    void Repack(COutputStream& pak, std::string FileDir, EWhichGame game, ECompressMode CompressMode);
    u32 GetResourceCount();
    u32 GetUniqueResourceCount();
    u32 GetNamedResourceCount();
};

#endif // CPRIMEPAK_H
