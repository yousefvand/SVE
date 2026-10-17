#include "mediaengine.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtMath>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char *message) {
    std::cout << (ok ? "PASS  " : "FAIL  ") << message << '\n';
    if (!ok) ++failures;
}
bool command(const QString &app, const QStringList &args, int timeoutMs = 60000) {
    QProcess proc;
    proc.start(app, args);
    if (!proc.waitForStarted(3000)) return false;
    if (!proc.waitForFinished(timeoutMs)) { proc.kill(); proc.waitForFinished(); return false; }
    if (proc.exitCode() != 0) std::cerr << proc.readAllStandardError().toStdString() << '\n';
    return proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0;
}
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    check(MediaEngine::timecode(3723123) == "01:02:03.123", "timecode formatting");
    check(MediaEngine::ffTime(1501) == "1.501", "FFmpeg seconds formatting");
    FilterOptions f;
    f.flipH = true; f.grayscale = true; f.height = 720;
    check(f.active() && f.graphFilters().join(',').contains("hflip") && f.graphFilters().join(',').contains("scale=-2:720"), "video filter graph");
    check(FilterOptions::fromJson(f.toJson()).height == 720, "filter settings JSON round trip");
    ExportOptions o;
    o.input = "/tmp/sve-source.mp4";
    o.output = "/tmp/sve-out.mp4";
    o.fullDurationMs = 4000;
    o.hasAudio = true;
    o.clips = {{1000, 2500}};
    auto cmd = MediaEngine::makeExport(o);
    check(cmd.valid() && cmd.expectedMs == 1500 && cmd.args.contains("-ss"), "single-range accurate export command");
    o.videoCodec = "Copy";
    o.filters = f;
    check(!MediaEngine::makeExport(o).valid(), "reject copy with filters");
    o.filters = FilterOptions{};
    o.clips = {{100, 500}, {1700, 2200}};
    check(!MediaEngine::makeExport(o).valid(), "reject multi-clip stream copy");
    o.videoCodec = "H.264";
    o.audioCodec = "AAC";
    cmd = MediaEngine::makeExport(o);
    check(cmd.valid() && cmd.expectedMs == 900 && cmd.args.contains("-filter_complex"), "multi-clip concat command");
    o.audioGainDb = 6;
    o.audioFadeInMs = 250;
    o.audioFadeOutMs = 250;
    cmd = MediaEngine::makeExport(o);
    check(cmd.valid() && cmd.args.join(' ').contains("volume=6dB") &&
          cmd.args.join(' ').contains("afade=t=in") && cmd.args.join(' ').contains("afade=t=out"),
          "assemble audio adjustments after concat");
    o.audioCodec = "Copy";
    check(!MediaEngine::makeExport(o).valid(), "reject audio copy combined with gain/fades");
    o.audioCodec = "AAC";
    o.clips = {{200, 1800}};
    cmd = MediaEngine::makeExport(o);
    check(cmd.valid() && cmd.args.contains("-af") && cmd.args.join(' ').contains("volume=6dB"),
          "single-range audio filter command");
    o.audioFadeInMs = 2500;
    check(!MediaEngine::makeExport(o).valid(), "reject audio fade longer than export");
    o.audioFadeInMs = 0;
    o.audioFadeOutMs = 0;
    o.audioGainDb = 0;
    o.clips = {{-1, 30}};
    check(!MediaEngine::makeExport(o).valid(), "reject negative range");
    o.clips = {{0, 1000}};
    o.container = "webm";
    check(!MediaEngine::makeExport(o).valid(), "WebM codec validation");
    if (QStandardPaths::findExecutable("ffmpeg").isEmpty() || QStandardPaths::findExecutable("ffprobe").isEmpty()) {
        std::cout << "SKIP  FFmpeg integration tests (ffmpeg or ffprobe missing)\n";
        return failures ? 1 : 0;
    }
    QTemporaryDir temp;
    check(temp.isValid(), "temporary integration test directory");
    if (temp.isValid()) {
        const QString source = temp.filePath("source.mp4");
        bool fixture = command("ffmpeg", {"-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i", "testsrc2=size=320x180:rate=25",
                "-f", "lavfi", "-i", "sine=frequency=480:sample_rate=48000", "-t", "3",
                "-c:v", "mpeg4", "-q:v", "5", "-c:a", "aac", "-shortest", "-y", source});
        check(fixture, "generate a real three-second test video");
        if (fixture) {
            MediaInfo info = MediaEngine::probe(source);
            check(info.valid && info.hasAudio && info.width == 320 && info.height == 180 && qAbs(info.durationMs - 3000) < 200, "probe real media");
            ExportOptions job;
            job.input = source; job.output = temp.filePath("trim.mp4");
            job.fullDurationMs = info.durationMs; job.hasAudio = true;
            job.videoCodec = "MPEG-4"; job.clips = {{500, 1900}};
            auto args = MediaEngine::makeExport(job);
            check(args.valid() && command("ffmpeg", args.args), "single segment export with real FFmpeg");
            MediaInfo trimmed = MediaEngine::probe(job.output);
            check(trimmed.valid && qAbs(trimmed.durationMs - 1400) < 260, "trim duration approximately correct");
            job.output = temp.filePath("concat.mp4");
            job.clips = {{0, 650}, {1450, 2150}};
            args = MediaEngine::makeExport(job);
            check(args.valid() && command("ffmpeg", args.args), "join two nonadjacent clips with audio");
            MediaInfo assembled = MediaEngine::probe(job.output);
            check(assembled.valid && assembled.hasAudio && qAbs(assembled.durationMs - 1350) < 280,
                "assembled result has expected approximate duration and audio");
            job.output = temp.filePath("audio_adjusted.mp4");
            job.audioGainDb = -3;
            job.audioFadeInMs = 200;
            job.audioFadeOutMs = 200;
            args = MediaEngine::makeExport(job);
            check(args.valid() && command("ffmpeg", args.args), "render joined clips with audio gain and fades");
            MediaInfo adjusted = MediaEngine::probe(job.output);
            check(adjusted.valid && adjusted.hasAudio, "adjusted export has usable audio and video");
            job.output = temp.filePath("remove_middle.mp4");
            job.clips = {{0, 400}, {2100, 2800}};
            job.audioGainDb = 0; job.audioFadeInMs = 0; job.audioFadeOutMs = 0;
            args = MediaEngine::makeExport(job);
            check(args.valid() && command("ffmpeg", args.args), "export video with middle selection excluded");
            MediaInfo withoutMiddle = MediaEngine::probe(job.output);
            check(withoutMiddle.valid && qAbs(withoutMiddle.durationMs - 1100) < 280,
                  "excluded section has expected shortened duration");
        }
    }
    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : "FAILED TESTS: " + std::to_string(failures)) << '\n';
    return failures ? 1 : 0;
}
