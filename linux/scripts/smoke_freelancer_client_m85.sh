#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
if [[ -n "${1:-}" ]]; then
  BUILD_DIR="$1"
elif [[ -x "$ROOT/linux/linux-x86/freelancer_linux_client" ]]; then
  BUILD_DIR="$ROOT/linux/linux-x86"
else
  BUILD_DIR="$ROOT/linux/build-x86"
fi
BIN="$BUILD_DIR/freelancer_linux_client"

if [[ ! -x "$BIN" ]]; then
  echo "[FAIL] client binary not found or not executable: $BIN" >&2
  exit 2
fi

LOG="$(mktemp /tmp/fl_m85_smoke.XXXXXX.log)"
cleanup() { rm -f "$LOG"; }
trap cleanup EXIT

echo "[smoke] running: $BIN --smoke-xdg-temp"
set +e
(cd "$BUILD_DIR" && timeout 90 ./freelancer_linux_client --smoke-xdg-temp >"$LOG" 2>&1)
RC=$?
set -e

if [[ $RC -ne 0 ]]; then
  echo "[FAIL] freelancer_linux_client exited with code $RC" >&2
  if [[ $RC -eq 20 ]]; then
    echo "[HINT] WindowCreate failed (exit 20): check active Wayland/X11 session, DISPLAY/WAYLAND_DISPLAY, SDL3 video backend, and 32-bit runtime libs." >&2
  elif [[ $RC -eq 21 ]]; then
    echo "[HINT] RenderInit failed (exit 21): check 32-bit Vulkan loader/ICD (vulkaninfo/i386), GPU drivers, and SDL3+Vulkan availability in this environment." >&2
  fi
  tail -n 120 "$LOG" >&2 || true
  exit 1
fi

require() {
  local pat="$1"
  if ! grep -qE "$pat" "$LOG"; then
    echo "[FAIL] missing expected log/text: $pat" >&2
    tail -n 160 "$LOG" >&2 || true
    exit 1
  fi
}

require "BOOT\[Startup\]"
require "BOOT\[DacomInit\]"
require "BOOT\[CoreRegistrations\]"
require "BOOT\[ConfigAndData\].*core init/config/DATA checks passed"
require "BOOT\[IFileSystemProbe\]"
require "IFileSystem stream Read \(.*solar\\\\solararch\\.ini.*\): rd=.* bytes=[1-9][0-9]*"

require "BOOT\[AudioUnblock\]"
require "M8\.4: null audio backend satisfied boot/menu calls"
require "BOOT\[PersistenceUnblock\]"
require "M8\.4: XDG config/registry/save shim used for early settings/save calls"
require "BOOT\[BlockerStubs\]"
require "M8\.4: boot/menu blockers stubbed/bypassed"

require "BOOT\[WindowCreate\]"
require "M8\.3/M8\.5: native window created via fl_port::Window \(SDL driver=.*\)"
require "\[fl-port\]\[window\] created: .*driver="
require "BOOT\[RenderInit\]"
require "M8\.3/M8\.5: render pipeline initialized and connected to client window"
require "BOOT\[MainMenu\]"
require "M8\.5: reached main menu \(equivalent stable in-engine bring-up state\)"
require "BOOT\[ControlledState\].*main menu stable-state loop completed \(presented=[1-9][0-9]*\)"
require "M8\.5: controlled state complete \(presented=[1-9][0-9]*.*\)"
require "freelancer_linux_client \(M8\.5\): native client initialized engine bring-up, opened window, reached main menu"
require "Status: M8\.5 bring-up acceptance satisfied"
require "BOOT\[Shutdown\]"
require "M8\.4/M8\.5: removed isolated XDG temp root"

echo "[PASS] M8.5/M8.6 bring-up smoke checks passed (boot/init, window+render, M8.4 unblocks, controlled main-menu state, clean exit)"
