#!/usr/bin/env bash
# Protect against the blank 'Video exported' Yes/No completion modal.
set -euo pipefail
MAIN="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/src/mainwindow.cpp"
if grep -E -q 'QMessageBox::(information|question|warning)\(this, "Video exported"' "$MAIN"; then
  echo 'FAIL: export completion modal reintroduced' >&2
  exit 1
fi
grep -F -q 'm_openFolder->show();' "$MAIN"
grep -F -q 'Export complete' "$MAIN"
grep -F -q 'std::rename(' "$MAIN"
if grep -F -q 'QFile::remove(m_lastOutput)' "$MAIN"; then
  echo 'FAIL: non-atomic old-output deletion reintroduced' >&2
  exit 1
fi
printf 'PASS: completion is inline; existing video uses atomic replacement\n'
