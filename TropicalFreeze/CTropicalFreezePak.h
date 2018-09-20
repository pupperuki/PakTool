#ifndef CTROPICALFREEZEPAK_H
#define CTROPICALFREEZEPAK_H

#include <string>
#include <FileIO/FileIO.h>

#include "SRFRMHeader.h"
#include "SSectionHeader.h"
#include "SFileID_128.h"

class CTropicalFreezePak
{
    struct SDirEntry
    {
        CFourCC ResType;
        SFileID_128 ID;
        u64 Offset;
        u64 Size;
    };
    struct SMetaEntry
    {
        SFileID_128 ID;
        u32 size;
        char *data;
    };
    struct SStringEntry
    {
        CFourCC ResType;
        SFileID_128 ID;
        std::string String;
    };

    std::vector<SDirEntry> DirEntries;
    std::vector<SMetaEntry> MetaEntries;
    std::vector<SStringEntry> StringEntries;

    CInputStream *PakFile;
    void ParseADIR(CInputStream& pak);
    void ParseMETA(CInputStream& pak);
    void ParseSTRG(CInputStream& pak);

    bool DecompressModel(CInputStream& txtr, COutputStream& out, const SMetaEntry& MetaData);
    bool DecompressTXTR(CInputStream& txtr, COutputStream& out, const SMetaEntry& MetaData);
    bool DecompressMTRL(CInputStream& txtr, COutputStream& out, const SMetaEntry& MetaData);

public:
    CTropicalFreezePak();
    CTropicalFreezePak(CInputStream& pak);
    void Extract(std::string OutputDir, bool handleEmbeddedCompression);
    void DumpList(std::string OutputFile);
    u32 GetDirEntryCount();
    u32 GetMetaEntryCount();
    u32 GetStringEntryCount();
};

#endif // CTROPICALFREEZEPAK_H
