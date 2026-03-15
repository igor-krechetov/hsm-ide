#include "HsmElement.hpp"

#include <QApplication>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QMimeData>

#include "HsmResizableElement.hpp"
#include "model/StateMachineEntity.hpp"
#include "ui/theme/ThemeManager.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

namespace view {

HsmElement::HsmElement(const HsmElementType elementType, const QSizeF& size)
    : HsmElement(elementType, nullptr, size) {}

HsmElement::HsmElement(const HsmElementType elementType,
                       const QSharedPointer<model::StateMachineEntity>& modelElement,
                       const QSizeF& size)
    : QGraphicsObject()
    , mType(elementType)
    , mModelElement(modelElement.toWeakRef()) {
    qDebug() << "CREATE: HsmElement: " << (int)elementType << ": " << this;
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setZValue(3);
    setData(USERDATA_HSM_ELEMENT_TYPE, static_cast<int>(mType));
    updateBoundingRect(QRect(0, 0, size.width(), size.height()));
}

HsmElement::~HsmElement() {
    qDebug() << "DELETE: HsmElement: " << this;
}

model::EntityID_t HsmElement::modelId() const {
    model::EntityID_t id = model::INVALID_MODEL_ID;
    auto modelElementPtr = mModelElement.toStrongRef();

    if (modelElementPtr) {
        id = modelElementPtr->id();
    }

    return id;
}

void HsmElement::hightlight(const bool enable) {
    qDebug() << Q_FUNC_INFO << enable;
    mHightlight = enable;
    update();
}

bool HsmElement::isHighligted() const {
    return mHightlight;
}

void HsmElement::setGroupDragMode(const bool enableGroup) {
    qDebug() << "setGroupDragMode: " << modelId();
    mDragMode = (enableGroup ? DragMode::GROUP : DragMode::NONE);
}

void HsmElement::setDragMode(const bool dragging) {
    if (DragMode::GROUP == mDragMode || DragMode::GROUP_NONE == mDragMode) {
        mDragMode = (dragging ? DragMode::GROUP : DragMode::GROUP_NONE);
    } else {
        mDragMode = (dragging ? DragMode::SINGLE : DragMode::NONE);
    }

    qDebug() << "HsmElement::setDragMode: " << dragging << (int)mDragMode << isDragged() << (int)mDragState;
}

bool HsmElement::isDragged() const {
    return ((DragState::DRAGGING == mDragState && DragMode::SINGLE == mDragMode) || DragMode::GROUP == mDragMode);
}

// void HsmElement::setModelId(const model::EntityID_t modelElementId) {
//     mModelElementId = modelElementId;
// }

HsmElementType HsmElement::elementType() const {
    return mType;
}

QRectF HsmElement::elementRect() const {
    return mOuterRect;
}

QPointF HsmElement::mapFromSceneToBody(const QPointF& point) const {
    return mapFromScene(point);
}

QWeakPointer<model::StateMachineEntity> HsmElement::modelElementPtr() const {
    return mModelElement;
}

void HsmElement::init(const QSharedPointer<model::StateMachineEntity>& modelEntity) {
    mModelElement = modelEntity.toWeakRef();
    updateBoundingRect();

    // Subscribe to modelDataChanged signal
    if (modelEntity) {
        QObject::connect(modelEntity.data(),
                         &model::StateMachineEntity::modelDataChanged,
                         this,
                         &HsmElement::onModelDataChanged);
    }
}

bool HsmElement::isConnectable() const {
    return false;
}

bool HsmElement::acceptsConnections() const {
    return false;
}

bool HsmElement::isResizable() const {
    return false;
}

// HsmElement* HsmElement::connectableElementAt(const QPointF& pos) const {
//     HsmElement* element = nullptr;

//     if (scene() != nullptr) {
//         QList<QGraphicsItem*> targetItems = scene()->items(pos);

//         // TODO: account for subitems
//         for (auto targetItem : targetItems) {
//             if (nullptr != targetItem) {
//                 QVariant elementType = targetItem->data(USERDATA_HSM_ELEMENT_TYPE);

//                 if (elementType.isValid() && elementType.toInt() != static_cast<int>(view::HsmElementType::UNKNOWN)) {
//                     // NOTE: because QGraphicsObject is a child to both QObject and QGraphicsItem it's crucial
//                     //       to use dynamic_cast. parentItem() != parentObject()
//                     element = dynamic_cast<HsmElement*>(targetItem);

//                     if (nullptr != element) {
//                         if (element->isConnectable() == false) {
//                             qDebug() << "Target is not connectable: " << element->modelId() << " | viewElementType=" <<
//                             elementType
//                                     << " | " << element;
//                             element = nullptr;
//                         } else {
//                             break;
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     return element;
// }

bool HsmElement::canBeTopLevel() const {
    return false;
}

bool HsmElement::acceptsChildElement(const HsmElementType type) const {
    return false;
}

bool HsmElement::acceptsChildElement(HsmElement* element) const {
    return (nullptr != element ? element->acceptsChildElement(element->elementType()) : false);
}

QList<QGraphicsItem*> HsmElement::hsmChildItems() const {
    return childItems();
}

bool HsmElement::isDirectChild(HsmElement* child) const {
    return hsmChildItems().contains(child);
}

bool HsmElement::containsChild(HsmElement* child) const {
    bool res = false;

    // qDebug() << "======= CHILDREN COUNT" << hsmChildItems().size();

    for (QGraphicsItem* item : hsmChildItems()) {
        if (auto element = qgraphicsitem_cast<HsmElement*>(item)) {
            // qDebug() << "====== parent" << this << "child X" << element;
            if ((child == element) || (element->containsChild(child) == true)) {
                res = true;
                break;
            }
        }
    }

    return res;
}

QRectF HsmElement::childrenRect() const {
    QRectF rect;

    for (QGraphicsItem* child : hsmChildItems()) {
        QVariant userType = child->data(USERDATA_HSM_ELEMENT_TYPE);

        if (userType.isValid()) {
            rect = rect.united(child->mapRectToParent(child->boundingRect()));
        }
    }

    return rect;
}

bool HsmElement::hasSubstates() const {
    // so far only HsmStateElement can have substates
    return false;
}

void HsmElement::addChildItem(HsmElement* child) {
    if (nullptr != child) {
        child->setParentItem(this);
    }
}

void HsmElement::removeChildItem(HsmElement* child) {
    qDebug() << "HsmElement::removeChildItem" << child;
    if (nullptr != child) {
        child->setParentItem(nullptr);
    }
}

void HsmElement::setHsmParentItem(HsmElement* parent) {
    if (nullptr != parent) {
        parent->addChildItem(this);
    } else {
        // reset parent
        setParentItem(nullptr);
    }

    mHsmParent = parent;
}

bool HsmElement::onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos) {
    // TODO: impl
    return true;
}

