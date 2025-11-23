CXX       ?= g++
CXXFLAGS  ?= -std=c++17 -O2 -Wall -Wextra
INCLUDES  := -Iinclude

# Default target
.PHONY: all
all: demo tests

# Build the demo
demo: examples/demo.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# ---- Tests ----
TESTS := \
	build/t81_bigint_test \
	build/t81_fraction_test \
	build/t81_tensor_transpose_test \
	build/t81_tensor_slice_test \
	build/t81_tensor_reshape_test \
	build/t81_tensor_loader_test \
	build/t81_canonfs_io_test

tests: $(TESTS)

build/t81_bigint_test: tests/cpp/bigint_roundtrip.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_fraction_test: tests/cpp/fraction_roundtrip.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_transpose_test: tests/cpp/tensor_transpose_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_slice_test: tests/cpp/tensor_slice_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_reshape_test: tests/cpp/tensor_reshape_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_loader_test: tests/cpp/tensor_loader_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_canonfs_io_test: tests/cpp/canonfs_io_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Run all tests (expects test data present, and nlohmann/json header if used)
.PHONY: run-tests
run-tests: tests
	@./build/t81_bigint_test || exit 1
	@./build/t81_fraction_test || exit 1
	@./build/t81_tensor_transpose_test || exit 1
	@./build/t81_tensor_slice_test || exit 1
	@./build/t81_tensor_reshape_test || exit 1
	@./build/t81_tensor_loader_test || exit 1
	@./build/t81_canonfs_io_test || exit 1
	@echo "All tests passed."

# Clean
.PHONY: clean
clean:
	@rm -rf build demo
