
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude

BUILD_DIR = build

COMMON_SOURCES = \
	src/Config.cpp \
	src/PhysicalMemory.cpp \
	src/FrameAllocator.cpp

PAGING_SOURCES = \
	src/SingleLevelPageTable.cpp \
	src/MultiLevelPageTable.cpp

SIMULATOR_SOURCES = \
	$(COMMON_SOURCES) \
	$(PAGING_SOURCES) \
	src/CommandParser.cpp

.PHONY: all test clean

all: vm_simulator

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

vm_simulator: src/main.cpp $(SIMULATOR_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_memory: tests/test_memory.cpp src/PhysicalMemory.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_allocator: tests/test_allocator.cpp src/FrameAllocator.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_single_level: tests/test_single_level.cpp $(COMMON_SOURCES) src/SingleLevelPageTable.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_multi_level: tests/test_multi_level.cpp $(COMMON_SOURCES) src/MultiLevelPageTable.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(BUILD_DIR)/test_memory $(BUILD_DIR)/test_allocator $(BUILD_DIR)/test_single_level $(BUILD_DIR)/test_multi_level
	./$(BUILD_DIR)/test_memory
	./$(BUILD_DIR)/test_allocator
	./$(BUILD_DIR)/test_single_level
	./$(BUILD_DIR)/test_multi_level

clean:
	rm -rf $(BUILD_DIR) vm_simulator
