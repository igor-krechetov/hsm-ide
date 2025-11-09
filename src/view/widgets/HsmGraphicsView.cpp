#include "HsmGraphicsView.hpp"

#include <QDebug>
#include <QDrag>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include "controllers/ProjectController.hpp"
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

#ifdef DEBUG_RENDERING
    // and two crossed lines at the 0,0 point using graphics items
    QGraphicsLineItem* hLine = new QGraphicsLineItem(-1000, 0, 1000, 0);
    QGraphicsLineItem* vLine = new QGraphicsLineItem(0, -1000, 0, 1000);
    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::DashLine);
    hLine->setPen(pen);
    vLine->setPen(pen);
    scene()->addItem(hLine);
    scene()->addItem(vLine);
#endif // DEBUG_RENDERING
}

view::HsmElement* HsmGraphicsView::createHsmElement(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                    const QString& elementTypeId,
                                                    const QPointF& pos,
                                                    const QSizeF& size,
                                                    const model::EntityID_t parentElementId) {
    view::HsmElement* newElement = view::HsmElementsFactory::createElement(elementTypeId, modelElement, size);
    view::HsmElement* parentElement = findHsmElement(parentElementId);

    qDebug() << "HsmGraphicsView::createHsmElement" << "ID=" << newElement->modelId() << ", pos=" << pos << "size" << size;
    qDebug() << "HsmGraphicsView::createHsmElement" << newElement;
    newElement->setPos(pos);

    if (nullptr != parentElement) {
        newElement->setHsmParentItem(parentElement);
        newElement->setPos(pos);

        // Resize parent item to fit new child if necessary
        auto* resizableParent = dynamic_cast<view::HsmResizableElement*>(parentElement);

        if (resizableParent) {
            resizableParent->resizeToFitChildItem(newElement);
        }
    } else {
        scene()->addItem(newElement);
    }

    // connect element events
    connect(newElement, &view::HsmElement::dragElementBegin, this, &HsmGraphicsView::dragElementBegin);
    connect(newElement, &view::HsmElement::dragElementEvent, this, &HsmGraphicsView::dragElementEvent);
    connect(newElement, &view::HsmElement::dropElementEvent, this, &HsmGraphicsView::dropElementEvent);

    mElements[modelElement->id()] = QPointer(newElement);

    return newElement;
}

