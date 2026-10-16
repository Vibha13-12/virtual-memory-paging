
#ifndef PHYSICAL_MEMORY_H
#define PHYSICAL_MEMORY_H

#include <cstdint>
#include <vector>

class PhysicalMemory {
private:
    std::vector<uint8_t> memory;

public:
    explicit PhysicalMemory(uint64_t size);

    uint64_t size() const;

    bool readByte(uint64_t address, uint8_t& value) const;

    bool writeByte(uint64_t address, uint8_t value);

    bool readEntry(uint64_t address, uint64_t entrySize,
                   uint64_t& value) const;

    bool writeEntry(uint64_t address, uint64_t entrySize,
                    uint64_t value);

    bool zeroFrame(uint64_t frameNumber, uint64_t pageSize);
};

#endif
