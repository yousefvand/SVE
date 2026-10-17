#include "mediaengine.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QtMath>
#include <algorithm>

namespace {
QString realNumber(double x) { return QString::number(x, 'f', 3); }
QString filterChain(const FilterOptions &f) { return f.graphFilters().join(','); }
QStringList audioFilters(const ExportOptions &o, qint64 durationMs) {
    QStringList filters;
    if (o.audioGainDb != 0) filters << QString("volume=%1dB").arg(o.audioGainDb);
    if (o.audioFadeInMs > 0)
        filters << QString("afade=t=in:st=0:d=%1").arg(MediaEngine::ffTime(o.audioFadeInMs));
    if (o.audioFadeOutMs > 0)
        filters << QString("afade=t=out:st=%1:d=%2")
            .arg(MediaEngine::ffTime(qMax(qint64(0), durationMs - o.audioFadeOutMs)))
            .arg(MediaEngine::ffTime(o.audioFadeOutMs));
    return filters;
}
}

bool FilterOptions::active() const {
    return grayscale || flipH || flipV || denoise || sharpen || rotate != 0 ||
           brightness != 0 || contrast != 100 || saturation != 100 ||
           cropLeft || cropRight || cropTop || cropBottom || height;
}

QStringList FilterOptions::graphFilters() const {
    QStringList result;
    if (cropLeft || cropRight || cropTop || cropBottom)
        result << QString("crop=iw-%1:ih-%2:%3:%4")
                      .arg(cropLeft + cropRight).arg(cropTop + cropBottom).arg(cropLeft).arg(cropTop);
    if (flipH) result << "hflip";
    if (flipV) result << "vflip";
    if (rotate == 90) result << "transpose=clock";
    else if (rotate == 180) result << "hflip" << "vflip";
    else if (rotate == 270) result << "transpose=cclock";
    if (grayscale) result << "hue=s=0";
    if (denoise) result << "hqdn3d=3:3:6:6";
    if (sharpen) result << "unsharp=5:5:0.8:3:3:0.0";
    if (brightness || contrast != 100 || saturation != 100)
        result << QString("eq=brightness=%1:contrast=%2:saturation=%3")
                      .arg(realNumber(brightness / 200.0))
                      .arg(realNumber(contrast / 100.0))
                      .arg(realNumber(saturation / 100.0));
    if (height) result << QString("scale=-2:%1:flags=lanczos").arg(height);
    result << "scale=trunc(iw/2)*2:trunc(ih/2)*2" << "format=yuv420p";
    return result;
}

QJsonObject FilterOptions::toJson() const {
    return {{"grayscale", grayscale}, {"flipH", flipH}, {"flipV", flipV},
            {"denoise", denoise}, {"sharpen", sharpen}, {"rotate", rotate},
            {"brightness", brightness}, {"contrast", contrast}, {"saturation", saturation},
            {"cropLeft", cropLeft}, {"cropRight", cropRight},
            {"cropTop", cropTop}, {"cropBottom", cropBottom}, {"height", height}};
}
FilterOptions FilterOptions::fromJson(const QJsonObject &o) {
    FilterOptions f;
    f.grayscale = o.value("grayscale").toBool(); f.flipH = o.value("flipH").toBool();
    f.flipV = o.value("flipV").toBool(); f.denoise = o.value("denoise").toBool();
    f.sharpen = o.value("sharpen").toBool(); f.rotate = o.value("rotate").toInt();
    f.brightness = o.value("brightness").toInt(); f.contrast = o.value("contrast").toInt(100);
    f.saturation = o.value("saturation").toInt(100);
    f.cropLeft = o.value("cropLeft").toInt(); f.cropRight = o.value("cropRight").toInt();
    f.cropTop = o.value("cropTop").toInt(); f.cropBottom = o.value("cropBottom").toInt();
    f.height = o.value("height").toInt();
    return f;
}

