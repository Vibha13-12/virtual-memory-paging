
# Virtual Memory Paging Simulator

**Name:** Reddy Gurucharana Lakshmi Vibha  
**NetID:** qc8297

## 1. Project Overview

This project implements a virtual memory paging simulator in C++17. It demonstrates address translation, physical memory management, page-table operations, frame allocation, and the comparison of single-level and multi-level paging.

The simulator operates on simulated memory and does not modify the host operating system's actual memory-management structures.

## 2. Features

- Configurable virtual and physical memory sizes
- Virtual address decomposition into virtual page number (VPN) and page offset
- Physical memory represented as a byte-addressable array
- Physical frame allocation, reservation, and release
- Single-level page-table implementation
- Multi-level page-table implementation
- Virtual-to-physical address translation
- Mapping and unmapping operations
- Byte-level read and write operations
- VALID-bit checking and unmapped-page fault reporting
- Error handling for invalid commands, duplicate mappings, and occupied frames
- Physical-memory exhaustion handling
- Allocation-failure rollback
- Reclamation of empty lower-level page tables
- Statistics for operations and frame usage
- Automated unit tests and command trace files

## 3. Project Structure

```text
virtual-memory-paging/
├── include/
│   ├── Config.h
│   ├── PhysicalMemory.h
│   ├── FrameAllocator.h
│   ├── SingleLevelPageTable.h
│   ├── MultiLevelPageTable.h
│   ├── Statistics.h
│   └── CommandParser.h
├── src/
│   ├── Config.cpp
│   ├── PhysicalMemory.cpp
│   ├── FrameAllocator.cpp
│   ├── SingleLevelPageTable.cpp
│   ├── MultiLevelPageTable.cpp
│   ├── CommandParser.cpp
│   └── main.cpp
├── tests/
│   ├── test_memory.cpp
│   ├── test_allocator.cpp
│   ├── test_single_level.cpp
│   └── test_multi_level.cpp
├── traces/
│   ├── example.txt
│   ├── error_cases.txt
│   └── exhaustion.txt
├── reports/
│   └── project_report.md
├── Makefile
└── README.md
```

## 4. Requirements

- A C++17-compatible compiler, such as Clang or GCC
- GNU Make
- A Unix-like terminal, such as macOS or Linux

## 5. Build Instructions

From the project root directory, clean previous build artifacts and compile the simulator:

```bash
make clean
make
```

Run all four unit-test suites:

```bash
make test
```

To rebuild and run the complete test suite in one command:

```bash
make clean && make && make test
```

## 6. Running the Simulator

### Single-Level Paging

Single-level paging is the default mode:

```bash
./vm_simulator < traces/example.txt
```

### Multi-Level Paging

Use the `--multi` option to run multi-level paging:

```bash
./vm_simulator --multi < traces/example.txt
```

### Run the Error-Handling Trace

```bash
./vm_simulator < traces/error_cases.txt
./vm_simulator --multi < traces/error_cases.txt
```

### Run the Memory-Exhaustion Trace

```bash
./vm_simulator < traces/exhaustion.txt
./vm_simulator --multi < traces/exhaustion.txt
```

The simulator reads commands from standard input. Input redirection allows a trace file to be processed sequentially.

## 7. Command Format

The supported commands are:

```text
CONFIG <physical_bytes> <virtual_bytes> <entry_bytes> <page_bytes>
MAP <virtual_page_number> <physical_frame_number>
UNMAP <virtual_page_number>
READ <virtual_address>
WRITE <virtual_address> <byte_value>
STATS
```

### Command Descriptions

| Command | Description |
|---|---|
| `CONFIG` | Configures physical memory, virtual memory, entry size, and page size |
| `MAP` | Maps a virtual page number to a specified physical frame |
| `UNMAP` | Removes an existing virtual-page mapping |
| `READ` | Reads one byte from a virtual address |
| `WRITE` | Writes one byte to a virtual address |
| `STATS` | Displays operation counters and frame usage |

The `CONFIG` command takes exactly four decimal byte counts. Mapping arguments and virtual addresses support decimal or hexadecimal notation through the command parser. `WRITE` accepts byte values from 0 to 255.

### Example Configuration

```text
CONFIG 65536 1048576 4 256
```

This configuration uses:

- 65,536 bytes of physical memory
- 1,048,576 bytes of virtual memory
- 4-byte page-table entries
- 256-byte pages

