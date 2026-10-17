#pragma once

#include <QWidget>

class TimelineWidget final : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget *parent = nullptr);
    void setDuration(qint64 duration);
    void setPosition(qint64 position);
    void setMarks(qint64 in, qint64 out);
    QSize sizeHint() const override { return {700, 58}; }
signals:
    void seekRequested(qint64 ms);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
private:
    qint64 hitPosition(double x) const;
    qreal xFor(qint64 ms) const;
    qint64 m_duration = 0;
    qint64 m_position = 0;
    qint64 m_in = 0;
    qint64 m_out = 0;
    bool m_dragging = false;
    qreal m_hover = -1;
};
