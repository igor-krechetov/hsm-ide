#ifndef QIMAGEVIEWAREA_HPP
#define QIMAGEVIEWAREA_HPP

#include <QScrollArea>
#include <QPoint>

class QMouseEvent;

class QImageViewArea : public QScrollArea
{
public:
    QImageViewArea(QWidget* parent = nullptr);
    virtual ~QImageViewArea() = default;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    bool mIsDragging = false;
    QPoint mPrevMousePosition;
};

#endif // QIMAGEVIEWAREA_HPP
