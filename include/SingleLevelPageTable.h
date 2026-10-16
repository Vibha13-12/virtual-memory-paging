
#ifndef SINGLE_LEVEL_PAGE_TABLE_H
#define SINGLE_LEVEL_PAGE_TABLE_H

#include "Config.h"
#include "FrameAllocator.h"
#include "PhysicalMemory.h"

#include <cstdint>
#include <string>

class SingleLevelPageTable {
private:
    const Config& config;
    PhysicalMemory& memory;
    FrameAllocator& allocator;

    uint64_t tableFrames;
    bool initialized;

    uint64_t entryAddress(uint64_t vpn) const;
    uint64_t validMask() const;

public:
    SingleLevelPageTable(
        const Config& config,
        PhysicalMemory& memory,
        FrameAllocator& allocator
    );

    bool initialize(std::string& error);

    bool map(
        uint64_t vpn,
        uint64_t pfn,
        std::string& error
    );

    bool unmap(
        uint64_t vpn,
        std::string& error
    );

    bool translate(
        uint64_t virtualAddress,
        uint64_t& physicalAddress,
        std::string& error
    );

    uint64_t getTableFrames() const;
};

#endif
