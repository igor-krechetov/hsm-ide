#ifndef QIMAGEVIEWAREA_HPP
#define QIMAGEVIEWAREA_HPP

#include <QScrollArea>
#include <QMouseEvent>
#include <QCursor>

class QImageViewArea : public QScrollArea
{
public:
    QImageViewArea(QWidget* parent = nullptr);
    virtual ~QImageViewArea() = default;

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    bool mIsDragging;
    QPoint mPrevMousePosition;
};

#endif // QIMAGEVIEWAREA_HPP
