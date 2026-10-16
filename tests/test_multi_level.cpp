
#include "Config.h"
#include "FrameAllocator.h"
#include "MultiLevelPageTable.h"
#include "PhysicalMemory.h"

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

    MultiLevelPageTable pageTable(config, memory, allocator);

    // The root directory must occupy frame 0.
    assert(pageTable.initialize(error));
    assert(pageTable.getRootFrame() == 0);
    assert(allocator.pageTableFrames() == 1);

    // Map two pages that share one lower-level table.
    assert(pageTable.map(1, 128, error));
    assert(pageTable.map(2, 129, error));

    assert(allocator.pageTableFrames() == 2);
    assert(allocator.dataFrames() == 2);
    assert(allocator.freeFrames() == 252);

    // Check the first mapping.
    uint64_t physicalAddress = 0;

    assert(pageTable.translate(
        0x00125, physicalAddress, error
    ));
    assert(physicalAddress == 0x8025);

    uint8_t value = 255;
    assert(memory.readByte(physicalAddress, value));
    assert(value == 0);

    // Write and read a byte through translated memory.
    assert(memory.writeByte(0x8104, 0xAB));

    assert(pageTable.translate(
        0x00204, physicalAddress, error
    ));
    assert(physicalAddress == 0x8104);

    assert(memory.readByte(physicalAddress, value));
    assert(value == 0xAB);

    // An unmapped page must fault.
    assert(!pageTable.translate(
        0x00300, physicalAddress, error
    ));
    assert(error == "Unmapped-page fault.");

    // Duplicate mapping and occupied frames must be rejected.
    assert(!pageTable.map(1, 130, error));
    assert(!pageTable.map(3, 128, error));
    assert(!pageTable.map(3, 0, error));

    // Unmapping one page must preserve the shared table.
    assert(pageTable.unmap(1, error));
    assert(allocator.dataFrames() == 1);
    assert(allocator.pageTableFrames() == 2);
    assert(allocator.freeFrames() == 253);

    // The second mapping and its data must still exist.
    assert(pageTable.translate(
        0x00204, physicalAddress, error
    ));
    assert(physicalAddress == 0x8104);

    assert(memory.readByte(physicalAddress, value));
    assert(value == 0xAB);

    // Unmapping the last page in the table reclaims that table.
    assert(pageTable.unmap(2, error));
    assert(allocator.dataFrames() == 0);
    assert(allocator.pageTableFrames() == 1);
    assert(allocator.freeFrames() == 255);

    // Unmapping an already-unmapped page must fail.
    assert(!pageTable.unmap(2, error));

    // Test a different directory index.
    assert(pageTable.map(0x080, 128, error));

    assert(pageTable.translate(
        0x08004, physicalAddress, error
    ));
    assert(physicalAddress == 0x8004);

    // The lower-level table should be reclaimed after unmapping.
    assert(pageTable.unmap(0x080, error));
    assert(allocator.pageTableFrames() == 1);
    assert(allocator.freeFrames() == 255);

    // ---------------------------------------------------------
    // Test rollback when physical memory is exhausted.
    // ---------------------------------------------------------

    Config smallConfig{};
    assert(initializeConfig(
        smallConfig, 1024, 65536, 4, 256, error
    ));

    PhysicalMemory smallMemory(smallConfig.physicalSize);
    FrameAllocator smallAllocator(smallConfig.numPhysicalFrames);

    MultiLevelPageTable smallTable(
        smallConfig, smallMemory, smallAllocator
    );

    // Root table occupies frame 0.
    assert(smallTable.initialize(error));

    // Map VPN 0 to PFN 2.
    // Frame 1 becomes a lower-level table.
    assert(smallTable.map(0x0, 0x2, error));

    assert(smallAllocator.pageTableFrames() == 2);
    assert(smallAllocator.dataFrames() == 1);
    assert(smallAllocator.freeFrames() == 1);

    // VPN 0x40 needs another lower-level table.
    // PFN 3 is reserved temporarily, leaving no frame for
    // the additional page table. The operation must roll back.
    assert(!smallTable.map(0x40, 0x3, error));
    assert(error == "Physical memory exhausted.");

    // Verify the failed mapping did not leak a frame.
    assert(smallAllocator.pageTableFrames() == 2);
    assert(smallAllocator.dataFrames() == 1);
    assert(smallAllocator.freeFrames() == 1);

    // The original mapping must still be valid.
    assert(smallTable.translate(
        0x000, physicalAddress, error
    ));
    assert(physicalAddress == 0x200);

    // PFN 3 must have been released and must be reusable.
    // VPN 1 shares the existing lower-level table.
    assert(smallTable.map(0x1, 0x3, error));

    assert(smallAllocator.pageTableFrames() == 2);
    assert(smallAllocator.dataFrames() == 2);
    assert(smallAllocator.freeFrames() == 0);

    std::cout << "All multi-level paging tests passed!\n";
    return 0;
}
