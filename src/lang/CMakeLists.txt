add_library(t81_lang_frontend STATIC
  ${PROJECT_SOURCE_DIR}/lang/frontend/builtin_registry.cpp
  ${PROJECT_SOURCE_DIR}/lang/frontend/ir_generator.cpp
  ${PROJECT_SOURCE_DIR}/lang/frontend/lexer.cpp
  ${PROJECT_SOURCE_DIR}/lang/frontend/parser.cpp
  ${PROJECT_SOURCE_DIR}/lang/frontend/semantic_analyzer.cpp
  ${PROJECT_SOURCE_DIR}/lang/frontend/symbol_table.cpp
)
t81_preserve_root_output(t81_lang_frontend)
target_link_libraries(t81_lang_frontend PUBLIC t81_core)
target_include_directories(t81_lang_frontend PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(T81_ENABLE_LLVM)
  find_package(LLVM CONFIG
    HINTS /opt/homebrew/opt/llvm/lib/cmake/llvm
          /usr/local/opt/llvm/lib/cmake/llvm
          /usr/lib/llvm-18/lib/cmake/llvm)
  if(LLVM_FOUND)
    message(STATUS "[t81] LLVM ${LLVM_PACKAGE_VERSION} -- building LLVM IR backend")
    include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
    llvm_map_components_to_libnames(t81_llvm_libs
      core support irreader bitwriter analysis passes)
    add_library(t81_llvm STATIC
      ${PROJECT_SOURCE_DIR}/lang/llvm/tisc_to_llvm.cpp)
    t81_preserve_root_output(t81_llvm)
    target_include_directories(t81_llvm PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
      ${LLVM_INCLUDE_DIRS})
    target_link_libraries(t81_llvm PUBLIC t81_core ${t81_llvm_libs})
    target_compile_definitions(t81_llvm PUBLIC T81_HAS_LLVM=1)
    target_compile_features(t81_llvm PUBLIC ${T81_CXX_STD_FEATURE})
    set(T81_HAS_LLVM_BACKEND ON CACHE INTERNAL "LLVM backend compiled")
  else()
    message(STATUS "[t81] LLVM not found -- t81_llvm remains a stub")
    add_library(t81_llvm INTERFACE)
    target_include_directories(t81_llvm INTERFACE
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_llvm INTERFACE t81_core)
  endif()
else()
  add_library(t81_llvm INTERFACE)
  target_include_directories(t81_llvm INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_llvm INTERFACE t81_core)
endif()

if(T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND LLVM_FOUND)
  find_package(MLIR CONFIG
    HINTS /opt/homebrew/opt/llvm/lib/cmake/mlir
          /usr/local/opt/llvm/lib/cmake/mlir
          /usr/lib/llvm-18/lib/cmake/mlir)
  if(MLIR_FOUND)
    message(STATUS "[t81] MLIR ${MLIR_PACKAGE_VERSION} -- building MLIR frontend")
    include_directories(SYSTEM ${MLIR_INCLUDE_DIRS})
    add_definitions(${MLIR_DEFINITIONS})
    add_library(t81_dmath_runtime STATIC
      ${PROJECT_SOURCE_DIR}/lang/mlir/dmath_c_wrappers.cpp)
    t81_preserve_root_output(t81_dmath_runtime)
    target_include_directories(t81_dmath_runtime PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_dmath_runtime PUBLIC t81_core)
    target_compile_features(t81_dmath_runtime PUBLIC ${T81_CXX_STD_FEATURE})
    add_library(t81_mlir STATIC
      ${PROJECT_SOURCE_DIR}/lang/mlir/t81_dialect.cpp
      ${PROJECT_SOURCE_DIR}/lang/mlir/tisc_to_mlir.cpp
      ${PROJECT_SOURCE_DIR}/lang/mlir/mlir_to_llvm.cpp)
    t81_preserve_root_output(t81_mlir)
    target_include_directories(t81_mlir PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
      ${MLIR_INCLUDE_DIRS}
      ${LLVM_INCLUDE_DIRS})
    target_link_libraries(t81_mlir PUBLIC
      t81_core
      t81_dmath_runtime
      MLIRAnalysis
      MLIRArithDialect
      MLIRArithToLLVM
      MLIRBuiltinToLLVMIRTranslation
      MLIRControlFlowDialect
      MLIRControlFlowToLLVM
      MLIRFuncDialect
      MLIRFuncToLLVM
      MLIRLLVMDialect
      MLIRLLVMToLLVMIRTranslation
      MLIRLLVMIRTransforms
      MLIRMathDialect
      MLIRMathToLLVM
      MLIRMemRefDialect
      MLIRMemRefToLLVM
      MLIRPass
      MLIRTargetLLVMIRExport
      MLIRTransforms
      ${t81_llvm_libs})
    target_compile_definitions(t81_mlir PUBLIC T81_HAS_MLIR=1 T81_HAS_LLVM=1)
    target_compile_features(t81_mlir PUBLIC ${T81_CXX_STD_FEATURE})
    set(T81_HAS_MLIR_BACKEND ON CACHE INTERNAL "MLIR backend compiled")
  else()
    message(STATUS "[t81] MLIR not found -- t81_mlir remains a stub")
    add_library(t81_mlir INTERFACE)
    target_include_directories(t81_mlir INTERFACE
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_mlir INTERFACE t81_core)
  endif()
else()
  add_library(t81_mlir INTERFACE)
  target_include_directories(t81_mlir INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_mlir INTERFACE t81_core)
endif()

if(T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND LLVM_FOUND AND MLIR_FOUND)
  find_path(T81_CLANG_C_INCLUDE_DIR
    NAMES clang-c/Index.h
    HINTS /opt/homebrew/opt/llvm/include
          /usr/local/opt/llvm/include
          /usr/lib/llvm-18/include)
  find_library(T81_CLANG_C_LIBRARY
    NAMES clang libclang
    HINTS /opt/homebrew/opt/llvm/lib
          /usr/local/opt/llvm/lib
          /usr/lib/llvm-18/lib)
  if(T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY)
    message(STATUS "[t81] libclang -- building experimental C frontend")
    add_library(t81_c_frontend STATIC
      ${PROJECT_SOURCE_DIR}/lang/c_frontend/compile.cpp)
    t81_preserve_root_output(t81_c_frontend)
    target_include_directories(t81_c_frontend PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
      ${T81_CLANG_C_INCLUDE_DIR})
    target_link_libraries(t81_c_frontend PUBLIC
      t81_core
      t81_mlir
      ${T81_CLANG_C_LIBRARY})
    target_compile_definitions(t81_c_frontend PUBLIC T81_HAS_C_FRONTEND=1 T81_HAS_MLIR=1 T81_HAS_LLVM=1)
    target_compile_features(t81_c_frontend PUBLIC ${T81_CXX_STD_FEATURE})
  else()
    message(STATUS "[t81] libclang not found -- t81_c_frontend remains a stub")
    add_library(t81_c_frontend INTERFACE)
    target_include_directories(t81_c_frontend INTERFACE
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_c_frontend INTERFACE t81_core)
  endif()
else()
  add_library(t81_c_frontend INTERFACE)
  target_include_directories(t81_c_frontend INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_c_frontend INTERFACE t81_core)
endif()

if(T81_ENABLE_RUST_FRONTEND AND T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND LLVM_FOUND AND MLIR_FOUND AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY)
  find_program(T81_RUSTC_EXECUTABLE NAMES rustc)
  if(T81_RUSTC_EXECUTABLE)
    message(STATUS "[t81] rustc -- building experimental Rust frontend subset")
    add_library(t81_rust_frontend STATIC
      ${PROJECT_SOURCE_DIR}/lang/rust_frontend/compile.cpp)
    t81_preserve_root_output(t81_rust_frontend)
    target_include_directories(t81_rust_frontend PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_rust_frontend PUBLIC
      t81_core
      t81_c_frontend
      t81_mlir)
    target_compile_definitions(t81_rust_frontend PUBLIC T81_HAS_RUST_FRONTEND=1 T81_HAS_MLIR=1 T81_HAS_LLVM=1)
    target_compile_features(t81_rust_frontend PUBLIC ${T81_CXX_STD_FEATURE})
  else()
    message(STATUS "[t81] rustc not found -- t81_rust_frontend remains a stub")
    add_library(t81_rust_frontend INTERFACE)
    target_include_directories(t81_rust_frontend INTERFACE
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_rust_frontend INTERFACE t81_core)
  endif()
else()
  add_library(t81_rust_frontend INTERFACE)
  target_include_directories(t81_rust_frontend INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_rust_frontend INTERFACE t81_core)
endif()

if(T81_ENABLE_PYTHON_FRONTEND AND T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND LLVM_FOUND AND MLIR_FOUND AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  if(Python3_EXECUTABLE)
    message(STATUS "[t81] python3 -- building experimental Python frontend subset")
    add_library(t81_python_frontend STATIC
      ${PROJECT_SOURCE_DIR}/lang/python_frontend/compile.cpp)
    t81_preserve_root_output(t81_python_frontend)
    target_include_directories(t81_python_frontend PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_python_frontend PUBLIC
      t81_core
      t81_c_frontend
      t81_mlir)
    target_compile_definitions(t81_python_frontend PUBLIC
      T81_HAS_PYTHON_FRONTEND=1
      T81_HAS_C_FRONTEND=1
      T81_HAS_MLIR=1
      T81_HAS_LLVM=1
      T81_PYTHON3_EXECUTABLE="${Python3_EXECUTABLE}"
      T81_PYTHON_FRONTEND_SCRIPT_PATH="${PROJECT_SOURCE_DIR}/scripts/python_frontend/normalize_subset.py")
    target_compile_features(t81_python_frontend PUBLIC ${T81_CXX_STD_FEATURE})
  else()
    message(STATUS "[t81] python3 not found -- t81_python_frontend remains a stub")
    add_library(t81_python_frontend INTERFACE)
    target_include_directories(t81_python_frontend INTERFACE
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_python_frontend INTERFACE t81_core)
  endif()
else()
  add_library(t81_python_frontend INTERFACE)
  target_include_directories(t81_python_frontend INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_python_frontend INTERFACE t81_core)
endif()
