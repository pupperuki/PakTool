#ifndef PAKENUM_H
#define PAKENUM_H

enum EPakVersion
{
    PrimePak = 0x00030005,
    CorruptionProtoPak,
    CorruptionPak = 0x00000002,
    TropicalFreezePak = 0x5246524D
};

enum EWhichGame
{
    InvalidGame = 0,
    MP1, MP2, MP3, DKCR, DKCTF,
    MP1Demo, MP2Demo, MP3Proto,
    Trilogy
};

enum ECompressMode
{
    NoCompression,
    Compress,
    CompressEmbedded
};

#endif // PAKENUM_H
