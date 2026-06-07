#!/bin/bash
# build.sh — local build helper for cpp-update-agent
set -e

BUILD_DIR="build"
TYPE="${1:-Release}"  # pass Debug as arg to get debug build

echo "=== cpp-update-agent build script ==="
echo "Build type: $TYPE"

# Check dependencies
for dep in cmake g++ pkg-config; do
    if ! command -v "$dep" &>/dev/null; then
        echo "[ERROR] Missing dependency: $dep"
        echo "Install with: sudo apt-get install cmake g++ pkg-config libcurl4-openssl-dev libssl-dev"
        exit 1
    fi
done

# Configure
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$TYPE"

# Build
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

echo ""
echo "=== Build successful ==="
echo "Binary: $BUILD_DIR/update_agent"
echo ""
echo "Run with:  ./$BUILD_DIR/update_agent config/agent.conf"
echo "Run tests: ctest --test-dir $BUILD_DIR --output-on-failure"
