#pragma once

#include <QImage>
#include <QWidget>

// Paint decoded frames in the normal Qt Widgets backing store, avoiding
// QVideoWidget's platform-specific native video surface on KDE Wayland.
class VideoSurface final : public QWidget {
public:
    explicit VideoSurface(QWidget *parent = nullptr);
    void setFrame(const QImage &frame);
    void clearFrame(const QString &message = {});
    bool hasFrame() const { return !m_frame.isNull(); }
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QImage m_frame;
    QString m_message;
};
