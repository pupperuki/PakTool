#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "types.h"
#include <FileIO/FileIO.h>

bool DecompressZlib(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out);
bool DecompressLZO(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out);
bool DecompressSegmented(CInputStream& src, u32 src_len, CMemoryOutStream& dst, u32 dst_len);

bool CompressZlib(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out);
bool CompressLZO(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out);
bool CompressZlibSegmented(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out, bool ensureSmaller);
bool CompressLZOSegmented(u8 *src, u32 src_len, u8 *dst, u32 dst_len, u32& total_out, bool ensureSmaller);

#endif // COMPRESSION_H
