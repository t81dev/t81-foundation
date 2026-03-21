#!/usr/bin/env sh
# install.sh — Download and install a prebuilt t81 binary.
#
# Usage:
#   curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
#
# Options (environment variables):
#   T81_INSTALL_DIR   Target directory (default: ~/.local/bin)
#   T81_VERSION       Pin a specific release tag, e.g. v1.9.0 (default: latest)

set -eu

REPO="t81dev/t81-foundation"
INSTALL_DIR="${T81_INSTALL_DIR:-$HOME/.local/bin}"

# ── Detect OS ─────────────────────────────────────────────────────────────────
case "$(uname -s)" in
  Linux)  OS="linux"  ;;
  Darwin) OS="macos"  ;;
  *)
    echo "Error: unsupported OS '$(uname -s)'. Build from source: https://github.com/${REPO}"
    exit 1
    ;;
esac

# ── Detect CPU architecture ───────────────────────────────────────────────────
case "$(uname -m)" in
  x86_64|amd64)   ARCH="x86_64" ;;
  arm64|aarch64)  ARCH="arm64"  ;;
  *)
    echo "Error: unsupported architecture '$(uname -m)'. Build from source: https://github.com/${REPO}"
    exit 1
    ;;
esac

PLATFORM="${OS}-${ARCH}"

# ── Pick a download tool ──────────────────────────────────────────────────────
if command -v curl > /dev/null 2>&1; then
  _get() { curl -fsSL "$1"; }
  _dl()  { curl -fsSL "$1" -o "$2"; }
elif command -v wget > /dev/null 2>&1; then
  _get() { wget -qO- "$1"; }
  _dl()  { wget -q "$1" -O "$2"; }
else
  echo "Error: curl or wget is required."
  exit 1
fi

# ── Resolve release tag ───────────────────────────────────────────────────────
if [ -n "${T81_VERSION:-}" ]; then
  TAG="$T81_VERSION"
else
  printf 'Fetching latest release tag ... '
  TAG=$(_get "https://api.github.com/repos/${REPO}/releases/latest" \
        | grep '"tag_name"' \
        | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')
  if [ -z "$TAG" ]; then
    echo "failed."
    echo "Error: could not determine the latest release. Check your network or set T81_VERSION."
    exit 1
  fi
  echo "$TAG"
fi

ARCHIVE="t81-${TAG}-${PLATFORM}.tar.gz"
URL="https://github.com/${REPO}/releases/download/${TAG}/${ARCHIVE}"

# ── Download ──────────────────────────────────────────────────────────────────
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "Downloading ${ARCHIVE} ..."
_dl "$URL" "$TMP/$ARCHIVE"

# ── Install ───────────────────────────────────────────────────────────────────
mkdir -p "$INSTALL_DIR"

# The archive mirrors cmake --install layout: ./bin/t81, ./lib/*, ./include/*
tar -xzf "$TMP/$ARCHIVE" -C "$TMP" ./bin/t81
install -m 755 "$TMP/bin/t81" "$INSTALL_DIR/t81"

echo ""
echo "  t81 ${TAG} installed → ${INSTALL_DIR}/t81"
echo ""

# ── PATH hint ─────────────────────────────────────────────────────────────────
case ":${PATH}:" in
  *":${INSTALL_DIR}:"*) ;;
  *)
    echo "  Add ${INSTALL_DIR} to your PATH to use t81 from any directory:"
    echo ""
    echo "    export PATH=\"\$PATH:${INSTALL_DIR}\""
    echo ""
    echo "  Paste the line above into ~/.bashrc, ~/.zshrc, or equivalent."
    echo ""
    ;;
esac

echo "  Run 't81 --version' to verify the installation."