It produces 4,096 virtual pages, 256 physical frames, an 8-bit page offset, 64 entries per page-sized table, and two paging levels.

## 8. Example Trace

The following commands are available in `traces/example.txt`:

```text
CONFIG 65536 1048576 4 256
MAP 0x001 0x80
MAP 0x002 0x81
READ 0x00125
WRITE 0x00204 0xAB
READ 0x00300
UNMAP 0x001
STATS
```

This trace:

1. Configures physical and virtual memory.
2. Maps two virtual pages to physical frames.
3. Reads a byte through a translated virtual address.
4. Writes a byte through a translated virtual address.
5. Attempts to access an unmapped page, producing a fault.
6. Unmaps one virtual page.
7. Displays the final statistics.

## 9. Statistics

The `STATS` command reports:

- Successful mappings
- Successful unmappings
- Successful reads
- Successful writes
- Unmapped-page faults
- Rejected commands
- Data frames in use
- Page-table frames in use
- Free physical frames

Statistics are useful for examining the behavior of each paging implementation and comparing physical-frame usage.

## 10. Testing

The project includes four unit-test suites:

- `tests/test_memory.cpp`: physical-memory operations
- `tests/test_allocator.cpp`: physical-frame allocation and release
- `tests/test_single_level.cpp`: single-level page-table operations
- `tests/test_multi_level.cpp`: multi-level page-table operations

Run the complete test suite with:

```bash
make clean && make && make test
```

The latest observed test run confirmed that all four unit-test suites passed.

The multi-level tests cover address translation, byte reads and writes, invalid mappings, unmapping, reclamation of empty lower-level tables, and allocation-failure rollback.

Additional trace files are provided:

- `traces/example.txt`: basic mapping, translation, read, write, and unmapping operations
- `traces/error_cases.txt`: invalid commands and mapping errors
- `traces/exhaustion.txt`: constrained physical memory and allocation-failure behavior

Both paging modes processed the example trace with the expected statistics. The multi-level exhaustion trace demonstrated that an existing mapping remained readable after a failed allocation.

## 11. Implementation Notes

### Single-Level Paging

The single-level implementation uses one page table containing an entry for each virtual page. For the example configuration, this table requires 16,384 bytes, or 64 physical frames.

### Multi-Level Paging

The multi-level implementation organizes page tables hierarchically. The root table is initialized first, and lower-level tables can be allocated when mappings require them.

Page-table entries are stored in simulated physical memory. Empty lower-level tables can be reclaimed after their mappings are removed.

### Frame Allocation and Mapping

The frame allocator distinguishes between data frames and page-table frames. A mapping operation checks frame availability and allocates the resources required for the mapping.

The tested allocation-failure scenario verifies that a failed multi-level mapping releases its temporarily reserved data frame and preserves an existing mapping.

The simulator is an educational model. Its results depend on the configured memory sizes, page-table representation, allocation strategy, and workload.

## 12. Experimental Comparison

For the example configuration and trace, the observed frame usage was:

| Metric | Single-Level | Multi-Level |
|---|---:|---:|
| Page-table frames | 64 | 2 |
| Data frames | 1 | 1 |
| Free frames | 191 | 253 |
| Page-table memory | 16,384 bytes | 512 bytes |

For this particular workload, multi-level paging used 15,872 fewer bytes of page-table storage than single-level paging.

This result illustrates how allocating lower-level tables as needed can reduce page-table memory usage for a sparse address space. The result is specific to this configuration and trace and is not a general benchmark of real operating systems.

## 13. Limitations and Future Improvements

Potential extensions include:

- More comprehensive tests for memory-write and cleanup failure paths
- Additional invalid-input and boundary-address tests
- Automated comparison of memory usage across different workloads
- Tests for page-table reclamation across larger paging hierarchies
- Reproducible performance measurements
- More detailed command documentation and error specifications

## 14. Conclusion

This project implements a C++17 virtual-memory paging simulator with single-level and multi-level page tables, simulated physical memory, frame allocation, address translation, mapping and unmapping operations, and statistics reporting.

The example trace demonstrates the different page-table memory requirements of the two implementations. The error-handling trace exercises invalid operations and unmapped-page faults. The memory-exhaustion experiment demonstrates a failed multi-level mapping while preserving an existing mapping.

The project provides an educational demonstration of virtual-memory paging, the trade-offs between paging structures, and the importance of correct frame allocation and error handling.
