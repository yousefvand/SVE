#!/usr/bin/env bash
# Offline AUR package template test: no network, no AUR repo, no publishing.
set -Eeuo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
bash -n "$root/aur.sh"
grep -Fq 'VERSION="${VERSION:-0.1.0}"' "$root/aur.sh"
grep -Fq 'GITHUB_REPOSITORY="${GITHUB_REPOSITORY:-SVE}"' "$root/aur.sh"
grep -Fq 'PREPARE_ONLY=1' "$root/aur.sh"
if (( EUID == 0 )); then
  echo 'PASS: AUR helper syntax and safe-mode static checks (run as a regular user for offline integration test)'
  exit 0
fi
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
mkdir -p "$work/archive/SVE-0.1.0" "$work/mockbin"
printf 'project(SVE VERSION 0.1.0 LANGUAGES CXX)\n' > "$work/archive/SVE-0.1.0/CMakeLists.txt"
printf 'MIT License\n' > "$work/archive/SVE-0.1.0/LICENSE"
tar -czf "$work/mock-tag.tar.gz" -C "$work/archive" SVE-0.1.0
cat > "$work/mockbin/curl" <<'SH'
#!/usr/bin/env bash
set -euo pipefail
file=''
while (( $# > 0 )); do
  case "$1" in -o) shift; file="$1" ;; esac
  shift
done
[[ -n "$file" ]] || exit 2
cp -- "$SVE_AUR_TEST_TARBALL" "$file"
SH
cat > "$work/mockbin/makepkg" <<'SH'
#!/usr/bin/env bash
set -euo pipefail
[[ "${1:-}" == --printsrcinfo ]] || exit 2
printf 'pkgbase = sve\n\tpkgver = 0.1.0\n\tpkgrel = 1\n'
SH
chmod +x "$work/mockbin/curl" "$work/mockbin/makepkg"
export PATH="$work/mockbin:$PATH"
export SVE_AUR_TEST_TARBALL="$work/mock-tag.tar.gz"
export AUR_WORKDIR="$work/prepared"
"$root/aur.sh" --prepare-only --skip-build
pkgb="$AUR_WORKDIR/PKGBUILD"
test -s "$pkgb" && test -s "$AUR_WORKDIR/.SRCINFO"
bash -n "$pkgb"
grep -Fqx 'pkgver=0.1.0' "$pkgb"
grep -Fqx "_srcdir='SVE-0.1.0'" "$pkgb"
grep -Fq "'qt6-multimedia-ffmpeg'" "$pkgb"
grep -Fq 'DESTDIR="${pkgdir}" cmake --install build' "$pkgb"
grep -Fq '/usr/share/licenses/${pkgname}/LICENSE' "$pkgb"
expected="$(sha256sum "$work/mock-tag.tar.gz" | awk '{print $1}')"
grep -Fqx "sha256sums=('$expected')" "$pkgb"
if "$root/aur.sh" --prepare-only --skip-build >"$work/negative.log" 2>&1; then
  echo 'FAIL: AUR helper overwrote non-empty staging directory' >&2
  exit 1
fi
printf 'PASS: Offline AUR preparation, dependency list, checksum, safe staging, version and license packaging\n'
