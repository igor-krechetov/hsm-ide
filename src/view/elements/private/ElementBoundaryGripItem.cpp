#include "ElementBoundaryGripItem.hpp"

#include "HsmResizableElement.hpp"
// #include <QGraphicsSceneMouseEvent>
// #include <QPainter>
#include <QDebug>

#include "view/elements/ElementTypeIds.hpp"

namespace view {

ElementBoundaryGripItem::~ElementBoundaryGripItem() {
    qDebug() << "DELETE: " << this;
}

ElementBoundaryGripItem::ElementBoundaryGripItem(HsmResizableElement* annotationElement, const GripDirection type)
    : ElementGripItem(annotationElement, type) {
    if ((GripDirection::North == direction()) || (GripDirection::South == direction())) {
        mGripDirectionType = GripDirectionType::Vertical;
    } else if ((GripDirection::West == direction()) || (GripDirection::East == direction())) {
        mGripDirectionType = GripDirectionType::Horizontal;
    } else if ((GripDirection::NorthEast == direction()) || (GripDirection::NorthWest == direction()) ||
               (GripDirection::SouthWest == direction()) || (GripDirection::SouthEast == direction())) {
        mGripDirectionType = GripDirectionType::Diagonal;
    }
}

HsmResizableElement* ElementBoundaryGripItem::annotationElement() const {
    return reinterpret_cast<HsmResizableElement*>(ElementGripItem::annotationElement());
}

GripDirectionType ElementBoundaryGripItem::directionType() const {
    return mGripDirectionType;
}

QVariant ElementBoundaryGripItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    QVariant res;

    res = ElementGripItem::itemChange(change, value);

    if (QGraphicsItem::ItemSelectedHasChanged == change) {
        if (false == isSelected()) {
            emit onGripLostFocus(this);
        }
    }

    return res;
}

void ElementBoundaryGripItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    ElementGripItem::mousePressEvent(event);
}

void ElementBoundaryGripItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    ElementGripItem::mouseReleaseEvent(event);
}

void ElementBoundaryGripItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    ElementGripItem::mouseMoveEvent(event);
}

int ElementBoundaryGripItem::type() const {
    return view::ELEMENT_TYPE_BOUNDARY_GRIP;
}

};  // namespace view
