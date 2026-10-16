
#include "MultiLevelPageTable.h"

#include <vector>

MultiLevelPageTable::MultiLevelPageTable(
    const Config& config,
    PhysicalMemory& memory,
    FrameAllocator& allocator
)
    : config(config),
      memory(memory),
      allocator(allocator),
      rootFrame(0),
      initialized(false),
      rootIndexBits(
          config.vpnBits -
          config.indexBits * (config.numLevels - 1)
      ) {
}

uint64_t MultiLevelPageTable::validMask() const {
    return uint64_t{1} << (config.entrySize * 8 - 1);
}

bool MultiLevelPageTable::isValidEntry(uint64_t entry) const {
    return (entry & validMask()) != 0;
}

uint64_t MultiLevelPageTable::getFrameNumber(
    uint64_t entry
) const {
    return entry & (validMask() - 1);
}

uint64_t MultiLevelPageTable::entryAddress(
    uint64_t tableFrame,
    uint64_t index
) const {
    return tableFrame * config.pageSize
           + index * config.entrySize;
}

uint64_t MultiLevelPageTable::getLevelIndex(
    uint64_t vpn,
    unsigned level
) const {
    const unsigned width =
        (level == 0) ? rootIndexBits : config.indexBits;

    const unsigned shift =
        config.indexBits * (config.numLevels - 1 - level);

    const uint64_t mask =
        (uint64_t{1} << width) - 1;

    return (vpn >> shift) & mask;
}

bool MultiLevelPageTable::initialize(std::string& error) {
    if (initialized) {
        error = "Multi-level page table is already initialized.";
        return false;
    }

    if (!allocator.reserve(0, FrameType::PageTable)) {
        error = "Unable to reserve frame 0 for the root directory.";
        return false;
    }

    if (!memory.zeroFrame(0, config.pageSize)) {
        allocator.release(0);
        error = "Unable to initialize the root directory.";
        return false;
    }

    rootFrame = 0;
    initialized = true;
    return true;
}

uint64_t MultiLevelPageTable::getRootFrame() const {
    return rootFrame;
}

uint64_t MultiLevelPageTable::getTableFrames() const {
    return allocator.pageTableFrames();
}

