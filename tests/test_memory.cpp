
#include "PhysicalMemory.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    PhysicalMemory memory(65536);

    assert(memory.size() == 65536);

    // Memory must initially contain zeros.
    uint8_t value = 255;
    assert(memory.readByte(0, value));
    assert(value == 0);

    // Test writing and reading a byte.
    assert(memory.writeByte(100, 0xAB));
    assert(memory.readByte(100, value));
    assert(value == 0xAB);

    // Test a 4-byte page-table entry.
    assert(memory.writeEntry(256, 4, 0x80000080));

    uint64_t entry = 0;
    assert(memory.readEntry(256, 4, entry));
    assert(entry == 0x80000080);

    // Test zero-initializing an entire frame.
    assert(memory.zeroFrame(1, 256));
    assert(memory.readByte(256, value));
    assert(value == 0);

    // Test invalid addresses.
    assert(!memory.readByte(65536, value));
    assert(!memory.writeByte(65536, 0xFF));
    assert(!memory.zeroFrame(256, 256));

    std::cout << "All physical memory tests passed!\n";
    return 0;
}
