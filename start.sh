#!/usr/bin/env bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

PORT=38921
URL="http://127.0.0.1:${PORT}"
PID_FILE="$DIR/.server.pid"
PROFILE_DIR="$HOME/.config/manyterminals/chrome-profile"

# Ensure dependencies are installed
if [ ! -d "$DIR/node_modules" ]; then
  echo "Installing dependencies..."
  npm install
fi

# Subcommand: server-only
if [ "$1" = "--server-only" ]; then
  echo "Starting ManyTerminals server in foreground..."
  exec node "$DIR/server.js"
fi

# Check if a server is already running
SERVER_PID=""
if curl -s -f "$URL/health" >/dev/null 2>&1; then
  echo "ManyTerminals backend is already running at $URL"
else
  echo "Starting ManyTerminals backend..."
  node "$DIR/server.js" > "$DIR/.server.log" 2>&1 &
  SERVER_PID=$!
  echo "$SERVER_PID" > "$PID_FILE"

  # Wait for server to be responsive
  ATTEMPTS=0
  until curl -s -f "$URL/health" >/dev/null 2>&1 || [ $ATTEMPTS -ge 40 ]; do
    sleep 0.1
    ATTEMPTS=$((ATTEMPTS + 1))
  done

  if [ $ATTEMPTS -ge 40 ]; then
    echo "Error: Backend failed to start. Log output:"
    cat "$DIR/.server.log"
    exit 1
  fi
  echo "Backend ready (PID $SERVER_PID)."
fi

# Clean up server on exit if we started it
cleanup() {
  if [ -n "$SERVER_PID" ]; then
    echo "Stopping background server (PID $SERVER_PID)..."
    kill "$SERVER_PID" 2>/dev/null || true
    rm -f "$PID_FILE"
  fi
}
trap cleanup EXIT INT TERM

# Find Chrome / Chromium binary
CHROME_BIN=""
for bin in google-chrome-stable google-chrome chromium; do
  if command -v "$bin" >/dev/null 2>&1; then
    CHROME_BIN="$bin"
    break
  fi
done

if [ -z "$CHROME_BIN" ]; then
  echo "Chrome/Chromium not found. Please open: $URL"
  echo "Press Ctrl+C to stop the server."
  wait "$SERVER_PID"
  exit 0
fi

mkdir -p "$PROFILE_DIR"
echo "Launching ManyTerminals app window ($CHROME_BIN)..."

"$CHROME_BIN" \
  --app="$URL" \
  --class="manyterminals" \
  --name="ManyTerminals" \
  --user-data-dir="$PROFILE_DIR" \
  --window-size=1280,820 \
  --enable-features=UseOzonePlatform \
  --ozone-platform=wayland \
  "$@" >/dev/null 2>&1

echo "ManyTerminals window closed."