void HsmElement::updateBoundingRect(const QRectF& newRect) {
    prepareGeometryChange();

    if (newRect.isNull()) {
        qDebug() << Q_FUNC_INFO << "RECT is NULL" << this;
        const qreal penWidth = ThemeManager::instance().theme().node.selectedBorderPen.widthF();

        // mOuterRect = QRectF(-mSize.width() / 2 - penWidth / 2,
        //                     -mSize.height() / 2 - penWidth / 2,
        //                     mSize.width() + penWidth,
        //                     mSize.height() + penWidth);
        mOuterRect = QRectF(0, 0, mSize.width() + penWidth * 2, mSize.height() + penWidth * 2);
    } else {
        mOuterRect = newRect;
        mSize = mOuterRect.size();
        qDebug() << "----------------- RECT changed" << modelId();
    }

    // update model element metadata
    auto ptrElement = mModelElement.toStrongRef();
    if (ptrElement) {
        ptrElement->setSize(mOuterRect.size().toSize());
    }
}

void HsmElement::setElementType(const HsmElementType newType) {
    mType = newType;
}

HsmGraphicsView* HsmElement::hsmView() const {
    HsmGraphicsView* view = nullptr;

    if (scene() != nullptr) {
        view = dynamic_cast<HsmGraphicsView*>(scene()->views().first());
    }

    return view;
}

