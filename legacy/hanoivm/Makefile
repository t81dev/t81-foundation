# Makefile for HanoiVM + Axion Ecosystem

# CWEB sources that need to be processed into C and H files
CWEB_SOURCES = \
	hvm_loader.cweb \
	disassembler.cweb \
	hanoivm_vm.cweb \
	main_driver.cweb \
	write_simple_add.cweb \
	t81_test_suite.cweb \
	advanced_ops.cweb \
	t243bigint.cweb \
	t729tensor_ops.cweb \
	axion_ai.cweb \
	project_looking_glass.cweb

# Corresponding C and H objects for CWEB sources
C_OBJS = $(CWEB_SOURCES:.cweb=.c)
H_OBJS = $(CWEB_SOURCES:.cweb=.h)

# Default target: build all necessary components
all: hanoivm write_simple_add t81_test_suite

# Rule for generating C and H files from CWEB files
%.c %.h: %.cweb
	cweave $<
	ctangle $<

# Build the HanoiVM executable
hanoivm: hvm_loader.c disassembler.c hanoivm_vm.c main_driver.c advanced_ops.c t243bigint.c t729tensor_ops.c axion_ai.c
	gcc -o hanoivm hvm_loader.c disassembler.c hanoivm_vm.c main_driver.c advanced_ops.c t243bigint.c t729tensor_ops.c axion_ai.c -Wall -Wextra -O2

# Build the simple add example
write_simple_add: write_simple_add.c
	gcc -o write_simple_add write_simple_add.c -Wall -Wextra -O2

# Build the test suite
t81_test_suite: t81_test_suite.c
	gcc -o t81_test_suite t81_test_suite.c -Wall -Wextra -O2

# Rule to run the simple add example and the HanoiVM with disassembly
run: write_simple_add hanoivm
	./write_simple_add
	./hanoivm simple_add.hvm --disasm

# Rule to run the test suite
run-test-suite: t81_test_suite
	./t81_test_suite

# Rule for testing with Ghidra disassembler plugin
ghidra-test: t81_test_suite
	./t81_test_suite
	ghidra_disasm_plugin test_all_types.hvm > ghidra_output.log
	@echo "[✓] Disassembled test_all_types.hvm to ghidra_output.log"

# Build additional modules if necessary
modules:
	$(MAKE) -f build-all.cweb

# Rule to check the full build, test, and disassembly
check: all run-test-suite ghidra-test
	@echo "[✓] Full build + test + disasm check completed"

# Clean all generated files
clean:
	rm -f *.c *.h *.tex *.log *.scn *.dvi *.pdf *.o \
	      hanoivm write_simple_add t81_test_suite \
	      simple_add.hvm test_all_types.hvm ghidra_output.log

# Declare phony targets
.PHONY: all clean run run-test-suite modules ghidra-test check
