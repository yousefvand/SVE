#!/usr/bin/env bash
# SVE AUR publisher. AUR stores only PKGBUILD and .SRCINFO; sources come from
# a public, immutable GitHub version tag. Run without sudo on Arch Linux.
set -Eeuo pipefail

MAINTAINER_NAME="${MAINTAINER_NAME:-Remisa Phillips}"
MAINTAINER_EMAIL="${MAINTAINER_EMAIL:-remisa.yousefvand@gmail.com}"
AUR_PACKAGE_NAME="${AUR_PACKAGE_NAME:-sve}"
GITHUB_OWNER="${GITHUB_OWNER:-yousefvand}"
GITHUB_REPOSITORY="${GITHUB_REPOSITORY:-SVE}"
VERSION="${VERSION:-0.1.0}"
PKGREL="${PKGREL:-1}"
# TODO: Confirm that the matching *public* GitHub tag exists before publishing.
# Override with GITHUB_TAG=v0.1.0 if releases use a 'v' prefix.
GITHUB_TAG="${GITHUB_TAG:-${VERSION}}"
# TODO: Confirm that this AUR package name is available/owned by your AUR account.
AUR_WORKDIR="${AUR_WORKDIR:-${TMPDIR:-/tmp}/${AUR_PACKAGE_NAME}-aur}"
AUR_SSH_URL="${AUR_SSH_URL:-ssh://aur@aur.archlinux.org/${AUR_PACKAGE_NAME}.git}"
UPSTREAM_URL="https://github.com/${GITHUB_OWNER}/${GITHUB_REPOSITORY}"
SOURCE_URL="${UPSTREAM_URL}/archive/refs/tags/${GITHUB_TAG}.tar.gz"
PREPARE_ONLY=0
SKIP_BUILD="${AUR_SKIP_BUILD:-0}"

log() { printf '[sve-aur] %s\n' "$*"; }
fail() { printf '[sve-aur] ERROR: %s\n' "$*" >&2; exit 1; }
usage() {
  cat <<'HELP'
Usage: ./aur.sh [--prepare-only] [--skip-build] [--help]

By default: download the public GitHub tag, compute its checksum, prepare the
AUR PKGBUILD/.SRCINFO, test-build the package, commit and push to AUR over SSH.
--prepare-only  Generate PKGBUILD/.SRCINFO locally without cloning or pushing.
--skip-build    Do not invoke makepkg's build/install-dependency checks.

Defaults: VERSION=0.1.0 AUR_PACKAGE_NAME=sve GITHUB_TAG=0.1.0.
Set GITHUB_TAG=v0.1.0 if your GitHub release has the 'v' prefix.
You can override MAINTAINER_NAME, MAINTAINER_EMAIL, PKGREL, AUR_WORKDIR,
AUR_SSH_URL, GITHUB_OWNER and GITHUB_REPOSITORY via environment variables.

Before publishing, confirm: (1) the public GitHub tag exists and contains the
current source; (2) the AUR package name is available or yours; (3) AUR SSH
keys and an Arch build environment are configured.
HELP
}
for arg in "$@"; do
  case "$arg" in
    --prepare-only) PREPARE_ONLY=1 ;;
    --skip-build) SKIP_BUILD=1 ;;
    -h|--help) usage; exit 0 ;;
    *) fail "Unknown argument: $arg (see --help)" ;;
  esac
done

