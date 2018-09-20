#include <iostream>
#include <vector>
#include <zlib.h>
#include <lzo/lzo1x.h>
#include "Compression.h"

// Metroid Prime: zlib
// Metroid Prime 2: Segmented LZO1X-999 (pak + MREA)
// Metroid Prime 3: Segmented LZO1X-999 (pak + MREA)
// DKCR: zlib (pak) and segmented zlib (MREA)
// DKCTF: zlib (MTRL) and LZSS (CMDL/SMDL/WMDL/TXTR); see TropicalFreeze/DecompressLZSS.cpp

bool DecompressZlib(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out)
{
    z_stream z;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    z.avail_in = src_len;
    z.next_in = src;
    z.avail_out = dst_len;
    z.next_out = dst;

    s32 ret = inflateInit(&z);

    if (ret == Z_OK)
    {
        ret = inflate(&z, Z_NO_FLUSH);

        if ((ret == Z_OK) || (ret == Z_STREAM_END))
            ret = inflateEnd(&z);

        total_out = z.total_out;
    }

    if ((ret != Z_OK) && (ret != Z_STREAM_END)) {
        std::cout << "\rzlib error: " << std::dec << ret << "\n";
        return false;
    }

    else return true;
}

bool DecompressLZO(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out)
{
    lzo_init();

    s32 error = lzo1x_decompress(src, src_len, dst, &total_out, LZO1X_MEM_DECOMPRESS);
    if (error) {
        std::cout << "\rlzo1x error: " << std::dec << error << "\n";
        return false;
    }

    return true;
}

bool DecompressSegmented(CInputStream& src, u32 src_len, CMemoryOutStream& dst, u32 dst_len)
{
    u32 dst_end = dst.Tell() + dst_len;

    while (dst.Tell() < dst_end)
    {
        s16 size = src.ReadShort();
        u16 type = src.PeekShort();
        u32 total_out;

        if (size < 0)
        {
            size = -size;
            std::vector<u8> buf(size);
            src.ReadBytes(buf.data(),buf.size());
            dst.WriteBytes(buf.data(), buf.size());
            continue;
        }

        else
        {
            std::vector<u8> buf(size);
            src.ReadBytes(buf.data(),buf.size());

            if ((type == 0x78DA) || (type == 0x789C) || (type == 0x7801))
            {
                bool success = DecompressZlib(buf.data(), buf.size(), (u8*) dst.DataAtPosition(), dst_len, total_out);
                if (!success) return false;
            }

            else
            {
                bool success = DecompressLZO(buf.data(), buf.size(), (u8*) dst.DataAtPosition(), dst_len, total_out);
                if (!success) return false;
            }
            dst.Seek(total_out, SEEK_CUR);
        }
    }

    return true;
}

bool CompressZlib(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out)
{
    z_stream z;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    z.avail_in = src_len;
    z.next_in = src;
    z.avail_out = dst_len;
    z.next_out = dst;

    s32 ret = deflateInit(&z, 9);

    if (!ret)
    {
        ret = deflate(&z, Z_FINISH);

        if ((ret == Z_OK) || (ret == Z_STREAM_END))
            ret = deflateEnd(&z);
    }

    if ((ret != Z_OK) && (ret != Z_STREAM_END))
    {
        std::cout << "\rzlib error: " << std::dec << ret << "\n";
        total_out = 0;
        return false;
    }

    total_out = z.total_out;
    return true;
}

bool CompressLZO(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out)
{
    lzo_init();

    u8 *workmem = (u8*) malloc(LZO1X_999_MEM_COMPRESS);
    s32 error = lzo1x_999_compress(src, src_len, dst, &total_out, workmem);
    free(workmem);

    if (error) {
        std::cout << "\rlzo1x error: " << std::dec << error << "\n";
        return false;
    }

    return true;
}

bool CompressZlibSegmented(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out, bool ensureSmaller)
{
    u8 *src_end = src + src_len;
    u8 *dst_start = dst;

    while (src < src_end)
    {
        // Each segment compressed separately. Segment size should always be 0x4000 unless there aren't that many bytes left.
        u16 size;
        u32 remaining = src_end - src;

        if (remaining < 0x4000) size = remaining;
        else size = 0x4000;

        std::vector<u8> cmp(size * 2);
        u32 out;

        CompressZlib(src, size, cmp.data(), cmp.size(), out);

        // In MREA files you will want to verify that the compressed data is actually smaller. If it is you leave it uncompressed (denoted by negative size value).
        // paks don't do this check. In paks all segments are compressed regardless.
        if ((ensureSmaller) && (out >= size))
        {
            // Write negative size value to destination (which signifies uncompressed)
            *dst++ = -size >> 8;
            *dst++ = -size & 0xFF;

            // Write original uncompressed data to destination
            memcpy(dst, src, size);
            out = size;
        }
        else
        {
            // Write new compressed size + data to destination
            *dst++ = (out >> 8) & 0xFF;
            *dst++ = (out & 0xFF);
            memcpy(dst, cmp.data(), out);
        }

        src += size;
        dst += out;
    }

    total_out = dst - dst_start;
    return true;
}

bool CompressLZOSegmented(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out, bool ensureSmaller)
{
    u8 *src_end = src + src_len;
    u8 *dst_start = dst;

    while (src < src_end)
    {
        // Each segment compressed separately. Segment size should always be 0x4000 unless there aren't that many bytes left.
        s16 size;
        u32 remaining = src_end - src;

        if (remaining < 0x4000) size = remaining;
        else size = 0x4000;

        std::vector<u8> cmp(size * 2);
        u32 out;

        CompressLZO(src, size, cmp.data(), cmp.size(), out);

        // In MREA files you will want to verify that the compressed data is actually smaller. If it is you leave it uncompressed (denoted by negative size value).
        // paks don't do this check. In paks all segments are compressed regardless.
        if ((ensureSmaller) && (out >= size))
        {
            // Write negative size value to destination (which signifies uncompressed)
            *dst++ = -size >> 8;
            *dst++ = -size & 0xFF;

            // Write original uncompressed data to destination
            memcpy(dst, src, size);
            out = size;
        }
        else
        {
            // Write new compressed size + data to destination
            *dst++ = (out >> 8) & 0xFF;
            *dst++ = (out & 0xFF);
            memcpy(dst, cmp.data(), out);
        }

        src += size;
        dst += out;
    }

    total_out = dst - dst_start;
    return true;
}
