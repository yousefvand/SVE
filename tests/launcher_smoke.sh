#!/usr/bin/env bash
# Run as a regular non-root user: bash tests/launcher_smoke.sh
# Exercises menu repair, non-standard XDG data dir, Qt/CMake desktop template,
# and uninstall, all without Qt headers or a KDE graphical session.
set -euo pipefail
source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
prefix="$work/program files/SVE"
datahome="$work/xdg data"
mkdir -p -- "$prefix/bin" "$prefix/share/applications" "$work/mockbin"
printf '#!/bin/sh\nexit 0\n' > "$prefix/bin/sve"
chmod +x "$prefix/bin/sve"
printf '[Desktop Entry]\nType=Application\nName=SVE\nExec=sve %%f\nTryExec=sve\n' > "$prefix/share/applications/sve.desktop"
# A mocked Plasma cache rebuild ensures that integration actually calls it.
cat > "$work/mockbin/kbuildsycoca6" <<'SH'
#!/bin/sh
printf '%s\n' "$*" >> "$SVE_TEST_SYCOCA_LOG"
SH
chmod +x "$work/mockbin/kbuildsycoca6"
export SVE_TEST_SYCOCA_LOG="$work/sycoca.log"
export XDG_DATA_HOME="$datahome"
export PREFIX="$prefix"
export PATH="$work/mockbin:$PATH"
"$source_dir/install.sh" --repair-launcher
in_prefix="$prefix/share/applications/sve.desktop"
in_menu="$datahome/applications/sve.desktop"
test -f "$in_menu"
test -f "$datahome/icons/hicolor/scalable/apps/sve.svg"
test -s "$datahome/icons/hicolor/256x256/apps/sve.png"
grep -Fqx "Icon=$datahome/icons/hicolor/256x256/apps/sve.png" "$in_menu"
grep -Fqx "TryExec=$prefix/bin/sve" "$in_prefix"
grep -Fqx "Exec=\"$prefix/bin/sve\" %f" "$in_menu"
grep -Fqx -- '--noincremental' "$SVE_TEST_SYCOCA_LOG"
! grep -q '@SVE_EXECUTABLE@' "$in_menu"
# Also test configure_file() from the CMake desktop template independently.
cat > "$work/render.cmake" <<'CMAKE'
configure_file("${SOURCE}" "${TARGET}" @ONLY)
CMAKE
cmake -DSOURCE="$source_dir/assets/sve.desktop.in" \
  -DTARGET="$work/configured.desktop" \
  -DSVE_EXECUTABLE="$prefix/bin/sve" -DSVE_ICON="$prefix/share/icons/hicolor/256x256/apps/sve.png" -P "$work/render.cmake"
grep -Fqx "TryExec=$prefix/bin/sve" "$work/configured.desktop"
grep -Fqx "Icon=$prefix/share/icons/hicolor/256x256/apps/sve.png" "$work/configured.desktop"
"$source_dir/uninstall.sh"
test ! -e "$in_prefix"
test ! -e "$in_menu"
test ! -e "$prefix/bin/sve"
test ! -e "$datahome/icons/hicolor/scalable/apps/sve.svg"
test ! -e "$datahome/icons/hicolor/256x256/apps/sve.png"
# A missing binary must not leave a fake SVE entry in the menu.
if "$source_dir/install.sh" --repair-launcher > "$work/negative.log" 2>&1; then
  echo 'ERROR: repair accepted a missing executable' >&2
  exit 1
fi
echo 'PASS: launcher repair, CMake template, KDE cache invocation, custom XDG path and uninstall'
