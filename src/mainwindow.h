#pragma once

#include "mediaengine.h"
#include <QMainWindow>
#include <QElapsedTimer>
#include <QVector>
#include <QByteArray>

class QAudioOutput;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QMediaPlayer;
class QPlainTextEdit;
class QProcess;
class QProgressBar;
class QDoubleSpinBox;
class QPushButton;
class QSlider;
class QSpinBox;
class QStackedWidget;
class QVideoSink;
class QTimer;
class VideoSurface;
class TimelineWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    bool openMedia(const QString &path);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    void buildUi();
    QWidget *makeInspector();
    QWidget *makeEditor();
    QWidget *makeExporter();
    void setBusy(bool busy);
    void startPreviewWatch();
    void startFallback(qint64 ms, bool play);
    void stopFallback();
    void requestFallbackFrame(qint64 ms);
    void playPreview();
    void pausePreview();
    void togglePreview();
    qint64 previewPosition() const;
    void updateMediaInfo();
    void updateTiming();
    void updateClips();
    void applySettings(const QJsonObject &json);
    QJsonObject projectJson() const;
    void chooseOpen();
    void chooseOutput();
    void saveProject();
    void loadProject();
    void openProjectPath(const QString &path);
    void seek(qint64 ms);
    void setMarkIn();
    void setMarkOut();
    void addClip();
    void removeClip();
    void moveClip(int step);
    void duplicateClip();
    void splitClip();
    void clearClips();
    void undoClips();
    void redoClips();
    void rememberClips();
    void previewFilters();
    void jumpToTime();
    void showOriginal();
    void resetFilters();
    void takeSnapshot();
    void exportVideo();
    void cancelExport();
    void handleProgress();
    void exportFinished(int exitCode, bool normal);
    void setStatus(const QString &message);
    ExportOptions options() const;

    MediaInfo m_info;
    QVector<Clip> m_clips;
    QVector<QVector<Clip>> m_undoClips;
    QVector<QVector<Clip>> m_redoClips;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audio = nullptr;
    VideoSurface *m_video = nullptr;
    QVideoSink *m_videoSink = nullptr;
    QProcess *m_fallback = nullptr;
    QProcess *m_stillFrame = nullptr;
    QTimer *m_fallbackTicker = nullptr;
    QElapsedTimer m_fallbackElapsed;
    QByteArray m_fallbackBytes;
    QByteArray m_stillBytes;
    qint64 m_fallbackBase = 0;
    qint64 m_fallbackPosition = 0;
    int m_previewGeneration = 0;
    bool m_nativeFrameSeen = false;
    bool m_fallbackMode = false;
    bool m_fallbackPlaying = false;
    bool m_pauseAfterFirstFrame = false;
    QStackedWidget *m_preview = nullptr;
    TimelineWidget *m_timeline = nullptr;
    QProcess *m_export = nullptr;
    QProcess *m_filterProcess = nullptr;
    QString m_stdoutBuffer;
    QString m_log;
    QString m_lastOutput;
    QString m_stagingOutput;
    qint64 m_expected = 0;
    qint64 m_in = 0;
    qint64 m_out = 0;
    bool m_cancelled = false;
    bool m_busy = false;

    QLabel *m_title = nullptr;
    QLabel *m_filterImage = nullptr;
    QLabel *m_fileInfo = nullptr;
    QLabel *m_previewBackendLabel = nullptr;
    QLabel *m_position = nullptr;
    QLabel *m_duration = nullptr;
    QLabel *m_inLabel = nullptr;
    QLabel *m_outLabel = nullptr;
    QLabel *m_clipTime = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_processState = nullptr;
    QListWidget *m_clipList = nullptr;
    QPushButton *m_play = nullptr;
    QPushButton *m_exportButton = nullptr;
    QPushButton *m_openFolder = nullptr;
    QPushButton *m_filterPreviewButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QProgressBar *m_progress = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    QLineEdit *m_outputPath = nullptr;
    QComboBox *m_exportMode = nullptr;
    QComboBox *m_videoCodec = nullptr;
    QComboBox *m_audioCodec = nullptr;
    QComboBox *m_container = nullptr;
    QComboBox *m_preset = nullptr;
    QComboBox *m_resolution = nullptr;
    QSpinBox *m_crf = nullptr;
    QSpinBox *m_audioGain = nullptr;
    QDoubleSpinBox *m_fadeIn = nullptr;
    QDoubleSpinBox *m_fadeOut = nullptr;
    QSpinBox *m_brightness = nullptr;
    QSpinBox *m_contrast = nullptr;
    QSpinBox *m_saturation = nullptr;
    QSpinBox *m_cropL = nullptr;
    QSpinBox *m_cropR = nullptr;
    QSpinBox *m_cropT = nullptr;
    QSpinBox *m_cropB = nullptr;
    QCheckBox *m_grayscale = nullptr;
    QCheckBox *m_flipH = nullptr;
    QCheckBox *m_flipV = nullptr;
    QCheckBox *m_denoise = nullptr;
    QCheckBox *m_sharpen = nullptr;
    QComboBox *m_rotation = nullptr;
};
