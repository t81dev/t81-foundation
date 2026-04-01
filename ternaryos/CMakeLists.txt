message(STATUS "[t81] T81_ENABLE_TERNARYOS=ON — building TernaryOS HAL, shell, and user environment")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

  add_library(t81_ternaryos_hal STATIC
    hal/hal_main.cpp
    hal/hal_c_abi.cpp
    hal/interrupt_table.cpp
    hal/hosted_stub.cpp
    hal/virtualbox_platform.cpp
    hal/virtualbox_guest_devices.cpp
    hal/qemu_platform.cpp
    hal/qemu_kernel_entry.cpp
    kernel/kernel_runtime.cpp
    kernel/kernel_actions.cpp
    kernel/kernel_abi.cpp
    kernel/kernel_abi_wire.cpp
    kernel/kernel_executable.cpp
    kernel/kernel_faults.cpp
    kernel/kernel_interrupts.cpp
    kernel/kernel_interrupt_policy.cpp
    kernel/kernel_loader.cpp
    kernel/kernel_lifecycle.cpp
    kernel/kernel_trap_shim.cpp
    hal/aarch64_trap_entry.cpp
    kernel/kernel_pager.cpp
    kernel/kernel_queries.cpp
    kernel/kernel_runtime_utils.cpp
    kernel/kernel_views.cpp
    dev/hosted_block_dev.cpp
    dev/virtio_blk_mmio.cpp
    dev/virtualbox_ahci_dev.cpp
    dev/virtualbox_e1000_dev.cpp
    dev/virtualbox_vmsvga_dev.cpp
    dev/canon_store.cpp
    dev/framebuffer.cpp
    dev/ttf.cpp
    dev/pl011_uart.cpp
    dev/gicv3.cpp
    mmu/ternary_page_alloc.cpp
    mmu/page_table.cpp
    sched/context_switch.cpp
    # Phase 3 — scheduling & IPC
    sched/run_queue.cpp
    sched/scheduler.cpp
    ipc/canon_message.cpp)

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    target_sources(t81_ternaryos_hal PRIVATE
      hal/aarch64_exception_vectors.S)
  endif()

  target_include_directories(t81_ternaryos_hal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

  target_link_libraries(t81_ternaryos_hal PUBLIC t81_core t81_axion)
  target_compile_features(t81_ternaryos_hal PUBLIC ${T81_CXX_STD_FEATURE})
  # The HAL library is only ever linked into hosted binaries (snapshot tools,
  # tests, demos). The bare-metal EFI images are compiled separately via
  # clang-18 custom commands and do NOT link t81_ternaryos_hal.
  # T81_TERNARYOS_HOSTED_BUILD disables raw MMIO dereferences that would
  # SIGSEGV when run on a Linux host (physical addresses are not mapped in
  # user-space).
  target_compile_definitions(t81_ternaryos_hal PUBLIC T81_TERNARYOS_HOSTED_BUILD=1)

  # DPE kernel wiring — compiled into t81_ternaryos_hal only when both flags are ON.
  if(T81_ENABLE_DPE)
    target_sources(t81_ternaryos_hal PRIVATE
      kernel/kernel_epoch.cpp)
    target_include_directories(t81_ternaryos_hal PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)
    target_link_libraries(t81_ternaryos_hal PUBLIC t81_dpe)
    target_compile_definitions(t81_ternaryos_hal PUBLIC T81_ENABLE_DPE=1)
  endif()

  add_library(t81_ternaryos_shell STATIC
    ${PROJECT_SOURCE_DIR}/kernel/axion/shell/shell_session.cpp)
  target_include_directories(t81_ternaryos_shell PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
  target_link_libraries(t81_ternaryos_shell PUBLIC t81_ternaryos_hal)
  target_compile_features(t81_ternaryos_shell PUBLIC ${T81_CXX_STD_FEATURE})

  # RFC-00B9 — TernaryOS User Environment (t81-init, session manager, t81sh)
  add_library(t81_ternaryos_userenv STATIC
    ${PROJECT_SOURCE_DIR}/kernel/axion/userenv/user_env.cpp)
  target_include_directories(t81_ternaryos_userenv PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
  target_link_libraries(t81_ternaryos_userenv PUBLIC t81_ternaryos_hal)
  target_compile_features(t81_ternaryos_userenv PUBLIC ${T81_CXX_STD_FEATURE})
  target_link_libraries(t81_ternaryos_shell PUBLIC t81_ternaryos_userenv)

  add_executable(t81_ternaryos_demo
    apps/demo.cpp)
  target_link_libraries(t81_ternaryos_demo PRIVATE t81_ternaryos_hal)

  add_executable(t81_ternaryos_shell_demo
    apps/shell_demo.cpp)
  target_link_libraries(t81_ternaryos_shell_demo PRIVATE t81_ternaryos_shell)

  add_executable(t81_ternaryos_shell_startup_snapshot
    shell/shell_startup_snapshot.cpp)
  target_link_libraries(t81_ternaryos_shell_startup_snapshot PRIVATE t81_ternaryos_shell)

  add_executable(t81_ternaryos_phase4_startup_snapshot
    dev/phase4_startup_snapshot.cpp)
  target_link_libraries(t81_ternaryos_phase4_startup_snapshot PRIVATE t81_ternaryos_hal)

  add_executable(t81_ternaryos_qemu_slice6_snapshot
    dev/qemu_slice6_startup_snapshot.cpp)
  target_link_libraries(t81_ternaryos_qemu_slice6_snapshot PRIVATE t81_ternaryos_hal)

  if(T81_BUILD_TUI)
    add_executable(t81_ternaryos_shell_tui
      apps/shell_tui.cpp)
    target_link_libraries(t81_ternaryos_shell_tui PRIVATE
      t81_ternaryos_shell
      ftxui::screen
      ftxui::dom
      ftxui::component)
    target_compile_features(t81_ternaryos_shell_tui PRIVATE ${T81_CXX_STD_FEATURE})
  endif()

  set(T81_TERNARYOS_GUEST_ARTIFACT_DEPS t81_ternaryos_demo)
  set(T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_demo)
  find_program(T81_LLVM_CLANG NAMES clang PATHS /opt/homebrew/opt/llvm/bin NO_DEFAULT_PATH)
  find_program(T81_LLD_LINK NAMES lld-link PATHS /opt/homebrew/bin /opt/homebrew/opt/lld/bin NO_DEFAULT_PATH)
  set(T81_TERNARYOS_QEMU_SLICE6_EFI_AVAILABLE OFF)
  if(T81_LLVM_CLANG)
    set(T81_TERNARYOS_EFI_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox/BOOTX64.obj)
    set(T81_TERNARYOS_X86_64_EFI_SHIM_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox/BOOTX64_shim.obj)
    set(T81_TERNARYOS_X86_64_EFI ${CMAKE_BINARY_DIR}/ternaryos/virtualbox/BOOTX64.EFI)
    set(T81_TERNARYOS_ARMV8_EFI_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64.obj)
    set(T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64_CTRL.obj)
    set(T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64_shim.obj)
    set(T81_TERNARYOS_ARMV8_EFI ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64.EFI)
    set(T81_TERNARYOS_ARMV8_EFI_CTRL ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64_CTRL.EFI)
    set(T81_TERNARYOS_ARMV8_STARTUP_SHELL_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-shell-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_shell.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_SESSION_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-session-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_session.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_HISTORY_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-history-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_history.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_STORE_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-store-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_store.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_REF_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-ref-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_REF_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_ref.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_REPORT_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-report-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_report.h)
    set(T81_TERNARYOS_ARMV8_PHASE4_STARTUP_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-phase4-generated.txt)
    set(T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_phase4_startup.h)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_EFI_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
      COMMAND ${T81_LLVM_CLANG}
              --target=x86_64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_efi_stub.c
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -o ${T81_TERNARYOS_EFI_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_efi_stub.c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/hal_c_abi.h)
    add_custom_target(t81_ternaryos_virtualbox_efi_obj
      DEPENDS ${T81_TERNARYOS_EFI_OBJ})
    list(APPEND T81_TERNARYOS_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_efi_obj)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
      COMMAND ${T81_LLVM_CLANG}
              --target=x86_64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_x86_64_efi_shim.c
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -o ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_x86_64_efi_shim.c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/hal_c_abi.h)
    add_custom_target(t81_ternaryos_virtualbox_x86_64_efi_shim_obj
      DEPENDS ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ})
    list(APPEND T81_TERNARYOS_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_x86_64_efi_shim_obj)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_STORE_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_REF_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_REF_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR}
             ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_TXT}
             ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/generated
      COMMAND $<TARGET_FILE:t81_ternaryos_shell_startup_snapshot>
              ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR}
      COMMAND $<TARGET_FILE:t81_ternaryos_phase4_startup_snapshot>
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_TXT}
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR}
      DEPENDS t81_ternaryos_shell_startup_snapshot
              t81_ternaryos_phase4_startup_snapshot)
    add_custom_target(t81_ternaryos_shell_startup_snapshot_artifact
      DEPENDS ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR}
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_TXT}
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR})
    list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_shell_startup_snapshot_artifact)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_ARMV8_EFI_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_armv8_efi_stub.c
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -I${CMAKE_BINARY_DIR}/ternaryos/generated
              -o ${T81_TERNARYOS_ARMV8_EFI_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_armv8_efi_stub.c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/hal_c_abi.h
              ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR}
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR})
    add_custom_target(t81_ternaryos_virtualbox_armv8_efi_obj
      DEPENDS ${T81_TERNARYOS_ARMV8_EFI_OBJ})
    list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi_obj)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_armv8_efi_control.c
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -o ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_armv8_efi_control.c)
    add_custom_target(t81_ternaryos_virtualbox_armv8_efi_control_obj
      DEPENDS ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ})
    list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi_control_obj)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_armv8_efi_shim.c
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -o ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/virtualbox_armv8_efi_shim.c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/hal_c_abi.h)
    add_custom_target(t81_ternaryos_virtualbox_armv8_efi_shim_obj
      DEPENDS ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ})
    list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi_shim_obj)

    # ── Slice 8: AArch64 exception vector table assembly ──────────────────────
    # Compile aarch64_exception_vectors.S for the AArch64 PE/COFF target so
    # the vector table binary is available for inclusion in EFI images.
    # The resulting object is not linked into any EFI binary in this slice;
    # it proves the assembly builds cleanly and can be inspected with objdump.
    set(T81_TERNARYOS_AARCH64_VECTORS_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/aarch64/aarch64_exception_vectors.obj)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_AARCH64_VECTORS_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/aarch64
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/aarch64_exception_vectors.S
              -o ${T81_TERNARYOS_AARCH64_VECTORS_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/aarch64_exception_vectors.S
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/aarch64_trap_entry.hpp)
    add_custom_target(t81_ternaryos_aarch64_exception_vectors_obj
      DEPENDS ${T81_TERNARYOS_AARCH64_VECTORS_OBJ})

    # ── Slice 9: EL0 init stub (axion_el0_entry) ──────────────────────────────
    # Compiled with the same freestanding AArch64 PE/COFF constraints as the
    # exception vector table.  Linked into BOOTAA64.EFI; provides the minimal
    # EL0 userland entry point that proves the EL0→SVC→EL1 roundtrip.
    set(T81_TERNARYOS_AARCH64_EL0_INIT_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/aarch64/BOOTAA64_el0_init.obj)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_AARCH64_EL0_INIT_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/aarch64
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-stack-protector
              -nostdlib
              -I${CMAKE_CURRENT_SOURCE_DIR}/hal
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/axion_el0_init.S
              -o ${T81_TERNARYOS_AARCH64_EL0_INIT_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/axion_el0_init.S
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/kernelcall_abi.inc)
    add_custom_target(t81_ternaryos_aarch64_el0_init_obj
      DEPENDS ${T81_TERNARYOS_AARCH64_EL0_INIT_OBJ})

    # ── Slice 6: QEMU AArch64 EDK2 guest image ────────────────────────────────
    set(T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64.obj)
    set(T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_bare.obj)
    set(T81_TERNARYOS_QEMU_SLICE6_CPP_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_cpp.obj)
    set(T81_TERNARYOS_QEMU_SLICE6_EFI
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64.EFI)
    set(T81_TERNARYOS_QEMU_SLICE6_STARTUP_TXT
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/slice6-startup-generated.txt)
    set(T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR
        ${CMAKE_BINARY_DIR}/ternaryos/generated/qemu_slice6_startup.h)

    add_custom_command(
      OUTPUT  ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_TXT}
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/generated
      COMMAND $<TARGET_FILE:t81_ternaryos_qemu_slice6_snapshot>
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_TXT}
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR}
      DEPENDS t81_ternaryos_qemu_slice6_snapshot)
    add_custom_target(t81_ternaryos_qemu_slice6_snapshot_data
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_TXT}
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR})

    add_custom_command(
      OUTPUT  ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_armv8_efi_stub.c
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -I${CMAKE_BINARY_DIR}/ternaryos/generated
              -o ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_armv8_efi_stub.c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/hal_c_abi.h
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR})
    add_custom_target(t81_ternaryos_qemu_slice6_efi_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ})

    # Bare-metal kernel entry (post-ExitBootServices PL011/GIC/timer probe).
    add_custom_command(
      OUTPUT  ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_bare_kernel.c
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -o ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_bare_kernel.c)
    add_custom_target(t81_ternaryos_qemu_slice6_bare_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ})

    # C++ kernel bridge — T81 banner + interactive t81> shell (freestanding).
    # Compiled with -ffreestanding -fno-exceptions -fno-rtti so it links into
    # the PE32+ EFI binary without a hosted C++ runtime.
    add_custom_command(
      OUTPUT  ${T81_TERNARYOS_QEMU_SLICE6_CPP_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-stack-protector
              -nostdlib
              -fno-exceptions
              -fno-rtti
              -fno-use-cxa-atexit
              -std=c++17
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_cpp_bridge.cpp
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -o ${T81_TERNARYOS_QEMU_SLICE6_CPP_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_cpp_bridge.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/shell/command_catalog.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/shell/shared_command_core.hpp)
    add_custom_target(t81_ternaryos_qemu_slice6_cpp_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_CPP_OBJ})

    # Hardware-interrupt wiring — GICv3 + ARM physical timer (~100Hz, PPI 30).
    # Provides bridge_hw_init_aarch64() and axion_irq_handler_aarch64() for
    # the slice6 EFI; the vector table in aarch64_exception_vectors.S calls the
    # latter after saving caller-saved registers.
    set(T81_TERNARYOS_QEMU_SLICE6_IRQ_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_irq.obj)
    add_custom_command(
      OUTPUT  ${T81_TERNARYOS_QEMU_SLICE6_IRQ_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-stack-protector
              -nostdlib
              -fno-exceptions
              -fno-rtti
              -fno-use-cxa-atexit
              -std=c++17
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_bridge_irq.cpp
              -I${CMAKE_CURRENT_SOURCE_DIR}
              -o ${T81_TERNARYOS_QEMU_SLICE6_IRQ_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_bridge_irq.cpp)
    add_custom_target(t81_ternaryos_qemu_slice6_irq_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_IRQ_OBJ})

    # ── Slice 10: EL0 MMU page table builder (Phase 5 — RFC-00BC) ─────────────
    # Compiles qemu_slice6_el0_mmu.cpp: builds the controlled TTBR0_EL1 page
    # tables (code page R+X, stack page R/W) and exports el0_mmu_init(),
    # el0_mmu_stack_top(), and el0_tva_valid() to the rest of the EFI binary.
    set(T81_TERNARYOS_QEMU_SLICE6_MMU_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_el0_mmu.obj)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_QEMU_SLICE6_MMU_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-exceptions
              -fno-rtti
              -fno-use-cxa-atexit
              -fno-stack-protector
              -nostdlib
              -std=c++17
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_el0_mmu.cpp
              -o ${T81_TERNARYOS_QEMU_SLICE6_MMU_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_el0_mmu.cpp)
    add_custom_target(t81_ternaryos_qemu_slice6_mmu_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_MMU_OBJ})

    # ── Slice 11: EL0 KernelCall SVC bridge (Phase 6 — RFC-00BC) ─────────────
    # Compiles qemu_slice6_el0_svc_bridge.cpp: freestanding subset of the
    # Axion kernel call wire protocol reachable from EL0 through SVC #1.
    # Exports el0_svc_kernel_call_dispatch() to the SVC dispatcher.
    # RFC-0048 governance: governed non-DCP transport layer.
    set(T81_TERNARYOS_QEMU_SLICE6_SVC_BRIDGE_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_el0_svc_bridge.obj)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_QEMU_SLICE6_SVC_BRIDGE_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-exceptions
              -fno-rtti
              -fno-use-cxa-atexit
              -fno-stack-protector
              -nostdlib
              -std=c++17
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_el0_svc_bridge.cpp
              -o ${T81_TERNARYOS_QEMU_SLICE6_SVC_BRIDGE_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_el0_svc_bridge.cpp)
    add_custom_target(t81_ternaryos_qemu_slice6_svc_bridge_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_SVC_BRIDGE_OBJ})

    # ── Slice 0: Freestanding CRT stubs (memset / memcpy / memmove) ──────────
    # The MSVC ABI emits calls to memset/memcpy for non-constant sizes even in
    # -ffreestanding mode.  This stub provides byte-loop implementations so the
    # /nodefaultlib EFI link succeeds without the MSVC CRT.
    set(T81_TERNARYOS_QEMU_SLICE6_CRT_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_crt.obj)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_QEMU_SLICE6_CRT_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_freestanding_crt.c
              -o ${T81_TERNARYOS_QEMU_SLICE6_CRT_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_slice6_freestanding_crt.c)
    add_custom_target(t81_ternaryos_qemu_slice6_crt_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_CRT_OBJ})

    # ── Slice 12: CanonFS process loader (Phase 7 — RFC-00BC) ─────────────────
    # Compiles canon_exec_loader.cpp: reads a T81X binary from CanonFS LBA 3,
    # copies the code section into the EL0 process code page, and ERets to EL0
    # with tid=2.  Exports canon_exec_load_and_run() to qemu_slice6_cpp_bridge.
    set(T81_TERNARYOS_QEMU_SLICE6_CANON_EXEC_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_canon_exec.obj)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_QEMU_SLICE6_CANON_EXEC_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-exceptions
              -fno-rtti
              -fno-use-cxa-atexit
              -nostdlib
              -std=c++17
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/canon_exec_loader.cpp
              -o ${T81_TERNARYOS_QEMU_SLICE6_CANON_EXEC_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/canon_exec_loader.cpp)
    add_custom_target(t81_ternaryos_qemu_slice6_canon_exec_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_CANON_EXEC_OBJ})

    # ── Slice 13: EL0 process stub (Phase 7 — RFC-00BC) ──────────────────────
    # Assembles el0_process_stub.S: position-independent AArch64 stub that runs
    # from the proc code page at EL0 and emits the Phase 7 CI gate banner.
    set(T81_TERNARYOS_QEMU_SLICE6_PROC_STUB_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_el0_proc_stub.obj)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_QEMU_SLICE6_PROC_STUB_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-exceptions
              -fno-rtti
              -nostdlib
              -I${CMAKE_CURRENT_SOURCE_DIR}/hal
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/el0_process_stub.S
              -o ${T81_TERNARYOS_QEMU_SLICE6_PROC_STUB_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/el0_process_stub.S
              ${CMAKE_CURRENT_SOURCE_DIR}/hal/kernelcall_abi.inc)
    add_custom_target(t81_ternaryos_qemu_slice6_proc_stub_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_PROC_STUB_OBJ})

    if(T81_LLD_LINK)
      set(T81_TERNARYOS_QEMU_SLICE6_EFI_AVAILABLE ON)
      add_custom_command(
        OUTPUT ${T81_TERNARYOS_X86_64_EFI}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:x64
                /nodefaultlib
                /out:${T81_TERNARYOS_X86_64_EFI}
                ${T81_TERNARYOS_EFI_OBJ}
                ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ}
        DEPENDS ${T81_TERNARYOS_EFI_OBJ}
                ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ})
      add_custom_target(t81_ternaryos_virtualbox_x86_64_efi
        DEPENDS ${T81_TERNARYOS_X86_64_EFI})
      list(APPEND T81_TERNARYOS_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_x86_64_efi)

      add_custom_command(
        OUTPUT ${T81_TERNARYOS_ARMV8_EFI}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:arm64
                /nodefaultlib
                /out:${T81_TERNARYOS_ARMV8_EFI}
                ${T81_TERNARYOS_ARMV8_EFI_OBJ}
                ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ}
        DEPENDS ${T81_TERNARYOS_ARMV8_EFI_OBJ}
                ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ})
      add_custom_target(t81_ternaryos_virtualbox_armv8_efi
        DEPENDS ${T81_TERNARYOS_ARMV8_EFI})
      list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi)

      add_custom_command(
        OUTPUT ${T81_TERNARYOS_ARMV8_EFI_CTRL}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:arm64
                /nodefaultlib
                /out:${T81_TERNARYOS_ARMV8_EFI_CTRL}
                ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ}
        DEPENDS ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ})
      add_custom_target(t81_ternaryos_virtualbox_armv8_efi_control
        DEPENDS ${T81_TERNARYOS_ARMV8_EFI_CTRL})
      list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi_control)

      # Slice 6: link QEMU slice6 BOOTAA64.EFI (12 TUs)
      #   stub (EFI entry + ExitBootServices)
      #   + bare kernel (Phase 2 probe, calls C++ bridge)
      #   + cpp bridge (T81 banner + t81> shell, freestanding C++)
      #   + exception vectors (VBAR_EL1 table; axion_irq_entry calls bridge IRQ)
      #   + bridge irq (GICv3 + ARM timer; SVC dispatcher; defines axion_irq_handler_aarch64)
      #   + el0 init (axion_el0_entry: Phase 6 EL0→SVC#1 KernelCall→EL1 roundtrip)
      #   + el0 mmu (TTBR0 page tables; el0_mmu_init / el0_tva_valid; Phase 5+7)
      #   + el0 svc bridge (KernelCall SVC#1 dispatcher; Phase 6+7 tid tracking)
      #   + freestanding CRT (memset/memcpy/memmove stubs for /nodefaultlib link)
      #   + canon exec loader (T81X loader; Phase 7 CanonFS process bring-up)
      #   + el0 process stub (Phase 7 EL0 process: tid=2 SVC sequence)
      #   + shim (ternaryos_hal_main_c stub)
      add_custom_command(
        OUTPUT ${T81_TERNARYOS_QEMU_SLICE6_EFI}
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:arm64
                /base:0x0
                /nodefaultlib
                /out:${T81_TERNARYOS_QEMU_SLICE6_EFI}
                ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_CPP_OBJ}
                ${T81_TERNARYOS_AARCH64_VECTORS_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_IRQ_OBJ}
                ${T81_TERNARYOS_AARCH64_EL0_INIT_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_MMU_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_SVC_BRIDGE_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_CRT_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_CANON_EXEC_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_PROC_STUB_OBJ}
                ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ}
        DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_CPP_OBJ}
                ${T81_TERNARYOS_AARCH64_VECTORS_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_IRQ_OBJ}
                ${T81_TERNARYOS_AARCH64_EL0_INIT_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_MMU_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_SVC_BRIDGE_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_CRT_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_CANON_EXEC_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_PROC_STUB_OBJ}
                ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ})
      add_custom_target(t81_ternaryos_qemu_slice6_efi
        DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_EFI})

      # ── x86_64: QEMU OVMF slice6 BOOTX64.EFI ─────────────────────────────
      # Three-object EFI binary: EFI stub + bare-metal C entry + C++ bridge.
      # Mirrors the AArch64 slice6 layout; uses COM1 UART + RDTSC instead of
      # PL011 MMIO + cntpct_el0.

      set(T81_TERNARYOS_QEMU_X86_EFI_DIR  ${CMAKE_BINARY_DIR}/ternaryos/qemu_x86_64)
      set(T81_TERNARYOS_QEMU_X86_STUB_OBJ ${T81_TERNARYOS_QEMU_X86_EFI_DIR}/BOOTX64_stub.obj)
      set(T81_TERNARYOS_QEMU_X86_BARE_OBJ ${T81_TERNARYOS_QEMU_X86_EFI_DIR}/BOOTX64_bare.obj)
      set(T81_TERNARYOS_QEMU_X86_CPP_OBJ  ${T81_TERNARYOS_QEMU_X86_EFI_DIR}/BOOTX64_cpp.obj)
      set(T81_TERNARYOS_QEMU_X86_EFI      ${T81_TERNARYOS_QEMU_X86_EFI_DIR}/BOOTX64.EFI)

      add_custom_command(
        OUTPUT  ${T81_TERNARYOS_QEMU_X86_STUB_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${T81_TERNARYOS_QEMU_X86_EFI_DIR}
        COMMAND ${T81_LLVM_CLANG}
                --target=x86_64-pc-windows-msvc
                -ffreestanding
                -fno-stack-protector
                -mno-stack-arg-probe
                -nostdlib
                -c
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_x86_64_efi_stub.c
                -I${CMAKE_CURRENT_SOURCE_DIR}
                -o ${T81_TERNARYOS_QEMU_X86_STUB_OBJ}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_x86_64_efi_stub.c)

      add_custom_command(
        OUTPUT  ${T81_TERNARYOS_QEMU_X86_BARE_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${T81_TERNARYOS_QEMU_X86_EFI_DIR}
        COMMAND ${T81_LLVM_CLANG}
                --target=x86_64-pc-windows-msvc
                -ffreestanding
                -fno-stack-protector
                -mno-stack-arg-probe
                -nostdlib
                -c
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_x86_64_bare_kernel.c
                -I${CMAKE_CURRENT_SOURCE_DIR}
                -o ${T81_TERNARYOS_QEMU_X86_BARE_OBJ}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_x86_64_bare_kernel.c)

      add_custom_command(
        OUTPUT  ${T81_TERNARYOS_QEMU_X86_CPP_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${T81_TERNARYOS_QEMU_X86_EFI_DIR}
        COMMAND ${T81_LLVM_CLANG}
                --target=x86_64-pc-windows-msvc
                -ffreestanding
                -fno-stack-protector
                -mno-stack-arg-probe
                -nostdlib
                -fno-exceptions
                -fno-rtti
                -fno-use-cxa-atexit
                -std=c++17
                -c
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_x86_64_cpp_bridge.cpp
                -I${CMAKE_CURRENT_SOURCE_DIR}
                -o ${T81_TERNARYOS_QEMU_X86_CPP_OBJ}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_x86_64_cpp_bridge.cpp)

      # Hardware-interrupt wiring — IDT + 8259 PIC + PIT ch0 (~100Hz).
      # Provides bridge_hw_init_x86_64() and axion_timer_stub_x86_64() for the
      # x86_64 OVMF EFI binary.
      set(T81_TERNARYOS_QEMU_X86_IRQ_OBJ
          ${T81_TERNARYOS_QEMU_X86_EFI_DIR}/BOOTX64_irq.obj)
      add_custom_command(
        OUTPUT  ${T81_TERNARYOS_QEMU_X86_IRQ_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${T81_TERNARYOS_QEMU_X86_EFI_DIR}
        COMMAND ${T81_LLVM_CLANG}
                --target=x86_64-pc-windows-msvc
                -ffreestanding
                -fno-stack-protector
                -mno-stack-arg-probe
                -nostdlib
                -fno-exceptions
                -fno-rtti
                -fno-use-cxa-atexit
                -std=c++17
                -c
                ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_x86_64_bridge_irq.cpp
                -I${CMAKE_CURRENT_SOURCE_DIR}
                -o ${T81_TERNARYOS_QEMU_X86_IRQ_OBJ}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hal/qemu_x86_64_bridge_irq.cpp)

      add_custom_command(
        OUTPUT ${T81_TERNARYOS_QEMU_X86_EFI}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${T81_TERNARYOS_QEMU_X86_EFI_DIR}
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:x64
                /nodefaultlib
                /out:${T81_TERNARYOS_QEMU_X86_EFI}
                ${T81_TERNARYOS_QEMU_X86_STUB_OBJ}
                ${T81_TERNARYOS_QEMU_X86_BARE_OBJ}
                ${T81_TERNARYOS_QEMU_X86_CPP_OBJ}
                ${T81_TERNARYOS_QEMU_X86_IRQ_OBJ}
                ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ}
        DEPENDS ${T81_TERNARYOS_QEMU_X86_STUB_OBJ}
                ${T81_TERNARYOS_QEMU_X86_BARE_OBJ}
                ${T81_TERNARYOS_QEMU_X86_CPP_OBJ}
                ${T81_TERNARYOS_QEMU_X86_IRQ_OBJ}
                ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ})
      add_custom_target(t81_ternaryos_qemu_x86_64_efi
        DEPENDS ${T81_TERNARYOS_QEMU_X86_EFI})
    else()
      message(WARNING
        "[t81] QEMU/EFI guest targets are disabled because lld-link was not found. "
        "Install Homebrew lld and re-run CMake. "
        "Expected path examples: /opt/homebrew/bin/lld-link or /opt/homebrew/opt/lld/bin/lld-link")
    endif()
  else()
    message(WARNING
      "[t81] QEMU/EFI guest targets are disabled because Homebrew clang was not found. "
      "Install Homebrew llvm and re-run CMake. "
      "Expected path example: /opt/homebrew/opt/llvm/bin/clang")
  endif()

  add_custom_target(t81_ternaryos_virtualbox_guest_artifact
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_virtualbox_guest_artifact.sh
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
            x86_64
    DEPENDS ${T81_TERNARYOS_GUEST_ARTIFACT_DEPS}
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/handoff
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/package_virtualbox_x86_64_handoff.sh
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/ternaryos/handoff
    DEPENDS t81_ternaryos_virtualbox_guest_artifact
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/virtualbox_x86_64_handoff.md
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff_bundle_smoke
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate_packaged_virtualbox_x86_64_handoff_bundle.sh
            ${CMAKE_BINARY_DIR}/ternaryos/handoff/ternos_virtualbox_x86_64_handoff
    DEPENDS t81_ternaryos_virtualbox_x86_64_handoff
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff_bundle_negative_smoke
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/expect_packaged_virtualbox_x86_64_handoff_bundle_failure.sh
            ${CMAKE_BINARY_DIR}/ternaryos/handoff/ternos_virtualbox_x86_64_handoff
    DEPENDS t81_ternaryos_virtualbox_x86_64_handoff
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_armv8_dev_artifact
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_virtualbox_guest_artifact.sh
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
            armv8
    DEPENDS ${T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS}
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_host_check
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_virtualbox_host.sh
            --soft
            x86_64
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_armv8_host_check
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_virtualbox_host.sh
            --soft
            ARMv8
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_armv8_efi_link_check
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_efi_link_toolchain.sh
            armv8
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/efi-link-status.txt
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_armv8_boot_probe
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/probe_virtualbox_armv8_vm.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.vdi
            5
    DEPENDS t81_ternaryos_virtualbox_armv8_dev_artifact
            t81_ternaryos_virtualbox_armv8_host_check
            t81_ternaryos_virtualbox_armv8_efi_link_check
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_qemu_armv8_efi_control_probe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_control
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_qemu_armv8_efi_control.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.img
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_control
    DEPENDS t81_ternaryos_virtualbox_armv8_dev_artifact
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_qemu_armv8_guest_probe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_qemu_armv8_guest_probe.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.img
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest
            8
    DEPENDS t81_ternaryos_virtualbox_armv8_dev_artifact
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_qemu_x86_64_guest_probe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_x86_64_guest
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_qemu_x86_64_guest_probe.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox/ternos_virtualbox_guest.img
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_x86_64_guest
    DEPENDS t81_ternaryos_virtualbox_guest_artifact
    USES_TERMINAL)

  # ── Slice 6: QEMU AArch64 EDK2 disk image + probe ─────────────────────────
  if(T81_TERNARYOS_QEMU_SLICE6_EFI_AVAILABLE)
    add_custom_target(t81_ternaryos_qemu_slice6_dev_artifact
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND /bin/zsh
              ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_qemu_slice6_artifact.sh
              ${CMAKE_BINARY_DIR}
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      DEPENDS t81_ternaryos_qemu_slice6_efi
      USES_TERMINAL)
  else()
    add_custom_target(t81_ternaryos_qemu_slice6_dev_artifact
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Cannot build t81_ternaryos_qemu_slice6_dev_artifact."
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Missing EFI prerequisites from configure time: Homebrew llvm clang and/or lld-link."
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Install them, then re-run: cmake -S . -B build -G Ninja"
      COMMAND ${CMAKE_COMMAND} -E false
      USES_TERMINAL)
  endif()

  add_custom_target(t81_ternaryos_qemu_armv8_slice6_probe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_slice6_probe
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_qemu_armv8_slice6_probe.sh
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/qemu_slice6_guest.img
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_slice6_probe
            8
    DEPENDS t81_ternaryos_qemu_slice6_dev_artifact
    USES_TERMINAL)

  if(T81_TERNARYOS_QEMU_SLICE6_EFI_AVAILABLE)
    add_custom_target(t81_ternaryos_qemu_slice6_shell
      COMMAND /bin/zsh
              ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_qemu_slice6_shell.sh
              ${CMAKE_BINARY_DIR}
      DEPENDS t81_ternaryos_qemu_slice6_efi
      USES_TERMINAL)
  else()
    add_custom_target(t81_ternaryos_qemu_slice6_shell
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Cannot boot t81_ternaryos_qemu_slice6_shell."
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Missing EFI prerequisites from configure time: Homebrew llvm clang and/or lld-link."
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Install them, then re-run: cmake -S . -B build -G Ninja"
      COMMAND ${CMAKE_COMMAND} -E false
      USES_TERMINAL)
  endif()

  if(T81_TERNARYOS_QEMU_SLICE6_EFI_AVAILABLE)
    add_custom_target(t81_ternaryos_qemu_slice6_shell_smoke
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6_shell_smoke
      COMMAND /bin/zsh
              ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_qemu_slice6_shell.sh
              ${CMAKE_BINARY_DIR}
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6_shell_smoke
      DEPENDS t81_ternaryos_qemu_slice6_efi
      USES_TERMINAL)
  else()
    add_custom_target(t81_ternaryos_qemu_slice6_shell_smoke
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Cannot run t81_ternaryos_qemu_slice6_shell_smoke."
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Missing EFI prerequisites from configure time: Homebrew llvm clang and/or lld-link."
      COMMAND ${CMAKE_COMMAND} -E echo "[t81] Install them, then re-run: cmake -S . -B build -G Ninja"
      COMMAND ${CMAKE_COMMAND} -E false
      USES_TERMINAL)
  endif()

  add_custom_target(t81_ternaryos_qemu_armv8_guest_probe_blocked_fixture
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest_blocked_fixture
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/dev/qemu_armv8_guest_probe_blocked_fixture
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest_blocked_fixture
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate_qemu_armv8_guest_reports.sh
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest_blocked_fixture
            blocked
            qemu-armv8-guest-probe-fixture
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff_fixture
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/dev/virtualbox_x86_64_handoff_validation_fixture
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate_virtualbox_x86_64_handoff.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture/expected-boot-report.txt
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture/expected-startup-status.txt
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture/recovered-artifacts
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff_negative_fixture
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/dev/virtualbox_x86_64_handoff_validation_negative_fixture
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/expect_virtualbox_x86_64_handoff_failure.sh
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate_virtualbox_x86_64_handoff.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture/expected-boot-report.txt
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture/expected-startup-status.txt
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture/recovered-artifacts
    USES_TERMINAL)

  if(T81_BUILD_TESTS)
    add_executable(t81_ternaryos_hal_boot_test
      tests/hal_boot_test.cpp)
    target_link_libraries(t81_ternaryos_hal_boot_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_hal_boot_test COMMAND t81_ternaryos_hal_boot_test)

    add_executable(t81_ternaryos_page_alloc_test
      tests/ternary_page_alloc_test.cpp)
    target_link_libraries(t81_ternaryos_page_alloc_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_page_alloc_test COMMAND t81_ternaryos_page_alloc_test)

    add_executable(t81_ternaryos_context_switch_test
      tests/context_switch_test.cpp)
    target_link_libraries(t81_ternaryos_context_switch_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_context_switch_test COMMAND t81_ternaryos_context_switch_test)

    add_executable(t81_ternaryos_mmu_test
      tests/mmu_test.cpp)
    target_link_libraries(t81_ternaryos_mmu_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_mmu_test COMMAND t81_ternaryos_mmu_test)

    # Phase 3 tests
    add_executable(t81_ternaryos_scheduler_test
      tests/scheduler_test.cpp)
    target_link_libraries(t81_ternaryos_scheduler_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_scheduler_test COMMAND t81_ternaryos_scheduler_test)

    add_executable(t81_ternaryos_ipc_test
      tests/ipc_test.cpp)
    target_link_libraries(t81_ternaryos_ipc_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_ipc_test COMMAND t81_ternaryos_ipc_test)

    add_executable(t81_ternaryos_device_driver_test
      tests/device_driver_test.cpp)
    target_link_libraries(t81_ternaryos_device_driver_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_device_driver_test COMMAND t81_ternaryos_device_driver_test)

    add_executable(t81_ternaryos_shell_session_test
      tests/shell_session_test.cpp)
    target_link_libraries(t81_ternaryos_shell_session_test PRIVATE t81_ternaryos_shell)
    add_test(NAME t81_ternaryos_shell_session_test COMMAND t81_ternaryos_shell_session_test)

    # RFC-00B9 — TernaryOS User Environment acceptance tests (AC-1 through AC-15)
    add_executable(t81_ternaryos_user_env_test
      tests/user_env_test.cpp)
    target_link_libraries(t81_ternaryos_user_env_test PRIVATE t81_ternaryos_userenv)
    add_test(NAME t81_ternaryos_user_env_test COMMAND t81_ternaryos_user_env_test)

    add_executable(t81_axion_userenv_include_surface_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/axion_userenv_include_surface_test.cpp)
    target_link_libraries(t81_axion_userenv_include_surface_test PRIVATE t81_ternaryos_shell)
    add_test(NAME t81_axion_userenv_include_surface_test COMMAND t81_axion_userenv_include_surface_test)

    if(T81_ENABLE_DPE)
      add_executable(t81_ternaryos_epoch_submission_test
        tests/epoch_submission_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_submission_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_submission_test COMMAND t81_ternaryos_epoch_submission_test)

      add_executable(t81_ternaryos_epoch_policy_test
        tests/epoch_policy_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_policy_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_policy_test COMMAND t81_ternaryos_epoch_policy_test)

      # RFC-DPE-0003 §10 / RFC-DPE-0006 §4 — SubmitEpoch KernelCallKind ABI tests [DPE-07]
      add_executable(t81_ternaryos_epoch_syscall_test
        tests/epoch_syscall_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_syscall_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_syscall_test COMMAND t81_ternaryos_epoch_syscall_test)

      # RFC-DPE-0007 — Epoch Execution Timeout tests [DPE-08]
      add_executable(t81_ternaryos_epoch_timeout_test
        tests/epoch_timeout_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_timeout_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_timeout_test COMMAND t81_ternaryos_epoch_timeout_test)

      # RFC-DPE-0008 — Epoch Audit Events tests [DPE-09]
      add_executable(t81_ternaryos_epoch_audit_test
        tests/epoch_audit_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_audit_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_audit_test COMMAND t81_ternaryos_epoch_audit_test)

      # RFC-DPE-0009 — Epoch History Ring tests [DPE-10]
      add_executable(t81_ternaryos_epoch_history_test
        tests/epoch_history_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_history_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_history_test COMMAND t81_ternaryos_epoch_history_test)
    endif()

    # RFC-00B6 §5.3.6 — ClaimDevice/ReleaseDevice/QueryDevice KernelCallKind ABI tests [AC-22d]
    add_executable(t81_ternaryos_device_arbitration_syscall_test
      tests/device_arbitration_syscall_test.cpp)
    target_link_libraries(t81_ternaryos_device_arbitration_syscall_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_device_arbitration_syscall_test
             COMMAND t81_ternaryos_device_arbitration_syscall_test)
  endif()
