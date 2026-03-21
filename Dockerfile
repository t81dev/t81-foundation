# T81 Foundation — Multi-stage Docker image
#
# Stage 1 (builder): compile the t81 CLI in Release mode.
# Stage 2 (runtime): minimal image with just the binary + examples.
#
# Usage:
#   docker run --rm -it ghcr.io/t81dev/t81-foundation          # interactive REPL
#   docker run --rm     ghcr.io/t81dev/t81-foundation demo     # guided demo (~60 s)
#   docker run --rm -it ghcr.io/t81dev/t81-foundation <cmd>    # any t81 subcommand

# ── Stage 1: build ───────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

RUN apt-get update -qq && \
    apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DT81_BUILD_TESTS=OFF \
      -DT81_BUILD_EXAMPLES=OFF \
      -DT81_BUILD_BENCHMARKS=OFF && \
    cmake --build build --parallel && \
    cmake --install build --prefix /t81-dist

# ── Stage 2: runtime ─────────────────────────────────────────────────────────
FROM ubuntu:24.04

LABEL org.opencontainers.image.title="T81 Foundation"
LABEL org.opencontainers.image.description="Deterministic ternary computing — interactive REPL and demo"
LABEL org.opencontainers.image.source="https://github.com/t81dev/t81-foundation"
LABEL org.opencontainers.image.licenses="Apache-2.0"

# The binary is dynamically linked against libstdc++6 / libc6, both present in
# ubuntu:24.04 by default. No extra apt packages needed.
COPY --from=builder /t81-dist/bin/t81 /usr/local/bin/t81

# Bundled examples and precompiled TISC binaries used by the demo
COPY examples/ /t81/examples/

# Entrypoint and demo scripts
COPY docker/ /t81/docker/
RUN chmod +x /t81/tools/docker/entrypoint.sh /t81/tools/docker/demo.sh

# Default working directory for user files mounted via -v
WORKDIR /workspace

ENTRYPOINT ["/t81/tools/docker/entrypoint.sh"]
