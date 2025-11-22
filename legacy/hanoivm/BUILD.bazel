# BUILD file for HanoiVM disassembler and symbolic ternary CI test

# ----------------------------- ADVANCED TESTS -----------------------------

genrule(
    name = "generate_test_advanced_hvm",
    srcs = ["test_advanced_hvm.cweb"],
    outs = ["tests/test_advanced.hvm"],
    cmd = "$(location //test_advanced_hvm:test_advanced_hvm) > $(OUTS)",
    tools = ["//test_advanced_hvm:test_advanced_hvm"],
)

sh_test(
    name = "disasm_advanced_test",
    srcs = ["tests/disasm_advanced_test.sh"],
    data = [
        "//disassembler:disassembler",
        "tests/test_advanced.hvm"
    ],
)

cc_binary(
    name = "test_advanced_hvm",
    srcs = ["test_advanced_hvm.cweb"],
    deps = [],
)

# ---------------------------- CONTROLFLOW TESTS -----------------------------

genrule(
    name = "generate_test_controlflow_hvm",
    srcs = ["test_controlflow_hvm.cweb"],
    outs = ["tests/test_controlflow.hvm"],
    cmd = "$(location //test_controlflow_hvm:test_controlflow_hvm) > $(OUTS)",
    tools = ["//test_controlflow_hvm:test_controlflow_hvm"],
)

sh_test(
    name = "disasm_controlflow_test",
    srcs = ["tests/disasm_controlflow_test.sh"],
    data = [
        "//disassembler:disassembler",
        "tests/test_controlflow.hvm"
    ],
)

cc_binary(
    name = "test_controlflow_hvm",
    srcs = ["test_controlflow_hvm.cweb"],
    deps = [],
)

# ----------------------------- LOGIC TESTS -----------------------------

genrule(
    name = "generate_test_logic_hvm",
    srcs = ["test_logic_hvm.cweb"],
    outs = ["tests/test_logic.hvm"],
    cmd = "$(location //test_logic_hvm:test_logic_hvm) > $(OUTS)",
    tools = ["//test_logic_hvm:test_logic_hvm"],
)

sh_test(
    name = "disasm_logic_test",
    srcs = ["tests/disasm_logic_test.sh"],
    data = [
        "//disassembler:disassembler",
        "tests/test_logic.hvm"
    ],
)

cc_binary(
    name = "test_logic_hvm",
    srcs = ["test_logic_hvm.cweb"],
    deps = [],
)

# ---------------------------- TENSOR & BIGINT TESTS -----------------------------

genrule(
    name = "generate_test_t729tensor_hvm",
    srcs = ["test_t729tensor_hvm.cweb"],
    outs = ["tests/test_t729tensor.hvm"],
    cmd = "$(location //test_t729tensor_hvm:test_t729tensor_hvm) > $(OUTS)",
    tools = ["//test_t729tensor_hvm:test_t729tensor_hvm"],
)

sh_test(
    name = "disasm_t729tensor_test",
    srcs = ["tests/disasm_t729tensor_test.sh"],
    data = [
        "//disassembler:disassembler",
        "tests/test_t729tensor.hvm"
    ],
)

cc_binary(
    name = "test_t729tensor_hvm",
    srcs = ["test_t729tensor_hvm.cweb"],
    deps = [],
)

genrule(
    name = "generate_test_t243bigint_hvm",
    srcs = ["test_t243bigint_hvm.cweb"],
    outs = ["tests/test_t243bigint.hvm"],
    cmd = "$(location //test_t243bigint_hvm:test_t243bigint_hvm) > $(OUTS)",
    tools = ["//test_t243bigint_hvm:test_t243bigint_hvm"],
)

sh_test(
    name = "disasm_t243bigint_test",
    srcs = ["tests/disasm_t243bigint_test.sh"],
    data = [
        "//disassembler:disassembler",
        "tests/test_t243bigint.hvm"
    ],
)

cc_binary(
    name = "test_t243bigint_hvm",
    srcs = ["test_t243bigint_hvm.cweb"],
    deps = [],
)

# --------------------------- AXION AI + GPU BACKEND TESTS ---------------------------

genrule(
    name = "generate_test_axion_ai_hvm",
    srcs = ["test_axion_ai_hvm.cweb"],
    outs = ["tests/test_axion_ai.hvm"],
    cmd = "$(location //test_axion_ai_hvm:test_axion_ai_hvm) > $(OUTS)",
    tools = ["//test_axion_ai_hvm:test_axion_ai_hvm"],
)

sh_test(
    name = "disasm_axion_ai_test",
    srcs = ["tests/disasm_axion_ai_test.sh"],
    data = [
        "//disassembler:disassembler",
        "tests/test_axion_ai.hvm"
    ],
)

cc_binary(
    name = "test_axion_ai_hvm",
    srcs = ["test_axion_ai_hvm.cweb"],
    deps = [],
)

# ---------------------------- GAIA/CUDA GPU BACKEND TEST ----------------------------

cc_binary(
    name = "test_gaia_transform",
    srcs = ["gaia_handle_request.cweb"],
    deps = [
        "//ai_hook:ai_hook",
        "//t81_patterns:t81_patterns",
    ],
)

cc_binary(
    name = "test_cuda_transform",
    srcs = ["cuda_handle_request.cu"],
    deps = [
        "//ai_hook:ai_hook",
        "//t81_patterns:t81_patterns",
    ],
)

# ---------------------------- T81 PATTERN TEST ----------------------------

cc_binary(
    name = "t81_patterns_test",
    srcs = ["t81_patterns.cweb"],
    deps = [
        "//ai_hook:ai_hook",
        "//gaia_handle_request:gaia_handle_request",
    ],
)

# --------------------------- SIMULATION + TIER PROMOTION ---------------------------

cc_binary(
    name = "recursive_tier_demo",
    srcs = ["recursive_tier_execution.cweb"],
    deps = [
        "//t81_stack:t81_stack",
        "//t243bigint:t243bigint",
        "//t729tensor:t729tensor",
        "//ai_hook:ai_hook",
    ],
)

# ----------------------------- CLEANUP -----------------------------

# Clean all generated test files and build artifacts
clean:
	rm -f tests/*.hvm *.o *.c *.h

# ---------------------------- T81LANG COMPILER PIPELINE ----------------------------

cc_binary(
    name = "t81lang_parser",
    srcs = ["t81lang_parser.cweb"],
    deps = [],
)

cc_binary(
    name = "t81lang_semantic",
    srcs = ["t81lang_semantic.cweb"],
    deps = [],
)

cc_binary(
    name = "t81lang_irgen",
    srcs = ["t81lang_irgen.cweb"],
    deps = [],
)

cc_binary(
    name = "emit_hvm",
    srcs = ["emit_hvm.cweb"],
    deps = [],
)

cc_binary(
    name = "t81lang_compiler",
    srcs = ["t81lang_compiler.cweb"],
    deps = [
        ":t81lang_parser",
        ":t81lang_semantic",
        ":t81lang_irgen",
        ":emit_hvm",
    ],
)

cc_binary(
    name = "hvm_interpreter",
    srcs = ["hvm_interpreter.cweb"],
    deps = [],
)
