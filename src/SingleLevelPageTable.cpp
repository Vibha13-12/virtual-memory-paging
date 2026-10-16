
#include "SingleLevelPageTable.h"

SingleLevelPageTable::SingleLevelPageTable(
    const Config& config,
    PhysicalMemory& memory,
    FrameAllocator& allocator
)
    : config(config),
      memory(memory),
      allocator(allocator),
      tableFrames(0),
      initialized(false) {
}

uint64_t SingleLevelPageTable::validMask() const {
    return uint64_t{1} << (config.entrySize * 8 - 1);
}

uint64_t SingleLevelPageTable::entryAddress(
    uint64_t vpn
) const {
    return vpn * config.entrySize;
}

uint64_t SingleLevelPageTable::getTableFrames() const {
    return tableFrames;
}

bool SingleLevelPageTable::initialize(std::string& error) {
    if (initialized) {
        error = "Single-level page table is already initialized.";
        return false;
    }

    // The complete single-level page table must occupy
    // contiguous frames beginning at physical frame 0.
    const uint64_t tableBytes =
        config.numVirtualPages * config.entrySize;

    tableFrames =
        (tableBytes + config.pageSize - 1) / config.pageSize;

    if (tableFrames > config.numPhysicalFrames) {
        error = "Page table does not fit in physical memory.";
        return false;
    }

    // Reserve the contiguous frames.
    uint64_t reserved = 0;

    for (; reserved < tableFrames; ++reserved) {
        if (!allocator.reserve(reserved, FrameType::PageTable)) {
            error = "Unable to reserve contiguous page-table frames.";

            while (reserved > 0) {
                --reserved;
                allocator.release(reserved);
            }

            return false;
        }

        if (!memory.zeroFrame(reserved, config.pageSize)) {
            allocator.release(reserved);

            while (reserved > 0) {
                --reserved;
                allocator.release(reserved);
            }

            error = "Unable to initialize page-table memory.";
            return false;
        }
    }

    // Initialize every PTE as invalid.
    for (uint64_t vpn = 0; vpn < config.numVirtualPages; ++vpn) {
        if (!memory.writeEntry(
                entryAddress(vpn), config.entrySize, 0)) {
            for (uint64_t frame = 0; frame < tableFrames; ++frame) {
                allocator.release(frame);
            }

            error = "Unable to initialize page-table entries.";
            return false;
        }
    }

    initialized = true;
    return true;
}

bool SingleLevelPageTable::map(
    uint64_t vpn,
    uint64_t pfn,
    std::string& error
) {
    if (!initialized) {
        error = "Page table is not initialized.";
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

    uint64_t entry = 0;

    if (!memory.readEntry(
            entryAddress(vpn), config.entrySize, entry)) {
        error = "Unable to read page-table entry.";
        return false;
    }

    if ((entry & validMask()) != 0) {
        error = "Virtual page is already mapped.";
        return false;
    }

    // Reserve the requested data frame.
    if (!allocator.reserve(pfn, FrameType::Data)) {
        error = "Physical frame is already occupied.";
        return false;
    }

    // Newly mapped data pages must begin with zero bytes.
    if (!memory.zeroFrame(pfn, config.pageSize)) {
        allocator.release(pfn);
        error = "Unable to initialize data frame.";
        return false;
    }

    // Encode VALID in the highest bit and PFN in the rest.
    const uint64_t pte = validMask() | pfn;

    if (!memory.writeEntry(
            entryAddress(vpn), config.entrySize, pte)) {
        allocator.release(pfn);
        error = "Unable to write page-table entry.";
        return false;
    }

    return true;
}

bool SingleLevelPageTable::unmap(
    uint64_t vpn,
    std::string& error
) {
    if (!initialized) {
        error = "Page table is not initialized.";
        return false;
    }

    if (vpn >= config.numVirtualPages) {
        error = "Virtual page number is out of range.";
        return false;
    }

    uint64_t entry = 0;

    if (!memory.readEntry(
            entryAddress(vpn), config.entrySize, entry)) {
        error = "Unable to read page-table entry.";
        return false;
    }

    if ((entry & validMask()) == 0) {
        error = "Virtual page is not mapped.";
        return false;
    }

    const uint64_t pfn = entry & (validMask() - 1);

    if (pfn >= config.numPhysicalFrames ||
        allocator.getType(pfn) != FrameType::Data) {
        error = "Invalid data-frame ownership.";
        return false;
    }

    if (!memory.writeEntry(
            entryAddress(vpn), config.entrySize, 0)) {
        error = "Unable to clear page-table entry.";
        return false;
    }

    if (!allocator.release(pfn)) {
        // Restore the previous entry if releasing fails.
        memory.writeEntry(entryAddress(vpn), config.entrySize, entry);
        error = "Unable to release data frame.";
        return false;
    }

    return true;
}

bool SingleLevelPageTable::translate(
    uint64_t virtualAddress,
    uint64_t& physicalAddress,
    std::string& error
) {
    if (!initialized) {
        error = "Page table is not initialized.";
        return false;
    }

    if (virtualAddress >= config.virtualSize) {
        error = "Virtual address is out of range.";
        return false;
    }

    const uint64_t vpn = getVPN(config, virtualAddress);
    const uint64_t offset = getOffset(config, virtualAddress);

    uint64_t entry = 0;

    if (!memory.readEntry(
            entryAddress(vpn), config.entrySize, entry)) {
        error = "Unable to read page-table entry.";
        return false;
    }

    if ((entry & validMask()) == 0) {
        error = "Unmapped-page fault.";
        return false;
    }

    const uint64_t pfn = entry & (validMask() - 1);

    if (pfn >= config.numPhysicalFrames ||
        allocator.getType(pfn) != FrameType::Data) {
        error = "Invalid physical-frame mapping.";
        return false;
    }

    physicalAddress =
        getPhysicalAddress(config, pfn, offset);

    return true;
}
