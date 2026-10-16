
#include "Config.h"

#include <limits>

bool isPowerOfTwo(uint64_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

unsigned log2Exact(uint64_t value) {
    unsigned bits = 0;

    while (value > 1) {
        value >>= 1;
        ++bits;
    }

    return bits;
}

bool initializeConfig(
    Config& config,
    uint64_t physicalSize,
    uint64_t virtualSize,
    uint64_t entrySize,
    uint64_t pageSize,
    std::string& error
) {
    if (!isPowerOfTwo(physicalSize) ||
        !isPowerOfTwo(virtualSize) ||
        !isPowerOfTwo(entrySize) ||
        !isPowerOfTwo(pageSize)) {
        error = "All sizes must be powers of two.";
        return false;
    }

    if (pageSize <= entrySize) {
        error = "Page size must be larger than entry size.";
        return false;
    }

    if (pageSize > physicalSize || pageSize > virtualSize) {
        error = "Page size cannot exceed either address space.";
        return false;
    }

    // The VALID bit occupies the highest bit of each entry.
    if (entrySize > std::numeric_limits<uint64_t>::max() / 8) {
        error = "Entry size is too large.";
        return false;
    }

    const uint64_t entryBits = entrySize * 8;
    const uint64_t pfnBits = entryBits - 1;

    const uint64_t physicalFrames = physicalSize / pageSize;

    // The remaining entry bits must represent every physical frame.
    if (pfnBits < 64 &&
        physicalFrames > (uint64_t{1} << pfnBits)) {
        error = "PFN cannot represent all physical frames.";
        return false;
    }

    config.physicalSize = physicalSize;
    config.virtualSize = virtualSize;
    config.entrySize = entrySize;
    config.pageSize = pageSize;

    config.physicalAddressBits = log2Exact(physicalSize);
    config.virtualAddressBits = log2Exact(virtualSize);
    config.offsetBits = log2Exact(pageSize);

    config.vpnBits =
        config.virtualAddressBits - config.offsetBits;

    config.numVirtualPages = virtualSize / pageSize;
    config.numPhysicalFrames = physicalFrames;
    config.entriesPerTable = pageSize / entrySize;
    config.indexBits = log2Exact(config.entriesPerTable);

    // At least one level is needed, even if there is only one VPN.
    config.numLevels =
        config.vpnBits == 0
            ? 1
            : (config.vpnBits + config.indexBits - 1)
                  / config.indexBits;

    return true;
}

uint64_t getVPN(const Config& config, uint64_t virtualAddress) {
    return virtualAddress >> config.offsetBits;
}

uint64_t getOffset(const Config& config, uint64_t virtualAddress) {
    return virtualAddress & (config.pageSize - 1);
}

uint64_t getPhysicalAddress(
    const Config& config,
    uint64_t frameNumber,
    uint64_t offset
) {
    return frameNumber * config.pageSize + offset;
}
