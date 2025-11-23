CXX       ?= g++
CXXFLAGS  ?= -std=c++20 -O2 -Wall -Wextra
INCLUDES  := -Iinclude -Isrc
CC		  ?= cc

# Default target
.PHONY: all
all: demo examples tests

# -------- Examples --------
EXAMPLES := \
	build/t81_demo \
	build/t81_tensor_ops \
	build/t81_ir_roundtrip \
	build/axion_demo

examples: $(EXAMPLES)

build/t81_demo: examples/demo.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_ops: examples/tensor_ops.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_ir_roundtrip: examples/ir_roundtrip.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/axion_demo: examples/axion_demo.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_unary_test: tests/cpp/tensor_unary_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_reduce_test: tests/cpp/tensor_reduce_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# -------- Tests --------
TESTS := \
	build/t81_bigint_test \
	build/t81_fraction_test \
	build/t81_tensor_transpose_test \
	build/t81_tensor_slice_test \
	build/t81_tensor_reshape_test \
	build/t81_tensor_loader_test \
	build/t81_canonfs_io_test \
	build/t81_ir_encoding_test \
	build/t81_hash_stub_test \
	build/t81_axion_stub_test \
	build/t81_codec_base243_test \
	build/t81_tensor_shape_test \
	build/t81_ternary_arith_test \
	build/t81_tensor_matmul_test \
	build/t81_tensor_reduce_test \
	build/t81_tensor_broadcast_test \
	build/t81_entropy_test \
	build/t81_c_api_bigint_test \
	build/t81_tensor_unary_test \
	build/t81_tensor_reduce_test

tests: $(TESTS)

build/t81_bigint_test: tests/cpp/bigint_roundtrip.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< src/bigint/divmod.cpp src/bigint/gcd.cpp -o $@

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

build/t81_tensor_loader_test: tests/cpp/tensor_loader_test.cpp src/io/tensor_loader.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

build/t81_canonfs_io_test: tests/cpp/canonfs_io_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_ir_encoding_test: tests/cpp/ir_encoding_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_hash_stub_test: tests/cpp/hash_stub_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< src/hash/canonhash81.cpp -o $@

build/t81_axion_stub_test: tests/cpp/axion_stub_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_codec_base243_test: tests/cpp/codec_base243_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_shape_test: tests/cpp/tensor_shape_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_ternary_arith_test: tests/cpp/ternary_arith_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_matmul_test: tests/cpp/tensor_matmul_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_reduce_test: tests/cpp/tensor_reduce_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_tensor_broadcast_test: tests/cpp/tensor_broadcast_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

build/t81_entropy_test: tests/cpp/entropy_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# C API object (C++)
build/t81_c_api.o: src/c_api/t81_c_api.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# C API bigint test (compile C + link with C++)
build/t81_c_api_bigint_test: tests/cpp/c_api_bigint_test.c build/t81_c_api.o
	@mkdir -p build
	$(CC)  $(INCLUDES) -c tests/cpp/c_api_bigint_test.c -o build/c_api_bigint_test.o
	$(CXX) $(CXXFLAGS) build/c_api_bigint_test.o build/t81_c_api.o -o $@

# Run all tests
.PHONY: run-tests
run-tests: tests
	@./build/t81_bigint_test || exit 1
	@./build/t81_fraction_test || exit 1
	@./build/t81_tensor_transpose_test || exit 1
	@./build/t81_tensor_slice_test || exit 1
	@./build/t81_tensor_reshape_test || exit 1
	@./build/t81_tensor_loader_test || exit 1
	@./build/t81_canonfs_io_test || exit 1
	@./build/t81_ir_encoding_test || exit 1
	@./build/t81_hash_stub_test || exit 1
	@./build/t81_axion_stub_test || exit 1
	@./build/t81_codec_base243_test || exit 1
	@./build/t81_tensor_shape_test || exit 1
	@./build/t81_ternary_arith_test || exit 1
	@./build/t81_tensor_matmul_test || exit 1
	@./build/t81_tensor_reduce_test || exit 1
	@./build/t81_tensor_broadcast_test || exit 1
	@./build/t81_entropy_test || exit 1
	@./build/t81_c_api_bigint_test || exit 1
	@echo "All tests passed."

# Convenience
.PHONY: run-examples
run-examples: examples
	@./build/t81_demo
	@./build/t81_tensor_ops
	@./build/t81_ir_roundtrip
	@./build/axion_demo

# Clean
.PHONY: clean
clean:
	@rm -rf build demo