bool MultiLevelPageTable::map(
    uint64_t vpn,
    uint64_t pfn,
    std::string& error
) {
    if (!initialized) {
        error = "Multi-level page table is not initialized.";
        return false;
    }

    if (vpn >= config.numVirtualPages) {
        error = "Virtual page number is out of range.";
        return false;
    }

    if (pfn >= config.numPhysicalFrames) {
        error = "Physical frame number is out of range.";
        return false;
    }

    const unsigned levels = config.numLevels;

    std::vector<uint64_t> tables(levels, 0);
    std::vector<uint64_t> indexes(levels, 0);

    tables[0] = rootFrame;

    // IMPORTANT FIX:
    // Calculate every index before walking the hierarchy.
    // Otherwise, indexes after a missing intermediate table
    // would remain zero and the leaf could be written incorrectly.
    for (unsigned level = 0; level < levels; ++level) {
        indexes[level] = getLevelIndex(vpn, level);
    }

    // Find the first missing table without modifying the hierarchy.
    unsigned missingLevel = levels;

    for (unsigned level = 0; level < levels; ++level) {
        uint64_t entry = 0;

        if (!memory.readEntry(
                entryAddress(tables[level], indexes[level]),
                config.entrySize, entry)) {
            error = "Unable to read page-table entry.";
            return false;
        }

        if (level == levels - 1) {
            if (isValidEntry(entry)) {
                error = "Virtual page is already mapped.";
                return false;
            }

            break;
        }

        if (!isValidEntry(entry)) {
            missingLevel = level;
            break;
        }

        const uint64_t childFrame = getFrameNumber(entry);

        if (childFrame >= config.numPhysicalFrames ||
            allocator.getType(childFrame) != FrameType::PageTable) {
            error = "Invalid page-table frame.";
            return false;
        }

        tables[level + 1] = childFrame;
    }

    // Reserve the requested data frame before allocating page tables.
    // This prevents automatic allocation from selecting the requested PFN.
    if (!allocator.reserve(pfn, FrameType::Data)) {
        error = "Physical frame is already occupied.";
        return false;
    }

    std::vector<uint64_t> allocatedTables;

    // Allocate all missing lower-level tables.
    if (missingLevel < levels) {
        for (unsigned level = missingLevel;
             level + 1 < levels;
             ++level) {

            const int64_t frame =
                allocator.allocate(FrameType::PageTable);

            if (frame < 0) {
                for (uint64_t allocated : allocatedTables) {
                    allocator.release(allocated);
                }

                allocator.release(pfn);
                error = "Physical memory exhausted.";
                return false;
            }

            const uint64_t frameNumber =
                static_cast<uint64_t>(frame);

            allocatedTables.push_back(frameNumber);
            tables[level + 1] = frameNumber;
        }
    }

    // Initialize all newly allocated page tables.
    for (uint64_t frame : allocatedTables) {
        if (!memory.zeroFrame(frame, config.pageSize)) {
            for (uint64_t allocated : allocatedTables) {
                allocator.release(allocated);
            }

            allocator.release(pfn);
            error = "Unable to initialize a page table.";
            return false;
        }
    }

    // Newly mapped data pages must start with zero bytes.
    if (!memory.zeroFrame(pfn, config.pageSize)) {
        for (uint64_t allocated : allocatedTables) {
            allocator.release(allocated);
        }

        allocator.release(pfn);
        error = "Unable to initialize data frame.";
        return false;
    }

    // Link new tables into the existing hierarchy.
    // Record only entries that this mapping creates so rollback
    // never clears a pre-existing parent entry.
    std::vector<uint64_t> linkedParents;

    if (missingLevel < levels) {
        for (unsigned level = missingLevel;
             level + 1 < levels;
             ++level) {

            const uint64_t parentAddress =
                entryAddress(tables[level], indexes[level]);

            const uint64_t childFrame = tables[level + 1];

            if (!memory.writeEntry(
                    parentAddress,
                    config.entrySize,
                    validMask() | childFrame)) {

                for (uint64_t address : linkedParents) {
                    memory.writeEntry(address, config.entrySize, 0);
                }

                for (uint64_t allocated : allocatedTables) {
                    allocator.release(allocated);
                }

                allocator.release(pfn);
                error = "Unable to link a page table.";
                return false;
            }

            linkedParents.push_back(parentAddress);
        }
    }

    // Write the leaf PTE only after the necessary tables exist.
    const uint64_t leafAddress =
        entryAddress(tables[levels - 1], indexes[levels - 1]);

    if (!memory.writeEntry(
            leafAddress,
            config.entrySize,
            validMask() | pfn)) {

        for (uint64_t address : linkedParents) {
            memory.writeEntry(address, config.entrySize, 0);
        }

        for (uint64_t allocated : allocatedTables) {
            allocator.release(allocated);
        }

        allocator.release(pfn);
        error = "Unable to write leaf entry.";
        return false;
    }

    return true;
}

bool MultiLevelPageTable::translate(
    uint64_t virtualAddress,
    uint64_t& physicalAddress,
    std::string& error
) {
    if (!initialized) {
        error = "Multi-level page table is not initialized.";
        return false;
    }

    if (virtualAddress >= config.virtualSize) {
        error = "Virtual address is out of range.";
        return false;
    }

    const uint64_t vpn = getVPN(config, virtualAddress);
    const uint64_t offset = getOffset(config, virtualAddress);

    uint64_t tableFrame = rootFrame;

    for (unsigned level = 0; level < config.numLevels; ++level) {
        const uint64_t index = getLevelIndex(vpn, level);

        uint64_t entry = 0;

        if (!memory.readEntry(
                entryAddress(tableFrame, index),
                config.entrySize, entry)) {
            error = "Unable to read page-table entry.";
            return false;
        }

        if (!isValidEntry(entry)) {
            error = "Unmapped-page fault.";
            return false;
        }

        const uint64_t frame = getFrameNumber(entry);

        if (level == config.numLevels - 1) {
            if (frame >= config.numPhysicalFrames ||
                allocator.getType(frame) != FrameType::Data) {
                error = "Invalid physical-frame mapping.";
                return false;
            }

            physicalAddress =
                getPhysicalAddress(config, frame, offset);

            return true;
        }

        if (frame >= config.numPhysicalFrames ||
            allocator.getType(frame) != FrameType::PageTable) {
            error = "Invalid page-table frame.";
            return false;
        }

        tableFrame = frame;
    }

    error = "Address translation failed.";
    return false;
}

