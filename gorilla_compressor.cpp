
#include <iostream>

#include "gorilla_compressor.hpp"


GorillaCompressor::GorillaCompressor(std::shared_ptr<std::vector<std::pair<uint64_t, float>>> uncompressedTimeseries):
    lastDelta{0},
    bitOffset{0},
    blockStart{1647838800},
    previousTimestamp{1647838800},
    uncompressedTimeseries{uncompressedTimeseries}
    {}


void GorillaCompressor::WriteToStream(int writeBits, int significantBits) {

    // std::cout << "Writing " << writeBits << " to stream: ";

    int firstBitSet = 1 <<31;
    bool isNegative = firstBitSet & writeBits;

    if (isNegative) {
        writeBits *= -1;
    }

    writeBits = writeBits << (32 - significantBits);

    int firstWrite = isNegative || (writeBits & firstBitSet) ? 1 : 0 ;
    // std::cout << firstWrite << ", ";
    this->bitStream.set(this->bitOffset++, isNegative || (writeBits & firstBitSet) ? 1 : 0);
    writeBits <<=1;

    for (int i = 1; i < significantBits; i++) {
        int nextWrites = writeBits & firstBitSet ? 1 : 0;
        // std::cout << nextWrites << ", ";
        this->bitStream.set(this->bitOffset++, writeBits & firstBitSet ? 1 : 0);
        writeBits <<=1;
    }

    // std::cout << std::endl;

    // 1 1 1 0
    //  -------> 0 1 1 1
}

/*

   <s>||64  32 16 8 4 2 1 = 63
 |  1       1  1  1 1 1 1 = -63
 |  1       0  0  0 0 0 0 = 64

 128 64 32 16 8 4 2 1 = 255

  match Δ |
  0              -> 0
  [-63, 64]      -> 10 -> value (7 bits)
  [-255, 256]    -> 110 -> value (9 bits)
  [-2047, 2048]  -> 1110 -> value (12 bits)
  1111 -> D 32 bits
*/
void GorillaCompressor::Compress() {


    for (auto ts: *this->uncompressedTimeseries) {

        uint64_t timestamp = std::get<0>(ts);
        
        int delta = timestamp - this->previousTimestamp;
        int deltaOfDelta = delta - this->lastDelta;

        std::cout << "Δ" << deltaOfDelta << std::endl;

        if (deltaOfDelta == 0) {
            this->bitStream.set(this->bitOffset++, 0);
        } else if (-63 <= deltaOfDelta && deltaOfDelta <= 64) {
            this->bitStream.set(this->bitOffset++, 1);
            this->bitStream.set(this->bitOffset++, 0);
            this->WriteToStream(deltaOfDelta, 7);
        } else if (-255 <= deltaOfDelta && deltaOfDelta <= 256) {
            this->bitStream.set(this->bitOffset++, 1);
            this->bitStream.set(this->bitOffset++, 1);
            this->bitStream.set(this->bitOffset++, 0);
            this->WriteToStream(deltaOfDelta, 9);
        } else if (-2047 <= deltaOfDelta && deltaOfDelta <= 2048) {
            this->bitStream.set(this->bitOffset++, 1);
            this->bitStream.set(this->bitOffset++, 1);
            this->bitStream.set(this->bitOffset++, 1);
            this->bitStream.set(this->bitOffset++, 0);
            this->WriteToStream(deltaOfDelta, 12);
            
        } else {
            this->WriteToStream(deltaOfDelta, 32);
        }
        
        this->lastDelta = delta;
        this->previousTimestamp = timestamp;

    }
    std::cout << "end" << std::endl;
}

/*
  match Δ |
  0              -> 0
  [-63, 64]      -> 10 -> value (7 bits)
  [-255, 256]    -> 110 -> value (9 bits)
  [-2047, 2048]  -> 1110 -> value (12 bits)
  1111 -> D 32 bits
*/
int GorillaCompressor::ExtractDelta(int currentOffset, int significantBits) {
    // 1 1 1 0
    //  -------> 0 1 1 1
    // 1 1 1 0


    /*

    Start: Sig Bits = 7: 0, 1, 1, 1, 1, 0, 0, 
    1Δ=120

    Start: Sig Bits = 7: 0, 1, 1, 1, 1, 0, 0, 0

    8 + 16 + 32 + 64

    4 + 8  + 16 + 32

    */

    int delta = 0;

    std::cout << "Start: Sig Bits = " << significantBits << ": ";
    for (int i = 0; i < significantBits; i++) {
        std::cout << this->bitStream[currentOffset] << ", ";
        delta |= this->bitStream[currentOffset++];

        if (i != significantBits - 1) {
            delta <<=1;
        }
    }
    std::cout << std::endl;

    if (significantBits == 7 && delta > 64) {
        delta ^= (1<<6);
        delta *= -1;
    } else if (significantBits == 9 && delta > 256) {
        delta ^= (1<<8);
        delta *= -1;
    } else if (significantBits == 12 && delta > 2048) {
        delta ^= (1<<11);
        delta *= -1;
    }

    return delta;
}

void GorillaCompressor::Decompress() {

    uint64_t currentTimestamp = this->blockStart;

    int blockIndex = 0;
    while (blockIndex < this->bitOffset) {
        if (this->bitStream[blockIndex] == 0) {
            std::cout << "1Δ=0" << std::endl;
            blockIndex++;
        } else if (this->bitStream[blockIndex] == 1 && this->bitStream[blockIndex + 1] == 0) {
            blockIndex += 2;
            int delta = this->ExtractDelta(blockIndex, 7);
            std::cout << "2Δ=" << delta << std::endl;
            blockIndex += 7;
        } else if (this->bitStream[blockIndex] == 1 && this->bitStream[blockIndex + 1] == 1 && this->bitStream[blockIndex + 2] == 0) {
            blockIndex += 3;
            int delta = this->ExtractDelta(blockIndex, 9);
            std::cout << "3Δ=" << delta << std::endl;
            blockIndex += 9;
        }
        else if (this->bitStream[blockIndex] == 1 && this->bitStream[blockIndex + 1] == 1 && this->bitStream[blockIndex + 2] == 1 && this->bitStream[blockIndex + 3] == 0) {
            blockIndex += 4;
            int delta = this->ExtractDelta(blockIndex, 12);
            std::cout << "4Δ=" << delta << std::endl;
            blockIndex += 12;
        } else {
            blockIndex += 4;
            int delta = this->ExtractDelta(blockIndex, 32);
            std::cout << "5Δ=" << delta << std::endl;
            blockIndex += 32;
        }
    }

}

void GorillaCompressor::PrintBitset() {

    std::cout << this->bitStream << std::endl;
}
