#!/usr/bin/env bash
# Called from lefthook.yml. Kept in a file because lefthook on Windows wraps an
# inline multi-line `run: |` block in sh -c "..." and the inner double quotes break it.
for file in "$@"; do
  if [[ -f "$file" ]]; then
    lines=$(wc -l < "$file" | tr -d ' ')
    if [[ $lines -gt 400 ]]; then
      echo "⚠️  Warning: $file has $lines lines (threshold: 400)"
      echo "   Consider splitting this file if it has multiple responsibilities."
    fi
  fi
done
