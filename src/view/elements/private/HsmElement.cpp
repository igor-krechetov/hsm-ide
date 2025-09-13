#include "HsmElement.hpp"

#include <QApplication>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QMimeData>

#include "HsmResizableElement.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

namespace view {

HsmElement::HsmElement(const HsmElementType elementType)
    : QGraphicsObject()
    , mSize(200.0, 40.0)
    , mType(elementType)
    , mPenHighlightMode(QColor("#7AE7C7"), 3.0, Qt::DotLine) {
    qDebug() << "CREATE: HsmElement: " << (int)elementType << ": " << this;
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setZValue(3);
    setData(USERDATA_HSM_ELEMENT_TYPE, static_cast<int>(mType));
}

HsmElement::HsmElement(const HsmElementType elementType, const model::EntityID_t modelElementId)
    : HsmElement(elementType) {
    mModelElementId = modelElementId;
    // setModelId(modelElementId);
    qDebug() << "CREATE: HsmElement (2): " << (int)elementType << ": " << this;
}

HsmElement::~HsmElement() {
    qDebug() << "DELETE: HsmElement: " << this;
}

model::EntityID_t HsmElement::modelId() const {
    return mModelElementId;
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

void HsmElement::init(const model::EntityID_t modelElementId) {
    mModelElementId = modelElementId;
    updateBoundingRect();
}

bool HsmElement::isConnectable() const {
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

bool HsmElement::acceptsChildren() const {
    return false;
}

bool HsmElement::isDirectChild(HsmElement* item) const {
    return childItems().contains(item);
}

QRectF HsmElement::childrenRect() const {
    QRectF rect;

    for (QGraphicsItem* child : childItems()) {
        QVariant userType = child->data(USERDATA_HSM_ELEMENT_TYPE);

        if (userType.isValid()) {
            rect = rect.united(child->mapRectToParent(child->boundingRect()));
        }
    }

    return rect;
}

bool HsmElement::onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos) {
    // TODO: impl
    return true;
}

void HsmElement::updateBoundingRect(const QRectF& newRect) {
    prepareGeometryChange();

    if (newRect.isNull()) {
        qDebug() << Q_FUNC_INFO << "RECT is NULL";
        constexpr qreal penWidth = 1.0;

        // mOuterRect = QRectF(-mSize.width() / 2 - penWidth / 2,
        //                     -mSize.height() / 2 - penWidth / 2,
        //                     mSize.width() + penWidth,
        //                     mSize.height() + penWidth);
        mOuterRect = QRectF(0, 0, mSize.width() + penWidth * 2, mSize.height() + penWidth * 2);
    } else {
        mOuterRect = newRect;
        mSize = mOuterRect.size();
    }
}

HsmGraphicsView* HsmElement::hsmView() const {
    HsmGraphicsView* view = nullptr;

    if (scene() != nullptr) {
        view = dynamic_cast<HsmGraphicsView*>(scene()->views().first());
    }

    return view;
}

void HsmElement::forEachChildElement(std::function<void(HsmElement*)> callback, const int depth) {
    for (QGraphicsItem* child : childItems()) {
        QVariant userType = child->data(USERDATA_HSM_ELEMENT_TYPE);

        if (userType.isValid()) {
            if (depth == DEPTH_INFINITE || depth > 1) {
                qgraphicsitem_cast<HsmElement*>(child)->forEachChildElement(callback,
                                                                            (depth != DEPTH_INFINITE ? depth - 1 : depth));
            }
            callback(qgraphicsitem_cast<HsmElement*>(child));
        }
    }
}

QRectF HsmElement::boundingRect() const {
    return mOuterRect;
}

void HsmElement::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << Q_FUNC_INFO << this;
    QGraphicsItem::mousePressEvent(event);

    mDragState = DragState::PREPARE;
}

void HsmElement::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << Q_FUNC_INFO;
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

        qDebug() << "HsmElement::itemChange: isDragged" << isDragged();

        if (isDragged() == false) {
            // if item is moved inside of the parent element
            resizeParentToFitChildItem();
        }

        notifyGeometryChanged();

        forEachChildElement([&](HsmElement* child) {
            if (child) {
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
    auto parent = dynamic_cast<HsmResizableElement*>(parentItem());

    // TODO: check if parent is also resizable
    if (parent != nullptr) {
        parent->resizeToFitChildItem(this);
    }
}

};  // namespace view