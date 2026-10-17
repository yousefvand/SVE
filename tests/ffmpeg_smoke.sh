#!/usr/bin/env bash
# Supplemental integration test for the filter graph and audio-processed concat.
# Does not require Qt headers and can be run on CI or a developer workstation.
set -euo pipefail
command -v ffmpeg >/dev/null
command -v ffprobe >/dev/null
TMP="$(mktemp -d)"
trap 'rm -rf -- "$TMP"' EXIT
ffmpeg -nostdin -hide_banner -loglevel error -y -f lavfi -i testsrc2=size=320x180:rate=25 \
  -f lavfi -i sine=frequency=440:sample_rate=48000 -t 3 -c:v mpeg4 -q:v 5 -c:a aac -shortest "$TMP/source.mp4"
# Same concat, audio gain and fades graph emitted by MediaEngine::makeExport.
ffmpeg -nostdin -hide_banner -loglevel error -y -i "$TMP/source.mp4" \
  -filter_complex '[0:v:0]trim=start=0.000:end=0.650,setpts=PTS-STARTPTS[v0];[0:a:0]atrim=start=0.000:end=0.650,asetpts=PTS-STARTPTS[a0];[0:v:0]trim=start=1.450:end=2.150,setpts=PTS-STARTPTS[v1];[0:a:0]atrim=start=1.450:end=2.150,asetpts=PTS-STARTPTS[a1];[v0][a0][v1][a1]concat=n=2:v=1:a=1[vout][acat];[acat]volume=-3dB,afade=t=in:st=0:d=0.200,afade=t=out:st=1.150:d=0.200[aout]' \
  -map '[vout]' -map '[aout]' -r 25 -c:v mpeg4 -q:v 5 -pix_fmt yuv420p -c:a aac -b:a 192k "$TMP/audio-adjusted.mp4"
ffprobe -v error -select_streams a:0 -show_entries stream=codec_name -of default=nw=1 "$TMP/audio-adjusted.mp4" | grep -q 'codec_name=aac'
# Same preview filter graph emitted by mainwindow.
ffmpeg -nostdin -hide_banner -loglevel error -y -ss 0.8 -i "$TMP/source.mp4" \
  -map 0:v:0 -frames:v 1 -vf 'hflip,eq=brightness=0.100:contrast=1.200:saturation=0.800,scale=trunc(iw/2)*2:trunc(ih/2)*2,format=yuv420p,format=rgb24' \
  -f image2pipe -vcodec png pipe:1 > "$TMP/preview.png"
ffprobe -v error -select_streams v:0 -show_entries stream=codec_name -of default=nw=1 "$TMP/preview.png" | grep -q 'codec_name=png'
# Export with middle discarded.
ffmpeg -nostdin -hide_banner -loglevel error -y -i "$TMP/source.mp4" \
  -filter_complex '[0:v:0]trim=start=0.000:end=0.400,setpts=PTS-STARTPTS[v0];[0:a:0]atrim=start=0.000:end=0.400,asetpts=PTS-STARTPTS[a0];[0:v:0]trim=start=2.100:end=2.800,setpts=PTS-STARTPTS[v1];[0:a:0]atrim=start=2.100:end=2.800,asetpts=PTS-STARTPTS[a1];[v0][a0][v1][a1]concat=n=2:v=1:a=1[vout][aout]' \
  -map '[vout]' -map '[aout]' -r 25 -c:v mpeg4 -q:v 5 -pix_fmt yuv420p -c:a aac -b:a 192k "$TMP/exclude.mp4"
DURATION="$(ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 "$TMP/exclude.mp4")"
awk -v duration="$DURATION" 'BEGIN { exit !(duration > 0.9 && duration < 1.4) }'
echo 'PASS: audio processing, filtered preview PNG, and exclusion export'
