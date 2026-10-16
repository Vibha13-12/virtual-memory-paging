
#ifndef STATISTICS_H
#define STATISTICS_H

#include <cstdint>

struct Statistics {
    uint64_t successfulMappings = 0;
    uint64_t successfulUnmappings = 0;
    uint64_t successfulReads = 0;
    uint64_t successfulWrites = 0;
    uint64_t unmappedPageFaults = 0;
    uint64_t rejectedCommands = 0;
};

#endif
