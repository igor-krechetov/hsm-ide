#include "HsmElement.hpp"

#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>

namespace view {

HsmElement::HsmElement(const HsmElementType elementType)
    : QGraphicsObject()
    , mSize(200.0, 40.0)
    , mType(elementType)
    , mPenHighlightMode(QColor("#7AE7C7"), 3.0, Qt::DotLine) {
    qDebug() << "CREATE: HsmElement: " << (int)elementType << ": " << this;
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
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
//                             qDebug() << "Target is not connectable: " << element->modelId() << " | viewElementType=" << elementType
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

        mOuterRect = QRectF(-mSize.width() / 2 - penWidth / 2,
                            -mSize.height() / 2 - penWidth / 2,
                            mSize.width() + penWidth,
                            mSize.height() + penWidth);
    } else {
        qDebug() << Q_FUNC_INFO << "RECT UPDATED: " << mOuterRect << " --> " << newRect;
        mOuterRect = newRect;
        qDebug() << Q_FUNC_INFO << "RECT UPDATED: " << mOuterRect << " --> " << newRect;
        mSize = mOuterRect.size();
    }
}

void HsmElement::forEachChildElement(std::function<void(HsmElement*)> callback) {
    for (QGraphicsItem* child : childItems()) {
        QVariant userType = child->data(USERDATA_HSM_ELEMENT_TYPE);

        if (userType.isValid()) {
            qgraphicsitem_cast<HsmElement*>(child)->forEachChildElement(callback);
            callback(qgraphicsitem_cast<HsmElement*>(child));
        }
    }
}

QRectF HsmElement::boundingRect() const {
    return mOuterRect;
}

void HsmElement::dragEnterEvent(QGraphicsSceneDragDropEvent* event) {
    qDebug() << "ITEM: dragEnterEvent: " << event->source() << ", <" << event->mimeData()->formats() << ">";
    QGraphicsObject::dragEnterEvent(event);
}

void HsmElement::dropEvent(QGraphicsSceneDragDropEvent* event) {
    qDebug() << "ITEM: dropEvent: " << event->source() << ", <" << event->mimeData()->formats() << ">";
    QGraphicsObject::dropEvent(event);
}

};  // namespace view