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
#include "view/common/ViewUtils.hpp"
#include "view/elements/HsmElementsFactory.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/HsmElement.hpp"
#include "view/elements/private/HsmResizableElement.hpp"

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
view::HsmElement* HsmGraphicsView::createHsmElement(const model::EntityID_t modelElementId,
                                                    const QString& elementTypeId,
                                                    const QPoint& pos,
                                                    const model::EntityID_t parentElementId) {
    view::HsmElement* newElement = view::HsmElementsFactory::createElement(elementTypeId, modelElementId);
    view::HsmElement* parentElement = findHsmElement(parentElementId);

    qDebug() << Q_FUNC_INFO << "ID=" << newElement->modelId() << ", pos=" << mapToScene(pos);
    qDebug() << Q_FUNC_INFO << newElement;
    newElement->setPos(mapToScene(pos));

    if (nullptr != parentElement) {
        newElement->setParentItem(parentElement);
        newElement->setPos(parentElement->mapFromScene(mapToScene(pos)));

        // Resize parent item to fit new child if necessary
        auto* resizableParent = dynamic_cast<view::HsmResizableElement*>(parentElement);
        if (resizableParent) {
            resizableParent->resizeToFitChildItem(newElement);
            // // Get child bounding rectangle in parent coordinates
            // QRectF childRect = newElement->mapRectToParent(newElement->boundingRect());

            // // Get parent's current rectangle
            // QRectF parentRect = resizableParent->boundingRect();

            // // qDebug() << Q_FUNC_INFO << newElement->boundingRect() << " --> ELEM: " << newElement->childrenBoundingRect();
            // // qDebug() << Q_FUNC_INFO << parentRect << resizableParent->childrenBoundingRect();

            // // Calculate required size to fit child
            // // qreal requiredWidth = qMax(parentRect.width(), childRect.right() - parentRect.left());
            // // qreal requiredHeight = qMax(parentRect.height(), childRect.bottom() - parentRect.top());

            // // parentRect.setLeft(qMin(parentRect.left(), childRect.left() - 5));
            // // parentRect.setTop(qMin(parentRect.top(), childRect.top() - 5));
            // // parentRect.setRight(qMax(parentRect.right(), childRect.right() + 5));
            // // parentRect.setBottom(qMax(parentRect.bottom(), childRect.bottom() + 5));
            // QRectF parentNewRect = parentRect.united(childRect);

            // // parentRect = resizableParent->childrenBoundingRect();

            // if (parentRect != parentNewRect) {
            //     qDebug() << Q_FUNC_INFO << resizableParent->boundingRect() << " --> " << parentRect;

            //     if ( parentRect.left() != parentNewRect.left() ) {
            //         parentNewRect.adjust(-5, 0, 0, 0);
            //     }
            //     if ( parentRect.top() != parentNewRect.top() ) {
            //         parentNewRect.adjust(0, -5, 0, 0);
            //     }
            //     if ( parentRect.right() != parentNewRect.right() ) {
            //         parentNewRect.adjust(0, 0, 5, 0);
            //     }
            //     if ( parentRect.bottom() != parentNewRect.bottom() ) {
            //         parentNewRect.adjust(0, 0, 0, 5);
            //     }

            //     // Expand parent if needed
            //     // if (requiredWidth > parentRect.width() || requiredHeight > parentRect.height()) {
            //         // Update parent size by moving the bottom-right grip
            //         // QPointF newSizePoint(parentRect.left() + requiredWidth, parentRect.top() + requiredHeight);

            //         // parentRect.setWidth(requiredWidth);
            //         // parentRect.setHeight(requiredHeight);
            //         resizableParent->resizeElement(parentNewRect);
            //         // parentElement->onGripMoved(
            //         //     nullptr, // No specific grip selected
            //         //     newSizePoint
            //         // );

            //         // Emit geometry changed signal
            //         // emit resizableParent->geometryChanged(resizableParent);
            //     // }
            // }


        }
    } else {
        scene()->addItem(newElement);
    }

    mElements[modelElementId] = QPointer(newElement);

    return newElement;
}

// TODO: do we need to return the value?
view::HsmTransition* HsmGraphicsView::createHsmTransition(const model::EntityID_t transitionId,
                                                          const model::EntityID_t fromElementId,
                                                          const model::EntityID_t toElementId) {
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

void HsmGraphicsView::reconnectHsmTransition(const model::EntityID_t transitionId,
                                             const model::EntityID_t fromElementId,
                                             const model::EntityID_t toElementId) {
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

void HsmGraphicsView::focusOutEvent(QFocusEvent* event) {
    QGraphicsView::focusOutEvent(event);
    scene()->clearSelection();
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
    QPointF pos = mapToScene(event->position().toPoint());
    QPointer<view::HsmElement> targetElement = view::ViewUtils::topHsmElementAt(scene(), pos, false, true);

    qDebug() << Q_FUNC_INFO << pos << targetElement << mDragTargetElement;

    if (targetElement != mDragTargetElement) {
        if (nullptr != mDragTargetElement) {
            mDragTargetElement->hightlight(false);
        }

        mDragTargetElement = targetElement;

        if (nullptr != mDragTargetElement) {
            mDragTargetElement->hightlight(true);
        }
    }

    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void HsmGraphicsView::dropEvent(QDropEvent* event) {
    qDebug() << Q_FUNC_INFO << "mDragTargetElement=" << mDragTargetElement;

    if (nullptr != mDragTargetElement) {
        mDragTargetElement->hightlight(false);
    }

    if (mProjectController) {
        mProjectController->handleViewDropEvent(
            event,
            (mDragTargetElement == nullptr ? model::INVALID_MODEL_ID : mDragTargetElement->modelId()));
    }

    mDragTargetElement = nullptr;
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

// TODO: has almost identical implementaion with HsmElement::connectableElementAt
// view::HsmElement* HsmGraphicsView::acceptsChildrenElementAt(const QPointF& pos) const {
//     view::HsmElement* element = nullptr;

//     if (scene() != nullptr) {
//         QList<QGraphicsItem*> targetItems = scene()->items(pos);

//         // TODO: account for subitems
//         for (auto targetItem : targetItems) {
//             if (nullptr != targetItem) {
//                 QVariant elementType = targetItem->data(view::USERDATA_HSM_ELEMENT_TYPE);

//                 if (elementType.isValid() && elementType.toInt() != static_cast<int>(view::HsmElementType::UNKNOWN)) {
//                     // NOTE: because QGraphicsObject is a child to both QObject and QGraphicsItem it's crucial
//                     //       to use dynamic_cast. parentItem() != parentObject()
//                     element = dynamic_cast<view::HsmElement*>(targetItem);

//                     if (nullptr != element) {
//                         if (element->acceptsChildren() == false) {
//                             qDebug() << "Target doesn't accept children: " << element->modelId() << " | viewElementType=" <<
//                             elementType
//                                     << " | " << element;
//                             element = nullptr;
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     return element;
// }

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
