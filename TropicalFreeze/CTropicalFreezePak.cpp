#include <iostream>
#include "CTropicalFreezePak.h"
#include "DecompressLZSS.h"
#include "../StringUtil.h"
#include "../Compression.h"

CTropicalFreezePak::CTropicalFreezePak()
{
}

CTropicalFreezePak::CTropicalFreezePak(CInputStream& pak)
{
    SRFRMHeader PACK = SRFRMHeader(pak);
    SRFRMHeader TOCC = SRFRMHeader(pak);

    // Three sections can be present - ADIR (should always be there), META, STRG
    for (u32 s = 0; s < 3; s++)
    {
        CFourCC peek = pak.PeekLong();

        if (peek == "ADIR")
            ParseADIR(pak);
        else if (peek == "META")
            ParseMETA(pak);
        else if (peek == "STRG")
            ParseSTRG(pak);
        else
            break;
    }

    PakFile = &pak;
}

void CTropicalFreezePak::ParseADIR(CInputStream& pak)
{
    SSectionHeader ADIR(pak);

    u32 DirEntryCount = pak.ReadLong();
    DirEntries.reserve(DirEntryCount);

    for (u32 d = 0; d < DirEntryCount; d++)
    {
        SDirEntry entry;
        entry.ResType = CFourCC(pak);
        entry.ID = SFileID_128(pak);
        entry.Offset = pak.ReadLongLong();
        entry.Size = pak.ReadLongLong();
        DirEntries.push_back(entry);
    }
}

void CTropicalFreezePak::ParseMETA(CInputStream& pak)
{
    SSectionHeader META(pak);

    u32 MetaEntryCount = pak.ReadLong();
    MetaEntries.reserve(MetaEntryCount);

    for (u32 m = 0; m < MetaEntryCount; m++)
    {
        SMetaEntry entry;
        entry.ID = SFileID_128(pak);
        pak.Seek(0x4, SEEK_CUR);
        MetaEntries.push_back(entry);
    }

    // Metadata structure is different between different resource types.
    // It's better to copy it into a buffer and handle it on a case by case basis.
    for (u32 m = 0; m < MetaEntryCount; m++)
    {
        MetaEntries[m].size = pak.ReadLong();
        MetaEntries[m].data = new char[MetaEntries[m].size];
        pak.ReadBytes(MetaEntries[m].data, MetaEntries[m].size);
    }
}

void CTropicalFreezePak::ParseSTRG(CInputStream &pak)
{
    SSectionHeader STRG(pak);

    u32 StringEntryCount = pak.ReadLong();
    StringEntries.reserve(StringEntryCount);

    for (u32 s = 0; s < StringEntryCount; s++)
    {
        SStringEntry entry;
        entry.ResType = CFourCC(pak);
        entry.ID = SFileID_128(pak);
        entry.String = pak.ReadString();
        StringEntries.push_back(entry);
    }
}

void CTropicalFreezePak::Extract(std::string OutputDir, bool handleEmbeddedCompression)
{
    if (DirEntries.empty())
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
    u32 MetaSearch = 0;

    for (u32 d = 0; d < DirEntries.size(); d++)
    {
        SDirEntry *e = &DirEntries[d];
        std::string OutName = StringUtil::resToStr(e->ID) + "." + e->ResType.asString();

        // Printing output is useful for the user, but we don't want to do it too much,
        // or else it'll have a negative and noticeable impact on performance.
        if (!(d & 0x1F) || (d == DirEntries.size() - 1))
            std::cout << "\rExtracting file " << std::dec << d + 1 << " of " << DirEntries.size() << " - " << OutName;

        PakFile->Seek((u32) e->Offset, SEEK_SET);

        std::vector<u8> buf((u32) e->Size);
        PakFile->ReadBytes(buf.data(), buf.size());

        CFileOutStream out(OutputDir + StringUtil::resToStr(e->ID) + "." + e->ResType.asString(), IOUtil::BigEndian);
        bool WroteWithMeta = false;
        // Paks have some metadata for some files. Sometimes this metadata is actually important.
        // Therefore if the resource has metadata we'll append it to the end of the file.
        if (!MetaEntries.empty())
        {
            for (u32 m = MetaSearch; m < MetaEntries.size(); m++)
            {
                // Entries are always organized in numerical order by file ID in Tropical Freeze.
                // I'm taking advantage of that to speed up lookup times.
                if (MetaEntries[m].ID == e->ID)
                {
                    if (handleEmbeddedCompression)
                    {
                        CMemoryInStream in(buf.data(), buf.size(), IOUtil::BigEndian);

                        if ((e->ResType == "CMDL") || (e->ResType == "SMDL") || (e->ResType == "WMDL"))
                        {
                            WroteWithMeta = DecompressModel(in, out, MetaEntries[m]);
                            if (!WroteWithMeta) std::cout << "\rError: Unable to decompress model: " << OutName << "\n";
                        }
                        else if (e->ResType == "TXTR")
                        {
                            WroteWithMeta = DecompressTXTR(in, out, MetaEntries[m]);
                            if (!WroteWithMeta) std::cout << "\rError: Unable to decompress TXTR: " << OutName << "\n";
                        }
                        else if (e->ResType == "MTRL")
                        {
                            WroteWithMeta = DecompressMTRL(in, out, MetaEntries[m]);
                            if (!WroteWithMeta) std::cout << "\rError: Unable to decompress MTRL: " << OutName << "\n";
                        }
                    }

                    if (!WroteWithMeta)
                    {
                        out.WriteBytes(buf.data(), buf.size());

                        SSectionHeader META;
                        META.type = "META";
                        META.size = MetaEntries[m].size;
                        META.Write(out);
                        out.WriteBytes(MetaEntries[m].data, MetaEntries[m].size);
                        MetaSearch = m;
                        WroteWithMeta = true;
                        break;
                    }
                }

                if (MetaEntries[m].ID > e->ID)
                    break;
            }
        }

        if (!WroteWithMeta)
            out.WriteBytes(buf.data(), buf.size());
    }
    std::cout << "\n";
}

