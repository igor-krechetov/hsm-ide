#include "HsmElement.hpp"

#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>

namespace view {

HsmElement::HsmElement(const HsmElementType elementType)
    : QGraphicsObject()
    , mSize(200.0, 40.0)
    , mType(elementType) {
    qDebug() << "CREATE: HsmElement: " << (int)elementType << ": " << this;
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
    setZValue(3);
    setData(USERDATA_HSM_ELEMENT_TYPE, static_cast<int>(mType));
}

HsmElement::HsmElement(const HsmElementType elementType, const uint32_t modelElementId)
    : HsmElement(elementType) {
    setModelId(modelElementId);
}

HsmElement::~HsmElement() {
    qDebug() << "DELETE: HsmElement: " << this;
}

uint32_t HsmElement::modelId() const {
    return mModelElementId;
}

void HsmElement::setModelId(const uint32_t modelElementId) {
    mModelElementId = modelElementId;
}

HsmElementType HsmElement::elementType() const {
    return mType;
}

QRectF HsmElement::elementRect() const {
    return mOuterRect;
}

void HsmElement::init() {
    updateBoundingRect();
}

bool HsmElement::isConnectable() const {
    return false;
}

bool HsmElement::onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos) {
    // TODO: impl
    return true;
}

void HsmElement::updateBoundingRect(const QRectF& newRect) {
    prepareGeometryChange();

    if (newRect.isNull()) {
        constexpr qreal penWidth = 1.0;

        mOuterRect = QRectF(-mSize.width() / 2 - penWidth / 2,
                            -mSize.height() / 2 - penWidth / 2,
                            mSize.width() + penWidth,
                            mSize.height() + penWidth);
    } else {
        mOuterRect = newRect;
        mSize = mOuterRect.size();
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

}; // namespace view