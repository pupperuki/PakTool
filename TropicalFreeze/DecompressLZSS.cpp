#include "../types.h"
#include <cstring>

// LZSS compression is used in Tropical Freeze to compress GPU buffer data in models and textures.
//
// The mode parameter refers to the first byte of the compressed data. The possible modes are as follows:
// 0 - Uncompressed
// 1 - 8-bit
// 2 - 16-bit
// 3 - 32-bit
//
// Tropical Freeze has separate decompression functions for each mode, but I've decided to write one
// function to handle all three.

bool DecompressLZSS(u8 *src, u32 src_len, u8 *dst, u32 dst_len)
{
    u8 *src_end = src + src_len;
    u8 *dst_end = dst + dst_len;

    // Read compressed buffer header
    u8 mode = *src++;
    src += 3; // Skipping unused bytes

    // Check for invalid mode set:
    if (mode > 3) return false;

    // If mode is 0, then we have uncompressed data.
    if (mode == 0) {
        memcpy(dst, src, dst_len);
        return true;
    }

    // Otherwise, start preparing for decompression.
    u8 header_byte = 0;
    u8 group = 0;

    while ((src < src_end) && (dst < dst_end))
    {
        // group will start at 8 and decrease by 1 with each data chunk read.
        // When group reaches 0, we read a new header byte and reset it to 8.
        if (!group)
        {
            header_byte = *src++;
            group = 8;
        }

        // header_byte will be shifted left one bit for every data group read, so 0x80 always corresponds to the current data group.
        // If 0x80 is set, then we read back from the decompressed buffer.
        if (header_byte & 0x80)
        {
            u8 bytes[2] = { *src++, *src++ };
            u32 count, length;

            // The exact way to calculate count and length varies depending on which mode is set:
            switch (mode) {
            case 1:
                count = (bytes[0] >> 4) + 3;
                length = ((bytes[0] & 0xF) << 0x8) | bytes[1];
                break;
            case 2:
                count = (bytes[0] >> 4) + 2;
                length = (((bytes[0] & 0xF) << 0x8) | bytes[1]) << 1;
                break;
            case 3:
                count = (bytes[0] >> 4) + 1;
                length = (((bytes[0] & 0xF) << 0x8) | bytes[1]) << 2;
                break;
            }

            // With the count and length calculated, we'll set a pointer to where we want to read back data from:
            u8 *seek = dst - length;

            // count refers to how many byte groups to read back; the size of one byte group varies depending on mode
            for (u32 byte = 0; byte < count; byte++)
            {
                switch (mode) {
                case 1:
                    *dst++ = *seek++;
                    break;
                case 2:
                    for (u32 b = 0; b < 2; b++)
                        *dst++ = *seek++;
                    break;
                case 3:
                    for (u32 b = 0; b < 4; b++)
                        *dst++ = *seek++;
                    break;
                }
            }
        }

        // If 0x80 is not set, then we read one byte group directly from the compressed buffer.
        else
        {
            switch (mode) {
            case 1:
                *dst++ = *src++;
                break;
            case 2:
                for (u32 b = 0; b < 2; b++)
                    *dst++ = *src++;
                break;
            case 3:
                for (u32 b = 0; b < 4; b++)
                    *dst++ = *src++;
                break;
            }
        }

        header_byte <<= 1;
        group--;
    }

    // We've finished decompressing; the last thing to do is check that we've reached the end of both buffers, to verify everything has decompressed correctly.
    return ((src == src_end) && (dst == dst_end));
}