void HsmElement::forEachHsmChildElement(std::function<void(HsmElement*)> callback, const int depth) {
    for (QGraphicsItem* child : hsmChildItems()) {
        QVariant userType = child->data(USERDATA_HSM_ELEMENT_TYPE);

        if (userType.isValid()) {
            if (depth == DEPTH_INFINITE || depth > 1) {
                qgraphicsitem_cast<HsmElement*>(child)->forEachHsmChildElement(callback,
                                                                               (depth != DEPTH_INFINITE ? depth - 1 : depth));
            }
            callback(qgraphicsitem_cast<HsmElement*>(child));
        }
    }
}

QRectF HsmElement::boundingRect() const {
    return mOuterRect;
}

void HsmElement::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    emit elementDoubleClickEvent(mModelElement);
    event->accept();
}

void HsmElement::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mousePressEvent(event);

    mDragState = DragState::PREPARE;
}

void HsmElement::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mouseReleaseEvent(event);

    if (DragState::DRAGGING == mDragState) {
        emit dropElementEvent(this, event->scenePos());
    }

    mDragState = DragState::NONE;
    mDragMode = DragMode::NONE;
}

QVariant HsmElement::itemChange(const GraphicsItemChange change, const QVariant& value) {
    if (QGraphicsItem::ItemPositionHasChanged == change) {
        if (DragMode::SINGLE == mDragMode || DragState::PREPARE == mDragState) {
            QGraphicsView* view = scene()->views().first();
            auto itemPos = view->mapToScene(view->mapFromGlobal(QCursor::pos()));

            if (DragState::PREPARE == mDragState) {
                mDragState = DragState::DRAGGING;
                emit dragElementBegin(this, itemPos);
            } else {
                emit dragElementEvent(this, itemPos);
            }
        }

        if (isDragged() == false) {
            // if item is moved inside of the parent element
            resizeParentToFitChildItem();
        }

        // update model element metadata
        auto ptrElement = mModelElement.toStrongRef();
        if (ptrElement) {
            ptrElement->setPos(pos());
        }

        notifyGeometryChanged();

        forEachHsmChildElement([&](HsmElement* child) {
            if (nullptr != child) {
                child->notifyGeometryChanged();
            }
        });
    }

    return QGraphicsItem::itemChange(change, value);
}

void HsmElement::dragEnterEvent(QGraphicsSceneDragDropEvent* event) {
    qDebug() << "ITEM: dragEnterEvent: " << event->source() << ", <" << event->mimeData()->formats() << ">";
    QGraphicsObject::dragEnterEvent(event);
}

void HsmElement::dropEvent(QGraphicsSceneDragDropEvent* event) {
    qDebug() << "ITEM: dropEvent: " << event->source() << ", <" << event->mimeData()->formats() << ">";
    QGraphicsObject::dropEvent(event);
}

void HsmElement::notifyGeometryChanged() {
    emit geometryChanged(this);
}

void HsmElement::resizeParentToFitChildItem() {
    auto parent = dynamic_cast<HsmResizableElement*>(hsmParentItem().get());

    // TODO: check if parent is also resizable
    if (parent != nullptr) {
        parent->resizeToFitChildItem(this);
    }
}

// Slot implementation
void HsmElement::onModelDataChanged() {
    qDebug() << "---- HsmElement::onModelDataChanged";
    // Default: redraw element
    update();
}

};  // namespace view
