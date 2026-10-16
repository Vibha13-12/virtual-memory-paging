
#ifndef MULTI_LEVEL_PAGE_TABLE_H
#define MULTI_LEVEL_PAGE_TABLE_H

#include "Config.h"
#include "FrameAllocator.h"
#include "PhysicalMemory.h"

#include <cstdint>
#include <string>
#include <vector>

class MultiLevelPageTable {
private:
    const Config& config;
    PhysicalMemory& memory;
    FrameAllocator& allocator;

    uint64_t rootFrame;
    bool initialized;

    // Number of index bits used by the root directory.
    unsigned rootIndexBits;

    // Extract the index belonging to a particular level.
    uint64_t getLevelIndex(
        uint64_t vpn,
        unsigned level
    ) const;

    // Calculate the physical address of an entry.
    uint64_t entryAddress(
        uint64_t tableFrame,
        uint64_t index
    ) const;

    // Encode or decode VALID/PFN entries.
    uint64_t validMask() const;

    bool isValidEntry(uint64_t entry) const;

    uint64_t getFrameNumber(uint64_t entry) const;

    // Check whether a page table contains only invalid entries.
    bool isTableEmpty(uint64_t frame) const;

public:
    MultiLevelPageTable(
        const Config& config,
        PhysicalMemory& memory,
        FrameAllocator& allocator
    );

    // Reserve and initialize the root directory.
    bool initialize(std::string& error);

    // Map a virtual page to a specified physical data frame.
    bool map(
        uint64_t vpn,
        uint64_t pfn,
        std::string& error
    );

    // Remove a mapping and reclaim empty lower-level tables.
    bool unmap(
        uint64_t vpn,
        std::string& error
    );

    // Translate a virtual byte address to a physical address.
    bool translate(
        uint64_t virtualAddress,
        uint64_t& physicalAddress,
        std::string& error
    );

    uint64_t getRootFrame() const;

    // Count all frames currently occupied by page tables.
    uint64_t getTableFrames() const;
};

#endif
