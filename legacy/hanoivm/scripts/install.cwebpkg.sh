#!/bin/bash
# install.cwebpkg.sh | Smart installer for hanoivm + axion kernel package

set -e
PACKAGE_NAME="hanoivm"
LOGFILE="install.log"
AUTO_LOAD=false
RUN_TESTS=false

echo "[install] Installing $PACKAGE_NAME from .cwebpkg..."

# Optional flags
while [[ "$#" -gt 0 ]]; do
  case $1 in
    --auto-load)
      AUTO_LOAD=true
      ;;
    --with-tests)
      RUN_TESTS=true
      ;;
  esac
  shift
done

# Ensure required tools exist
for tool in ctangle make insmod; do
  if ! command -v $tool &>/dev/null; then
    echo "[error] Required tool '$tool' not found." | tee -a $LOGFILE
    exit 1
  fi
done

# Step 1: Tangle all .cweb files into .c
echo "[step 1] Tangling source files..." | tee -a $LOGFILE
./tangle-all.sh | tee -a $LOGFILE

# Step 2: Build kernel modules
echo "[step 2] Building kernel modules..." | tee -a $LOGFILE
make -f build-all.cweb | tee -a $LOGFILE

# Step 3: Optionally load main modules
if [ "$AUTO_LOAD" = true ]; then
  echo "[step 3] Auto-loading modules..." | tee -a $LOGFILE
  sudo insmod axion-ai.ko || true
  sudo insmod hanoivm_vm.ko || true
fi

# Step 4: Optionally run kernel tests
if [ "$RUN_TESTS" = true ]; then
  echo "[step 4] Running tests..." | tee -a $LOGFILE
  sudo insmod hanoivm-test.ko || true
  sleep 1
  echo "[test output]" | tee -a $LOGFILE
  sudo cat /sys/kernel/debug/hanoivm-test | tee -a $LOGFILE
fi

# Step 5: Log registration (if Axion is active)
if [ -f /var/log/axion/meta.log ]; then
  echo "[axion] Registering $PACKAGE_NAME with local AI metadata..." | tee -a $LOGFILE
  echo "{ \"pkg\": \"$PACKAGE_NAME\", \"installed\": true, \"ts\": \"$(date -Iseconds)\" }" | sudo tee -a /var/log/axion/meta.log
fi

echo "[install] Completed installation of $PACKAGE_NAME." | tee -a $LOGFILE
