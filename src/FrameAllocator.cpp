
#include "FrameAllocator.h"

FrameAllocator::FrameAllocator(uint64_t totalFrames)
    : frames(static_cast<std::size_t>(totalFrames), FrameType::Free) {
}

int64_t FrameAllocator::allocate(FrameType type) {
    if (type == FrameType::Free) {
        return -1;
    }

    for (uint64_t i = 0; i < frames.size(); ++i) {
        if (frames[i] == FrameType::Free) {
            frames[i] = type;
            return static_cast<int64_t>(i);
        }
    }

    return -1;
}

bool FrameAllocator::reserve(
    uint64_t frameNumber, FrameType type
) {
    if (frameNumber >= frames.size() ||
        type == FrameType::Free ||
        frames[frameNumber] != FrameType::Free) {
        return false;
    }

    frames[frameNumber] = type;
    return true;
}

bool FrameAllocator::release(uint64_t frameNumber) {
    if (frameNumber >= frames.size() ||
        frames[frameNumber] == FrameType::Free) {
        return false;
    }

    frames[frameNumber] = FrameType::Free;
    return true;
}

FrameType FrameAllocator::getType(uint64_t frameNumber) const {
    if (frameNumber >= frames.size()) {
        return FrameType::Free;
    }

    return frames[frameNumber];
}

uint64_t FrameAllocator::totalFrames() const {
    return static_cast<uint64_t>(frames.size());
}

uint64_t FrameAllocator::freeFrames() const {
    uint64_t count = 0;

    for (FrameType type : frames) {
        if (type == FrameType::Free) {
            ++count;
        }
    }

    return count;
}

uint64_t FrameAllocator::dataFrames() const {
    uint64_t count = 0;

    for (FrameType type : frames) {
        if (type == FrameType::Data) {
            ++count;
        }
    }

    return count;
}

uint64_t FrameAllocator::pageTableFrames() const {
    uint64_t count = 0;

    for (FrameType type : frames) {
        if (type == FrameType::PageTable) {
            ++count;
        }
    }

    return count;
}
