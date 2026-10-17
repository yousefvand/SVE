#include "mainwindow.h"

#include <QApplication>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QStyleFactory>
#include <QTextStream>
#include <QByteArray>

int main(int argc, char **argv) {
    // Request the codec-capable Qt FFmpeg backend on Arch. Respect any
    // explicit user override for diagnosing a different media backend.
    if (qEnvironmentVariableIsEmpty("QT_MEDIA_BACKEND"))
        qputenv("QT_MEDIA_BACKEND", QByteArray("ffmpeg"));
    QApplication app(argc, argv);
    app.setApplicationName("SVE");
    app.setDesktopFileName("sve");
    app.setOrganizationName("SVE");
    app.setApplicationVersion(SVE_VERSION);
    app.setWindowIcon(QIcon(":/sve.png"));
    app.setStyle(QStyleFactory::create("Fusion"));
    QFont font = app.font();
    font.setFamily("Noto Sans");
    font.setPointSize(10);
    app.setFont(font);

    QStringList args = app.arguments();
    if (args.contains("--version")) { QTextStream(stdout) << "SVE " << SVE_VERSION << Qt::endl; return 0; }
    if (args.contains("--help")) {
        QTextStream(stdout) << "SVE - Simple Video Editor\nUsage: sve [video-file]\n"
                            << "Options: --help, --version\n";
        return 0;
    }
    MainWindow window;
    window.show();
    if (args.size() > 1) window.openMedia(QFileInfo(args.at(1)).absoluteFilePath());
    return app.exec();
}
