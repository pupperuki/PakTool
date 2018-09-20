#ifndef SFILEID_128_H
#define SFILEID_128_H

#include <FileIO/FileIO.h>
#include "../types.h"

struct SFileID_128
{
    u64 a;
    u64 b;

    SFileID_128() { a = b = 0; }

    SFileID_128(CInputStream& input)
    {
        a = input.ReadLongLong();
        b = input.ReadLongLong();
    }

    inline void Write(COutputStream& output)
    {
        output.WriteLongLong(a);
        output.WriteLongLong(b);
    }
};

inline bool operator== (const SFileID_128& L, const SFileID_128& R) { return ((L.a == R.a) && (L.b == R.b)); }
inline bool operator<(const SFileID_128& L, const SFileID_128& R)
{
    if (L.a == R.a) return L.b < R.b;
    else return L.a < R.a;
}
inline bool operator>(const SFileID_128& L, const SFileID_128& R) { return (R < L); }
inline bool operator>=(const SFileID_128& L, const SFileID_128& R) { return !(L < R); }
inline bool operator<=(const SFileID_128& L, const SFileID_128& R) { return !(L > R); }

#endif // SFILEID_128_H
