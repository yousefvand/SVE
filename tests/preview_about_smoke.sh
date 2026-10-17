#!/usr/bin/env bash
# Static GUI wiring checks + end-to-end FFmpeg MP4 preview decoding test.
# A full Qt/Wayland integration run is still needed on the target Arch machine.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source="$root/src/mainwindow.cpp"
main="$root/src/main.cpp"
grep -Fq 'QVideoSink::videoFrameChanged' "$source"
grep -Fq 'frame.toImage()' "$source"
grep -Fq 'startFallback(' "$source"
grep -Fq 'setDesktopFileName("sve")' "$main"
grep -Fq 'QT_MEDIA_BACKEND' "$main"
grep -Fq 'QMessageBox::aboutQt(this, "About Qt");' "$source"
grep -Fq 'details->setAlignment(Qt::AlignLeft | Qt::AlignTop);' "$source"
grep -Fq 'Remisa Phillips' "$source"
grep -Fq 'https://github.com/yousefvand/SVE' "$source"
! grep -Fq 'new QVideoWidget' "$source"
python3 - "$root" <<'PY'
from pathlib import Path
import sys
root=Path(sys.argv[1]); desktop=(root/'assets/sve.desktop.in').read_text()
assert 'Icon=@SVE_ICON@' in desktop
for size in (48,128,256):
    p=root/f'assets/sve-{size}.png'
    b=p.read_bytes()
    assert b[:8] == b'\x89PNG\r\n\x1a\n'
    assert int.from_bytes(b[16:20], 'big') == size
    assert int.from_bytes(b[20:24], 'big') == size
PY
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
# Prefer a real H.264 MP4, but remain useful with codec-limited FFmpeg builds.
codec=mpeg4
if ffmpeg -hide_banner -encoders 2>/dev/null | grep -q 'libx264'; then codec=libx264; fi
if [[ "$codec" == libx264 ]]; then
  ffmpeg -nostdin -hide_banner -loglevel error -y -f lavfi -i 'testsrc2=size=320x180:rate=15:duration=2' \
    -an -c:v libx264 -preset ultrafast -pix_fmt yuv420p "$work/example.mp4"
else
  ffmpeg -nostdin -hide_banner -loglevel error -y -f lavfi -i 'testsrc2=size=320x180:rate=15:duration=2' \
    -an -c:v mpeg4 -q:v 3 "$work/example.mp4"
fi
ffprobe -v error -select_streams v:0 -show_entries stream=codec_name -of csv=p=0 "$work/example.mp4" | grep -Eq 'h264|mpeg4'
ffmpeg -hide_banner -nostdin -v error -ss 0.400 -i "$work/example.mp4" -map 0:v:0 -frames:v 1 \
  -vf 'scale=960:540:force_original_aspect_ratio=decrease' -f image2pipe -vcodec png "$work/frame.png"
file "$work/frame.png" | grep -q 'PNG image data'
ffmpeg -nostdin -hide_banner -v error -ss 0 -i "$work/example.mp4" -map 0:v:0 -an \
  -vf 'fps=15,scale=960:540:force_original_aspect_ratio=decrease,pad=960:540:(ow-iw)/2:(oh-ih)/2,format=rgb24' \
  -pix_fmt rgb24 -f rawvideo "$work/frames.rgb"
python3 - "$work/frames.rgb" <<'PY'
from pathlib import Path
import sys
n=Path(sys.argv[1]).stat().st_size
assert n >= 960*540*3*12, n
assert n % (960*540*3) == 0, n
print('PASS: SVE Qt frame sink wiring, About dialogs (left-aligned SVE and default Qt), icons, MP4 decoding and streaming fallback')
PY
