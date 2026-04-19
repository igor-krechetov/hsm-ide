#include "HsmGraphicsView.hpp"

#include <QDebug>
#include <QDrag>
#include <QGraphicsRectItem>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include "controllers/ProjectController.hpp"
#include "view/common/ViewUtils.hpp"
#include "view/elements/ElementTypeIds.hpp"
#include "view/elements/HsmElementsFactory.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/HsmElement.hpp"
#include "view/elements/private/HsmResizableElement.hpp"
#include "view/theme/ThemeManager.hpp"

HsmGraphicsView::HsmGraphicsView(QWidget* parent)
    : QGraphicsView(parent) {
    setAcceptDrops(true);
    // setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setRubberBandSelectionMode(Qt::ContainsItemBoundingRect);
    setDragMode(QGraphicsView::RubberBandDrag);
    // setSceneRect(-500, -500, 1000, 1000);
    // scale(0.3, 0.3);
    // translate(100, 0);

    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        if (scene() != nullptr) {
            scene()->update();
        }

        viewport()->update();
    });
}

HsmGraphicsView::~HsmGraphicsView() {
    qDebug() << "DELETE HsmGraphicsView:" << this;
}

QWeakPointer<ProjectController> HsmGraphicsView::projectController() const {
    return mProjectController;
}

void HsmGraphicsView::setProjectController(const QWeakPointer<ProjectController>& controller) {
    mProjectController = controller;

    // qDebug() << "--- INIT";
    // view::HsmElement* newElement = new view::HsmStateElement();
    // qDebug() << "--- CREATE" << newElement;
    // // QGraphicsRectItem* child = new QGraphicsRectItem(QRect(10, 10, 40, 40));
    // view::HsmElement* child = new view::HsmStateElement();
    // qDebug() << "--- CREATE" << child;

    // newElement->init(nullptr);

    // scene()->addItem(newElement);
    // qDebug() << "add to parent";
    // // child->setParentItem(newElement);
    // newElement->addChildItem(child);
    // qDebug() << "DONE";

#ifdef DEBUG_RENDERING
    // and two crossed lines at the 0,0 point using graphics items
    QGraphicsLineItem* hLine = new QGraphicsLineItem(-1000, 0, 1000, 0);
    QGraphicsLineItem* vLine = new QGraphicsLineItem(0, -1000, 0, 1000);
    const auto& theme = ThemeManager::instance().theme();
    QPen pen(theme.grid.majorLinePen);
    hLine->setPen(pen);
    vLine->setPen(pen);
    scene()->addItem(hLine);
    scene()->addItem(vLine);
#endif  // DEBUG_RENDERING
}

