#include "videosurface.h"
#include <QPainter>
#include <QPaintEvent>

VideoSurface::VideoSurface(QWidget *parent) : QWidget(parent) {
    setMinimumSize(220, 160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoSurface::setFrame(const QImage &frame) {
    if (frame.isNull()) return;
    m_frame = frame;
    m_message.clear();
    update();
}

void VideoSurface::clearFrame(const QString &message) {
    m_frame = {};
    m_message = message;
    update();
}

void VideoSurface::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#070c13"));
    if (!m_frame.isNull()) {
        QSize scaled = m_frame.size();
        scaled.scale(size(), Qt::KeepAspectRatio);
        const QRect destination(QPoint((width() - scaled.width()) / 2,
                                      (height() - scaled.height()) / 2), scaled);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(destination, m_frame);
    } else {
        painter.setPen(QColor("#92a5b7"));
        painter.drawText(rect().adjusted(16, 16, -16, -16), Qt::AlignCenter | Qt::TextWordWrap,
                         m_message.isEmpty() ? QStringLiteral("Open a video to preview") : m_message);
    }
}
