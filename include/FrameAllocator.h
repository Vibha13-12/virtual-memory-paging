

#ifndef FRAME_ALLOCATOR_H
#define FRAME_ALLOCATOR_H

#include <cstdint>
#include <vector>

enum class FrameType {
    Free,
    Data,
    PageTable
};

class FrameAllocator {
private:
    std::vector<FrameType> frames;

public:
    explicit FrameAllocator(uint64_t totalFrames);

    // Allocate the lowest-numbered free frame.
    int64_t allocate(FrameType type);

    // Reserve a particular frame number.
    bool reserve(uint64_t frameNumber, FrameType type);

    // Release a previously occupied frame.
    bool release(uint64_t frameNumber);

    // Inspect a frame's current state.
    FrameType getType(uint64_t frameNumber) const;

    uint64_t totalFrames() const;
    uint64_t freeFrames() const;
    uint64_t dataFrames() const;
    uint64_t pageTableFrames() const;

    // Release all remaining frames when simulation ends.
    void releaseAll();
};

#endif
