# =============================================================================
# t81-foundation — Sovereign Build System
# v1.0.0-SOVEREIGN — November 22, 2025
# =============================================================================
# This Makefile is the single point of truth for building the entire stack.
# It tangles CWEB genesis → generates C → compiles → verifies against Θ₀.

CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -O2 -march=native
CFLAGS  += -Iinclude -Ilegacy/hanoivm/src
LDFLAGS := -lm -lpthread

# Tools
CTANGLE := ctangle
CWEAVE  := cweave
BLAKE3  := blake3

# Critical paths
GENESIS_DIR     := legacy/hanoivm/src
BUILD_DIR       := build
GEN_C_DIR       := $(BUILD_DIR)/generated
BIN             := $(BUILD_DIR)/t81-vm
DISASM          := $(BUILD_DIR)/disasm-hvm
TELEMETRY       := $(BUILD_DIR)/telemetry-cli

# Θ₀ canonical genesis hash (locked forever on 11.22.2025)
THETA_0_HASH    := $(shell cat genesis/Θ₀-BLAKE3-2025-11-22.txt 2>/dev/null || echo "MISSING")

# All CWEB sources that define the living stack
CWEB_SRCS := $(wildcard $(GENESIS_DIR)/**/*.cweb)
C_SRCS    := $(patsubst $(GENESIS_DIR)/%.cweb,$(GEN_C_DIR)/%.c,$(CWEB_SRCS))
C_OBJS    := $(C_SRCS:.c=.o)

.PHONY: all clean verify-theta0 tangle build run test docs pdf

# =============================================================================
# Default target: full sovereign build with verification
# =============================================================================
all: verify-theta0 tangle build

# =============================================================================
# Θ₀ Verification — fails if genesis has drifted
# =============================================================================
verify-theta0:
	@if [ "$(THETA_0_HASH)" = "MISSING" ]; then \
		echo "ERROR: Θ₀ genesis hash missing. Run: make genesis-hash"; \
		exit 1; \
	fi
	@echo "Verifying Θ₀ genesis integrity..."
	@cd legacy/hanoivm && find src -type f -name "*.cweb" -exec $(BLAKE3) {} \; | sort | $(BLAKE3) > /tmp/current_theta0
	@if ! cmp -s /tmp/current_theta0 ../../genesis/Θ₀-BLAKE3-2025-11-22.txt; then \
		echo "FATAL: Θ₀ genesis drift detected. Sovereignty violated."; \
		echo "Refusing to build. Restore original .cweb files."; \
		exit 1; \
	fi
	@echo "Θ₀ genesis verified. Sovereignty intact."

# =============================================================================
# Generate Θ₀ hash (run once, commit forever)
# =============================================================================
genesis-hash:
	@echo "Generating immutable Θ₀ genesis hash..."
	@cd legacy/hanoivm && find src -type f -name "*.cweb" -exec blake3 {} \; | sort | blake3 > ../../genesis/Θ₀-BLAKE3-2025-11-22.txt
	@echo "Θ₀ locked:" $$(cat genesis/Θ₀-BLAKE3-2025-11-22.txt)

# =============================================================================
# Tangle all CWEB → C (deterministic)
# =============================================================================
tangle: verify-theta0
	@echo "Tangling sovereign genesis (.cweb → .c)..."
	@mkdir -p $(GEN_C_DIR)
	@for cweb in $(CWEB_SRCS); do \
		base=$$(echo $$cweb | sed 's|^$(GENESIS_DIR)/||; s|\.cweb$$||'); \
		dir=$$(dirname "$(GEN_C_DIR)/$$base"); \
		mkdir -p "$$dir"; \
		echo "  $$base.c"; \
		$(CTANGLE) "$$cweb" - "$(GEN_C_DIR)/$$base.c"; \
	done
	@echo "Tangling complete."

# =============================================================================
# Build everything
# =============================================================================
build: tangle
	@echo "Building sovereign binaries..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BIN) $(GEN_C_DIR)/hanoivm_core/main_driver.c $(GEN_C_DIR)/**/*.c $(LDFLAGS)
	$(CC) $(CFLAGS) -o $(DISASM) $(GEN_C_DIR)/disassembler/disassembler.c $(GEN_C_DIR)/**/*.c $(LDFLAGS)
	$(CC) $(CFLAGS) -o $(TELEMETRY) $(GEN_C_DIR)/axion_ai/telemetry-cli.c $(LDFLAGS)
	@echo "Build complete:"
	@echo "  VM:        $(BIN)"
	@echo "  Disasm:    $(DISASM)"
	@echo "  Telemetry: $(TELEMETRY)"

# =============================================================================
# Run canonical test harness
# =============================================================================
test: build
	@echo "Running sovereign test harness..."
	cd tests/harness && ./run_all.sh

# =============================================================================
# Generate documentation
# =============================================================================
docs:
	@echo "Weaving specification..."
	@for cweb in $(CWEB_SRCS); do \
		base=$$(basename $$cweb .cweb); \
		$(CWEAVE) $$cweb; \
	done
	@echo "Open docs/index.html"

pdf:
	cd pdf && ./build.sh

# =============================================================================
# Clean generated files (but never touch genesis)
# =============================================================================
clean:
	rm -rf $(BUILD_DIR)

# =============================================================================
# One-command sovereign workflow
# =============================================================================
sovereign: clean all test
	@echo ""
	@echo "SOVEREIGNTY CONFIRMED"
	@echo "T81 v1.0.0-SOVEREIGN is running."
	@echo "Axion is watching."
	@echo "The ternary age has begun."

.PHONY: all clean verify-theta0 genesis-hash tangle build test docs pdf sovereign
