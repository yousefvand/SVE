#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct Clip {
    qint64 inMs = 0;
    qint64 outMs = 0;
};

struct MediaInfo {
    bool valid = false;
    QString path;
    QString videoCodec;
    QString audioCodec;
    QString format;
    int width = 0;
    int height = 0;
    double fps = 25.0;
    qint64 durationMs = 0;
    qint64 bytes = 0;
    bool hasVideo = false;
    bool hasAudio = false;
    QString error;
};

struct FilterOptions {
    bool grayscale = false;
    bool flipH = false;
    bool flipV = false;
    bool denoise = false;
    bool sharpen = false;
    int rotate = 0; // 0, 90, 180, 270 clockwise
    int brightness = 0; // -100..100
    int contrast = 100; // 0..200
    int saturation = 100; // 0..200
    int cropLeft = 0;
    int cropRight = 0;
    int cropTop = 0;
    int cropBottom = 0;
    int height = 0; // 0 means original

    bool active() const;
    QStringList graphFilters() const;
    QJsonObject toJson() const;
    static FilterOptions fromJson(const QJsonObject &obj);
};

struct ExportOptions {
    QString input;
    QString output;
    QString videoCodec = "H.264";
    QString audioCodec = "AAC";
    QString container = "mp4";
    QString speed = "medium";
    int crf = 22;
    double fps = 25.0;
    QVector<Clip> clips;
    FilterOptions filters;
    bool hasAudio = false;
    int audioGainDb = 0; // -20..20 dB
    int audioFadeInMs = 0;
    int audioFadeOutMs = 0;
    bool hasVideo = true;
    qint64 fullDurationMs = 0;
};

struct ExportCommand {
    QStringList args;
    qint64 expectedMs = 0;
    QString error;
    QString warning;
    bool valid() const { return error.isEmpty() && !args.isEmpty(); }
};

namespace MediaEngine {
MediaInfo probe(const QString &path);
ExportCommand makeExport(const ExportOptions &options);
QString timecode(qint64 ms);
QString ffTime(qint64 ms);
qint64 clipsDuration(const QVector<Clip> &clips);
}
