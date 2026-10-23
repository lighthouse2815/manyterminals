#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

# Build if binary doesn't exist
if [ ! -f "$DIR/build/manyterminals" ]; then
  echo "Building Native ManyTerminals (Qt6)..."
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j$(nproc)
fi

exec "$DIR/build/manyterminals" "$@"
