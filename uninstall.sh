#!/usr/bin/env bash
# Remove SVE's user-local files, not Qt, FFmpeg, projects or media.
set -euo pipefail
PREFIX="${PREFIX:-${HOME}/.local}"
DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
[[ "$DATA_HOME" = /* ]] || DATA_HOME="${HOME}/.local/share"

if [[ ${1:-} == '--help' || ${1:-} == '-h' ]]; then
  cat <<'TEXT'
Usage: ./uninstall.sh

Removes the SVE executable, desktop entry and SVG icon from PREFIX
(default: ~/.local), plus an XDG menu launcher created for a custom prefix.
Never removes FFmpeg, Qt, your videos, projects or SVE settings.

Environment: PREFIX=/some/prefix XDG_DATA_HOME=/some/dir
Run as your normal KDE user without sudo.
TEXT
  exit 0
fi
if (( $# > 0 )); then echo 'Unknown argument' >&2; exit 2; fi
if (( EUID == 0 )); then echo 'Run as your normal KDE user, not with sudo.' >&2; exit 1; fi

case "$PREFIX" in
  ''|'/'|'/usr'|'/usr/local'|'/bin'|'/home')
    echo 'Refusing unsafe installation prefix. Set PREFIX to your installation directory.' >&2
    exit 1
    ;;
esac

# Avoid accidentally deleting a launcher managed by another installation.
menu_desktop="$DATA_HOME/applications/sve.desktop"
prefix_desktop="$PREFIX/share/applications/sve.desktop"
menu_icon="$DATA_HOME/icons/hicolor/scalable/apps/sve.svg"
prefix_icon="$PREFIX/share/icons/hicolor/scalable/apps/sve.svg"
remove_menu_copy=0
if [[ "$menu_desktop" != "$prefix_desktop" && -f "$menu_desktop" ]] &&
   grep -Fqx "TryExec=$PREFIX/bin/sve" "$menu_desktop"; then
  remove_menu_copy=1
fi
if (( remove_menu_copy )); then
  rm -- "$menu_desktop"
  printf 'Removed %s\n' "$menu_desktop"
  if [[ "$menu_icon" != "$prefix_icon" && -f "$menu_icon" && -f "$prefix_icon" ]] &&
     cmp -s -- "$menu_icon" "$prefix_icon"; then
    rm -- "$menu_icon"
    printf 'Removed %s\n' "$menu_icon"
  fi
fi

# Remove themed PNGs; a shared XDG copy only belongs to us if it matches
# the prefix's installed SVE icon and launcher.
for size in 48 128 256; do
  prefix_png="$PREFIX/share/icons/hicolor/${size}x${size}/apps/sve.png"
  menu_png="$DATA_HOME/icons/hicolor/${size}x${size}/apps/sve.png"
  if (( remove_menu_copy )) && [[ "$menu_png" != "$prefix_png" && -f "$menu_png" && -f "$prefix_png" ]] &&
     cmp -s -- "$menu_png" "$prefix_png"; then
    rm -- "$menu_png"
    printf 'Removed %s\n' "$menu_png"
  fi
  if [[ -f "$prefix_png" ]]; then
    rm -- "$prefix_png"
    printf 'Removed %s\n' "$prefix_png"
  fi
done

paths=(
  "$PREFIX/bin/sve"
  "$prefix_desktop"
  "$prefix_icon"
)
for path in "${paths[@]}"; do
  if [[ -f "$path" ]]; then
    rm -- "$path"
    printf 'Removed %s\n' "$path"
  fi
done
if command -v update-desktop-database >/dev/null 2>&1 && [[ -d "$DATA_HOME/applications" ]]; then
  update-desktop-database "$DATA_HOME/applications" >/dev/null 2>&1 || true
fi
if command -v kbuildsycoca6 >/dev/null 2>&1; then
  kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
elif command -v kbuildsycoca5 >/dev/null 2>&1; then
  kbuildsycoca5 --noincremental >/dev/null 2>&1 || true
fi
printf 'SVE uninstalled from %s. Personal files and system packages are preserved.\n' "$PREFIX"
