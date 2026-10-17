#include "timelinewidget.h"
#include "mediaengine.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumHeight(58);
    setCursor(Qt::PointingHandCursor);
}
void TimelineWidget::setDuration(qint64 value) { m_duration = qMax(qint64(0), value); update(); }
void TimelineWidget::setPosition(qint64 value) { m_position = qBound(qint64(0), value, m_duration); update(); }
void TimelineWidget::setMarks(qint64 in, qint64 out) {
    m_in = qBound(qint64(0), in, m_duration);
    m_out = qBound(qint64(0), out, m_duration);
    update();
}
qreal TimelineWidget::xFor(qint64 ms) const {
    if (!m_duration) return 18;
    return 18 + (width() - 36) * qBound(0.0, static_cast<double>(ms) / m_duration, 1.0);
}
qint64 TimelineWidget::hitPosition(double x) const {
    if (!m_duration) return 0;
    return qRound64(m_duration * qBound(0.0, (x - 18) / qMax(1.0, width() - 36.0), 1.0));
}
void TimelineWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const qreal left = 18, widthBar = width() - 36, y = 18, h = 13;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#29313d"));
    p.drawRoundedRect(QRectF(left, y, widthBar, h), 6, 6);
    if (m_duration) {
        const qreal inx = xFor(m_in), outx = xFor(m_out), posx = xFor(m_position);
        p.setBrush(QColor("#355f67"));
        p.drawRoundedRect(QRectF(inx, y, qMax(0.0, outx - inx), h), 5, 5);
        p.setBrush(QColor("#23ccb1"));
        p.drawRoundedRect(QRectF(left, y, qMax(0.0, posx - left), h), 5, 5);
        p.setPen(QPen(QColor("#79e4d4"), 2));
        p.drawLine(QPointF(inx, y - 3), QPointF(inx, y + h + 3));
        p.setPen(QPen(QColor("#ffca88"), 2));
        p.drawLine(QPointF(outx, y - 3), QPointF(outx, y + h + 3));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f7fafc"));
        p.drawEllipse(QPointF(posx, y + h / 2), 7, 7);
        if (m_hover >= 0) {
            const qreal hx = qBound(left, m_hover, left + widthBar);
            p.setBrush(QColor("#cbd5e1"));
            p.drawEllipse(QPointF(hx, y + h / 2), 2.5, 2.5);
        }
        p.setPen(QColor("#8c9cae"));
        const QFontMetrics fontMetrics(p.font());
        p.drawText(QRectF(left, 40, 150, 14), Qt::AlignLeft, "00:00:00");
        p.drawText(QRectF(left + widthBar - 150, 40, 150, 14), Qt::AlignRight, MediaEngine::timecode(m_duration).left(8));
        p.drawText(QRectF(left, 40, widthBar, 14), Qt::AlignCenter, QString("IN %1    •    OUT %2")
                       .arg(MediaEngine::timecode(m_in).left(8), MediaEngine::timecode(m_out).left(8)));
    }
}
void TimelineWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton || !m_duration) return;
    m_dragging = true;
    emit seekRequested(hitPosition(e->position().x()));
}
void TimelineWidget::mouseMoveEvent(QMouseEvent *e) {
    m_hover = e->position().x();
    if (m_dragging) emit seekRequested(hitPosition(m_hover));
    update();
}
void TimelineWidget::mouseReleaseEvent(QMouseEvent *) { m_dragging = false; }
void TimelineWidget::leaveEvent(QEvent *) { m_hover = -1; update(); }