(( EUID != 0 )) || fail 'Do not run aur.sh/makepkg as root.'
[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+([.][0-9]+)?$ ]] || fail 'VERSION must be a numeric release (such as 0.1.0).'
[[ "$PKGREL" =~ ^[0-9]+$ ]] || fail 'PKGREL must be a positive integer.'
[[ "$GITHUB_TAG" =~ ^[A-Za-z0-9._-]+$ ]] || fail 'GITHUB_TAG has invalid characters.'
[[ "$GITHUB_OWNER" =~ ^[A-Za-z0-9._-]+$ && "$GITHUB_REPOSITORY" =~ ^[A-Za-z0-9._-]+$ ]] || fail 'Invalid GitHub owner/repository.'
[[ "$AUR_PACKAGE_NAME" =~ ^[a-z0-9@._+-]+$ ]] || fail 'Invalid AUR package name.'
[[ "$AUR_WORKDIR" = /* ]] || fail 'AUR_WORKDIR must be an absolute path.'
for command_name in curl sha256sum tar makepkg; do
  command -v "$command_name" >/dev/null 2>&1 || fail "Missing required command: ${command_name}"
done
if (( ! PREPARE_ONLY )); then
  command -v git >/dev/null 2>&1 || fail 'Missing required command: git'
fi

log "Fetching GitHub tag '${GITHUB_TAG}' for checksum (not a local ZIP)."
tarball="$(mktemp --suffix=.tar.gz)"
trap 'rm -f -- "$tarball"' EXIT
curl --fail --location --silent --show-error "$SOURCE_URL" -o "$tarball" \
  || fail "Could not fetch $SOURCE_URL. Create/push the public release tag first."
tar -tzf "$tarball" >/dev/null || fail 'Downloaded GitHub archive is not a valid tar.gz.'
# Prevent publishing a historical tag whose CMake project version differs
# from the requested AUR release. It is not enough that a tarball downloads.
source_cmake="${GITHUB_REPOSITORY}-${GITHUB_TAG}/CMakeLists.txt"
if ! tar -xOzf "$tarball" "$source_cmake" 2>/dev/null | \
     grep -Eq "^[[:space:]]*project\(SVE[[:space:]]+VERSION[[:space:]]+${VERSION}([[:space:]]|\))"; then
  fail "GitHub tag '$GITHUB_TAG' does not contain SVE CMake project version $VERSION. Publish the updated source tag first."
fi
checksum="$(sha256sum "$tarball" | awk '{print $1}')"
[[ "$checksum" =~ ^[0-9a-f]{64}$ ]] || fail 'Invalid source checksum.'

if (( PREPARE_ONLY )); then
  mkdir -p -- "$AUR_WORKDIR"
  # Avoid modifying an existing AUR checkout or arbitrary directory.
  [[ ! -e "$AUR_WORKDIR/.git" ]] || fail 'Prepare-only requires a separate staging directory, not a Git checkout.'
  if [[ -n "$(find "$AUR_WORKDIR" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    fail "Prepare-only destination must be empty: $AUR_WORKDIR"
  fi
else
  if [[ -d "$AUR_WORKDIR/.git" ]]; then
    [[ "$(git -C "$AUR_WORKDIR" remote get-url origin)" == "$AUR_SSH_URL" ]] \
      || fail 'Existing AUR checkout has a different remote origin.'
    log 'Refreshing existing AUR checkout.'
    git -C "$AUR_WORKDIR" fetch origin
  elif [[ ! -e "$AUR_WORKDIR" ]]; then
    log "Cloning $AUR_SSH_URL"
    git clone "$AUR_SSH_URL" "$AUR_WORKDIR"
  else
    fail "AUR_WORKDIR already exists but is not an AUR Git checkout: $AUR_WORKDIR"
  fi
  if git -C "$AUR_WORKDIR" show-ref --verify --quiet refs/remotes/origin/master; then
    git -C "$AUR_WORKDIR" checkout -B master origin/master
  else
    git -C "$AUR_WORKDIR" checkout -B master
  fi
  extra="$(git -C "$AUR_WORKDIR" ls-files | grep -Ev '^(PKGBUILD|\.SRCINFO)$' || true)"
  [[ -z "$extra" ]] || fail "Unexpected tracked files in AUR checkout (remove manually): $extra"
  git -C "$AUR_WORKDIR" config user.name "$MAINTAINER_NAME"
  git -C "$AUR_WORKDIR" config user.email "$MAINTAINER_EMAIL"
fi

cat > "$AUR_WORKDIR/PKGBUILD" <<PKGBUILD
# Maintainer: ${MAINTAINER_NAME} <${MAINTAINER_EMAIL}>
pkgname=${AUR_PACKAGE_NAME}
pkgver=${VERSION}
pkgrel=${PKGREL}
pkgdesc='Native Qt 6 video editor with trimming, filters and FFmpeg export'
arch=('x86_64')
url='${UPSTREAM_URL}'
license=('MIT')
depends=('qt6-base' 'qt6-svg' 'qt6-multimedia' 'qt6-multimedia-ffmpeg' 'qt6-wayland' 'ffmpeg' 'hicolor-icon-theme')
makedepends=('cmake' 'ninja')
_tag='${GITHUB_TAG}'
_srcdir='${GITHUB_REPOSITORY}-${GITHUB_TAG}'
source=("\${pkgname}-\${pkgver}.tar.gz::\${url}/archive/refs/tags/\${_tag}.tar.gz")
sha256sums=('${checksum}')

build() {
  cmake -S "\${srcdir}/\${_srcdir}" -B build -G Ninja \\
    -DCMAKE_BUILD_TYPE=Release \\
    -DCMAKE_INSTALL_PREFIX=/usr \\
    -DBUILD_TESTING=OFF
  cmake --build build
}

package() {
  DESTDIR="\${pkgdir}" cmake --install build
  install -Dm644 "\${srcdir}/\${_srcdir}/LICENSE" \\
    "\${pkgdir}/usr/share/licenses/\${pkgname}/LICENSE"
}
PKGBUILD

(
  cd "$AUR_WORKDIR"
  makepkg --printsrcinfo > .SRCINFO
  if [[ "$SKIP_BUILD" != 1 ]]; then
    log 'Running Arch package build and dependency verification.'
    makepkg --cleanbuild --syncdeps --noconfirm
    # Generated build outputs stay out of the AUR repository.
    rm -rf -- pkg src
    rm -f -- ./*.pkg.tar.zst ./*.pkg.tar.xz ./*.pkg.tar.gz
  fi
  if (( PREPARE_ONLY )); then
    log "Prepared $AUR_WORKDIR/{PKGBUILD,.SRCINFO} (NOT published)."
    exit 0
  fi
  git add PKGBUILD .SRCINFO
  if git diff --cached --quiet; then
    log 'AUR metadata unchanged; nothing to push.'
    exit 0
  fi
  git commit -m "Update SVE to ${VERSION}-${PKGREL}"
  git push origin master
)
if (( ! PREPARE_ONLY )); then
  log "AUR package metadata published: ${AUR_PACKAGE_NAME} ${VERSION}-${PKGREL}."
fi
