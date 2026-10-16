
#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include "Config.h"
#include "FrameAllocator.h"
#include "MultiLevelPageTable.h"
#include "PhysicalMemory.h"
#include "SingleLevelPageTable.h"
#include "Statistics.h"

#include <cstdint>
#include <istream>
#include <memory>
#include <string>

class CommandParser {
private:
    Config config{};
    std::unique_ptr<PhysicalMemory> memory;
    std::unique_ptr<FrameAllocator> allocator;

    std::unique_ptr<SingleLevelPageTable> singleLevel;
    std::unique_ptr<MultiLevelPageTable> multiLevel;

    Statistics stats;
    bool configured = false;
    bool useMultiLevel;

    bool executeLine(const std::string& line);
    void printStats() const;

public:
    explicit CommandParser(bool useMultiLevel);

    void run(std::istream& input);
};

#endif