view::HsmElement* HsmGraphicsView::createHsmElement(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                    const QString& elementTypeId,
                                                    const QPointF& pos,
                                                    const QSizeF& size,
                                                    const model::EntityID_t parentElementId) {
    view::HsmElement* newElement = view::HsmElementsFactory::createElement(elementTypeId, modelElement, size);
    view::HsmElement* parentElement = findHsmElement(parentElementId);

    qDebug() << "HsmGraphicsView::createHsmElement"
             << "ID=" << newElement->modelId() << ", pos=" << pos << "size" << size;
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
    connect(newElement, &view::HsmElement::elementDoubleClickEvent, this, &HsmGraphicsView::hsmElementDoubleClickEvent);
    connect(newElement, &view::HsmElement::elementDoubleClickEvent, this, &HsmGraphicsView::hsmElementDoubleClickEvent);
    connect(this, &HsmGraphicsView::transformChanged, newElement, &view::HsmElement::viewTransformChanged);

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

void HsmGraphicsView::clearAllHsmElements() {
    qDebug() << Q_FUNC_INFO;
    for (auto it = mElements.begin(); it != mElements.end(); ++it) {
        delete *it;
    }

    mElements.clear();
}

void HsmGraphicsView::moveHsmElement(const model::EntityID_t elementId, const model::EntityID_t newParentId) {
    // qDebug() << "--- HsmGraphicsView::moveHsmElement: elementId" << elementId << " -> " << newParentId;

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

void HsmGraphicsView::selectAllHsmElements() {
    if (nullptr != scene()) {
        scene()->clearSelection();

        for (auto it = mElements.begin(); it != mElements.end(); ++it) {
            if (nullptr != it.value()) {
                it.value()->setSelected(true);
            }
        }
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
    if (auto controller = mProjectController.toStrongRef()) {
        controller->handleDeleteElements(getSelectedElements());
    }
}

void HsmGraphicsView::clearSelection() {
    scene()->clearSelection();
}

void HsmGraphicsView::zoomIn() {
    applyScale(1.15);
    emit transformChanged();
}

void HsmGraphicsView::zoomOut() {
    applyScale(1.0 / 1.15);
    emit transformChanged();
}

void HsmGraphicsView::resetZoom() {
    setTransform(QTransform());
    emit transformChanged();
}

void HsmGraphicsView::fitSceneToView() {
    if ((nullptr != scene()) && (false == mElements.isEmpty())) {
        QRectF boundingRect;
        bool first = true;

        for (auto it = mElements.begin(); it != mElements.end(); ++it) {
            if (it.value()) {
                QRectF itemRect = it.value()->sceneBoundingRect();
                if (first) {
                    boundingRect = itemRect;
                    first = false;
                } else {
                    boundingRect = boundingRect.united(itemRect);
                }
            }
        }

        if (!first && boundingRect.isValid()) {
            fitInView(boundingRect, Qt::KeepAspectRatio);
            emit transformChanged();
        }
    }
}

bool HsmGraphicsView::keyboardShiftPressed() const {
    return mKeyboardModifiers & ShiftModifier;
}

bool HsmGraphicsView::keyboardSpacePressed() const {
    return mKeyboardModifiers & SpaceModifier;
}

bool HsmGraphicsView::keyboardCtrlPressed() const {
    return mKeyboardModifiers & ControlModifier;
}

bool HsmGraphicsView::keyboardAltPressed() const {
    return mKeyboardModifiers & AltModifier;
}

bool HsmGraphicsView::keyboardModifierPressed(const KeyboardModifier modifier) const {
    return mKeyboardModifiers & modifier;
}

bool HsmGraphicsView::keyboardReparentModifierPressed() const {
    return keyboardModifierPressed(KeyboardModifier::R_Modifier);
}

void HsmGraphicsView::focusOutEvent(QFocusEvent* event) {
    QGraphicsView::focusOutEvent(event);
}

bool HsmGraphicsView::handleElementDragEvent(const QPointF& scenePos, view::HsmElement* element, const QString& mimetype) {
    bool elementAllowed = true;
    QPointer<view::HsmElement> targetElement;
    view::HsmElementType sourceElementType = view::HsmElementType::UNKNOWN;

    if (mimetype.isEmpty() == false) {
        sourceElementType = view::HsmElementsFactory::elementTypeFromTypeId(mimetype);
    } else if (nullptr != element) {
        sourceElementType = element->elementType();
    }

    targetElement = view::ViewUtils::topHsmElementAt(scene(), scenePos, false, false, true, mimetype.isEmpty(), element, sourceElementType);

    qDebug() << "--- handleElementDragEvent: " << scenePos << "elem=" << (element ? element->modelId() : 0) << "mimetype"
             << mimetype << (int)sourceElementType << "target=" << (targetElement ? targetElement->modelId() : 0)
             << "mDragTargetElement=" << (mDragTargetElement ? mDragTargetElement->modelId() : 0);

    if (targetElement != mDragTargetElement) {
        if (nullptr != mDragTargetElement) {
            mDragTargetElement->hightlight(false);
        }

        // Check if the element under cursor accepts children of specific type
        // if (nullptr != targetElement && targetElement->acceptsElement(sourceElementType) == true) {
        mDragTargetElement = targetElement;

        if (nullptr != mDragTargetElement) {
            mDragTargetElement->hightlight(true);
        }
        // TODO: check if element is allowed to be top level
    }

    if (nullptr == mDragTargetElement) {
        // if we don't have a valid target element, check if element is allowed to be on a top level
        switch (sourceElementType) {
            case view::HsmElementType::INITIAL:
            case view::HsmElementType::FINAL:
            case view::HsmElementType::STATE:
            case view::HsmElementType::INCLUDE:
                // Allow dropping these elements on top level
                // qDebug() << "--- Allow dropping these elements on top level";
                break;
            case view::HsmElementType::ENTRY_POINT:
            case view::HsmElementType::EXIT_POINT:
            case view::HsmElementType::TRANSITION:
            case view::HsmElementType::HISTORY:
            default:
                // Other element types cannot be dropped on top level
                // qDebug() << "--- Other element types cannot be dropped on top level";
                elementAllowed = false;
                break;
        }
    }

    return elementAllowed;
}

bool HsmGraphicsView::handleElementDropEvent(view::HsmElement* element, const QPointF& scenePos) {
    qDebug() << "handleElementDropEvent";
    if (nullptr != mDragTargetElement) {
        mDragTargetElement->hightlight(false);
    }

    // Re-select child elements which were deselected during drag operation
    for (view::HsmElement* childElement : mDraggedChildElements) {
        childElement->setSelected(true);
    }

    mDragTargetElement = nullptr;
    mDraggedElement = nullptr;
    mDragRevertPositions.clear();
    mDraggedChildElements.clear();

    QGuiApplication::restoreOverrideCursor();

    // TODO: implement
    return true;
}

/*

Drag Use-Cases:

* - selection
# - dragged element

OK S* -> S
OK S{e*} -> S{e}                    XXX
OK S{S*} -> S S
OK S S* -> S{S}
OK S*{S*} -> S{S}
OK S*{e*} -> S{e}
OK S*{e*} S* -> S{e} S
OK    #S1
OK    #e
OK    #S2
OK S{e*} S* -> S{e} S               XXX
OK    #e
OK    #S
OK S{ S* S* } -> S S S
OK S{ e* S* } -> S{e} S             XXX
OK    #e
OK    #S
OK S{e*} S* S -> S S{e S}
OK    #e
OK    #S
OK S{e1*} S{e2*} S -> S S{e1 e2}
OK    #e1
OK    #e2
OK S{e*} S{S*} -> S{e*} S S*        XXX
OK    #e
OK    #S
OK S{S*{e*}} S -> S S{S*{e*}}
OK    #S
OK*   #e    -> block moving the elements outside of the parent
OK S{e*} S S* -> S{e*} S{S*}
OK    #S
OK    #e

*/

// NOTE: this is called only for the element under cursor (even if multiple elements are dragged)
void HsmGraphicsView::dragElementBegin(view::HsmElement* element, const QPointF& scenePos) {
    qDebug() << Q_FUNC_INFO << "id=" << element->modelId() << "type=" << (int)element->elementType() << element << scenePos;

    mDraggedChildElements.clear();
    mDragRevertPositions.clear();

    // TODO: there is still a small bug remaining.
    //   A { B }
    //   C {}
    // 1. Select all 3 and drag A or C
    // 2. Without cancelling selection, start dragging using B
    // Observed: A nd C get deselected, only B is dragged
    // Rootcause: it's because B got deselected during first drag operation
    // Solution: keep a list of deselected elements and restore selection after dragging is done

    // Store position for primary dragged element
    mDragRevertPositions.insert(element, element->pos());

    forEachSelectedElement([&](view::HsmElement* parentSelectedElement) {
        forEachSelectedElement([&](view::HsmElement* childSelectedElement) {
            if (parentSelectedElement != childSelectedElement) {
                if (parentSelectedElement->containsChild(childSelectedElement) == false) {
                    qDebug() << "---- BEGIN DRAGGING, id=" << childSelectedElement->modelId()
                             << "type=" << (int)childSelectedElement->elementType() << childSelectedElement;

                    // store position for secondary dragged element
                    mDragRevertPositions.insert(childSelectedElement, childSelectedElement->pos());

                    // activate group drag mode for all secondary selected elements
                    if (childSelectedElement != element) {
                        childSelectedElement->setGroupDragMode(true);
                    }
                } else {
                    qDebug() << "--- found child. add to skip list" << childSelectedElement;
                    mDraggedChildElements.push_back(childSelectedElement);
                }
            }
        });
    });

    // Unselect child elements. They will be dragged within their parent context
    for (view::HsmElement* childElement : mDraggedChildElements) {
        childElement->setSelected(false);
    }

    mDraggedElement = element;

    qDebug() << "mDraggedElement->parentItem" << mDraggedElement->hsmParentItem();
    qDebug() << "keyboardReparentModifierPressed" << keyboardReparentModifierPressed();

    // TODO:
    /*
        A { B C }
        B - state
        C - entry point

        When selecting both B and C it should not be possible to move them outside of A
    */
    qDebug() << "dragElementBegin: DRAG";

    if ((mDraggedElement->hsmParentItem() == nullptr) || keyboardReparentModifierPressed()) {
        QGuiApplication::setOverrideCursor(Qt::DragMoveCursor);
        mDraggedElement->setDragMode(true);
    } else {
        QGuiApplication::setOverrideCursor(Qt::OpenHandCursor);
        forEachSelectedElement([&](view::HsmElement* element) { element->setDragMode(false); });
    }

    if (auto controller = mProjectController.toStrongRef()) {
        controller->beginHistoryTransaction("Drag elements");
    }
}

void HsmGraphicsView::dragElementEvent(view::HsmElement* element, const QPointF& scenePos) {
    if (handleElementDragEvent(scenePos, element)) {
        QGuiApplication::changeOverrideCursor(Qt::DragMoveCursor);
    } else {
        QGuiApplication::changeOverrideCursor(Qt::ForbiddenCursor);
    }
}

void HsmGraphicsView::dropElementEvent(view::HsmElement* element, const QPointF& scenePos) {
    qDebug() << Q_FUNC_INFO << element << scenePos
             << "target: " << (mDragTargetElement == nullptr ? model::INVALID_MODEL_ID : mDragTargetElement->modelId());
    // TODO: block drag(move) events if element is not allowed to be dropped here
    //       if element is not allowed, we need to return it to it's original position -> need to store that position
    //       when dragging starts

    if (auto controller = mProjectController.toStrongRef()) {
        forEachSelectedElement([&](view::HsmElement* selectedElement) {
            qDebug() << "----- dropElementEvent::selectedElement" << selectedElement;
            const bool elementDraggingAllowed = handleElementDragEvent(scenePos, selectedElement);

            if (true == elementDraggingAllowed) {
                // element's position is updated every time move event is triggered. since we only care about
                // the final position we mark it manually instead of triggering model change notifications
                controller->markHistoryElement(selectedElement->modelId());

                // qDebug() << "------ dropElementEvent:ALLOWED";
                view::HsmResizableElement* currentParent = elementToHsmResizableElement(selectedElement->hsmParentItem());

                selectedElement->setGroupDragMode(false);

                if ((nullptr != selectedElement) && (selectedElement->hsmParentItem() == nullptr) ||
                    keyboardReparentModifierPressed()) {
                    qDebug() << "------ dropElementEvent:ALLOWED: NEW PARENT or TOP";
                    // If we are dragging element into a new parent or to a top level
                    // const auto targetElementId = (mDragTargetElement == nullptr ? model::INVALID_MODEL_ID :
                    // mDragTargetElement->modelId());

                    if ((nullptr != mDragTargetElement) && (currentParent == mDragTargetElement) &&
                        (mDragTargetElement->containsChild(selectedElement) == true)) {
                        qDebug() << "---- NO MOVE, JUST RESIZE PARENT";
                        currentParent->resizeToFitChildItem(selectedElement);
                    } else {
                        qDebug() << "---- MOVE TO NEW PARENT";
                        controller->handleViewMoveEvent(
                            selectedElement->modelId(),
                            (mDragTargetElement == nullptr ? model::INVALID_MODEL_ID : mDragTargetElement->modelId()));
                    }
                } else {
                    qDebug() << "------ dropElementEvent:ALLOWED: INTERNALLY" << selectedElement->modelId();
                    // if we a moving element within current parent or top level
                }

                // NOTE: has to be after disconnecting the child from old parrent (if applicable)
                //       otherwise previous parent gets resized incorrectly
                if (nullptr != currentParent) {
                    // NOTE: need to normalize parent bounding rect and position since it might have gone negative
                    currentParent->normalizeElementRect();
                }
            } else {
                qDebug() << "------ dropElementEvent:BLOCK ELEMENT" << selectedElement;
                auto itPos = mDragRevertPositions.find(selectedElement);

                if (itPos != mDragRevertPositions.end()) {
                    if (nullptr != itPos.key()) {
                        itPos.key()->setPos(itPos.value());
                    }
                }

                // since we reset the element, unmark it so it doesnt affect history
                controller->unmarkHistoryElement(selectedElement->modelId());
            }
        });

        controller->commitHistoryTransaction();
    }

    handleElementDropEvent(nullptr, scenePos);
}

void HsmGraphicsView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("hsm/element")) {
        qDebug() << "HsmGraphicsView::dragEnterEvent:" << event->source() << "," << event->mimeData()->formats();
        qDebug() << event->mimeData()->data("hsm/element");
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void HsmGraphicsView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("hsm/element")) {
        const bool elementAllowed =
            handleElementDragEvent(mapToScene(event->position().toPoint()), nullptr, event->mimeData()->data("hsm/element"));

        if (true == elementAllowed) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        } else {
            event->ignore();
        }
    }
}

