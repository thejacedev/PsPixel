#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$ROOT_DIR/desktop"
BUILD_DIR="$ROOT_DIR/build"
BIN="$BUILD_DIR/bin/PsPixel"

# Configure if build dir doesn't exist yet
if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo ":: Configuring project..."
    cmake -B "$BUILD_DIR" -G Ninja "$PROJECT_DIR"
fi

# Build (incremental - only recompiles changed files)
echo ":: Building..."
ninja -C "$BUILD_DIR"

# Run
echo ":: Running PsPixel"
exec "$BIN" "$@"
