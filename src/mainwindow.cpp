#include "mainwindow.h"
#include "timelinewidget.h"
#include "videosurface.h"

#include <QApplication>
#include <QAction>
#include <QAudioOutput>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QPixmap>
#include <QSaveFile>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMediaPlayer>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStandardPaths>
#include <QUuid>
#include <QTimer>
#include <QUrl>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QVBoxLayout>
#include <cstdio>
#include <algorithm>

namespace {
const QString baseStyle = R"CSS(
QMainWindow, QWidget#appSurface { background: #101720; color: #e5edf4; }
QWidget { color: #d9e3ed; font-size: 12px; }
QLabel { background: transparent; }
QLabel#appName { color: #f6fafc; font-size: 23px; font-weight: 800; }
QLabel#sectionTitle { color: #f1f6fa; font-size: 14px; font-weight: 700; }
QLabel#secondary { color: #91a1b4; }
QLabel#largeTime { color: #f6fafb; font-size: 16px; font-weight: 650; font-family: "Noto Sans Mono",monospace; }
QWidget#topBar { background: #17202b; border-bottom: 1px solid #2d3745; }
QWidget#editorPanel { background: #111a24; }
QScrollArea, QScrollArea > QWidget > QWidget { background: #151e29; border: none; }
QWidget#sidePanel { background: #151e29; }
QGroupBox { background: #1b2632; border: 1px solid #303f4f; border-radius: 11px; margin-top: 16px; padding: 12px 10px 10px; font-weight: 700; color: #e4edf5; }
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 12px; padding: 0 6px; color: #aabacc; }
QPushButton { background: #273442; border: 1px solid #3a4a5a; border-radius: 8px; padding: 8px 12px; color: #e6eef5; font-weight: 600; }
QPushButton:hover { background: #334556; border-color: #577186; }
QPushButton:pressed { background: #233240; }
QPushButton:disabled { color: #718194; background: #212c36; border-color: #2c3a48; }
QPushButton#accentButton { background: #20c8ae; color: #092b31; border: 1px solid #29e0c3; font-weight: 800; }
QPushButton#accentButton:hover { background: #4ce7d0; }
QPushButton#subtleButton { background: transparent; border-color: #344456; }
QPushButton#aboutButton { background: #1d2b38; border-color: #344c5f; padding: 8px 9px; }
QPushButton#aboutButton:hover { background: #2d4355; border-color: #508296; }
QLineEdit, QComboBox, QSpinBox { background: #111c27; color: #ecf4fa; selection-background-color: #248978; border: 1px solid #3b4d5f; border-radius: 7px; padding: 7px 9px; min-height: 22px; }
QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border: 1px solid #29cdb3; }
QComboBox::drop-down { border: 0; width: 24px; }
QComboBox QAbstractItemView { background: #202d3b; selection-background-color: #236e68; border: 1px solid #4c6273; }
QCheckBox { spacing: 8px; padding: 3px 0; }
QCheckBox::indicator { width: 17px; height: 17px; }
QListWidget, QPlainTextEdit { background: #121e2a; border: 1px solid #344454; border-radius: 8px; selection-background-color: #276f6c; padding: 5px; }
QListWidget::item { padding: 9px 5px; border-bottom: 1px solid #293746; }
QProgressBar { background: #243340; border: 0; border-radius: 5px; text-align: center; min-height: 10px; color: #f3f5f7; }
QProgressBar::chunk { background: #24ccb1; border-radius: 5px; }
QSlider::groove:horizontal { background: #334554; height: 5px; border-radius: 2px; }
QSlider::handle:horizontal { background: #27dbc0; width: 15px; margin: -5px 0; border-radius: 7px; }
QSplitter::handle { background: #2a3744; width: 2px; }
QStatusBar { background: #131d28; border-top: 1px solid #2f4050; color: #9faec0; }
QToolTip { background: #263645; color: #eff6f8; border: 1px solid #4b6374; }
QMessageBox { background: #1a2633; }
QMessageBox QLabel { color: #f0f5fa; background: transparent; font-size: 13px; min-width: 160px; }
QMessageBox QPushButton { min-width: 72px; color: #e6eef5; }
QDialog { background: #1a2633; }
QDialog QLabel { color: #e6eef5; }
QDoubleSpinBox { background: #111c27; color: #ecf4fa; border: 1px solid #3b4d5f; border-radius: 7px; padding: 7px 9px; min-height: 22px; }
)CSS";

QLabel *secondary(const QString &text = {}) {
    auto *label = new QLabel(text);
    label->setObjectName("secondary");
    label->setWordWrap(true);
    return label;
}
QPushButton *makeButton(const QString &text, const QString &toolTip = {}, bool accent = false) {
    auto *b = new QPushButton(text);
    if (accent) b->setObjectName("accentButton");
    if (!toolTip.isEmpty()) b->setToolTip(toolTip);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}
QSpinBox *makeSpin(int min, int max, int value = 0, const QString &suffix = {}) {
    auto *s = new QSpinBox;
    s->setRange(min, max); s->setValue(value); s->setSuffix(suffix);
    return s;
}
QComboBox *makeCombo(const QStringList &values) {
    auto *combo = new QComboBox;
    combo->addItems(values);
    return combo;
}
QWidget *horizontalLine() {
    auto *line = new QFrame;
    line->setFixedHeight(1);
    line->setStyleSheet("background:#334252;");
    return line;
}
QString friendlySize(qint64 bytes) {
    double size = bytes;
    const QStringList units = {"B", "KiB", "MiB", "GiB"};
    int i = 0;
    while (size >= 1024 && i < units.size() - 1) { size /= 1024; ++i; }
    return QString::number(size, 'f', i ? 1 : 0) + " " + units.at(i);
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_player = new QMediaPlayer(this);
    m_audio = new QAudioOutput(this);
    m_player->setAudioOutput(m_audio);
    m_audio->setVolume(0.7);
    m_videoSink = new QVideoSink(this);
    m_player->setVideoSink(m_videoSink);
    m_fallback = new QProcess(this);
    m_stillFrame = new QProcess(this);
    m_fallbackTicker = new QTimer(this);
    m_fallbackTicker->setInterval(100);
    m_export = new QProcess(this);
    m_filterProcess = new QProcess(this);
    buildUi();
    setAcceptDrops(true);
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64) {
        if (!m_fallbackMode) updateTiming();
    });
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (!m_info.valid || m_fallbackMode || !frame.isValid()) return;
        const QImage image = frame.toImage();
        if (image.isNull()) return;
        m_nativeFrameSeen = true;
        m_video->setFrame(image);
        m_previewBackendLabel->setText("QT / FFMPEG");
        if (m_pauseAfterFirstFrame) {
            m_pauseAfterFirstFrame = false;
            m_player->pause();
        }
    });
    connect(m_fallback, &QProcess::readyReadStandardOutput, this, [this] {
        // FFmpeg supplies 15 fps RGB24 frames at 960x540. Consume complete
        // frames only and copy bytes before the process buffer is reused.
        constexpr qsizetype frameBytes = 960 * 540 * 3;
        m_fallbackBytes.append(m_fallback->readAllStandardOutput());
        while (m_fallbackBytes.size() >= frameBytes) {
            QImage image(reinterpret_cast<const uchar *>(m_fallbackBytes.constData()),
                         960, 540, 960 * 3, QImage::Format_RGB888);
            m_video->setFrame(image.copy());
            m_fallbackBytes.remove(0, frameBytes);
        }
    });
    connect(m_fallback, &QProcess::readyReadStandardError, this, [this] {
        const QString err = QString::fromUtf8(m_fallback->readAllStandardError()).trimmed();
        if (!err.isEmpty()) setStatus("FFmpeg preview: " + err.right(280));
    });
    connect(m_fallback, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int, QProcess::ExitStatus) {
        if (m_fallbackMode && m_fallbackPlaying) {
            m_fallbackPlaying = false;
            m_fallbackTicker->stop();
            m_play->setText("▶  Play");
        }
    });
    connect(m_stillFrame, &QProcess::readyReadStandardOutput, this, [this] {
        m_stillBytes.append(m_stillFrame->readAllStandardOutput());
    });
    connect(m_stillFrame, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
        m_stillBytes.append(m_stillFrame->readAllStandardOutput());
        QImage image;
        if (code == 0 && status == QProcess::NormalExit && image.loadFromData(m_stillBytes, "PNG")
            && !m_nativeFrameSeen && !m_fallbackPlaying && m_info.valid)
            m_video->setFrame(image);
        m_stillBytes.clear();
    });
    connect(m_fallbackTicker, &QTimer::timeout, this, [this] {
        if (!m_fallbackPlaying) return;
        m_fallbackPosition = qMin(m_info.durationMs, m_fallbackBase + m_fallbackElapsed.elapsed());
        updateTiming();
        if (m_fallbackPosition >= m_info.durationMs) pausePreview();
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (!m_fallbackMode)
            m_play->setText(state == QMediaPlayer::PlayingState ? "Ⅱ  Pause" : "▶  Play");
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &message) {
        if (m_info.valid && !m_fallbackMode) {
            setStatus("Qt preview unavailable: " + message + ". Switching to FFmpeg video preview.");
            startFallback(previewPosition(), false);
        }
    });
    connect(m_filterProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
        m_filterPreviewButton->setEnabled(true);
        const QByteArray bytes = m_filterProcess->readAllStandardOutput();
        QPixmap pixmap;
        if (code != 0 || status != QProcess::NormalExit || !pixmap.loadFromData(bytes, "PNG")) {
            if (m_preview->currentIndex() == 2) showOriginal();
            const QString error = QString::fromUtf8(m_filterProcess->readAllStandardError()).trimmed();
            QMessageBox::warning(this, "Filter preview failed", error.isEmpty() ? "FFmpeg could not render this preview." : error.right(900));
        } else {
            m_filterImage->setPixmap(pixmap.scaled(m_filterImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            setStatus("Filtered frame preview ready. Select Source view to return to playback.");
        }
    });
    connect(m_filterProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            m_filterPreviewButton->setEnabled(true);
            showOriginal();
            QMessageBox::warning(this, "Filter preview failed", "Cannot start FFmpeg.");
        }
    });
    connect(m_export, &QProcess::readyReadStandardOutput, this, &MainWindow::handleProgress);
    connect(m_export, &QProcess::readyReadStandardError, this, [this] {
        const QString chunk = QString::fromUtf8(m_export->readAllStandardError());
        m_log += chunk;
        if (m_log.size() > 20000) m_log = m_log.right(20000);
        m_logView->setPlainText(m_log);
        auto *bar = m_logView->verticalScrollBar();
        if (bar) bar->setValue(bar->maximum());
    });
    connect(m_export, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) { exportFinished(code, status == QProcess::NormalExit); });
    connect(m_export, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_log += "\nFailed to start ffmpeg. Is it installed?";
            m_logView->setPlainText(m_log);
        }
    });

    auto shortcut = [this](const QKeySequence &seq, auto func) {
        auto *s = new QShortcut(seq, this);
        connect(s, &QShortcut::activated, this, func);
    };
    shortcut(QKeySequence("Ctrl+O"), [this] { chooseOpen(); });
    shortcut(QKeySequence("Ctrl+S"), [this] { saveProject(); });
    shortcut(QKeySequence("Ctrl+Shift+O"), [this] { loadProject(); });
    shortcut(QKeySequence("Ctrl+E"), [this] { exportVideo(); });
    shortcut(QKeySequence("Ctrl+Z"), [this] { undoClips(); });
    shortcut(QKeySequence("Ctrl+Shift+Z"), [this] { redoClips(); });
    shortcut(QKeySequence("Delete"), [this] { if (m_clipList->hasFocus()) removeClip(); });
    shortcut(QKeySequence("Ctrl+G"), [this] { jumpToTime(); });
    shortcut(QKeySequence("PageDown"), [this] { seek(previewPosition() + 5000); });
    shortcut(QKeySequence("PageUp"), [this] { seek(previewPosition() - 5000); });
    shortcut(QKeySequence(Qt::Key_Space), [this] {
        if (!m_info.valid) return;
        togglePreview();
    });
    shortcut(QKeySequence("I"), [this] { setMarkIn(); });
    shortcut(QKeySequence("O"), [this] { setMarkOut(); });
    shortcut(QKeySequence("Left"), [this] { pausePreview(); seek(previewPosition() - qRound64(1000.0 / m_info.fps)); });
    shortcut(QKeySequence("Right"), [this] { pausePreview(); seek(previewPosition() + qRound64(1000.0 / m_info.fps)); });
    QSettings settings;
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    if (!isMaximized() && size().width() < 900) resize(1480, 900);
    if (QStandardPaths::findExecutable("ffmpeg").isEmpty() || QStandardPaths::findExecutable("ffprobe").isEmpty())
        QTimer::singleShot(0, this, [this] { QMessageBox::warning(this, "Missing FFmpeg", "SVE requires ffmpeg and ffprobe. Install the 'ffmpeg' package."); });
}
MainWindow::~MainWindow() {
    stopFallback();
    if (m_stillFrame->state() != QProcess::NotRunning) {
        m_stillFrame->kill(); m_stillFrame->waitForFinished(400);
    }
    if (m_filterProcess->state() != QProcess::NotRunning) {
        m_filterProcess->kill(); m_filterProcess->waitForFinished(400);
    }
    if (m_export->state() != QProcess::NotRunning) { m_export->kill(); m_export->waitForFinished(2000); }
}

void MainWindow::buildUi() {
    setWindowTitle("SVE  ·  Video Editor");
    setMinimumSize(1050, 700);
    resize(1480, 900);
    setStyleSheet(baseStyle);
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("Open video…  Ctrl+O", this, &MainWindow::chooseOpen);
    fileMenu->addAction("Open project…  Ctrl+Shift+O", this, &MainWindow::loadProject);
    fileMenu->addAction("Save project…  Ctrl+S", this, &MainWindow::saveProject);
    fileMenu->addSeparator();
    fileMenu->addAction("Export video  Ctrl+E", this, &MainWindow::exportVideo);
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", this, &QWidget::close);
    auto *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Undo clip edit  Ctrl+Z", this, &MainWindow::undoClips);
    editMenu->addAction("Redo clip edit  Ctrl+Shift+Z", this, &MainWindow::redoClips);
    editMenu->addAction("Go to timecode  Ctrl+G", this, &MainWindow::jumpToTime);
    editMenu->addAction("Reset video filters", this, &MainWindow::resetFilters);
    auto *playbackMenu = menuBar()->addMenu("&Playback");
    playbackMenu->addAction("Use FFmpeg preview (video only)", this, [this] {
        if (m_info.valid) {
            setStatus("Using FFmpeg video-only preview. Reopen the file to retry Qt audio/video.");
            startFallback(previewPosition(), false);
        }
    });
    auto *helpMenu = menuBar()->addMenu("&Help");
    auto *aboutSveAction = helpMenu->addAction("About SVE", this, [this] {
        // A simple, icon-free dialog leaves the entire content area for text.
        // Align its text to the left, including the credits and project URL.
        QDialog about(this);
        about.setWindowTitle("About SVE");
        about.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
        about.setMinimumWidth(440);

        auto *layout = new QVBoxLayout(&about);
        layout->setContentsMargins(12, 16, 12, 12);
        layout->setSpacing(10);

        auto *details = new QLabel(&about);
        details->setObjectName("aboutSveText");
        details->setTextFormat(Qt::RichText);
        details->setText(QStringLiteral("<h3>SVE %1</h3>"
            "<p>Simple Video Editor</p>"
            "<p>Remisa Phillips</p>"
            "<p>Project home page:<br/>"
            "<a href=\"https://github.com/yousefvand/SVE\">"
            "https://github.com/yousefvand/SVE</a></p>"
            "<p>Qt 6 &amp; FFmpeg · KDE Plasma / Wayland</p>")
            .arg(QString::fromLatin1(SVE_VERSION)));
        details->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        details->setWordWrap(true);
        details->setTextInteractionFlags(Qt::TextBrowserInteraction);
        details->setOpenExternalLinks(true);
        layout->addWidget(details, 0, Qt::AlignLeft);

        auto *close = makeButton("OK");
        connect(close, &QPushButton::clicked, &about, &QDialog::accept);
        layout->addWidget(close, 0, Qt::AlignRight);
        about.exec();
    });
    auto *aboutQtAction = helpMenu->addAction("About Qt", this, [this] {
        // Use Qt's official About Qt dialog (logo, licensing, runtime details).
        QMessageBox::aboutQt(this, "About Qt");
    });
    auto *surface = new QWidget;
    surface->setObjectName("appSurface");
    auto *root = new QVBoxLayout(surface);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *top = new QWidget;
    top->setObjectName("topBar");
    auto *bar = new QHBoxLayout(top);
    bar->setContentsMargins(22, 11, 22, 11);
    bar->setSpacing(10);
    auto *logo = new QLabel;
    logo->setPixmap(QIcon(":/sve.png").pixmap(42, 42));
    bar->addWidget(logo);
    auto *name = new QVBoxLayout;
    name->setSpacing(0);
    auto *brand = new QLabel("SVE"); brand->setObjectName("appName");
    name->addWidget(brand);
    name->addWidget(secondary("SIMPLE VIDEO EDITOR"));
    bar->addLayout(name);
    bar->addSpacing(27);
    m_title = new QLabel("No media loaded");
    m_title->setObjectName("sectionTitle");
    m_title->setMinimumWidth(200);
    m_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    bar->addWidget(m_title, 1);
    auto *projectOpen = makeButton("Open project", "Open an SVE project (Ctrl+Shift+O)");
    auto *aboutSveButton = makeButton("About SVE", "About SVE · Remisa Phillips and project home page");
    auto *aboutQtButton = makeButton("About Qt", "About the Qt framework");
    aboutSveButton->setObjectName("aboutButton");
    aboutQtButton->setObjectName("aboutButton");
    auto *projectSave = makeButton("Save project", "Save edit settings to .sve.json (Ctrl+S)");
    auto *open = makeButton("＋  Open video", "Open a video (Ctrl+O)", true);
    // Keep About buttons directly LEFT of Open project, in that order.
    // Reuse the Help actions so toolbar and menu open the same dialogs.
    bar->addWidget(aboutSveButton);
    bar->addWidget(aboutQtButton);
    bar->addWidget(projectOpen);
    bar->addWidget(projectSave);
    bar->addWidget(open);
    connect(open, &QPushButton::clicked, this, &MainWindow::chooseOpen);
    connect(projectOpen, &QPushButton::clicked, this, &MainWindow::loadProject);
    connect(aboutSveButton, &QPushButton::clicked, aboutSveAction, &QAction::trigger);
    connect(aboutQtButton, &QPushButton::clicked, aboutQtAction, &QAction::trigger);
    connect(projectSave, &QPushButton::clicked, this, &MainWindow::saveProject);
    root->addWidget(top);

    auto *split = new QSplitter(Qt::Horizontal);
    split->setChildrenCollapsible(false);
    split->addWidget(makeInspector());
    split->addWidget(makeEditor());
    split->addWidget(makeExporter());
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setStretchFactor(2, 0);
    split->setSizes({290, 860, 320});
    root->addWidget(split, 1);
    setCentralWidget(surface);

    m_status = new QLabel("Ready  •  Open or drop a video to begin");
    statusBar()->addWidget(m_status, 1);
    auto *version = secondary(QString("SVE %1  ·  Qt 6 + FFmpeg").arg(QString::fromLatin1(SVE_VERSION)));
    statusBar()->addPermanentWidget(version);
}

QWidget *MainWindow::makeInspector() {
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(235);
    scroll->setMaximumWidth(430);
    auto *pane = new QWidget;
    pane->setObjectName("sidePanel");
    auto *lay = new QVBoxLayout(pane);
    lay->setContentsMargins(13, 16, 13, 18);
    lay->setSpacing(13);
    auto *h = new QLabel("WORKSPACE"); h->setObjectName("sectionTitle"); lay->addWidget(h);
    auto *info = new QGroupBox("MEDIA DETAILS");
    auto *il = new QVBoxLayout(info);
    m_fileInfo = secondary("Open a file to inspect its tracks, resolution, frame rate, and duration.");
    m_fileInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    il->addWidget(m_fileInfo);
    lay->addWidget(info);

    auto *clips = new QGroupBox("CLIP ASSEMBLY");
    auto *cl = new QVBoxLayout(clips);
    cl->setSpacing(9);
    cl->addWidget(secondary("Keep several ranges in your chosen order. The original file is never modified."));
    m_clipList = new QListWidget;
    m_clipList->setMinimumHeight(144);
    cl->addWidget(m_clipList);
    m_clipTime = secondary("No clips added");
    cl->addWidget(m_clipTime);
    auto *clipRow = new QHBoxLayout;
    auto *add = makeButton("＋ Add A–B", "Add the marked range to the clip list");
    auto *remove = makeButton("Remove");
    clipRow->addWidget(add, 1); clipRow->addWidget(remove);
    cl->addLayout(clipRow);
    auto *order = new QHBoxLayout;
    auto *up = makeButton("↑ Earlier");
    auto *down = makeButton("↓ Later");
    order->addWidget(up); order->addWidget(down);
    cl->addLayout(order);
    auto *clipActions = new QHBoxLayout;
    auto *split = makeButton("✂ Split", "Split selected clip at the playhead");
    auto *duplicate = makeButton("Duplicate");
    auto *clear = makeButton("Clear");
    clipActions->addWidget(split); clipActions->addWidget(duplicate); clipActions->addWidget(clear);
    cl->addLayout(clipActions);
    auto *history = new QHBoxLayout;
    auto *undo = makeButton("↶ Undo", "Undo clip-list operation (Ctrl+Z)");
    auto *redo = makeButton("↷ Redo", "Redo clip-list operation (Ctrl+Shift+Z)");
    history->addWidget(undo); history->addWidget(redo);
    cl->addLayout(history);
    connect(split, &QPushButton::clicked, this, &MainWindow::splitClip);
    connect(duplicate, &QPushButton::clicked, this, &MainWindow::duplicateClip);
    connect(clear, &QPushButton::clicked, this, &MainWindow::clearClips);
    connect(undo, &QPushButton::clicked, this, &MainWindow::undoClips);
    connect(redo, &QPushButton::clicked, this, &MainWindow::redoClips);
    connect(add, &QPushButton::clicked, this, &MainWindow::addClip);
    connect(remove, &QPushButton::clicked, this, &MainWindow::removeClip);
    connect(up, &QPushButton::clicked, this, [this] { moveClip(-1); });
    connect(down, &QPushButton::clicked, this, [this] { moveClip(1); });
    connect(m_clipList, &QListWidget::itemDoubleClicked, this, [this] {
        int row = m_clipList->currentRow();
        if (row >= 0 && row < m_clips.size()) seek(m_clips.at(row).inMs);
    });
    lay->addWidget(clips);

    auto *filters = new QGroupBox("VIDEO FILTERS");
    auto *fl = new QVBoxLayout(filters);
    fl->setSpacing(9);
    fl->addWidget(secondary("Filters are applied during export. Preview shows the original video."));
    m_grayscale = new QCheckBox("Black & white");
    m_flipH = new QCheckBox("Flip horizontally");
    m_flipV = new QCheckBox("Flip vertically");
    m_denoise = new QCheckBox("Denoise");
    m_sharpen = new QCheckBox("Sharpen");
    fl->addWidget(m_grayscale); fl->addWidget(m_flipH); fl->addWidget(m_flipV);
    fl->addWidget(m_denoise); fl->addWidget(m_sharpen);
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft);
    m_rotation = makeCombo({"None", "90° clockwise", "180°", "270° clockwise"});
    m_resolution = makeCombo({"Source", "1080p", "720p", "480p"});
    m_brightness = makeSpin(-100, 100, 0, "%");
    m_contrast = makeSpin(0, 200, 100, "%");
    m_saturation = makeSpin(0, 200, 100, "%");
    form->addRow("Rotate", m_rotation);
    form->addRow("Scale", m_resolution);
    form->addRow("Brightness", m_brightness);
    form->addRow("Contrast", m_contrast);
    form->addRow("Saturation", m_saturation);
    fl->addLayout(form);
    fl->addWidget(secondary("CROP · pixels removed from each edge"));
    auto *crop = new QFormLayout;
    m_cropL = makeSpin(0, 16000); m_cropR = makeSpin(0, 16000);
    m_cropT = makeSpin(0, 16000); m_cropB = makeSpin(0, 16000);
    crop->addRow("Left", m_cropL); crop->addRow("Right", m_cropR);
    crop->addRow("Top", m_cropT); crop->addRow("Bottom", m_cropB);
    fl->addLayout(crop);
    auto *reset = makeButton("↺  Reset filters");
    fl->addWidget(reset);
    connect(reset, &QPushButton::clicked, this, &MainWindow::resetFilters);
    lay->addWidget(filters);
    lay->addStretch();
    scroll->setWidget(pane);
    return scroll;
}

QWidget *MainWindow::makeEditor() {
    auto *pane = new QWidget;
    pane->setObjectName("editorPanel");
    pane->setMinimumWidth(440);
    auto *lay = new QVBoxLayout(pane);
    lay->setContentsMargins(20, 16, 20, 18);
    lay->setSpacing(13);
    auto *row = new QHBoxLayout;
    auto *previewTitle = new QLabel("PREVIEW"); previewTitle->setObjectName("sectionTitle");
    row->addWidget(previewTitle);
    row->addStretch();
    row->addWidget(secondary("SOURCE MONITOR"));
    m_previewBackendLabel = secondary("AWAITING VIDEO");
    row->addWidget(m_previewBackendLabel);
    lay->addLayout(row);

    m_preview = new QStackedWidget;
    m_preview->setMinimumHeight(250);
    auto *empty = new QWidget;
    empty->setStyleSheet("background: #080d13; border:1px solid #304052; border-radius:12px;");
    auto *el = new QVBoxLayout(empty);
    el->setAlignment(Qt::AlignCenter);
    auto *emptyIcon = new QLabel("▣");
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyIcon->setStyleSheet("color:#27c9b1;font-size:66px;border:0;background:transparent;");
    auto *emptyTitle = new QLabel("Your next cut starts here");
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyTitle->setStyleSheet("font-size:19px;font-weight:700;border:0;background:transparent;");
    auto *emptyHelp = secondary("Drop a video into SVE or select Open video above");
    emptyHelp->setAlignment(Qt::AlignCenter);
    auto *emptyOpen = makeButton("Open a video", {}, true);
    connect(emptyOpen, &QPushButton::clicked, this, &MainWindow::chooseOpen);
    auto *center = new QHBoxLayout;
    center->addStretch(); center->addWidget(emptyOpen); center->addStretch();
    el->addStretch(); el->addWidget(emptyIcon); el->addWidget(emptyTitle); el->addWidget(emptyHelp);
    el->addSpacing(20); el->addLayout(center); el->addStretch();
    m_preview->addWidget(empty);
    m_video = new VideoSurface;
    m_video->clearFrame("Decoding video…");
    m_preview->addWidget(m_video);
    m_filterImage = new QLabel("Rendering filtered frame…");
    m_filterImage->setStyleSheet("background: #090e14; color: #ecf4f7; padding: 8px;");
    m_filterImage->setAlignment(Qt::AlignCenter);
    m_filterImage->setMinimumSize(220, 160);
    m_preview->addWidget(m_filterImage);
    lay->addWidget(m_preview, 1);

    auto *transport = new QHBoxLayout;
    transport->setSpacing(8);
    auto *start = makeButton("|◀", "Seek to start");
    auto *back = makeButton("‹ Frame", "Step backward one frame (Left)");
    m_play = makeButton("▶  Play", "Play or pause (Space)", true);
    auto *forward = makeButton("Frame ›", "Step forward one frame (Right)");
    auto *end = makeButton("▶|", "Seek to end");
    auto *goTo = makeButton("Go to…", "Jump to timecode (Ctrl+G)");
    m_position = new QLabel("00:00:00.000"); m_position->setObjectName("largeTime");
    m_duration = secondary("/  00:00:00.000");
    transport->addWidget(start); transport->addWidget(back);
    transport->addWidget(m_play); transport->addWidget(forward); transport->addWidget(end);
    transport->addWidget(goTo);
    transport->addStretch();
    transport->addWidget(m_position); transport->addWidget(m_duration);
    lay->addLayout(transport);
    connect(goTo, &QPushButton::clicked, this, &MainWindow::jumpToTime);
    connect(start, &QPushButton::clicked, this, [this] { seek(0); });
    connect(end, &QPushButton::clicked, this, [this] { seek(m_info.durationMs); });
    connect(back, &QPushButton::clicked, this, [this] { pausePreview(); seek(previewPosition() - qRound64(1000.0 / m_info.fps)); });
    connect(forward, &QPushButton::clicked, this, [this] { pausePreview(); seek(previewPosition() + qRound64(1000.0 / m_info.fps)); });
    connect(m_play, &QPushButton::clicked, this, [this] {
        if (!m_info.valid) return;
        showOriginal();
        togglePreview();
    });

    m_timeline = new TimelineWidget;
    connect(m_timeline, &TimelineWidget::seekRequested, this, &MainWindow::seek);
    lay->addWidget(m_timeline);
    auto *marks = new QHBoxLayout;
    auto *markIn = makeButton("[  Set A", "Mark selection start at playhead (I)");
    auto *markOut = makeButton("Set B  ]", "Mark selection end at playhead (O)");
    auto *reset = makeButton("Full range", "Reset marks to entire video");
    m_inLabel = secondary("A  00:00:00.000");
    m_outLabel = secondary("B  00:00:00.000");
    marks->addWidget(markIn); marks->addWidget(markOut); marks->addWidget(reset);
    marks->addStretch(); marks->addWidget(m_inLabel); marks->addSpacing(14); marks->addWidget(m_outLabel);
    lay->addLayout(marks);
    connect(markIn, &QPushButton::clicked, this, &MainWindow::setMarkIn);
    connect(markOut, &QPushButton::clicked, this, &MainWindow::setMarkOut);
    connect(reset, &QPushButton::clicked, this, [this] {
        m_in = 0; m_out = m_info.durationMs; updateTiming();
    });
    lay->addWidget(horizontalLine());
    auto *tools = new QHBoxLayout;
    auto *snap = makeButton("▣  Save current frame", "Export a PNG screenshot of the current frame");
    auto *tip = secondary("I / O = marks  •  Ctrl+G = go to");
    auto *volume = new QSlider(Qt::Horizontal);
    volume->setRange(0, 100); volume->setValue(70);
    volume->setFixedWidth(92);
    volume->setToolTip("Source-monitor volume; does not change exported audio");
    auto *volumeLabel = secondary("Volume");
    connect(volume, &QSlider::valueChanged, this, [this](int value) { m_audio->setVolume(value / 100.0); });
    m_filterPreviewButton = makeButton("◉ Preview filters", "Render filtered still frame using FFmpeg");
    auto *original = makeButton("Source view", "Return to original video playback");
    tools->addWidget(snap);
    tools->addWidget(m_filterPreviewButton);
    tools->addWidget(original);
    tools->addStretch(); tools->addWidget(volumeLabel); tools->addWidget(volume);
    tools->addWidget(tip);
    connect(m_filterPreviewButton, &QPushButton::clicked, this, &MainWindow::previewFilters);
    connect(original, &QPushButton::clicked, this, &MainWindow::showOriginal);
    lay->addLayout(tools);
    connect(snap, &QPushButton::clicked, this, &MainWindow::takeSnapshot);
    return pane;
}

QWidget *MainWindow::makeExporter() {
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(265);
    scroll->setMaximumWidth(455);
    auto *pane = new QWidget;
    pane->setObjectName("sidePanel");
    auto *lay = new QVBoxLayout(pane);
    lay->setContentsMargins(14, 16, 14, 18);
    lay->setSpacing(13);
    auto *title = new QLabel("EXPORT STUDIO"); title->setObjectName("sectionTitle");
    lay->addWidget(title);
    auto *settings = new QGroupBox("OUTPUT SETTINGS");
    auto *setLay = new QVBoxLayout(settings);
    setLay->setSpacing(9);
    auto *fields = new QFormLayout;
    m_exportMode = makeCombo({"Full video", "A–B selection", "Clip assembly", "Exclude A–B"});
    m_container = makeCombo({"mp4", "mkv", "webm", "mov"});
    m_videoCodec = makeCombo({"H.264", "H.265", "VP9", "AV1", "MPEG-4", "Copy"});
    m_audioCodec = makeCombo({"AAC", "Opus", "MP3", "Copy", "None"});
    m_preset = makeCombo({"ultrafast", "fast", "medium", "slow", "veryslow"});
    m_preset->setCurrentText("medium");
    m_crf = makeSpin(0, 51, 22);
    m_crf->setToolTip("Lower CRF means higher quality and larger files. H.264 generally uses 18–28.");
    fields->addRow("Range", m_exportMode);
    fields->addRow("Container", m_container);
    fields->addRow("Video", m_videoCodec);
    fields->addRow("Audio", m_audioCodec);
    fields->addRow("Preset", m_preset);
    fields->addRow("Quality CRF", m_crf);
    setLay->addLayout(fields);
    auto *audioForm = new QFormLayout;
    m_audioGain = makeSpin(-20, 20, 0, " dB");
    m_fadeIn = new QDoubleSpinBox;
    m_fadeOut = new QDoubleSpinBox;
    for (auto *fade : {m_fadeIn, m_fadeOut}) {
        fade->setRange(0, 600); fade->setDecimals(1); fade->setSingleStep(0.5); fade->setSuffix(" s");
    }
    audioForm->addRow("Audio gain", m_audioGain);
    audioForm->addRow("Audio fade in", m_fadeIn);
    audioForm->addRow("Audio fade out", m_fadeOut);
    setLay->addWidget(secondary("AUDIO ADJUSTMENTS"));
    setLay->addLayout(audioForm);
    setLay->addWidget(secondary("Copy = fast stream remux, no video filters or multi-clip assembly. Other modes re-encode."));
    lay->addWidget(settings);

    auto *dest = new QGroupBox("DESTINATION");
    auto *dl = new QVBoxLayout(dest);
    dl->setSpacing(9);
    m_outputPath = new QLineEdit;
    m_outputPath->setPlaceholderText("Choose a file destination…");
    m_outputPath->setToolTip("Output file path");
    dl->addWidget(m_outputPath);
    auto *browse = makeButton("Browse output…");
    dl->addWidget(browse);
    connect(browse, &QPushButton::clicked, this, &MainWindow::chooseOutput);
    connect(m_container, &QComboBox::currentTextChanged, this, [this](const QString &ext) {
        if (m_outputPath->text().isEmpty()) return;
        const QFileInfo f(m_outputPath->text());
        m_outputPath->setText(f.dir().filePath(f.completeBaseName() + "." + ext));
    });
    lay->addWidget(dest);

    auto *job = new QGroupBox("ENCODE JOB");
    auto *jl = new QVBoxLayout(job);
    jl->setSpacing(10);
    m_processState = secondary("Ready to encode");
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_exportButton = makeButton("▶  Export video", "Start FFmpeg export (Ctrl+E)", true);
    m_exportButton->setMinimumHeight(44);
    m_cancelButton = makeButton("Cancel encoding");
    m_cancelButton->setEnabled(false);
    jl->addWidget(m_processState);
    jl->addWidget(m_progress);
    jl->addWidget(m_exportButton);
    jl->addWidget(m_cancelButton);
    m_openFolder = makeButton("↗ Open output folder", "Open the folder containing the completed export");
    m_openFolder->hide();
    jl->addWidget(m_openFolder);
    connect(m_openFolder, &QPushButton::clicked, this, [this] {
        if (!m_lastOutput.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_lastOutput).absolutePath()));
    });
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::exportVideo);
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancelExport);
    lay->addWidget(job);
    auto *log = new QGroupBox("FFMPEG LOG");
    auto *ll = new QVBoxLayout(log);
    m_logView = new QPlainTextEdit;
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(250);
    m_logView->setPlaceholderText("Export messages will appear here.");
    m_logView->setMinimumHeight(95);
    ll->addWidget(m_logView);
    lay->addWidget(log);
    lay->addStretch();
    scroll->setWidget(pane);
    return scroll;
}

qint64 MainWindow::previewPosition() const {
    return m_fallbackMode ? m_fallbackPosition : m_player->position();
}

void MainWindow::startPreviewWatch() {
    const int generation = m_previewGeneration;
    QTimer::singleShot(3000, this, [this, generation] {
        if (generation != m_previewGeneration || !m_info.valid || m_nativeFrameSeen || m_fallbackMode) return;
        setStatus("Qt produced no video frames. Switching to FFmpeg preview (video-only fallback).");
        startFallback(m_pauseAfterFirstFrame ? 0 : m_player->position(), false);
    });
}

void MainWindow::stopFallback() {
    m_fallbackTicker->stop();
    m_fallbackPlaying = false;
    m_fallbackMode = false;
    m_fallbackBytes.clear();
    if (m_fallback->state() != QProcess::NotRunning) {
        m_fallback->kill();
        m_fallback->waitForFinished(350);
    }
}

void MainWindow::requestFallbackFrame(qint64 ms) {
    if (!m_info.valid || m_nativeFrameSeen || m_fallbackPlaying) return;
    if (m_stillFrame->state() != QProcess::NotRunning) {
        m_stillFrame->kill();
        m_stillFrame->waitForFinished(200);
    }
    m_stillBytes.clear();
    m_stillFrame->start("ffmpeg", {
        "-hide_banner", "-nostdin", "-v", "error", "-ss", MediaEngine::ffTime(ms),
        "-i", m_info.path, "-map", "0:v:0", "-frames:v", "1", "-vf",
        "scale=960:540:force_original_aspect_ratio=decrease", "-f", "image2pipe", "-vcodec", "png", "pipe:1"
    });
}

void MainWindow::startFallback(qint64 ms, bool play) {
    if (!m_info.valid) return;
    if (m_fallback->state() != QProcess::NotRunning) {
        m_fallback->kill();
        m_fallback->waitForFinished(350);
    }
    m_player->pause();
    m_fallbackMode = true;
    m_previewBackendLabel->setText("FFMPEG FALLBACK · VIDEO ONLY");
    m_fallbackPosition = qBound(qint64(0), ms, m_info.durationMs);
    m_fallbackBase = m_fallbackPosition;
    m_fallbackPlaying = play;
    m_fallbackBytes.clear();
    if (!play) {
        m_fallbackTicker->stop();
        m_play->setText("▶  Play");
        requestFallbackFrame(m_fallbackPosition);
        updateTiming();
        return;
    }
    m_play->setText("Ⅱ  Pause");
    m_fallbackElapsed.start();
    m_fallbackTicker->start();
    m_fallback->start("ffmpeg", {
        "-hide_banner", "-nostdin", "-v", "error", "-ss", MediaEngine::ffTime(m_fallbackPosition),
        "-re", "-i", m_info.path, "-map", "0:v:0", "-an", "-vf",
        "fps=15,scale=960:540:force_original_aspect_ratio=decrease,"
        "pad=960:540:(ow-iw)/2:(oh-ih)/2,format=rgb24",
        "-pix_fmt", "rgb24", "-f", "rawvideo", "pipe:1"
    });
}

void MainWindow::playPreview() {
    if (!m_info.valid) return;
    showOriginal();
    if (m_fallbackMode) startFallback(previewPosition() >= m_info.durationMs ? 0 : previewPosition(), true);
    else {
        m_pauseAfterFirstFrame = false;
        m_player->play();
        if (!m_nativeFrameSeen) startPreviewWatch();
    }
}

void MainWindow::pausePreview() {
    if (m_fallbackMode) {
        if (m_fallbackPlaying) {
            m_fallbackPosition = qMin(m_info.durationMs, m_fallbackBase + m_fallbackElapsed.elapsed());
            startFallback(m_fallbackPosition, false);
        }
    } else m_player->pause();
}

void MainWindow::togglePreview() {
    if (!m_info.valid) return;
    if (m_fallbackMode ? m_fallbackPlaying : m_player->playbackState() == QMediaPlayer::PlayingState)
        pausePreview();
    else playPreview();
}

void MainWindow::setStatus(const QString &message) { m_status->setText(message); }

bool MainWindow::openMedia(const QString &path) {
    if (m_busy) return false;
    if (path.endsWith(".sve.json", Qt::CaseInsensitive)) {
        openProjectPath(path);
        return true;
    }
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    MediaInfo candidate = MediaEngine::probe(absolutePath);
    if (!candidate.valid) {
        QMessageBox::warning(this, "Cannot open media", candidate.error);
        return false;
    }
    stopFallback();
    m_stillFrame->kill();
    m_stillFrame->waitForFinished(300);
    ++m_previewGeneration;
    m_nativeFrameSeen = false;
    m_pauseAfterFirstFrame = true;
    m_video->clearFrame("Decoding video…");
    m_previewBackendLabel->setText("DECODING...");
    m_player->stop();
    m_info = candidate;
    m_clips.clear();
    m_undoClips.clear(); m_redoClips.clear();
    m_in = 0; m_out = m_info.durationMs;
    m_player->setSource(QUrl::fromLocalFile(absolutePath));
    m_player->play();
    requestFallbackFrame(0);
    startPreviewWatch();
    m_openFolder->hide();
    m_preview->setCurrentIndex(1);
    m_title->setText(QFileInfo(absolutePath).fileName());
    m_title->setToolTip(absolutePath);
    m_exportMode->setCurrentIndex(0);
    m_outputPath->setText(QFileInfo(absolutePath).dir().filePath(QFileInfo(absolutePath).completeBaseName() + "_sve." + m_container->currentText()));
    updateMediaInfo();
    updateTiming();
    updateClips();
    setWindowTitle("SVE  ·  " + QFileInfo(path).fileName());
    setStatus("Loaded " + QFileInfo(path).fileName() + "  •  Ready to edit");
    QSettings settings; settings.setValue("lastFolder", QFileInfo(path).absolutePath());
    return true;
}
void MainWindow::updateMediaInfo() {
    if (!m_info.valid) return;
    m_fileInfo->setText(QString("%1 × %2  •  %3 fps\nVideo: %4\nAudio: %5\nLength: %6\nSize: %7\nContainer: %8")
        .arg(m_info.width).arg(m_info.height).arg(QString::number(m_info.fps, 'f', 2))
        .arg(m_info.videoCodec, m_info.hasAudio ? m_info.audioCodec : "No audio")
        .arg(MediaEngine::timecode(m_info.durationMs), friendlySize(m_info.bytes), m_info.format));
    m_timeline->setDuration(m_info.durationMs);
}
void MainWindow::updateTiming() {
    m_position->setText(MediaEngine::timecode(previewPosition()));
    m_duration->setText("/  " + MediaEngine::timecode(m_info.durationMs));
    m_inLabel->setText("A  " + MediaEngine::timecode(m_in));
    m_outLabel->setText("B  " + MediaEngine::timecode(m_out));
    m_timeline->setPosition(previewPosition());
    m_timeline->setMarks(m_in, m_out);
}
void MainWindow::updateClips() {
    int old = m_clipList->currentRow();
    m_clipList->clear();
    for (int i = 0; i < m_clips.size(); ++i) {
        const Clip &clip = m_clips.at(i);
        m_clipList->addItem(QString("%1   %2  →  %3\n       Length  %4")
            .arg(i + 1, 2, 10, QLatin1Char('0'))
            .arg(MediaEngine::timecode(clip.inMs).left(8), MediaEngine::timecode(clip.outMs).left(8),
                 MediaEngine::timecode(clip.outMs - clip.inMs).left(8)));
    }
    if (old >= 0 && old < m_clips.size()) m_clipList->setCurrentRow(old);
    m_clipTime->setText(m_clips.isEmpty()
        ? "No clips added. Mark A–B and click Add."
        : QString("%1 clips  •  total %2").arg(m_clips.size()).arg(MediaEngine::timecode(MediaEngine::clipsDuration(m_clips))));
}
void MainWindow::chooseOpen() {
    QSettings s;
    QString path = QFileDialog::getOpenFileName(this, "Open video", s.value("lastFolder", QDir::homePath()).toString(),
        "Video files (*.mp4 *.mkv *.webm *.mov *.avi *.mpeg *.mpg *.m4v *.ts *.flv);;All files (*)");
    if (!path.isEmpty()) openMedia(path);
}
void MainWindow::chooseOutput() {
    QString path = QFileDialog::getSaveFileName(this, "Save exported video", m_outputPath->text(),
        "Video output (*." + m_container->currentText() + ");;All files (*)");
    if (!path.isEmpty()) {
        if (QFileInfo(path).suffix().isEmpty()) path += "." + m_container->currentText();
        m_outputPath->setText(path);
    }
}
void MainWindow::seek(qint64 ms) {
    if (!m_info.valid) return;
    showOriginal();
    const qint64 bounded = qBound(qint64(0), ms, m_info.durationMs);
    if (m_fallbackMode) {
        startFallback(bounded, m_fallbackPlaying);
    } else {
        m_player->setPosition(bounded);
        if (!m_nativeFrameSeen) requestFallbackFrame(bounded);
    }
    updateTiming();
}
void MainWindow::setMarkIn() {
    if (!m_info.valid) return;
    m_in = qMin(previewPosition(), m_info.durationMs);
    if (m_in >= m_out) m_out = m_info.durationMs;
    updateTiming();
    setStatus("Mark A set  ·  " + MediaEngine::timecode(m_in));
}
void MainWindow::setMarkOut() {
    if (!m_info.valid) return;
    m_out = qBound(qint64(0), previewPosition(), m_info.durationMs);
    if (m_out <= m_in) m_in = 0;
    updateTiming();
    setStatus("Mark B set  ·  " + MediaEngine::timecode(m_out));
}
void MainWindow::addClip() {
    if (!m_info.valid) return;
    if (m_out <= m_in) { QMessageBox::information(this, "Invalid range", "Set a valid A–B selection before adding a clip."); return; }
    rememberClips();
    m_clips.append({m_in, m_out});
    updateClips();
    m_clipList->setCurrentRow(m_clips.size() - 1);
    m_exportMode->setCurrentIndex(2);
    setStatus("Clip added  ·  " + QString::number(m_clips.size()) + " clip(s)");
}
void MainWindow::removeClip() {
    int row = m_clipList->currentRow();
    if (row < 0 || row >= m_clips.size()) return;
    rememberClips();
    m_clips.removeAt(row);
    updateClips();
    if (!m_clips.isEmpty()) m_clipList->setCurrentRow(qMin(row, m_clips.size() - 1));
    else m_exportMode->setCurrentIndex(1);
}
void MainWindow::moveClip(int step) {
    int row = m_clipList->currentRow();
    int dest = row + step;
    if (row < 0 || dest < 0 || dest >= m_clips.size()) return;
    rememberClips();
    m_clips.swapItemsAt(row, dest);
    updateClips(); m_clipList->setCurrentRow(dest);
}
void MainWindow::rememberClips() {
    m_undoClips.push_back(m_clips);
    if (m_undoClips.size() > 100) m_undoClips.removeFirst();
    m_redoClips.clear();
}
void MainWindow::undoClips() {
    if (m_undoClips.isEmpty()) return;
    m_redoClips.push_back(m_clips);
    m_clips = m_undoClips.takeLast();
    updateClips(); setStatus("Clip edit undone");
}
void MainWindow::redoClips() {
    if (m_redoClips.isEmpty()) return;
    m_undoClips.push_back(m_clips);
    m_clips = m_redoClips.takeLast();
    updateClips(); setStatus("Clip edit redone");
}
void MainWindow::duplicateClip() {
    const int row = m_clipList->currentRow();
    if (row < 0 || row >= m_clips.size()) return;
    rememberClips();
    m_clips.insert(row + 1, m_clips.at(row));
    updateClips(); m_clipList->setCurrentRow(row + 1);
    m_exportMode->setCurrentIndex(2);
}
void MainWindow::splitClip() {
    const int row = m_clipList->currentRow();
    if (row < 0 || row >= m_clips.size()) return;
    const Clip part = m_clips.at(row);
    const qint64 at = previewPosition();
    if (at <= part.inMs || at >= part.outMs) {
        QMessageBox::information(this, "Cannot split here", "Move the playhead inside the selected clip before splitting it.");
        return;
    }
    rememberClips();
    m_clips[row].outMs = at;
    m_clips.insert(row + 1, {at, part.outMs});
    updateClips(); m_clipList->setCurrentRow(row + 1);
    m_exportMode->setCurrentIndex(2);
}
void MainWindow::clearClips() {
    if (m_clips.isEmpty()) return;
    rememberClips(); m_clips.clear(); updateClips();
    m_exportMode->setCurrentIndex(1);
}
void MainWindow::showOriginal() {
    if (m_info.valid && m_preview && m_preview->currentIndex() == 2)
        m_preview->setCurrentIndex(1);
}
void MainWindow::jumpToTime() {
    if (!m_info.valid) return;
    bool ok = false;
    const QString text = QInputDialog::getText(this, "Go to time", "Timecode (HH:MM:SS.mmm)",
                                                QLineEdit::Normal, MediaEngine::timecode(previewPosition()), &ok);
    if (!ok) return;
    const QStringList parts = text.trimmed().split(':');
    if (parts.size() != 3) {
        QMessageBox::warning(this, "Invalid timecode", "Enter a timecode such as 00:01:23.500."); return;
    }
    bool hOk = false, mOk = false, sOk = false;
    const qint64 hours = parts.at(0).toLongLong(&hOk);
    const int minutes = parts.at(1).toInt(&mOk);
    const double seconds = parts.at(2).toDouble(&sOk);
    if (!hOk || !mOk || !sOk || hours < 0 || minutes < 0 || minutes > 59 ||
        seconds < 0 || seconds >= 60 || hours > 100000) {
        QMessageBox::warning(this, "Invalid timecode", "Use non-negative HH:MM:SS.mmm with minutes and seconds below 60."); return;
    }
    seek((hours * 3600 + minutes * 60) * 1000 + qRound64(seconds * 1000));
}
void MainWindow::previewFilters() {
    if (!m_info.valid || m_filterProcess->state() != QProcess::NotRunning) return;
    pausePreview();
    m_filterPreviewButton->setEnabled(false);
    m_filterImage->setPixmap(QPixmap());
    m_filterImage->setText("Rendering filtered frame…");
    m_preview->setCurrentIndex(2);
    const QStringList filters = options().filters.graphFilters();
    QStringList args = {"-hide_banner", "-nostdin", "-v", "error", "-ss", MediaEngine::ffTime(previewPosition()),
                        "-i", m_info.path, "-map", "0:v:0", "-frames:v", "1"};
    if (!filters.isEmpty()) args << "-vf" << (filters.join(',') + ",format=rgb24");
    args << "-f" << "image2pipe" << "-vcodec" << "png" << "pipe:1";
    m_filterProcess->start("ffmpeg", args);
}

void MainWindow::resetFilters() {
    for (auto *c : {m_grayscale, m_flipH, m_flipV, m_denoise, m_sharpen}) c->setChecked(false);
    m_rotation->setCurrentIndex(0); m_resolution->setCurrentIndex(0);
    m_brightness->setValue(0); m_contrast->setValue(100); m_saturation->setValue(100);
    for (auto *spin : {m_cropL, m_cropR, m_cropT, m_cropB}) spin->setValue(0);
    setStatus("Video filters reset");
}
ExportOptions MainWindow::options() const {
    ExportOptions o;
    o.input = m_info.path;
    o.output = m_outputPath->text().trimmed();
    o.fullDurationMs = m_info.durationMs;
    o.hasAudio = m_info.hasAudio;
    o.hasVideo = m_info.hasVideo;
    o.videoCodec = m_videoCodec->currentText();
    o.audioCodec = m_audioCodec->currentText();
    o.container = m_container->currentText();
    o.speed = m_preset->currentText();
    o.crf = m_crf->value();
    o.fps = m_info.fps;
    if (m_exportMode->currentIndex() == 1) o.clips = {{m_in, m_out}};
    else if (m_exportMode->currentIndex() == 2) o.clips = m_clips;
    else if (m_exportMode->currentIndex() == 3) {
        if (m_in > 0) o.clips.append({0, m_in});
        if (m_out < m_info.durationMs) o.clips.append({m_out, m_info.durationMs});
    } else o.clips = {{0, m_info.durationMs}};
    o.audioGainDb = m_audioGain->value();
    o.audioFadeInMs = qRound64(m_fadeIn->value() * 1000);
    o.audioFadeOutMs = qRound64(m_fadeOut->value() * 1000);
    o.filters.grayscale = m_grayscale->isChecked();
    o.filters.flipH = m_flipH->isChecked();
    o.filters.flipV = m_flipV->isChecked();
    o.filters.denoise = m_denoise->isChecked();
    o.filters.sharpen = m_sharpen->isChecked();
    o.filters.rotate = m_rotation->currentIndex() * 90;
    o.filters.height = m_resolution->currentIndex() == 1 ? 1080 : m_resolution->currentIndex() == 2 ? 720 : m_resolution->currentIndex() == 3 ? 480 : 0;
    o.filters.brightness = m_brightness->value();
    o.filters.contrast = m_contrast->value();
    o.filters.saturation = m_saturation->value();
    o.filters.cropLeft = m_cropL->value(); o.filters.cropRight = m_cropR->value();
    o.filters.cropTop = m_cropT->value(); o.filters.cropBottom = m_cropB->value();
    return o;
}
void MainWindow::setBusy(bool busy) {
    m_busy = busy;
    m_exportButton->setEnabled(!busy);
    m_cancelButton->setEnabled(busy);
}
void MainWindow::exportVideo() {
    if (m_busy) return;
    if (!m_info.valid) { QMessageBox::information(this, "Open video first", "Load a video before exporting."); return; }
    ExportOptions o = options();
    if (m_exportMode->currentIndex() == 3 && m_in == 0 && m_out == m_info.durationMs) {
        QMessageBox::warning(this, "Nothing to export", "The excluded range covers the entire video. Set a smaller A–B range."); return;
    }
    if (m_exportMode->currentIndex() == 2 && m_clips.isEmpty()) {
        QMessageBox::warning(this, "Empty clip list", "Add at least one clip or select a different export range."); return;
    }
    if (o.filters.cropLeft + o.filters.cropRight >= m_info.width ||
        o.filters.cropTop + o.filters.cropBottom >= m_info.height) {
        QMessageBox::warning(this, "Invalid crop", "Crop values remove the entire image. Reduce the crop dimensions."); return;
    }
    auto command = MediaEngine::makeExport(o);
    if (!command.valid()) { QMessageBox::warning(this, "Cannot export", command.error); return; }
    const QFileInfo destination(o.output);
    if (!destination.dir().exists()) { QMessageBox::warning(this, "Invalid destination", "The output folder does not exist."); return; }
    if (destination.exists()) {
        if (QMessageBox::question(this, "Replace existing video?", "This file already exists:\n" + o.output + "\n\nReplace it?") != QMessageBox::Yes) return;
    }
    if (!command.warning.isEmpty()) {
        if (QMessageBox::warning(this, "Stream-copy trimming", command.warning, QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok) return;
    }
    m_expected = command.expectedMs;
    m_lastOutput = o.output;
    // Render into a sibling temporary file so failed/cancelled jobs never destroy an existing export.
    m_stagingOutput = destination.dir().filePath("." + destination.completeBaseName()
        + "-sve-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + o.container);
    // makeExport always places the output path as its last argument.
    command.args.last() = m_stagingOutput;
    m_openFolder->hide();
    m_log.clear(); m_stdoutBuffer.clear(); m_cancelled = false;
    m_logView->clear();
    m_progress->setValue(0);
    m_processState->setText("Encoding...  " + MediaEngine::timecode(m_expected));
    pausePreview();
    setBusy(true);
    m_export->start("ffmpeg", command.args);
    if (!m_export->waitForStarted(3000)) {
        setBusy(false);
        m_processState->setText("Could not start FFmpeg");
        QMessageBox::warning(this, "FFmpeg error", "Could not launch ffmpeg. Check that it is installed and on PATH.");
        return;
    }
    setStatus("Encoding in progress  •  Output: " + o.output);
}
void MainWindow::handleProgress() {
    m_stdoutBuffer += QString::fromUtf8(m_export->readAllStandardOutput());
    int end = 0;
    while ((end = m_stdoutBuffer.indexOf('\n')) >= 0) {
        const QString line = m_stdoutBuffer.left(end).trimmed();
        m_stdoutBuffer.remove(0, end + 1);
        if (line.startsWith("out_time_us=") || line.startsWith("out_time_ms=")) {
            bool ok = false;
            const qint64 microseconds = line.section('=', 1).toLongLong(&ok);
            if (ok && m_expected > 0) {
                int percent = qBound(0, static_cast<int>((microseconds / 1000.0 / m_expected) * 100), 99);
                m_progress->setValue(percent);
                m_processState->setText(QString("Encoding  %1%  •  %2 / %3")
                    .arg(percent).arg(MediaEngine::timecode(microseconds / 1000), MediaEngine::timecode(m_expected)));
            }
        }
    }
}
void MainWindow::cancelExport() {
    if (!m_busy || m_export->state() == QProcess::NotRunning) return;
    m_cancelled = true;
    m_processState->setText("Cancelling…");
    m_export->terminate();
    QTimer::singleShot(2200, this, [this] { if (m_export->state() != QProcess::NotRunning) m_export->kill(); });
}
void MainWindow::exportFinished(int exitCode, bool normal) {
    bool success = normal && exitCode == 0 && !m_cancelled && QFileInfo(m_stagingOutput).isFile()
        && QFileInfo(m_stagingOutput).size() > 0;
    if (success) {
        // Staging file is a sibling of the output, so rename() is an atomic
        // replacement on Linux and preserves the existing output if it fails.
        // QFile::remove before rename would risk deleting a good previous export.
        const QByteArray from = QFile::encodeName(m_stagingOutput);
        const QByteArray to = QFile::encodeName(m_lastOutput);
        if (std::rename(from.constData(), to.constData()) != 0) {
            m_log += "\nUnable to atomically install completed video at destination.";
            success = false;
        }
    }
    if (!success) QFile::remove(m_stagingOutput);
    setBusy(false);
    if (success) {
        m_progress->setValue(100);
        m_processState->setText("Export complete  ✓  " + QFileInfo(m_lastOutput).fileName());
        m_processState->setToolTip(m_lastOutput);
        m_openFolder->show();
        setStatus("Export complete  •  " + m_lastOutput);
    } else if (m_cancelled) {
        m_processState->setText("Export cancelled");
        setStatus("Export cancelled");
    } else {
        m_processState->setText("Export failed  •  inspect FFmpeg log");
        setStatus("Export failed; see the FFmpeg log in the right panel");
        QMessageBox::warning(this, "Export failed", "FFmpeg exited with code " + QString::number(exitCode) + ".\n\n" + m_log.right(1100));
    }
}
void MainWindow::takeSnapshot() {
    if (!m_info.valid) return;
    QString suggested = QFileInfo(m_info.path).dir().filePath(QFileInfo(m_info.path).completeBaseName() + "_frame.png");
    QString path = QFileDialog::getSaveFileName(this, "Save current frame as PNG", suggested, "PNG image (*.png)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".png", Qt::CaseInsensitive)) path += ".png";
    if (QFileInfo(path).exists() && QMessageBox::question(this, "Replace screenshot?", "Replace existing file?") != QMessageBox::Yes) return;
    QProcess ff;
    ff.start("ffmpeg", {"-hide_banner", "-v", "error", "-y", "-ss", MediaEngine::ffTime(previewPosition()),
                        "-i", m_info.path, "-frames:v", "1", "-update", "1", path});
    if (!ff.waitForStarted(3000) || !ff.waitForFinished(30000) || ff.exitCode() != 0) {
        QMessageBox::warning(this, "Screenshot failed", QString::fromUtf8(ff.readAllStandardError()));
        return;
    }
    setStatus("Saved video frame  •  " + path);
}

QJsonObject MainWindow::projectJson() const {
    ExportOptions o = options();
    QJsonArray clips;
    for (const Clip &clip : m_clips) clips.append(QJsonObject{{"in", static_cast<double>(clip.inMs)}, {"out", static_cast<double>(clip.outMs)}});
    return {{"format", "SVE project"}, {"version", 1}, {"source", m_info.path},
            {"in", static_cast<double>(m_in)}, {"out", static_cast<double>(m_out)},
            {"clips", clips}, {"exportMode", m_exportMode->currentIndex()},
            {"container", o.container}, {"output", o.output}, {"videoCodec", o.videoCodec},
            {"audioCodec", o.audioCodec}, {"preset", o.speed}, {"crf", o.crf},
            {"audioGainDb", o.audioGainDb}, {"audioFadeInMs", o.audioFadeInMs}, {"audioFadeOutMs", o.audioFadeOutMs},
            {"filters", o.filters.toJson()}};
}
void MainWindow::saveProject() {
    if (!m_info.valid) { QMessageBox::information(this, "No project", "Open a video before saving an SVE project."); return; }
    QString suggested = QFileInfo(m_info.path).dir().filePath(QFileInfo(m_info.path).completeBaseName() + ".sve.json");
    QString path = QFileDialog::getSaveFileName(this, "Save SVE project", suggested, "SVE project (*.sve.json);;JSON (*.json)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".sve.json";
    if (QFileInfo(path).exists() && QMessageBox::question(this, "Replace project?", "Replace existing project file?") != QMessageBox::Yes) return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(projectJson()).toJson(QJsonDocument::Indented)) < 0 ||
        !file.commit()) {
        QMessageBox::warning(this, "Save failed", file.errorString()); return;
    }
    setStatus("Project saved  •  " + path);
}
void MainWindow::loadProject() {
    if (m_busy) return;
    QString path = QFileDialog::getOpenFileName(this, "Open SVE project", QDir::homePath(), "SVE project (*.sve.json *.json)");
    if (path.isEmpty()) return;
    openProjectPath(path);
}
void MainWindow::openProjectPath(const QString &path) {
    if (m_busy) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, "Open failed", file.errorString()); return; }
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject() || document.object().value("format").toString() != "SVE project") {
        QMessageBox::warning(this, "Invalid project", "Not a valid SVE project JSON file."); return;
    }
    QJsonObject data = document.object();
    QString source = data.value("source").toString();
    if (!QFileInfo(source).isFile()) {
        QString newPath = QFileDialog::getOpenFileName(this, "Locate missing source video", QFileInfo(path).absolutePath());
        if (newPath.isEmpty()) return;
        source = newPath;
    }
    if (!openMedia(source)) return;
    applySettings(data);
    setStatus("Project opened  •  " + path);
}
void MainWindow::applySettings(const QJsonObject &o) {
    m_in = qBound(qint64(0), static_cast<qint64>(o.value("in").toDouble()), m_info.durationMs);
    m_out = qBound(qint64(0), static_cast<qint64>(o.value("out").toDouble()), m_info.durationMs);
    if (m_in >= m_out) { m_in = 0; m_out = m_info.durationMs; }
    m_clips.clear(); m_undoClips.clear(); m_redoClips.clear();
    for (const QJsonValue &value : o.value("clips").toArray()) {
        const auto obj = value.toObject();
        Clip clip{static_cast<qint64>(obj.value("in").toDouble()), static_cast<qint64>(obj.value("out").toDouble())};
        if (clip.inMs >= 0 && clip.outMs <= m_info.durationMs && clip.outMs > clip.inMs) m_clips.push_back(clip);
    }
    m_container->setCurrentText(o.value("container").toString("mp4"));
    m_videoCodec->setCurrentText(o.value("videoCodec").toString("H.264"));
    m_audioCodec->setCurrentText(o.value("audioCodec").toString("AAC"));
    m_preset->setCurrentText(o.value("preset").toString("medium"));
    m_crf->setValue(o.value("crf").toInt(22));
    m_audioGain->setValue(o.value("audioGainDb").toInt());
    m_fadeIn->setValue(o.value("audioFadeInMs").toInt() / 1000.0);
    m_fadeOut->setValue(o.value("audioFadeOutMs").toInt() / 1000.0);
    m_exportMode->setCurrentIndex(qBound(0, o.value("exportMode").toInt(), 3));
    m_outputPath->setText(o.value("output").toString(m_outputPath->text()));
    auto f = FilterOptions::fromJson(o.value("filters").toObject());
    m_grayscale->setChecked(f.grayscale); m_flipH->setChecked(f.flipH); m_flipV->setChecked(f.flipV);
    m_denoise->setChecked(f.denoise); m_sharpen->setChecked(f.sharpen);
    m_rotation->setCurrentIndex(qBound(0, f.rotate / 90, 3));
    m_resolution->setCurrentIndex(f.height == 1080 ? 1 : f.height == 720 ? 2 : f.height == 480 ? 3 : 0);
    m_brightness->setValue(f.brightness); m_contrast->setValue(f.contrast); m_saturation->setValue(f.saturation);
    m_cropL->setValue(f.cropLeft); m_cropR->setValue(f.cropRight);
    m_cropT->setValue(f.cropTop); m_cropB->setValue(f.cropBottom);
    updateClips(); updateTiming();
}
void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}
void MainWindow::dropEvent(QDropEvent *event) {
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            const QString path = url.toLocalFile();
            if (path.endsWith(".sve.json", Qt::CaseInsensitive)) {
                openProjectPath(path);
            } else openMedia(path);
            event->acceptProposedAction();
            return;
        }
    }
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_busy && QMessageBox::question(this, "Encoding in progress", "Cancel the current export and quit?") != QMessageBox::Yes) {
        event->ignore(); return;
    }
    if (m_filterProcess->state() != QProcess::NotRunning) { m_filterProcess->kill(); m_filterProcess->waitForFinished(1500); }
    if (m_busy) { m_cancelled = true; m_export->kill(); m_export->waitForFinished(3000); QFile::remove(m_stagingOutput); }
    QSettings settings; settings.setValue("windowGeometry", saveGeometry());
    QMainWindow::closeEvent(event);
}