void HsmGraphicsView::dropEvent(QDropEvent* event) {
    qDebug() << "HsmGraphicsView::dropEvent"
             << "mDragTargetElement=" << mDragTargetElement << "pos" << event->position() << "scenePos"
             << mapToScene(event->position().toPoint());
    // event contains position of the drop in the local coordinate system of the receiving widget
    const QPointF scenePos = mapToScene(event->position().toPoint());

    if (auto controller = mProjectController.toStrongRef()) {
        scene()->clearSelection();
        const QPointF parentPos = (mDragTargetElement == nullptr ? scenePos : mDragTargetElement->mapFromSceneToBody(scenePos));

        controller->handleViewDropEvent(
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
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void HsmGraphicsView::applyScale(const double scaleFactor) {
    scale(scaleFactor, scaleFactor);
}

void HsmGraphicsView::keyPressEvent(QKeyEvent* event) {
    QGraphicsView::keyPressEvent(event);

    // If the scene didn't accept it, we can handle it here if needed
    if (!event->isAccepted()) {
        if (false == event->isAutoRepeat() && event->key() == Qt::Key_Space) {
            qDebug() << "PRESS: " << event->key() << "   " << event->isAutoRepeat();
            mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers | HsmGraphicsView::SpaceModifier);
            QGuiApplication::setOverrideCursor(Qt::OpenHandCursor);
            event->accept();
        } else if (false == event->isAutoRepeat() && event->key() == Qt::Key_Control) {
            qDebug() << "PRESS: CTRL";

            mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers | HsmGraphicsView::ControlModifier);
            qDebug() << mKeyboardModifiers;
        } else if (false == event->isAutoRepeat() && event->key() == Qt::Key_R) {
            qDebug() << "PRESS: R";
            if (mDraggedElement && mDraggedElement->hsmParentItem() != nullptr) {
                QGuiApplication::changeOverrideCursor(Qt::DragMoveCursor);
                forEachSelectedElement([&](view::HsmElement* element) { element->setDragMode(true); });

                if (mDragTargetElement) {
                    mDragTargetElement->hightlight(true);
                }
            }

            mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers | HsmGraphicsView::R_Modifier);
            qDebug() << mKeyboardModifiers;
        }
    }
}

