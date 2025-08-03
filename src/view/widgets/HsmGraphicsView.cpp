#include "HsmGraphicsView.hpp"

#include <QDebug>
#include <QDrag>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include "controllers/ProjectController.hpp"
#include "model/StateMachineEntity.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"
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

void HsmGraphicsView::setProjectController(QPointer<ProjectController> controller) {
    mProjectController = controller;
}

// TODO: how can we add sub elements?
view::HsmElement* HsmGraphicsView::createHsmElement(const model::EntityID_t modelElementId, const QString& elementTypeId, const QPoint& pos) {
    view::HsmElement* newElement = view::HsmElementsFactory::createElement(elementTypeId, modelElementId);

    qDebug() << Q_FUNC_INFO << "ID=" << newElement->modelId() << ", pos=" << mapToScene(pos);
    qDebug() << Q_FUNC_INFO << newElement;
    newElement->setPos(mapToScene(pos));
    scene()->addItem(newElement);
    mElements[modelElementId] = QPointer(newElement);

    return newElement;
}

// TODO: do we need to return the value?
view::HsmTransition* HsmGraphicsView::createHsmTransition(const model::EntityID_t transitionId, const model::EntityID_t fromElementId, const model::EntityID_t toElementId) {
    qDebug() << Q_FUNC_INFO;
    view::HsmTransition* transition = nullptr;
    QPointer<view::HsmElement> fromElement = findHsmElement(fromElementId);
    QPointer<view::HsmElement> toElement = findHsmElement(toElementId);

    // TODO: does QPointer have bool() operator?
    if (fromElement && toElement) {
        transition = new view::HsmTransition();

        transition->init(transitionId);
        transition->connectElements(fromElement, toElement);
        // TODO: how do we add it to substates?
        scene()->addItem(transition);
        mElements[transitionId] = QPointer(transition);
        qDebug() << "transition added to view";
    } else {
        qWarning() << "Cant find elements with ID " << fromElementId << " or " << toElementId;
    }

    return transition;
}

void HsmGraphicsView::reconnectHsmTransition(const model::EntityID_t transitionId, const model::EntityID_t fromElementId, const model::EntityID_t toElementId) {
    qDebug() << Q_FUNC_INFO;
    QPointer<view::HsmTransition> transition = findHsmTransition(transitionId);

    if (false == transition.isNull()) {
        QPointer<view::HsmElement> fromElement = findHsmElement(fromElementId);
        QPointer<view::HsmElement> toElement = findHsmElement(toElementId);

        if (fromElement.isNull() == false && toElement.isNull() == false) {
            transition->connectElements(fromElement, toElement);
            qDebug() << "transition updated";
        } else {
            qWarning() << "Cant find elements with ID " << fromElementId << " or " << toElementId;
        }
    }
}

void HsmGraphicsView::deleteHsmElement(const model::EntityID_t modelElementId) {
    auto it = mElements.find(modelElementId);

    if (it != mElements.end()) {
        delete *it;
        mElements.erase(it);
    }
}

void HsmGraphicsView::deleteSelectedItems() {
    // TODO: need to re-think how to handle transition
    //       What to do when one of the states is deleted? Delete transition too?
    //       Do we want to allow dangling transitions?
    auto items = scene()->selectedItems();
    QList<model::EntityID_t> selectedHsmElements;

    for (QGraphicsItem* item : items) {
        view::HsmElement* ptr = itemToHsmElement(item);

        if (nullptr != ptr) {
            selectedHsmElements.append(ptr->modelId());
        }
    }

    mProjectController->handleDeleteElements(selectedHsmElements);
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

QPointer<view::HsmElement> HsmGraphicsView::findHsmElement(const model::EntityID_t id) const {
    auto it = mElements.find(id);
    return (it != mElements.end() ? *it : nullptr);
}

QPointer<view::HsmTransition> HsmGraphicsView::findHsmTransition(const model::EntityID_t id) const {
    QPointer<view::HsmTransition> res;
    QPointer<view::HsmElement> ptr = findHsmElement(id);

    if (ptr && ptr->elementType() == view::HsmElementType::TRANSITION) {
        res = dynamic_cast<view::HsmTransition*>(ptr.data());
    }

    return res;
}

view::HsmElement* HsmGraphicsView::itemToHsmElement(QGraphicsItem* item) const {
    view::HsmElement* element = nullptr;

    if (nullptr != item) {
        QVariant elementType = item->data(view::USERDATA_HSM_ELEMENT_TYPE);

        if (elementType.isValid() && elementType.toInt() != static_cast<int>(view::HsmElementType::UNKNOWN)) {
            // NOTE: because QGraphicsObject is a child to both QObject and QGraphicsItem it's crucial
            //       to use dynamic_cast. parentItem() != parentObject()
            element = dynamic_cast<view::HsmElement*>(item);
        }
    }

    return element;
}