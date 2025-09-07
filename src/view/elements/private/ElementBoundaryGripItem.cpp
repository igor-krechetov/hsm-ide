#include "ElementBoundaryGripItem.hpp"

#include "HsmResizableElement.hpp"
// #include <QGraphicsSceneMouseEvent>
// #include <QPainter>
#include <QDebug>

namespace view {

ElementBoundaryGripItem::ElementBoundaryGripItem(HsmResizableElement* annotationElement, const GripDirection direction)
    : ElementGripItem(annotationElement)
    , mGripDirection(direction) {
    qDebug() << "CREATE: ElementBoundaryGripItem: " << this << "parent: " << annotationElement->modelId() << " | "
             << annotationElement;

    if ((GripDirection::North == mGripDirection) || (GripDirection::South == mGripDirection)) {
        mGripDirectionType = GripDirectionType::Vertical;
    } else if ((GripDirection::West == mGripDirection) || (GripDirection::East == mGripDirection)) {
        mGripDirectionType = GripDirectionType::Horizontal;
    } else if ((GripDirection::NorthEast == mGripDirection) || (GripDirection::NorthWest == mGripDirection) ||
               (GripDirection::SouthWest == mGripDirection) || (GripDirection::SouthEast == mGripDirection)) {
        mGripDirectionType = GripDirectionType::Diagonal;
    }
}

HsmResizableElement* ElementBoundaryGripItem::annotationElement() const {
    return reinterpret_cast<HsmResizableElement*>(ElementGripItem::annotationElement());
}

GripDirection ElementBoundaryGripItem::direction() const {
    return mGripDirection;
}

GripDirectionType ElementBoundaryGripItem::directionType() const {
    return mGripDirectionType;
}

QVariant ElementBoundaryGripItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    const QPointF newPos = value.toPointF();
    QVariant res;
    QPointF p;

    if ((QGraphicsItem::ItemPositionChange == change) && isEnabled()) {
        p = QPointF(pos());

        if (GripDirectionType::Horizontal == mGripDirectionType) {
            p.setX(newPos.x());
        } else if (GripDirectionType::Vertical == mGripDirectionType) {
            p.setY(newPos.y());
        } else {
            p.setX(newPos.x());
            p.setY(newPos.y());
        }

        if (false == annotationElement()->onGripMoved(this, p)) {
            p = pos();
        }

        res = p;
    } else {
        res = ElementGripItem::itemChange(change, value);
    }

    if (QGraphicsItem::ItemSelectedHasChanged == change) {
        if (false == isSelected()) {
            annotationElement()->onGripLostFocus(this);
        }
    }

    return res;
}

};  // namespace view