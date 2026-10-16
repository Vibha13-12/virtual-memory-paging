
# Virtual Memory Paging Simulator — Project Report

**Name:** Reddy Gurucharana Lakshmi Vibha  
**NetID:** qc8297

## 1. Abstract

This project implements a virtual memory paging simulator in C++17. It demonstrates address translation, physical memory management, page-table operations, frame allocation, and the behavior of single-level and multi-level paging.

The simulator accepts commands through standard input and reports mapping results, memory-access results, page faults, allocation errors, and statistics. It operates on simulated physical memory rather than the host operating system's actual memory-management structures.

## 2. Objectives

The objectives of this project are:

- Configure virtual and physical memory using four decimal byte counts.
- Calculate address widths, page offsets, virtual page numbers, and physical frame numbers.
- Simulate byte-addressable physical memory.
- Allocate, track, and release physical frames.
- Implement single-level and multi-level page tables.
- Translate virtual addresses into physical addresses.
- Support mapping, unmapping, reading, and writing operations.
- Handle invalid commands, invalid mappings, and unmapped-page faults.
- Detect physical memory exhaustion and test allocation rollback.
- Collect statistics and compare paging implementations.

## 3. System Design

The simulator consists of the following components.

### 3.1 Configuration

The configuration component validates memory sizes and calculates address widths, page offsets, virtual page counts, physical frame counts, entries per table, index widths, and the number of paging levels.

The configuration requires power-of-two sizes. The page size must be larger than the page-table entry size and cannot exceed either address space.

### 3.2 Physical Memory

Physical memory is represented by a byte-addressable storage structure. The simulator reads and writes individual bytes using translated physical addresses.

The memory component also supports reading and writing page-table entries and clearing frames.

### 3.3 Frame Allocator

The frame allocator tracks physical frames and distinguishes among free frames, data frames, and page-table frames.

It supports allocation, reservation, release, and frame-usage statistics. The allocator reports when a requested frame cannot be allocated.

### 3.4 Single-Level Page Table

The single-level implementation uses one page table to map virtual page numbers to physical frame numbers.

Each page-table entry contains a VALID bit in its highest-order bit and the physical frame number in the remaining bits. Invalid entries are represented by zero.

The complete page table is allocated during initialization. Consequently, its memory requirement depends on the size of the virtual address space, even if only a few pages are mapped.

### 3.5 Multi-Level Page Table

The multi-level implementation organizes page tables hierarchically. The root table is allocated during initialization, while lower-level tables are allocated as needed.

Page-table entries are stored in simulated physical-memory frames. Address translation uses the appropriate virtual page number indexes to traverse the hierarchy.

The implementation also supports reclaiming empty lower-level page tables after unmapping.

### 3.6 Command Parser and Statistics

The command parser processes configuration, mapping, unmapping, read, write, and statistics commands in input order.

It validates command arguments, reports errors, and continues processing subsequent commands. Statistics track successful operations, rejected commands, and unmapped-page faults.

## 4. Implementation

The project is implemented in C++17 and organized into header files, source files, unit tests, and trace files.

The main executable supports two modes:

- **Single-level paging:** the default mode, launched using `./vm_simulator`.
- **Multi-level paging:** launched using `./vm_simulator --multi`.

The configuration and address-translation components calculate the virtual page number and offset from a virtual address. The page-table implementation translates the virtual page number into a physical frame number. The physical address is then calculated by combining the physical frame number with the page offset.

The simulator uses a VALID bit to distinguish valid mappings from invalid entries. Mapping and unmapping operations update page-table entries and the frame allocator.

## 5. Testing Methodology

The project includes four unit-test suites:

1. Physical memory tests.
2. Frame allocator tests.
3. Single-level paging tests.
4. Multi-level paging tests.

The following trace files were exercised:

- `traces/example.txt`
- `traces/error_cases.txt`
- `traces/exhaustion.txt`

The tests and traces cover ordinary mappings, reads, writes, unmapping, unmapped-page faults, invalid commands, duplicate mappings, occupied physical frames, and constrained-memory configurations.

The exhaustion trace was executed in both single-level and multi-level modes to examine their different physical-memory requirements.

## 6. Experimental Results

### 6.1 Unit Tests

