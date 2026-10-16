
#include "PhysicalMemory.h"

#include <limits>

PhysicalMemory::PhysicalMemory(uint64_t size)
    : memory(static_cast<std::size_t>(size), 0) {
}

uint64_t PhysicalMemory::size() const {
    return static_cast<uint64_t>(memory.size());
}

bool PhysicalMemory::readByte(
    uint64_t address, uint8_t& value
) const {
    if (address >= memory.size()) {
        return false;
    }

    value = memory[static_cast<std::size_t>(address)];
    return true;
}

bool PhysicalMemory::writeByte(
    uint64_t address, uint8_t value
) {
    if (address >= memory.size()) {
        return false;
    }

    memory[static_cast<std::size_t>(address)] = value;
    return true;
}

bool PhysicalMemory::readEntry(
    uint64_t address,
    uint64_t entrySize,
    uint64_t& value
) const {
    if (entrySize == 0 || entrySize > 8 ||
        address > size() ||
        entrySize > size() - address) {
        return false;
    }

    value = 0;

    for (uint64_t i = 0; i < entrySize; ++i) {
        value |=
            static_cast<uint64_t>(
                memory[static_cast<std::size_t>(address + i)]
            ) << (8 * i);
    }

    return true;
}

bool PhysicalMemory::writeEntry(
    uint64_t address,
    uint64_t entrySize,
    uint64_t value
) {
    if (entrySize == 0 || entrySize > 8 ||
        address > size() ||
        entrySize > size() - address) {
        return false;
    }

    for (uint64_t i = 0; i < entrySize; ++i) {
        memory[static_cast<std::size_t>(address + i)] =
            static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
    }

    return true;
}

bool PhysicalMemory::zeroFrame(
    uint64_t frameNumber,
    uint64_t pageSize
) {
    if (pageSize == 0 ||
        frameNumber >= size() / pageSize) {
        return false;
    }

    const uint64_t start = frameNumber * pageSize;

    for (uint64_t i = 0; i < pageSize; ++i) {
        memory[static_cast<std::size_t>(start + i)] = 0;
    }

    return true;
}