void CTropicalFreezePak::DumpList(std::string OutputFile)
{
    if (DirEntries.empty() && StringEntries.empty())
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
    for (u32 s = 0; s < StringEntries.size(); s++)
    {
        SStringEntry *strg = &StringEntries[s];
        fprintf(f, "%s.%s %s\n", StringUtil::resToStr(strg->ID).c_str(), strg->ResType.asString().c_str(), strg->String.c_str());
    }
    fprintf(f, "\n");

    // Dump file list
    for (u32 d = 0; d < DirEntries.size(); d++)
    {
        SDirEntry *dir = &DirEntries[d];
        fprintf(f, "%s.%s\n", StringUtil::resToStr(dir->ID).c_str(), dir->ResType.asString().c_str());
    }
}

bool CTropicalFreezePak::DecompressModel(CInputStream& mdl, COutputStream& out, const SMetaEntry& MetaData)
{
    if (!mdl.IsValid()) return false;
    if (!out.IsValid()) return false;

    CMemoryInStream meta(MetaData.data, MetaData.size, IOUtil::BigEndian);

    meta.Seek(0x4, SEEK_CUR);
    u32 GPU_offset = meta.ReadLong();

    u32 GPU_split_count = meta.ReadLong();
    u32 compressed_size = 0;

    for (u32 s = 0; s < GPU_split_count; s++)
    {
        compressed_size += meta.ReadLong();
        meta.Seek(0x4, SEEK_CUR);
    }

    struct SModelMetaBuffer
    {
        u32 split;
        u32 offset;
        u32 compressed_size;
        u32 decompressed_size;
    };

    // Vertex buffers + index buffers are listed in the metadata; each has their own compressed buffer
    u32 decompressed_total = 0;
    u32 VertexBufferCount = meta.ReadLong();
    std::vector<SModelMetaBuffer> VertexBuffers;
    VertexBuffers.reserve(VertexBufferCount);

    for (u32 v = 0; v < VertexBufferCount; v++)
    {
        SModelMetaBuffer vb;
        vb.split = meta.ReadLong();
        vb.offset = meta.ReadLong();
        vb.compressed_size = meta.ReadLong();
        vb.decompressed_size = meta.ReadLong();
        decompressed_total += vb.decompressed_size;
        VertexBuffers.push_back(vb);
    }

    u32 IndexBufferCount = meta.ReadLong();
    std::vector<SModelMetaBuffer> IndexBuffers;
    IndexBuffers.reserve(IndexBufferCount);

    for (u32 i = 0; i < IndexBufferCount; i++)
    {
        SModelMetaBuffer ib;
        ib.split = meta.ReadLong();
        ib.offset = meta.ReadLong();
        ib.compressed_size = meta.ReadLong();
        ib.decompressed_size = meta.ReadLong();
        decompressed_total += ib.decompressed_size;
        IndexBuffers.push_back(ib);
    }

    // Done reading the metadata - next step is to actually decompress the data
    std::vector<u8> file_start_buffer(GPU_offset);
    mdl.ReadBytes(file_start_buffer.data(), file_start_buffer.size());

    SSectionHeader GPU(mdl);
    std::vector<u8> GPUData((u32) GPU.size);
    mdl.ReadBytes(GPUData.data(), GPUData.size());

    // Decompress each buffer
    u8 *src = GPUData.data();

    u32 extra = 8 + (4 * VertexBufferCount) + (4 * IndexBufferCount);
    CVectorOutStream GPUDst(decompressed_total + extra, IOUtil::BigEndian);

    GPUDst.WriteLong(VertexBufferCount);
    for (u32 vb = 0; vb < VertexBufferCount; vb++)
    {
        SModelMetaBuffer *b = &VertexBuffers[vb];
        GPUDst.WriteLong(b->decompressed_size);

        bool success = DecompressLZSS(src, b->compressed_size, (u8*) GPUDst.DataAtPosition(), b->decompressed_size);
        if (!success) return false;

        src += b->compressed_size;
        GPUDst.Seek(b->decompressed_size, SEEK_CUR);
    }

    GPUDst.WriteLong(IndexBufferCount);
    for (u32 ib = 0; ib < IndexBufferCount; ib++)
    {
        SModelMetaBuffer *b = &IndexBuffers[ib];
        GPUDst.WriteLong(b->decompressed_size);

        bool success = DecompressLZSS(src, b->compressed_size, (u8*) GPUDst.DataAtPosition(), b->decompressed_size);
        if (!success) return false;

        src += b->compressed_size;
        GPUDst.Seek(b->decompressed_size, SEEK_CUR);
    }

    // Data is decompressed - now write the output file
    out.WriteBytes(file_start_buffer.data(), file_start_buffer.size());
    GPU.size = GPUDst.Size();
    GPU.Write(out);
    out.WriteBytes(GPUDst.Data(), GPUDst.Size());
    u64 newRFRMsize = out.Tell() - 0x20;
    out.Seek(0x4, SEEK_SET);
    out.WriteLongLong(newRFRMsize);

    return true;
}

