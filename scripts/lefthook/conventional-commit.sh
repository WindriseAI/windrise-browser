#!/usr/bin/env bash
# Run from lefthook.yml. Kept in a file because lefthook on Windows wraps inline
# multi-line run: blocks in sh -c "..." and the inner double quotes break the quoting.
msg_file="$1"
msg=$(head -1 "$msg_file")
if [[ ! "$msg" =~ ^(feat|fix|docs|style|refactor|perf|test|chore|ci|build|revert)(\(.+\))?\!?:\ .+ ]]; then
  echo "Commit message must follow Conventional Commits format:"
  echo "  <type>(<optional scope>): <description>"
  echo "  Types: feat, fix, docs, style, refactor, perf, test, chore, ci, build, revert"
  echo ""
  echo "Examples:"
  echo "  feat(auth): add OAuth2 support"
  echo "  fix: resolve null pointer exception"
  exit 1
fi
