
#include "FrameAllocator.h"

#include <cassert>
#include <iostream>

int main() {
    FrameAllocator allocator(256);

    // Initially, all frames must be free.
    assert(allocator.totalFrames() == 256);
    assert(allocator.freeFrames() == 256);
    assert(allocator.dataFrames() == 0);
    assert(allocator.pageTableFrames() == 0);

    // Reserve frame 0 for a page table.
    assert(allocator.reserve(0, FrameType::PageTable));

    // Reserve frame 128 for data.
    assert(allocator.reserve(128, FrameType::Data));

    // Cannot reserve an occupied frame twice.
    assert(!allocator.reserve(128, FrameType::Data));
    assert(!allocator.reserve(0, FrameType::Data));

    assert(allocator.freeFrames() == 254);
    assert(allocator.dataFrames() == 1);
    assert(allocator.pageTableFrames() == 1);

    // Allocation selects the lowest-numbered free frame.
    int64_t allocated = allocator.allocate(FrameType::PageTable);
    assert(allocated == 1);

    // Release an occupied frame.
    assert(allocator.release(128));
    assert(allocator.freeFrames() == 254);
    assert(allocator.dataFrames() == 0);

    // Cannot release a frame twice.
    assert(!allocator.release(128));

    // Invalid frame numbers must be rejected.
    assert(!allocator.reserve(256, FrameType::Data));
    assert(!allocator.release(256));

    // Allocating a Free frame type is not allowed.
    assert(allocator.allocate(FrameType::Free) == -1);

    // Exhaustion must be reported.
    FrameAllocator smallAllocator(2);
    assert(smallAllocator.allocate(FrameType::Data) == 0);
    assert(smallAllocator.allocate(FrameType::PageTable) == 1);
    assert(smallAllocator.allocate(FrameType::Data) == -1);

    std::cout << "All frame allocator tests passed!\n";
    return 0;
}