The command `make clean && make && make test` is used to rebuild the project and execute the unit tests.

The previously observed test output confirmed that all four unit-test suites passed. The tests should be rerun after the final source and documentation changes to confirm the final project state.

### 6.2 Example Trace

Both paging implementations processed the example trace.

The observed statistics for each implementation were:

| Metric | Result |
|---|---:|
| Successful mappings | 2 |
| Successful unmappings | 1 |
| Successful reads | 1 |
| Successful writes | 1 |
| Unmapped-page faults | 1 |
| Rejected commands | 0 |

With the example configuration, the single-level implementation reported 64 page-table frames, one data frame, and 191 free frames.

The multi-level implementation reported two page-table frames, one data frame, and 253 free frames.

These results demonstrate that the multi-level implementation used fewer page-table frames for this particular workload.

### 6.3 Error-Handling Trace

Both implementations processed the tested invalid operations without crashing.

The observed statistics were:

| Metric | Result |
|---|---:|
| Successful mappings | 1 |
| Successful unmappings | 0 |
| Successful reads | 1 |
| Successful writes | 0 |
| Unmapped-page faults | 1 |
| Rejected commands | 5 |

The trace exercised duplicate mappings, occupied physical frames, invalid write values, invalid unmapping, and unknown commands.

The results demonstrate that invalid operations are reported and that subsequent commands can still be processed.

### 6.4 Memory-Exhaustion and Rollback Test

The memory-exhaustion experiment used the following configuration:

| Parameter | Value |
|---|---:|
| Physical memory | 1,024 bytes |
| Virtual memory | 65,536 bytes |
| Page-table entry size | 4 bytes |
| Page size | 256 bytes |
| Physical frames | 4 |
| Virtual pages | 256 |

The multi-level trace performed the following operations:

1. Mapped virtual page `0x0` to physical frame `0x2`.
2. Attempted to map virtual page `0x40` to physical frame `0x3`.
3. Printed statistics after the failed mapping.
4. Read from virtual address `0x000`.
5. Printed the statistics again.

The first mapping succeeded. The second mapping failed with the message `Physical memory exhausted.` because the simulator could not allocate the additional lower-level page table required for the mapping.

The subsequent read succeeded and returned `0x0`. This demonstrated that the existing mapping remained usable after the failed allocation attempt.

The final multi-level allocation counts were:

| Resource | Count |
|---|---:|
| Data frames | 1 |
| Page-table frames | 2 |
| Free frames | 1 |
| Total physical frames | 4 |

These counts are consistent with the temporary reservation being released after the failed mapping.

The same trace was executed in single-level mode. With 256 virtual pages and four-byte entries, the single-level page table required four frames. Because physical memory also contained only four frames, the page table consumed all available frames during initialization.

Both mapping attempts were consequently rejected because their requested physical frames were already occupied by page-table storage. The subsequent read produced an unmapped-page fault.

This experiment demonstrates the different memory requirements of the two implementations under a severely constrained physical-memory configuration.

## 7. Comparison of Paging Implementations

| Characteristic | Single-Level Paging | Multi-Level Paging |
|---|---|---|
| Table organization | One page table | Hierarchical page tables |
| Table allocation | Full table reservation | Lower-level tables allocated as needed |
| Sparse address-space overhead | Can be higher | Can be lower |
| Translation | One page-table lookup in the model | Multiple hierarchical lookups |
| Example page-table frames | 64 | 2 |
| Example data frames | 1 | 1 |
| Example free frames | 191 | 253 |

The example workload demonstrates that the multi-level implementation uses fewer page-table frames than the single-level implementation.

However, the advantage depends on the address space and the pages being accessed. Multi-level paging introduces additional lookup steps, while single-level paging provides a simpler table organization.

These results are measurements from the simulator and should not be interpreted as benchmarks for real operating systems.

## 8. Required Analysis

### 8.1 Address and Page-Table Calculations

For the example configuration, the physical address space is 65,536 bytes, the virtual address space is 1,048,576 bytes, the page size is 256 bytes, and the page-table entry size is four bytes.

**Physical address width**

Since the physical address space is \(2^{16}\) bytes, the physical address width is 16 bits.

**Virtual address width**

Since the virtual address space is \(2^{20}\) bytes, the virtual address width is 20 bits.

