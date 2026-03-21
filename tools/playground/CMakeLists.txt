# tools/playground/CMakeLists.txt
#
# T81Lang In-Browser Playground — Emscripten Wasm build
#
# Prerequisites:
#   source /path/to/emsdk/emsdk_env.sh
#   cmake -B build-wasm -S . -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
#         -DT81_BUILD_PLAYGROUND=ON -DT81_BUILD_TESTS=OFF -DT81_BUILD_EXAMPLES=OFF \
#         -DT81_BUILD_BENCHMARKS=OFF -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-wasm --target t81_playground
#
# Outputs:
#   build-wasm/tools/playground/t81vm.js
#   build-wasm/tools/playground/t81vm.wasm
#
# Copy both files next to tools/playground/web/ and open index.html.

cmake_minimum_required(VERSION 3.16)

add_executable(t81_playground
    src/wasm_api.cpp
)

target_include_directories(t81_playground PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Link against the minimal set needed for lex → parse → IR → VM.
# t81_tool_cli provides build_program_from_source().
# t81_vm provides make_interpreter_vm().
target_link_libraries(t81_playground PRIVATE
    t81_tool_cli
    t81_vm
    t81_lang_frontend
    t81_isa
    t81_core
)

# Disable platform-specific SIMD that cannot cross-compile to Wasm.
target_compile_definitions(t81_playground PRIVATE
    EMSCRIPTEN_BUILD=1
    T81_DISABLE_AVX2=1
    T81_DISABLE_NEON=1
)

if(EMSCRIPTEN)
    # Use embind for JS↔C++ bindings.
    target_compile_options(t81_playground PRIVATE --bind)

    set_target_properties(t81_playground PROPERTIES
        OUTPUT_NAME "t81vm"
        SUFFIX ".js"
    )

    target_link_options(t81_playground PRIVATE
        --bind
        -s MODULARIZE=1
        -s EXPORT_NAME=T81VM
        -s ALLOW_MEMORY_GROWTH=1
        -s INITIAL_MEMORY=67108864    # 64 MB initial
        -s MAXIMUM_MEMORY=536870912   # 512 MB ceiling
        -s STACK_SIZE=4194304         # 4 MB stack
        -s ENVIRONMENT=web,worker
        -s DYNAMIC_EXECUTION=0
        -s ASSERTIONS=0
        -O3
    )
endif()
