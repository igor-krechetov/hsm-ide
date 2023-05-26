#include "QImageViewArea.hpp"

QImageViewArea::QImageViewArea(QWidget* parent)
    : QScrollArea(parent), mIsDragging(false)
{
}

void QImageViewArea::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        setCursor(Qt::OpenHandCursor);
        mPrevMousePosition = event->pos();
        mIsDragging = true;
    }
    else
    {
        QScrollArea::mousePressEvent(event);
    }
}

void QImageViewArea::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        setCursor(Qt::ArrowCursor);
        mIsDragging = false;
    }
    else
    {
        QScrollArea::mouseReleaseEvent(event);
    }
}

void QImageViewArea::mouseMoveEvent(QMouseEvent* event)
{
    QScrollArea::mouseMoveEvent(event);
    if (mIsDragging)
    {
        QPoint delta = event->pos() - mPrevMousePosition;
        mPrevMousePosition = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }
}
