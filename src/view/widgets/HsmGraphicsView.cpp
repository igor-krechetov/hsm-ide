#include "HsmGraphicsView.hpp"

#include <QDebug>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>

#include "controllers/ProjectController.hpp"
#include "view/elements/HsmElementsFactory.hpp"

HsmGraphicsView::HsmGraphicsView(QWidget* parent)
    : QGraphicsView(parent) {
    setAcceptDrops(true);
    // setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setRubberBandSelectionMode(Qt::ContainsItemBoundingRect);
    setDragMode(QGraphicsView::RubberBandDrag);
    // setSceneRect(-500, -500, 1000, 1000);
    // scale(0.3, 0.3);
    // translate(100, 0);
}

void HsmGraphicsView::setProjectController(QSharedPointer<ProjectController> controller) {
    mProjectController = controller;
}

void HsmGraphicsView::createHsmElement(const QString& elementId, const QPoint& pos) {
    HsmElement* e1 = HsmElementsFactory::createElement(elementId);

    e1->setPos(mapToScene(pos));
    scene()->addItem(e1);
}

void HsmGraphicsView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("hsm/element")) {
        qDebug() << "dragEnterEvent:" << event->source() << "," << event->mimeData()->formats();
        qDebug() << event->mimeData()->data("hsm/element");
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void HsmGraphicsView::dragMoveEvent(QDragMoveEvent* event) {
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void HsmGraphicsView::dropEvent(QDropEvent* event) {
    if (mProjectController) {
        mProjectController->handleViewDropEvent(event);
    }
}

void HsmGraphicsView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom in/out
        double scaleFactor = 1.15;
        if (event->angleDelta().y() > 0) {
            scale(scaleFactor, scaleFactor);
        } else {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void HsmGraphicsView::keyPressEvent(QKeyEvent* event) {
    if (false == event->isAutoRepeat() && event->key() == Qt::Key_Space) {
        qDebug() << "PRESS: " << event->key() << "   " << event->isAutoRepeat();
        mSpacePressed = true;
        viewport()->setCursor(Qt::OpenHandCursor);
        event->accept();
        // if (!event->isAutoRepeat() && event->key() == Qt::Key_Delete) {
        //     // Emit signal to request deletion
        //     emit deleteItemsRequested();
        //     event->accept();
        //     return;
        // }
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void HsmGraphicsView::keyReleaseEvent(QKeyEvent* event) {
    if (false == event->isAutoRepeat() && event->key() == Qt::Key_Space) {
        qDebug() << "RELEASE: " << event->key() << " " << event->isAutoRepeat();
        mSpacePressed = false;
        setPanningMode(false);
        event->accept();
    } else {
        QGraphicsView::keyReleaseEvent(event);
    }
}

void HsmGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        setPanningMode(true);
        event->accept();
    } else if (event->button() == Qt::LeftButton && mSpacePressed) {
        setPanningMode(true);
        mLastPanPoint = event->pos();
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void HsmGraphicsView::mouseMoveEvent(QMouseEvent* event) {
    if (mPanning) {
        QPointF delta = mLastPanPoint - event->pos();

        // viewport()->setCursor(Qt::ClosedHandCursor);
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());
        mLastPanPoint = event->pos();
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void HsmGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
    if (mPanning && event->button() == Qt::MiddleButton) {
        setPanningMode(false);
        event->accept();
    } else if (mPanning && event->button() == Qt::LeftButton && mSpacePressed) {
        setPanningMode(false);
        viewport()->setCursor(Qt::OpenHandCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void HsmGraphicsView::setPanningMode(const bool enable) {
    qDebug() << "setPanningMode: " << enable;
    mPanning = enable;
    viewport()->setCursor(true == mPanning ? Qt::ClosedHandCursor : Qt::ArrowCursor);
}
