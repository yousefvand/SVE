#!/usr/bin/env bash
# SVE installer for Arch Linux / KDE Plasma (Wayland or X11).
# The application and its launcher are installed into the invoking user's account.
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${PREFIX:-${HOME}/.local}"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
SKIP_DEPS="${SKIP_DEPS:-0}"
REPAIR_LAUNCHER=0

usage() {
  cat <<'TEXT'
SVE install.sh

Usage: ./install.sh [--skip-deps] [--repair-launcher] [--help]

Default: install Arch dependencies, compile, test and install SVE in ~/.local.
--skip-deps        Do not invoke pacman to install dependencies.
--repair-launcher  Repair KDE application-menu registration without rebuilding.

Environment:
  PREFIX=/some/prefix   Install root (default: ~/.local)
  BUILD_DIR=/some/dir   CMake build directory (default: ./build)
  XDG_DATA_HOME=...     Menu/icon data directory (default: ~/.local/share)

Run this script as your normal desktop user, NOT with sudo. The script uses
sudo only for missing system dependencies. SVE is then installed user-locally.
TEXT
}
for arg in "$@"; do
  case "$arg" in
    --help|-h) usage; exit 0 ;;
    --skip-deps) SKIP_DEPS=1 ;;
    --repair-launcher) REPAIR_LAUNCHER=1 ;;
    *) printf 'Unknown option: %s\n' "$arg" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ "$PREFIX" != /* ]]; then
  echo "PREFIX must be an absolute path (for example: $HOME/.local)." >&2
  exit 2
fi

if (( EUID == 0 )); then
  echo 'Do not run ./install.sh with sudo/root. Run it as your regular KDE user.' >&2
  echo 'The script will request sudo only if Arch system packages are missing.' >&2
  exit 1
fi

# By the XDG base-directory specification, relative XDG_DATA_HOME is ignored.
DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
if [[ "$DATA_HOME" != /* ]]; then
  printf 'Ignoring relative XDG_DATA_HOME (%s), using ~/.local/share\n' "$DATA_HOME" >&2
  DATA_HOME="${HOME}/.local/share"
fi

# The desktop file is generated with the full absolute binary path, so KDE's
# TryExec check succeeds even if ~/.local/bin is absent from Plasma's PATH.
register_launcher() {
  local executable="$PREFIX/bin/sve"
  local in_prefix="$PREFIX/share/applications/sve.desktop"
  local in_menu="$DATA_HOME/applications/sve.desktop"
  local icon_in_prefix="$PREFIX/share/icons/hicolor/scalable/apps/sve.svg"
  local png_in_menu="$DATA_HOME/icons/hicolor/256x256/apps/sve.png"
  local icon_in_menu="$DATA_HOME/icons/hicolor/scalable/apps/sve.svg"
  local contents

  if [[ ! -x "$executable" ]]; then
    printf 'Cannot register SVE: executable not found: %s\n' "$executable" >&2
    return 1
  fi

  # Repair both the menu entry and icon, even after a partial older install.
  if [[ ! -f "$icon_in_prefix" ]]; then
    install -Dm644 -- "$ROOT_DIR/assets/sve.svg" "$icon_in_prefix"
  fi
  # Use an absolute PNG icon path so KDE does not rely on hicolor theme cache
  # discovery or support for SVG thumbnail providers.
  local size
  for size in 48 128 256; do
    install -Dm644 -- "$ROOT_DIR/assets/sve-${size}.png" \
      "$PREFIX/share/icons/hicolor/${size}x${size}/apps/sve.png"
    if [[ "$DATA_HOME" != "$PREFIX/share" ]]; then
      install -Dm644 -- "$ROOT_DIR/assets/sve-${size}.png" \
        "$DATA_HOME/icons/hicolor/${size}x${size}/apps/sve.png"
    fi
  done
  contents="$(< "$ROOT_DIR/assets/sve.desktop.in")"
  contents="${contents//@SVE_EXECUTABLE@/$executable}"
  contents="${contents//@SVE_ICON@/$png_in_menu}"
  install -d -- "$(dirname -- "$in_prefix")"
  printf '%s\n' "$contents" > "$in_prefix"
  chmod 644 "$in_prefix"

  # Custom PREFIX is not necessarily searched by Plasma. Register in the
  # standard XDG user applications directory as well when it differs.
  if [[ "$in_menu" != "$in_prefix" ]]; then
    install -Dm644 -- "$in_prefix" "$in_menu"
  fi
  if [[ "$icon_in_menu" != "$icon_in_prefix" ]]; then
    install -Dm644 -- "$ROOT_DIR/assets/sve.svg" "$icon_in_menu"
  fi

  if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$(dirname -- "$in_menu")" >/dev/null 2>&1 || true
  fi
  if command -v gtk-update-icon-cache >/dev/null 2>&1 && [[ -d "$DATA_HOME/icons/hicolor" ]]; then
    gtk-update-icon-cache -f -t "$DATA_HOME/icons/hicolor" >/dev/null 2>&1 || true
  fi
  # update-desktop-database refreshes MIME associations, not KDE's app index.
  # Ask Plasma to rebuild the menu index so that the new entry appears now.
  if command -v kbuildsycoca6 >/dev/null 2>&1; then
    kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
  elif command -v kbuildsycoca5 >/dev/null 2>&1; then
    kbuildsycoca5 --noincremental >/dev/null 2>&1 || true
  fi

  printf 'SVE menu entry: %s\n' "$in_menu"
  printf 'SVE executable: %s\n' "$executable"
  printf 'SVE icon: %s\n' "$png_in_menu"
}

if (( REPAIR_LAUNCHER == 1 )); then
  echo '==> Repairing the KDE launcher for the existing SVE installation'
  register_launcher
  echo 'Launcher updated. Search for "SVE" in the KDE application menu.'
  exit 0
fi

if [[ "$SKIP_DEPS" != "1" ]]; then
  if ! command -v pacman >/dev/null; then
    echo 'This installer targets Arch Linux and requires pacman. Use --skip-deps if dependencies are already installed.' >&2
    exit 1
  fi
  packages=(base-devel cmake ninja pkgconf ffmpeg qt6-base qt6-svg qt6-wayland qt6-multimedia qt6-multimedia-ffmpeg)
  echo '==> Checking Arch Linux dependencies'
  missing=()
  for package in "${packages[@]}"; do
    if ! pacman -Q -- "$package" >/dev/null 2>&1; then missing+=("$package"); fi
  done
  if (( ${#missing[@]} > 0 )); then
    printf '==> Installing missing packages: %s\n' "${missing[*]}"
    if ! command -v sudo >/dev/null; then
      echo 'sudo is required to install missing system packages, or use --skip-deps.' >&2
      exit 1
    fi
    sudo pacman -S --needed -- "${missing[@]}"
  fi
fi

command -v cmake >/dev/null || { echo 'cmake is missing' >&2; exit 1; }
command -v ninja >/dev/null || { echo 'ninja is missing' >&2; exit 1; }
command -v ffmpeg >/dev/null || { echo 'ffmpeg is missing' >&2; exit 1; }
command -v ffprobe >/dev/null || { echo 'ffprobe is missing' >&2; exit 1; }

printf '==> Configuring SVE at %s\n' "$PREFIX"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DBUILD_TESTING=ON

echo '==> Building SVE'
cmake --build "$BUILD_DIR" --parallel "$(nproc)"
echo '==> Running backend tests'
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo '==> Installing SVE'
cmake --install "$BUILD_DIR"
register_launcher
cat <<TEXT

SVE installed successfully.

Open the KDE application menu and search for SVE.
Or run: "$PREFIX/bin/sve"

To repair the KDE launcher later: ./install.sh --repair-launcher
To uninstall: ./uninstall.sh
TEXT