QString MediaEngine::timecode(qint64 ms) {
    ms = qMax(qint64(0), ms);
    const qint64 h = ms / 3600000;
    const qint64 m = ms / 60000 % 60;
    const qint64 s = ms / 1000 % 60;
    return QString("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(ms % 1000, 3, 10, QLatin1Char('0'));
}
QString MediaEngine::ffTime(qint64 ms) { return QString::number(qMax(qint64(0), ms) / 1000.0, 'f', 3); }
qint64 MediaEngine::clipsDuration(const QVector<Clip> &clips) {
    qint64 total = 0;
    for (const Clip &clip : clips) total += qMax(qint64(0), clip.outMs - clip.inMs);
    return total;
}

MediaInfo MediaEngine::probe(const QString &path) {
    MediaInfo result;
    result.path = path;
    QFileInfo file(path);
    if (!file.isFile()) { result.error = "The media file does not exist."; return result; }
    QProcess ffprobe;
    ffprobe.start("ffprobe", {"-v", "error", "-show_format", "-show_streams", "-of", "json", path});
    if (!ffprobe.waitForStarted(5000)) { result.error = "Cannot start ffprobe. Install FFmpeg."; return result; }
    if (!ffprobe.waitForFinished(20000)) {
        ffprobe.kill(); ffprobe.waitForFinished();
        result.error = "ffprobe timed out while analyzing this file."; return result;
    }
    if (ffprobe.exitCode() != 0) {
        result.error = QString::fromUtf8(ffprobe.readAllStandardError()).trimmed();
        if (result.error.isEmpty()) result.error = "ffprobe could not read this media.";
        return result;
    }
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(ffprobe.readAllStandardOutput(), &error);
    if (error.error != QJsonParseError::NoError || !json.isObject()) {
        result.error = "Invalid media metadata returned by ffprobe."; return result;
    }
    QJsonObject doc = json.object();
    auto streams = doc.value("streams").toArray();
    double longestStreamDuration = 0;
    for (const QJsonValue &item : streams) {
        QJsonObject s = item.toObject();
        const QString type = s.value("codec_type").toString();
        const QString codec = s.value("codec_name").toString("unknown");
        longestStreamDuration = qMax(longestStreamDuration, s.value("duration").toString().toDouble());
        if (type == "video" && !result.hasVideo && !s.value("disposition").toObject().value("attached_pic").toInt()) {
            result.hasVideo = true;
            result.videoCodec = codec;
            result.width = s.value("width").toInt();
            result.height = s.value("height").toInt();
            QString rate = s.value("avg_frame_rate").toString();
            if (rate.isEmpty() || rate == "0/0") rate = s.value("r_frame_rate").toString();
            QStringList parts = rate.split('/');
            double fps = parts.first().toDouble();
            if (parts.size() == 2 && parts.last().toDouble() != 0) fps /= parts.last().toDouble();
            if (fps > 0 && fps < 1000) result.fps = fps;
        }
        if (type == "audio" && !result.hasAudio) { result.hasAudio = true; result.audioCodec = codec; }
    }
    QJsonObject format = doc.value("format").toObject();
    result.durationMs = qRound64(qMax(longestStreamDuration, format.value("duration").toString().toDouble()) * 1000.0);
    result.format = format.value("format_name").toString();
    result.bytes = file.size();
    result.valid = result.hasVideo && result.durationMs > 0;
    if (!result.valid) result.error = "SVE needs a video track and a known duration.";
    return result;
}

ExportCommand MediaEngine::makeExport(const ExportOptions &o) {
    ExportCommand c;
    if (o.input.isEmpty() || o.output.isEmpty()) { c.error = "Choose an input and output path."; return c; }
    if (QFileInfo(o.input).absoluteFilePath() == QFileInfo(o.output).absoluteFilePath()) {
        c.error = "Output cannot overwrite the source video."; return c;
    }
    if (!o.hasVideo) { c.error = "The source has no video stream."; return c; }
    QVector<Clip> clips = o.clips;
    if (clips.isEmpty()) clips.push_back({0, o.fullDurationMs});
    if (o.fullDurationMs <= 0) { c.error = "Media duration is unknown."; return c; }
    for (const Clip &clip : clips) {
        if (clip.inMs < 0 || clip.outMs > o.fullDurationMs || clip.outMs <= clip.inMs) {
            c.error = "Every clip needs a valid start and end time."; return c;
        }
    }
    if (o.filters.cropLeft + o.filters.cropRight >= 100000 || o.filters.cropTop + o.filters.cropBottom >= 100000) {
        c.error = "Crop settings are too large."; return c;
    }
    if (o.videoCodec == "Copy" && (o.filters.active() || clips.size() > 1)) {
        c.error = "Stream copy cannot apply filters or join clips. Select an encoder."; return c;
    }
    if (clips.size() > 1 && o.audioCodec == "Copy" && o.hasAudio) {
        c.error = "Audio stream copy cannot join clips in this mode. Choose AAC, Opus, MP3, or None."; return c;
    }
    const bool hasAudio = o.hasAudio && o.audioCodec != "None";
    if (hasAudio && (o.audioFadeInMs < 0 || o.audioFadeOutMs < 0 ||
                     o.audioGainDb < -20 || o.audioGainDb > 20)) {
        c.error = "Invalid audio adjustment settings."; return c;
    }
    if (hasAudio && o.audioCodec == "Copy" && (o.audioGainDb || o.audioFadeInMs || o.audioFadeOutMs)) {
        c.error = "Audio copy cannot apply gain or fades. Choose AAC, Opus, or MP3."; return c;
    }
    if (o.container == "webm" && !(o.videoCodec == "VP9" || o.videoCodec == "AV1" || o.videoCodec == "Copy")) {
        c.error = "WebM requires VP9, AV1, or an already-compatible copied video stream."; return c;
    }
    if (o.container == "webm" && hasAudio && o.audioCodec != "Opus" && o.audioCodec != "Copy") {
        c.error = "WebM audio should be Opus or a compatible copied audio track."; return c;
    }
    c.expectedMs = clipsDuration(clips);
    if (hasAudio && (o.audioFadeInMs > c.expectedMs || o.audioFadeOutMs > c.expectedMs)) {
        c.error = "Audio fades cannot be longer than the exported video."; return c;
    }
    const QStringList afilters = hasAudio ? audioFilters(o, c.expectedMs) : QStringList{};
    c.args = {"-hide_banner", "-nostdin", "-y", "-loglevel", "error", "-stats_period", "0.2", "-progress", "pipe:1"};
    if (clips.size() == 1) {
        const auto &clip = clips.front();
        if (o.videoCodec == "Copy") {
            // Input seeking allows fast packet remuxing; edit points must be keyframe-aligned.
            c.args << "-ss" << ffTime(clip.inMs);
            c.warning = "Stream copy is fast but cuts may be aligned to nearby keyframes. Choose H.264 for frame-accurate trimming.";
        }
        c.args << "-i" << o.input;
        if (o.videoCodec != "Copy") c.args << "-ss" << ffTime(clip.inMs);
        c.args << "-t" << ffTime(clip.outMs - clip.inMs);
        c.args << "-map" << "0:v:0";
        if (hasAudio) c.args << "-map" << "0:a:0?";
        if (o.videoCodec != "Copy" && o.filters.active()) c.args << "-vf" << filterChain(o.filters);
        if (!afilters.isEmpty()) c.args << "-af" << afilters.join(',');
    } else {
        c.args << "-i" << o.input;
        QStringList parts;
        for (int i = 0; i < clips.size(); ++i) {
            const Clip &clip = clips.at(i);
            const QString pre = QString("[%1:v:0]trim=start=%2:end=%3,setpts=PTS-STARTPTS")
                .arg(0).arg(ffTime(clip.inMs)).arg(ffTime(clip.outMs));
            QString f = pre;
            if (o.filters.active()) f += "," + filterChain(o.filters);
            f += QString("[v%1]").arg(i);
            parts << f;
            if (hasAudio) {
                parts << QString("[0:a:0]atrim=start=%1:end=%2,asetpts=PTS-STARTPTS[a%3]")
                             .arg(ffTime(clip.inMs)).arg(ffTime(clip.outMs)).arg(i);
            }
        }
        QString inputs;
        for (int i = 0; i < clips.size(); ++i) {
            inputs += QString("[v%1]").arg(i);
            if (hasAudio) inputs += QString("[a%1]").arg(i);
        }
        const QString audioIntermediate = afilters.isEmpty() ? "[aout]" : "[acat]";
        parts << QString("%1concat=n=%2:v=1:a=%3%4")
                     .arg(inputs).arg(clips.size()).arg(hasAudio ? 1 : 0)
                     .arg(hasAudio ? "[vout]" + audioIntermediate : "[vout]");
        if (hasAudio && !afilters.isEmpty())
            parts << "[acat]" + afilters.join(',') + "[aout]";
        c.args << "-filter_complex" << parts.join(';') << "-map" << "[vout]";
        if (hasAudio) c.args << "-map" << "[aout]";
        // concat emits a 1/1000000 timebase; force a usable timebase for MPEG-4 and other encoders.
        c.args << "-r" << QString::number(qBound(1.0, o.fps, 240.0), 'f', 3);
    }
    if (o.videoCodec == "Copy") c.args << "-c:v" << "copy";
    else {
        QString encoder;
        if (o.videoCodec == "H.264") encoder = "libx264";
        else if (o.videoCodec == "H.265") encoder = "libx265";
        else if (o.videoCodec == "VP9") encoder = "libvpx-vp9";
        else if (o.videoCodec == "AV1") encoder = "libsvtav1";
        else if (o.videoCodec == "MPEG-4") encoder = "mpeg4";
        else { c.error = "Unknown video encoder."; return c; }
        c.args << "-c:v" << encoder;
        if (o.videoCodec == "MPEG-4") c.args << "-q:v" << QString::number(qBound(1, o.crf / 3, 31));
        else c.args << "-crf" << QString::number(o.crf);
        if (o.videoCodec == "H.264" || o.videoCodec == "H.265") c.args << "-preset" << o.speed;
        else if (o.videoCodec == "VP9") c.args << "-b:v" << "0" << "-deadline" << "good" << "-cpu-used" << "2";
        else if (o.videoCodec == "AV1") c.args << "-preset" << "6";
        c.args << "-pix_fmt" << "yuv420p";
    }
    if (!hasAudio) c.args << "-an";
    else if (o.audioCodec == "Copy") c.args << "-c:a" << "copy";
    else if (o.audioCodec == "AAC") c.args << "-c:a" << "aac" << "-b:a" << "192k";
    else if (o.audioCodec == "Opus") c.args << "-c:a" << "libopus" << "-b:a" << "160k";
    else if (o.audioCodec == "MP3") c.args << "-c:a" << "libmp3lame" << "-b:a" << "192k";
    else { c.error = "Unknown audio encoder."; return c; }
    if (o.container == "mp4" || o.container == "mov") c.args << "-movflags" << "+faststart";
    c.args << "-f" << (o.container == "mkv" ? "matroska" : o.container);
    c.args << o.output;
    return c;
}
