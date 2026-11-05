# Sealed CPA006 suite only. No evaluator is added to any hosted production library.
set(T81_CPA006_CASE "C01" CACHE STRING "Frozen CPA006 case embedded into the EFI")
file(READ ${CMAKE_CURRENT_LIST_DIR}/frozen/manifest.json CPA006_MANIFEST)
string(JSON CPA006_PROGRAM_HASH GET "${CPA006_MANIFEST}" files "${T81_CPA006_CASE}.raw13" sha256)
set(CPA006_PROGRAM ${CMAKE_CURRENT_LIST_DIR}/frozen/${T81_CPA006_CASE}.raw13)
file(SHA256 ${CPA006_PROGRAM} CPA006_ACTUAL_HASH)
if(NOT CPA006_PROGRAM_HASH STREQUAL CPA006_ACTUAL_HASH)
  message(FATAL_ERROR "CPA006 frozen input identity differs")
endif()
file(READ ${CMAKE_CURRENT_LIST_DIR}/frozen/suite.json CPA006_SUITE)
string(JSON CPA006_CASE_COUNT LENGTH "${CPA006_SUITE}" cases)
math(EXPR CPA006_LAST_CASE "${CPA006_CASE_COUNT}-1")
set(CPA006_LIMIT "")
foreach(INDEX RANGE ${CPA006_LAST_CASE})
  string(JSON CASE_ID GET "${CPA006_SUITE}" cases ${INDEX} id)
  if(CASE_ID STREQUAL T81_CPA006_CASE)
    string(JSON CPA006_LIMIT GET "${CPA006_SUITE}" cases ${INDEX} limit)
  endif()
endforeach()
if(CPA006_LIMIT STREQUAL "" OR CPA006_LIMIT GREATER 64)
  message(FATAL_ERROR "CPA006 unknown case or excessive limit")
endif()
file(READ ${CPA006_PROGRAM} CPA006_HEX HEX)
string(REGEX REPLACE "(..)" "0x\\1," CPA006_BYTES "${CPA006_HEX}")
file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/ternaryos/generated/cpa006_program.hpp
 CONTENT "#pragma once\nstatic constexpr unsigned char kCpa006Program[] = {${CPA006_BYTES}};\nstatic constexpr const char* kCpa006Digest = \"${CPA006_PROGRAM_HASH}\";\nstatic constexpr unsigned kCpa006Limit = ${CPA006_LIMIT};\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CPA006_PROGRAM}
 ${CMAKE_CURRENT_LIST_DIR}/frozen/suite.json ${CMAKE_CURRENT_LIST_DIR}/frozen/manifest.json)
set(T81_CPA006_OBJECT ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_cpa006.obj)
add_custom_command(OUTPUT ${T81_CPA006_OBJECT}
 COMMAND ${T81_LLVM_CLANG} --target=aarch64-pc-windows-msvc
 -O2 -std=c++17 -ffreestanding -nostdlib -fno-stack-protector -fno-exceptions -fno-rtti
 -fno-use-cxa-atexit -fstack-usage -I${CMAKE_BINARY_DIR}/ternaryos/generated
 -c ${CMAKE_CURRENT_LIST_DIR}/cpa006_evaluator.cpp -o ${T81_CPA006_OBJECT}
 DEPENDS ${CMAKE_CURRENT_LIST_DIR}/cpa006_evaluator.cpp ${CMAKE_CURRENT_LIST_DIR}/cpa006_evaluator.hpp
 ${CMAKE_BINARY_DIR}/ternaryos/generated/cpa006_program.hpp ${CPA006_PROGRAM})
list(APPEND T81_CPA004_OBJECTS ${T81_CPA006_OBJECT})
list(APPEND T81_CPA004_DEFINITIONS -DT81_CPA006_EXPERIMENT=1)
