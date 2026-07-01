CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -pedantic -Iinclude
BUILD_DIR := build
TARGET := $(BUILD_DIR)/compilador
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean test test-keep

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: src/%.cpp include/*.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TARGET)
	python3 tools/test.py --no-build

test-keep: $(TARGET)
	python3 tools/test.py --no-build --keep-artifacts

clean:
	rm -rf $(BUILD_DIR) test-artifacts
