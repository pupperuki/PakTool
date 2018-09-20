#ifndef DECOMPRESSMREA_H
#define DECOMPRESSMREA_H

#include <FileIO/FileIO.h>
#include <vector>
#include "types.h"

bool DecompressMREA(CInputStream& input, u32 size, std::vector<u8>& out);
bool CompressMREA(CInputStream& input, COutputStream& output, u32 size);

#endif // DECOMPRESSMREA_H
