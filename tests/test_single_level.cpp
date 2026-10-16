
#include "Config.h"
#include "FrameAllocator.h"
#include "PhysicalMemory.h"
#include "SingleLevelPageTable.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    Config config{};
    std::string error;

    assert(initializeConfig(
        config, 65536, 1048576, 4, 256, error
    ));

    PhysicalMemory memory(config.physicalSize);
    FrameAllocator allocator(config.numPhysicalFrames);

    SingleLevelPageTable pageTable(config, memory, allocator);

    // Initialize the table.
    assert(pageTable.initialize(error));
    assert(pageTable.getTableFrames() == 64);
    assert(allocator.pageTableFrames() == 64);
    assert(allocator.freeFrames() == 192);

    // Map VPN 1 to PFN 128.
    assert(pageTable.map(1, 128, error));
    assert(allocator.dataFrames() == 1);
    assert(allocator.freeFrames() == 191);

    // Translate address 0x00125.
    uint64_t physicalAddress = 0;
    assert(pageTable.translate(
        0x00125, physicalAddress, error
    ));
    assert(physicalAddress == 0x8025);

    // The data frame must initially contain zeros.
    uint8_t value = 255;
    assert(memory.readByte(physicalAddress, value));
    assert(value == 0);

    // Write a byte and read it back.
    assert(memory.writeByte(physicalAddress, 0xAB));
    assert(memory.readByte(physicalAddress, value));
    assert(value == 0xAB);

    // Duplicate mappings must be rejected.
    assert(!pageTable.map(1, 129, error));

    // A frame already occupied by the page table cannot be used.
    assert(!pageTable.map(2, 0, error));

    // An unmapped virtual page must fault.
    assert(!pageTable.translate(
        0x00300, physicalAddress, error
    ));
    assert(error == "Unmapped-page fault.");

    // Unmap VPN 1 and release its data frame.
    assert(pageTable.unmap(1, error));
    assert(allocator.dataFrames() == 0);
    assert(allocator.freeFrames() == 192);

    // Translation after unmapping must fault.
    assert(!pageTable.translate(
        0x00125, physicalAddress, error
    ));

    // Repeated unmapping must be rejected.
    assert(!pageTable.unmap(1, error));

    // Out-of-range VPN and PFN must be rejected.
    assert(!pageTable.map(config.numVirtualPages, 128, error));
    assert(!pageTable.map(2, config.numPhysicalFrames, error));

    std::cout << "All single-level paging tests passed!\n";
    return 0;
}