// TODO: do we need to return the value?
view::HsmTransition* HsmGraphicsView::createHsmTransition(const QSharedPointer<model::StateMachineEntity>& transition,
                                                          const model::EntityID_t fromElementId,
                                                          const model::EntityID_t toElementId) {
    qDebug() << Q_FUNC_INFO;
    view::HsmTransition* transitionElement = nullptr;
    QPointer<view::HsmElement> fromElement = findHsmElement(fromElementId);
    QPointer<view::HsmElement> toElement = findHsmElement(toElementId);

    if (nullptr != fromElement && nullptr != toElement) {
        transitionElement = new view::HsmTransition();
        transitionElement->init(transition);
        transitionElement->connectElements(fromElement, toElement);
        mElements[transition->id()] = QPointer(transitionElement);
        qDebug() << "transition added to view";
    } else {
        qWarning() << "Cant find elements with ID " << fromElementId << " or " << toElementId;
    }

    return transitionElement;
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

void HsmGraphicsView::moveHsmElement(const model::EntityID_t elementId, const model::EntityID_t newParentId) {
    if (elementId != newParentId) {
        view::HsmElement* element = findHsmElement(elementId);

        if (nullptr != element) {
            QPointF oldPos = element->scenePos();
            view::HsmElement* newParent = findHsmElement(newParentId);

            if (nullptr != newParent) {
                auto newPos = newParent->mapFromSceneToBody(oldPos);

                element->setHsmParentItem(newParent);
                element->setPos(newPos);

                auto* resizableParent = dynamic_cast<view::HsmResizableElement*>(newParent);

                if (resizableParent) {
                    resizableParent->resizeToFitChildItem(element);
                }
            } else {
                element->setHsmParentItem(nullptr);
                element->setPos(oldPos);
            }
        }
    } else {
        qCritical() << Q_FUNC_INFO << "elementId == newParentId";
    }
}

void HsmGraphicsView::selectHsmElement(const model::EntityID_t id) {
    auto element = findHsmElement(id);

    scene()->clearSelection();

    if (element) {
        element->setSelected(true);
    }
}

QList<model::EntityID_t> HsmGraphicsView::getSelectedElements() const {
    auto items = scene()->selectedItems();
    QList<model::EntityID_t> selectedHsmElements;

    for (QGraphicsItem* item : items) {
        view::HsmElement* ptr = itemToHsmElement(item);

        if (nullptr != ptr) {
            selectedHsmElements.append(ptr->modelId());
        }
    }

    return selectedHsmElements;
}

void HsmGraphicsView::deleteSelectedItems() {
    // TODO: need to re-think how to handle transition
    //       What to do when one of the states is deleted? Delete transition too?
    //       Do we want to allow dangling transitions?
    mProjectController->handleDeleteElements(getSelectedElements());
}

bool HsmGraphicsView::keyboardShiftPressed() const {
    return mKeyboardModifiers & ShiftModifier;
}

bool HsmGraphicsView::keyboardSpacePressed() const {
    return mKeyboardModifiers & SpaceModifier;
}

bool HsmGraphicsView::keyboardCtrlPressed() const {
    qDebug() << "keyboardCtrlPressed: " << (bool)(mKeyboardModifiers & ControlModifier);
    return mKeyboardModifiers & ControlModifier;
}

bool HsmGraphicsView::keyboardAltPressed() const {
    return mKeyboardModifiers & AltModifier;
}

void HsmGraphicsView::focusOutEvent(QFocusEvent* event) {
    QGraphicsView::focusOutEvent(event);
    scene()->clearSelection();
}

void HsmGraphicsView::handleElementDragEvent(view::HsmElement* element, const QPointF& scenePos) {
    QPointer<view::HsmElement> targetElement = view::ViewUtils::topHsmElementAt(scene(), scenePos, false, false, true, element);

    // qDebug() << "handleElementDragEvent: " << scenePos << "elem=" << (element ? element->modelId() : -1)
    //          << "target=" << (targetElement ? targetElement->modelId() : -1);

    if (targetElement != mDragTargetElement) {
        if (nullptr != mDragTargetElement) {
            mDragTargetElement->hightlight(false);
        }

        mDragTargetElement = targetElement;

        if (nullptr != mDragTargetElement) {
            mDragTargetElement->hightlight(true);
        }
    }
}

void HsmGraphicsView::handleElementDropEvent(view::HsmElement* element, const QPointF& scenePos) {
    qDebug() << "handleElementDropEvent";
    if (nullptr != mDragTargetElement) {
        mDragTargetElement->hightlight(false);
    }

    mDragTargetElement = nullptr;
    mDraggedElement = nullptr;
}

// NOTE: this is called only for the element under cursor (even if multiple elements are dragged)
void HsmGraphicsView::dragElementBegin(view::HsmElement* element, const QPointF& scenePos) {
    qDebug() << Q_FUNC_INFO << element << scenePos;
    forEachSelectedElement([element](view::HsmElement* selectedElement) {
        if (selectedElement != element) {
            selectedElement->setGroupDragMode(true);
        }
    });

    mDraggedElement = element;

    qDebug() << "mDraggedElement->parentItem" << mDraggedElement->hsmParentItem();
    qDebug() << "keyboardCtrlPressed" << keyboardCtrlPressed();

    if ((mDraggedElement->hsmParentItem() == nullptr) || keyboardCtrlPressed()) {
        qDebug() << "dragElementBegin: DRAG";
        QGuiApplication::setOverrideCursor(Qt::DragMoveCursor);
        mDraggedElement->setDragMode(true);
    } else {
        forEachSelectedElement([&](view::HsmElement* element) { element->setDragMode(false); });
    }
}

void HsmGraphicsView::dragElementEvent(view::HsmElement* element, const QPointF& scenePos) {
    // qDebug() << Q_FUNC_INFO << element << scenePos;
    handleElementDragEvent(element, scenePos);
}

void HsmGraphicsView::dropElementEvent(view::HsmElement* element, const QPointF& scenePos) {
    qDebug() << Q_FUNC_INFO << element << scenePos
             << "target: " << (mDragTargetElement == nullptr ? model::INVALID_MODEL_ID : mDragTargetElement->modelId());

    if (mDraggedElement && (mDraggedElement->hsmParentItem() == nullptr) || keyboardCtrlPressed()) {
        // If we are dragging element into a new parent or to a top level
        if (mProjectController) {
            qDebug() << "mDragTargetElement" << mDragTargetElement;
            forEachSelectedElement([&](view::HsmElement* element) {
                qDebug() << element->modelId();
                // TODO: fix parentItem() call
                view::HsmResizableElement* currentParent = elementToHsmResizableElement(element->hsmParentItem());

                if (nullptr != currentParent) {
                    // NOTE: need to normalize parent bounding rect and position since it might have gone negative
                    currentParent->normalizeElementRect();
                }

                // NOTE: no need to do it for the target because it will be done automatically in handleViewMoveEvent

                element->setGroupDragMode(false);
                mProjectController->handleViewMoveEvent(
                    element->modelId(),
                    (mDragTargetElement == nullptr ? model::INVALID_MODEL_ID : mDragTargetElement->modelId()));
            });
        }

        qDebug() << "---- QGuiApplication::restoreOverrideCursor()";
        // TODO: there seems to be a case when setOverrideCursor() was called twice or restoreOverrideCursor() is not called
        QGuiApplication::restoreOverrideCursor();
    } else {
        // if we a moving element within current parent or top level
        forEachSelectedElement([&](view::HsmElement* element) {
            view::HsmResizableElement* currentParent = elementToHsmResizableElement(element->hsmParentItem());

            element->setGroupDragMode(false);

            if (nullptr != currentParent) {
                // NOTE: need to normalize parent bounding rect and position since it might have gone negative
                currentParent->normalizeElementRect();
            }
        });
    }

    handleElementDropEvent(nullptr, scenePos);
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
    handleElementDragEvent(nullptr, mapToScene(event->position().toPoint()));

    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void HsmGraphicsView::dropEvent(QDropEvent* event) {
    qDebug() << "HsmGraphicsView::dropEvent" << "mDragTargetElement=" << mDragTargetElement << "pos" << event->position() << "scenePos" << mapToScene(event->position().toPoint());
    // event contains position of the drop in the local coordinate system of the receiving widget
    const QPointF scenePos = mapToScene(event->position().toPoint());

    if (mProjectController) {
        const QPointF parentPos = (mDragTargetElement == nullptr ? scenePos : mDragTargetElement->mapFromSceneToBody(scenePos));

        mProjectController->handleViewDropEvent(
            event->mimeData()->data("hsm/element").data(),
            parentPos,
            (mDragTargetElement == nullptr ? model::INVALID_MODEL_ID : mDragTargetElement->modelId()));
    }

    handleElementDropEvent(nullptr, scenePos);

    event->setDropAction(Qt::CopyAction);
    event->accept();
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
        mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers | HsmGraphicsView::SpaceModifier);
        viewport()->setCursor(Qt::OpenHandCursor);
        event->accept();
        // if (!event->isAutoRepeat() && event->key() == Qt::Key_Delete) {
        //     // Emit signal to request deletion
        //     emit deleteItemsRequested();
        //     event->accept();
        //     return;
        // }
    } else if (false == event->isAutoRepeat() && event->key() == Qt::Key_Control) {
        qDebug() << "PRESS: CTRL";
        if (mDraggedElement && mDraggedElement->hsmParentItem() != nullptr) {
            qDebug() << "---- QGuiApplication::setOverrideCursor(Qt::DragMoveCursor)";
            QGuiApplication::setOverrideCursor(Qt::DragMoveCursor);
            forEachSelectedElement([&](view::HsmElement* element) { element->setDragMode(true); });

            if (mDragTargetElement) {
                mDragTargetElement->hightlight(true);
            }
        }

        mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers | HsmGraphicsView::ControlModifier);
        qDebug() << mKeyboardModifiers;
        QGraphicsView::keyPressEvent(event);
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void HsmGraphicsView::keyReleaseEvent(QKeyEvent* event) {
    if (false == event->isAutoRepeat() && event->key() == Qt::Key_Space) {
        qDebug() << "RELEASE: " << event->key() << " " << event->isAutoRepeat();
        mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers & ~HsmGraphicsView::SpaceModifier);
        setPanningMode(false);
        event->accept();
    } else if (false == event->isAutoRepeat() && event->key() == Qt::Key_Control) {
        qDebug() << "RELEASE: CTRL";
        if (mDraggedElement && mDraggedElement->hsmParentItem() != nullptr) {
            qDebug() << "---- QGuiApplication::restoreOverrideCursor()";
            QGuiApplication::restoreOverrideCursor();
            forEachSelectedElement([&](view::HsmElement* element) { element->setDragMode(false); });

            if (mDragTargetElement) {
                mDragTargetElement->hightlight(false);
            }
        }
        mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers & ~HsmGraphicsView::ControlModifier);
        QGraphicsView::keyReleaseEvent(event);
    } else {
        QGraphicsView::keyReleaseEvent(event);
    }
}

void HsmGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        setPanningMode(true);
        event->accept();
    } else if (event->button() == Qt::LeftButton && keyboardSpacePressed()) {
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
    } else if (mPanning && event->button() == Qt::LeftButton && keyboardSpacePressed()) {
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

// view::HsmResizableElement* HsmGraphicsView::itemToHsmResizableElement(QGraphicsItem* item) const {
//     view::HsmResizableElement* resizableElement = nullptr;
//     view::HsmElement* element = itemToHsmElement(item);

//     if (nullptr != element && element->isResizable()) {
//         resizableElement = dynamic_cast<view::HsmResizableElement*>(element);
//     }

//     return resizableElement;
// }

view::HsmResizableElement* HsmGraphicsView::elementToHsmResizableElement(view::HsmElement* element) const {
    view::HsmResizableElement* resizableElement = nullptr;

    if (nullptr != element && element->isResizable()) {
        resizableElement = dynamic_cast<view::HsmResizableElement*>(element);
    }

    return resizableElement;
}

void HsmGraphicsView::forEachSelectedElement(std::function<void(view::HsmElement*)> callback) {
    auto items = scene()->selectedItems();
    QList<model::EntityID_t> selectedHsmElements;

    for (QGraphicsItem* item : items) {
        view::HsmElement* ptr = itemToHsmElement(item);

        if (nullptr != ptr) {
            callback(ptr);
        }
    }
}