bool CTropicalFreezePak::DecompressTXTR(CInputStream& txtr, COutputStream& out, const SMetaEntry& MetaData)
{
    if (!txtr.IsValid()) return false;
    if (!out.IsValid()) return false;

    CMemoryInStream meta(MetaData.data, MetaData.size, IOUtil::BigEndian);

    meta.Seek(0x8, SEEK_CUR);
    u32 GPU_offset = meta.ReadLong();
    meta.Seek(0xC, SEEK_CUR);
    u32 compressed_buffer_count = meta.ReadLong(); // ? not totally sure

    if (compressed_buffer_count != 1) return false;

    u32 decompressed_size = meta.ReadLong();
    u32 compressed_size = meta.ReadLong();

    std::vector<u8> file_start_buffer(GPU_offset);
    txtr.ReadBytes(file_start_buffer.data(), file_start_buffer.size());

    SSectionHeader GPU(txtr);
    std::vector<u8> GPUdata((u32) GPU.size);
    txtr.ReadBytes(GPUdata.data(), GPUdata.size());

    // Decompress
    std::vector<u8> GPUdst(decompressed_size);
    u8 *src = GPUdata.data();
    u8 *dst = GPUdst.data();

    bool success = DecompressLZSS(src, compressed_size, dst, decompressed_size);
    if (!success) return false;

    // Write output file
    out.WriteBytes(file_start_buffer.data(), file_start_buffer.size());
    GPU.size = decompressed_size;
    GPU.Write(out);
    out.WriteBytes(GPUdst.data(), GPUdst.size());
    u64 newRFRMsize = out.Tell() - 0x20;
    out.Seek(0x4, SEEK_SET);
    out.WriteLongLong(newRFRMsize);

    return true;
}

bool CTropicalFreezePak::DecompressMTRL(CInputStream& mtrl, COutputStream& out, const SMetaEntry& MetaData)
{
    if (!mtrl.IsValid()) return false;
    if (!out.IsValid()) return false;

    CMemoryInStream meta(MetaData.data, MetaData.size, IOUtil::BigEndian);

    meta.Seek(0x8, SEEK_CUR);
    u32 CmpSize = meta.ReadLong();
    u32 DecmpSize = meta.ReadLong();
    meta.Seek(0x4, SEEK_CUR);

    // Compressed size of 0 usually denotes uncompressed file
    // Don't know if this can happen with MTRL so I'm putting this here just in case
    if (!CmpSize) return false;

    SRFRMHeader RFRM(mtrl);
    std::vector<u8> compressed_buf(CmpSize);
    std::vector<u8> decompressed_buf(DecmpSize);
    mtrl.ReadBytes(compressed_buf.data(), compressed_buf.size());

    u32 total_out;
    bool success = DecompressZlib(compressed_buf.data(), compressed_buf.size(), decompressed_buf.data(), decompressed_buf.size(), total_out);
    if (!success) return false;

    // Write output file
    RFRM.size = DecmpSize;
    RFRM.Write(out);
    out.WriteBytes(decompressed_buf.data(), decompressed_buf.size());

    return true;
}

u32 CTropicalFreezePak::GetDirEntryCount()
{
    return DirEntries.size();
}

u32 CTropicalFreezePak::GetMetaEntryCount()
{
    return MetaEntries.size();
}

u32 CTropicalFreezePak::GetStringEntryCount()
{
    return StringEntries.size();
}
