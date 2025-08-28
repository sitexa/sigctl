#!/usr/bin/env bash
set -euo pipefail

# 构建
cargo build --release

# 复制到目标（本机模拟路径；实际设备上请调整为 /mnt/nandflash）
TARGET_BIN=target/release/sigctl
OUT_DIR=/mnt/nandflash
if [ -f "$TARGET_BIN" ]; then
  echo "Copying $TARGET_BIN -> $OUT_DIR"
  mkdir -p "$OUT_DIR"
  cp "$TARGET_BIN" "$OUT_DIR/"
  echo "Done. Run: $OUT_DIR/sigctl"
else
  echo "Build output not found: $TARGET_BIN" >&2
  exit 1
fi