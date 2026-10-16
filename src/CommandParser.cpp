
#include "CommandParser.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

CommandParser::CommandParser(bool useMultiLevel)
    : useMultiLevel(useMultiLevel) {
}

void CommandParser::run(std::istream& input) {
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        executeLine(line);
    }
}

void CommandParser::printStats() const {
    std::cout << "Successful mappings: "
              << stats.successfulMappings << '\n';
    std::cout << "Successful unmappings: "
              << stats.successfulUnmappings << '\n';
    std::cout << "Successful reads: "
              << stats.successfulReads << '\n';
    std::cout << "Successful writes: "
              << stats.successfulWrites << '\n';
    std::cout << "Unmapped-page faults: "
              << stats.unmappedPageFaults << '\n';
    std::cout << "Rejected commands: "
              << stats.rejectedCommands << '\n';

    if (configured) {
        std::cout << "Data frames: "
                  << allocator->dataFrames() << '\n';
        std::cout << "Page-table frames: "
                  << allocator->pageTableFrames() << '\n';
        std::cout << "Free frames: "
                  << allocator->freeFrames() << '\n';
    }
}

bool CommandParser::executeLine(const std::string& line) {
    std::istringstream input(line);
    std::string command;

    if (!(input >> command)) {
        return true;
    }

    auto reject = [&](const std::string& message) {
        ++stats.rejectedCommands;
        std::cout << "ERROR: " << message << '\n';
        return false;
    };

    auto parseNumber = [](const std::string& text,
                          uint64_t& value,
                          int base) -> bool {
        try {
            std::size_t used = 0;
            value = std::stoull(text, &used, base);
            return used == text.size();
        } catch (...) {
            return false;
        }
    };

    if (command == "CONFIG") {
        std::string pText, vText, eText, sText, extra;

        if (!(input >> pText >> vText >> eText >> sText) ||
            (input >> extra)) {
            return reject(
                "CONFIG requires exactly four decimal byte counts."
            );
        }

        uint64_t p, v, e, s;

        if (!parseNumber(pText, p, 10) ||
            !parseNumber(vText, v, 10) ||
            !parseNumber(eText, e, 10) ||
            !parseNumber(sText, s, 10)) {
            return reject("Invalid CONFIG numeric parameter.");
        }

        if (configured) {
            return reject("CONFIG has already been processed.");
        }

        std::string error;

        if (!initializeConfig(config, p, v, e, s, error)) {
            return reject(error);
        }

        memory = std::make_unique<PhysicalMemory>(p);
        allocator = std::make_unique<FrameAllocator>(
            config.numPhysicalFrames
        );

        if (useMultiLevel) {
            multiLevel = std::make_unique<MultiLevelPageTable>(
                config, *memory, *allocator
            );

            if (!multiLevel->initialize(error)) {
                memory.reset();
                allocator.reset();
                multiLevel.reset();
                return reject(error);
            }
        } else {
            singleLevel = std::make_unique<SingleLevelPageTable>(
                config, *memory, *allocator
            );

            if (!singleLevel->initialize(error)) {
                memory.reset();
                allocator.reset();
                singleLevel.reset();
                return reject(error);
            }
        }

        configured = true;

        std::cout << "CONFIG OK\n";
        std::cout << "Virtual pages: "
                  << config.numVirtualPages << '\n';
        std::cout << "Physical frames: "
                  << config.numPhysicalFrames << '\n';
        std::cout << "Page-table levels: "
                  << config.numLevels << '\n';

        return true;
    }

    if (command == "STATS") {
        std::string extra;

        if (input >> extra) {
            return reject("STATS takes no arguments.");
        }

        printStats();
        return true;
    }

    if (!configured) {
        return reject("CONFIG must be processed first.");
    }

    std::string error;

    if (command == "MAP") {
        std::string vpnText, pfnText, extra;

        if (!(input >> vpnText >> pfnText) || (input >> extra)) {
            return reject("MAP requires a VPN and PFN.");
        }

        uint64_t vpn, pfn;

        if (!parseNumber(vpnText, vpn, 0) ||
            !parseNumber(pfnText, pfn, 0)) {
            return reject("Invalid MAP argument.");
        }

        bool success = useMultiLevel
            ? multiLevel->map(vpn, pfn, error)
            : singleLevel->map(vpn, pfn, error);

        if (!success) {
            return reject(error);
        }

        ++stats.successfulMappings;
        std::cout << "MAP OK\n";
        return true;
    }

    if (command == "UNMAP") {
        std::string vpnText, extra;

        if (!(input >> vpnText) || (input >> extra)) {
            return reject("UNMAP requires one VPN.");
        }

        uint64_t vpn;

        if (!parseNumber(vpnText, vpn, 0)) {
            return reject("Invalid UNMAP argument.");
        }

        bool success = useMultiLevel
            ? multiLevel->unmap(vpn, error)
            : singleLevel->unmap(vpn, error);

        if (!success) {
            return reject(error);
        }

        ++stats.successfulUnmappings;
        std::cout << "UNMAP OK\n";
        return true;
    }

    if (command == "READ" || command == "WRITE") {
        std::string addressText, valueText, extra;

        if (!(input >> addressText)) {
            return reject(command + " requires an address.");
        }

        uint64_t virtualAddress, writeValue = 0;

        if (!parseNumber(addressText, virtualAddress, 0)) {
            return reject("Invalid virtual address.");
        }

        if (command == "WRITE") {
            if (!(input >> valueText) ||
                !parseNumber(valueText, writeValue, 0) ||
                writeValue > 255) {
                return reject(
                    "WRITE value must be an integer from 0 to 255."
                );
            }
        }

        if (input >> extra) {
            return reject(command + " has too many arguments.");
        }

        uint64_t physicalAddress = 0;

        bool translated = useMultiLevel
            ? multiLevel->translate(
                  virtualAddress, physicalAddress, error)
            : singleLevel->translate(
                  virtualAddress, physicalAddress, error);

        if (!translated) {
            if (error == "Unmapped-page fault.") {
                ++stats.unmappedPageFaults;
            }

            std::cout << "FAULT: " << error << '\n';
            return false;
        }

        if (command == "READ") {
            uint8_t value = 0;

            if (!memory->readByte(physicalAddress, value)) {
                return reject("Physical memory read failed.");
            }

            ++stats.successfulReads;

            std::cout << "READ 0x"
                      << std::hex << std::uppercase
                      << physicalAddress
                      << " = 0x" << static_cast<unsigned>(value)
                      << std::dec << '\n';
        } else {
            if (!memory->writeByte(
                    physicalAddress,
                    static_cast<uint8_t>(writeValue))) {
                return reject("Physical memory write failed.");
            }

            ++stats.successfulWrites;

            std::cout << "WRITE 0x"
                      << std::hex << std::uppercase
                      << physicalAddress
                      << " = 0x" << writeValue
                      << std::dec << '\n';
        }

        return true;
    }

    return reject("Unknown command: " + command);
}
