#include "ElementBoundaryGripItem.hpp"

#include "HsmResizableElement.hpp"
// #include <QGraphicsSceneMouseEvent>
// #include <QPainter>
#include <QDebug>

namespace view {

ElementBoundaryGripItem::ElementBoundaryGripItem(HsmResizableElement* annotationElement, const GripDirection direction)
    : ElementGripItem(annotationElement)
    , mGripDirection(direction) {
    // qDebug() << "CREATE: ElementBoundaryGripItem: " << this << "parent: " << annotationElement->modelId() << " | "
    //          << annotationElement;

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
    const QPointF valuePos = value.toPointF();
    QVariant res;
    QPointF newPos;

    if ((QGraphicsItem::ItemPositionChange == change) && isEnabled()) {
        newPos = QPointF(pos());

        // qDebug() << "ElementBoundaryGripItem: MOVE:" << pos() << " -> " << valuePos;

        if (GripDirectionType::Horizontal == mGripDirectionType) {
            newPos.setX(valuePos.x());
        } else if (GripDirectionType::Vertical == mGripDirectionType) {
            newPos.setY(valuePos.y());
        } else {
            newPos.setX(valuePos.x());
            newPos.setY(valuePos.y());
        }

        if (false == annotationElement()->onGripMoved(this, newPos - pos())) {
            newPos = pos();
        } else {
            switch (direction()) {
                case GripDirection::SouthWest:
                case GripDirection::West:
                    newPos.setX(0);
                    break;
                case GripDirection::NorthWest:
                    newPos.setX(0);
                    newPos.setY(0);
                    break;
                case GripDirection::North:
                case GripDirection::NorthEast:
                    newPos.setY(0);
                    break;

                case GripDirection::East:
                case GripDirection::SouthEast:
                case GripDirection::South:
                    break;

                default:
                    break;
            }
        }

        res = newPos;
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