void HsmGraphicsView::keyReleaseEvent(QKeyEvent* event) {
    if (false == event->isAutoRepeat() && event->key() == Qt::Key_Space) {
        qDebug() << "RELEASE: " << event->key() << " " << event->isAutoRepeat();
        mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers & ~HsmGraphicsView::SpaceModifier);
        setPanningMode(false);
        QGuiApplication::restoreOverrideCursor();
        event->accept();
    } else if (false == event->isAutoRepeat() && event->key() == Qt::Key_Control) {
        qDebug() << "RELEASE: CTRL";
        mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers & ~HsmGraphicsView::ControlModifier);
        QGraphicsView::keyReleaseEvent(event);
    } else if (false == event->isAutoRepeat() && event->key() == Qt::Key_R) {
        qDebug() << "RELEASE: R";
        if (mDraggedElement && mDraggedElement->hsmParentItem() != nullptr) {
            qDebug() << "---- QGuiApplication::restoreOverrideCursor()";
            QGuiApplication::restoreOverrideCursor();
            forEachSelectedElement([&](view::HsmElement* element) { element->setDragMode(false); });

            if (mDragTargetElement) {
                mDragTargetElement->hightlight(false);
            }
        }
        mKeyboardModifiers = static_cast<KeyboardModifier>(mKeyboardModifiers & ~HsmGraphicsView::R_Modifier);
        QGraphicsView::keyReleaseEvent(event);
    } else {
        QGraphicsView::keyReleaseEvent(event);
    }
}

void HsmGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        setPanningMode(true);
        mLastPanPoint = event->pos();
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

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());
        mLastPanPoint = event->pos();
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }

#ifdef DEBUG_RENDERING
    emit mouseMoved(mapToScene(event->pos()));
#endif
}

void HsmGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
    if (mPanning && event->button() == Qt::MiddleButton) {
        setPanningMode(false);
        event->accept();
    } else if (mPanning && event->button() == Qt::LeftButton && keyboardSpacePressed()) {
        setPanningMode(false);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

/*
    Hold Space              : open hand         (push x1)
    Hold Space -> Hold LMB  : closed hand       (push x2)
    Release Space           : normal            (pop x2)
    Release LMB             : open hand         (pop x1)

    Hold MMB                : closed hand       (push x1)
    Release MMB             : normal            (pop x1)
*/
void HsmGraphicsView::setPanningMode(const bool enable) {
    qDebug() << "setPanningMode: " << enable;
    mPanning = enable;

    if (true == mPanning) {
        QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
    } else {
        QGuiApplication::restoreOverrideCursor();
    }
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

    if ((nullptr != item) && IS_HSM_ELEMENT_TYPE(item->type())) {
        // NOTE: because QGraphicsObject is a child to both QObject and QGraphicsItem it's crucial
        //       to use dynamic_cast. parentItem() != parentObject()
        element = dynamic_cast<view::HsmElement*>(item);
    }

    return element;
}

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
