#ifndef GORILLA_COMPRESSOR_HPP
#define GORILLA_COMPRESSOR_HPP

#include <vector>
#include <memory>
#include <bitset>

#include <string>

const int MAX_BIT_ALLOC = (4 + 32) * 120;

/*

Allocate for the worst case...
2 hour block = 
    Tag Bits   = 4
    Value Bits = 32
               = 36

               * 120 (block minutes) = 4320 bits worst case
               (best case = 120...)

*/

class GorillaCompressor {
public:
    GorillaCompressor(std::shared_ptr<std::vector<std::pair<uint64_t, float>>>);
    ~GorillaCompressor();

    void WriteToStream(int writeBits, int significantBits);
    int ExtractDelta(int currentOffset, int significantBits);
    void Compress();
    void Decompress();

    void PrintBitset();

private:
    uint64_t blockStart;
    uint64_t previousTimestamp;
    int lastDelta;
    int bitOffset;
    std::shared_ptr<std::vector<std::pair<uint64_t, float>>> uncompressedTimeseries;
    std::bitset<MAX_BIT_ALLOC> bitStream;
};

#endif