bool MultiLevelPageTable::isTableEmpty(uint64_t frame) const {
    for (uint64_t index = 0;
         index < config.entriesPerTable;
         ++index) {

        uint64_t entry = 0;

        if (!memory.readEntry(
                entryAddress(frame, index),
                config.entrySize, entry)) {
            return false;
        }

        if (isValidEntry(entry)) {
            return false;
        }
    }

    return true;
}

bool MultiLevelPageTable::unmap(
    uint64_t vpn,
    std::string& error
) {
    if (!initialized) {
        error = "Multi-level page table is not initialized.";
        return false;
    }

    if (vpn >= config.numVirtualPages) {
        error = "Virtual page number is out of range.";
        return false;
    }

    const unsigned levels = config.numLevels;

    std::vector<uint64_t> tables(levels, 0);
    std::vector<uint64_t> indexes(levels, 0);
    std::vector<uint64_t> entries(levels, 0);

    tables[0] = rootFrame;

    // Read and validate the complete path before modifying anything.
    for (unsigned level = 0; level < levels; ++level) {
        indexes[level] = getLevelIndex(vpn, level);

        if (!memory.readEntry(
                entryAddress(tables[level], indexes[level]),
                config.entrySize, entries[level])) {
            error = "Unable to read page-table entry.";
            return false;
        }

        if (!isValidEntry(entries[level])) {
            error = "Virtual page is not mapped.";
            return false;
        }

        const uint64_t frame = getFrameNumber(entries[level]);

        if (level == levels - 1) {
            if (frame >= config.numPhysicalFrames ||
                allocator.getType(frame) != FrameType::Data) {
                error = "Invalid data-frame ownership.";
                return false;
            }
        } else {
            if (frame >= config.numPhysicalFrames ||
                allocator.getType(frame) != FrameType::PageTable) {
                error = "Invalid page-table frame.";
                return false;
            }

            tables[level + 1] = frame;
        }
    }

    const uint64_t dataFrame =
        getFrameNumber(entries[levels - 1]);

    const uint64_t leafAddress =
        entryAddress(tables[levels - 1], indexes[levels - 1]);

    // Clear the leaf before releasing its data frame.
    if (!memory.writeEntry(leafAddress, config.entrySize, 0)) {
        error = "Unable to clear leaf entry.";
        return false;
    }

    if (!allocator.release(dataFrame)) {
        memory.writeEntry(
            leafAddress,
            config.entrySize,
            entries[levels - 1]
        );

        error = "Unable to release data frame.";
        return false;
    }

    // Reclaim empty lower-level tables from the bottom up.
    // The root directory must remain allocated.
    for (int level = static_cast<int>(levels) - 1;
         level > 0;
         --level) {

        const uint64_t childFrame = tables[level];

        if (!isTableEmpty(childFrame)) {
            break;
        }

        const uint64_t parentAddress =
            entryAddress(
                tables[level - 1],
                indexes[level - 1]
            );

        if (!memory.writeEntry(
                parentAddress, config.entrySize, 0)) {
            error = "Unable to clear empty page-table pointer.";
            return false;
        }

        if (!allocator.release(childFrame)) {
            memory.writeEntry(
                parentAddress,
                config.entrySize,
                entries[level - 1]
            );

            error = "Unable to release empty page table.";
            return false;
        }
    }

    return true;
}