**Page offset**

The page size is \(256 = 2^8\) bytes. Therefore, the page offset requires 8 bits.

**Number of virtual pages**

\[
\frac{1,048,576}{256}=4,096
\]

The virtual address space contains 4,096 virtual pages.

**Number of physical frames**

\[
\frac{65,536}{256}=256
\]

Physical memory contains 256 frames.

**Entries per page-sized table**

\[
\frac{256}{4}=64
\]

A page-sized table can contain 64 entries.

**Index width and number of levels**

Since 64 is \(2^6\), each full table index requires 6 bits. The virtual page number contains \(20-8=12\) bits. Therefore, two levels are required, with a 6-bit index at each level for this configuration.

The resulting virtual address decomposition is:

| Field | Width |
|---|---:|
| Level 1 index | 6 bits |
| Level 2 index | 6 bits |
| Page offset | 8 bits |
| Total | 20 bits |

### 8.2 Single-Level Page-Table Memory

The single-level page table requires one entry for every virtual page.

\[
4,096 \times 4 = 16,384\text{ bytes}
\]

The number of frames required is:

\[
\frac{16,384}{256}=64\text{ frames}
\]

Therefore, the single-level page table occupies 16,384 bytes, equivalent to 64 physical frames, under the example configuration.

### 8.3 Multi-Level Page-Table Memory

The multi-level implementation allocates lower-level tables when required by a mapping.

For the example trace, the observed page-table allocation was two frames: one root table and one lower-level table.

The corresponding page-table memory is:

\[
2 \times 256=512\text{ bytes}
\]

For this workload, multi-level paging therefore uses 512 bytes of page-table storage, compared with 16,384 bytes for single-level paging.

The memory savings for this example are:

\[
16,384-512=15,872\text{ bytes}
\]

This comparison applies to the observed example workload, not every possible virtual-memory configuration.

### 8.4 Why Multi-Level Paging Helps Sparse Address Spaces

A single-level page table requires an entry for every virtual page, even when most pages are unused.

Multi-level paging divides the table into smaller structures. The root table is allocated initially, while lower-level tables can be created only for portions of the virtual address space that need mappings.

For a sparse address space, this avoids allocating lower-level tables for unused regions and can substantially reduce page-table memory usage.

If a workload accesses many different regions of the virtual address space, more lower-level tables will be required, reducing the memory savings.

### 8.5 Behavior When Physical Memory Is Exhausted

When physical memory cannot supply the frames required for a mapping, the simulator reports an allocation error.

In the tested multi-level scenario, the simulator could not allocate the additional page-table frame required for a new mapping. The operation failed, the existing mapping remained readable, and the final frame counts showed one data frame, two page-table frames, and one free frame.

In the single-level scenario, initialization reserved all four available frames for the page table. No frames remained for data pages, so mapping attempts failed.

These experiments illustrate that page-table organization directly affects how much physical memory remains available for mapped data.

## 9. Limitations and Future Improvements

Although the existing tests cover core paging operations and the tested allocation-failure scenario, additional work could strengthen the project:

- Add automated tests for virtual-address and physical-frame boundaries.
- Expand malformed-input and invalid-configuration tests.
- Add automated checks for allocation rollback across different failure points.
- Test unmapping and reclamation of empty lower-level page tables across several hierarchy levels.
- Compare frame usage across multiple sparse and dense workloads.
- Add reproducible performance measurements.
- Expand command documentation and error specifications.

## 10. Conclusion

The project implements a C++17 virtual-memory paging simulator with single-level and multi-level page tables, simulated physical memory, frame allocation, address translation, mapping and unmapping operations, and statistics reporting.

The example workload demonstrated that multi-level paging used fewer page-table frames than single-level paging. The error-handling trace exercised invalid operations and unmapped-page faults. The constrained-memory experiment demonstrated that a multi-level mapping could fail because of physical-memory exhaustion while an existing mapping remained readable.

The single-level exhaustion experiment also demonstrated that reserving the complete page table can consume all physical frames in a small-memory configuration.

Together, these experiments illustrate the trade-offs between single-level and multi-level paging and demonstrate the importance of frame allocation, page-table organization, validity checking, and allocation-failure handling.