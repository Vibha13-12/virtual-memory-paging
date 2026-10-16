
#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>

struct Config {
    uint64_t physicalSize;
    uint64_t virtualSize;
    uint64_t entrySize;
    uint64_t pageSize;

    unsigned physicalAddressBits;
    unsigned virtualAddressBits;
    unsigned offsetBits;
    unsigned vpnBits;
    unsigned indexBits;
    unsigned numLevels;

    uint64_t numVirtualPages;
    uint64_t numPhysicalFrames;
    uint64_t entriesPerTable;
};

bool isPowerOfTwo(uint64_t value);

unsigned log2Exact(uint64_t value);

bool initializeConfig(
    Config& config,
    uint64_t physicalSize,
    uint64_t virtualSize,
    uint64_t entrySize,
    uint64_t pageSize,
    std::string& error
);

uint64_t getVPN(const Config& config, uint64_t virtualAddress);

uint64_t getOffset(const Config& config, uint64_t virtualAddress);

uint64_t getPhysicalAddress(
    const Config& config,
    uint64_t frameNumber,
    uint64_t offset
);

#endif
