#include "ElementBoundaryGripItem.hpp"
// #include <QGraphicsSceneMouseEvent>
// #include <QPainter>

ElementBoundaryGripItem::ElementBoundaryGripItem(AnnotationElement* annotationElement, GripDirection direction)
    : ElementGripItem(annotationElement)
    , mGripDirection(direction) {
    if ((GripDirection::North == mGripDirection) || (GripDirection::South == mGripDirection)) {
        mGripDirectionType = GripDirectionType::Vertical;
    } else if ((GripDirection::West == mGripDirection) || (GripDirection::East == mGripDirection)) {
        mGripDirectionType = GripDirectionType::Horizontal;
    } else if ((GripDirection::NorthEast == mGripDirection) || (GripDirection::NorthWest == mGripDirection) ||
               (GripDirection::SouthWest == mGripDirection) || (GripDirection::SouthEast == mGripDirection)) {
        mGripDirectionType = GripDirectionType::Diagonal;
    }
}

ElementBoundaryGripItem::GripDirection ElementBoundaryGripItem::direction() const {
    return mGripDirection;
}

ElementBoundaryGripItem::GripDirectionType ElementBoundaryGripItem::directionType() const {
    return mGripDirectionType;
}

QPointF ElementBoundaryGripItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QPointF& value) {
    QPointF p;

    if ((QGraphicsItem::ItemPositionChange == change) && isEnabled()) {
        p = QPointF(pos());

        if (GripDirectionType::Horizontal == mGripDirectionType) {
            p.setX(value.x());
        } else if (GripDirectionType::Vertical == mGripDirectionType) {
            p.setY(value.y());
        } else {
            p.setX(value.x());
            p.setY(value.y());
        }

        if (false == parent()->onGripMoved(this, p)) {
            p = pos();
        }
    } else {
        p = ElementGripItem::itemChange(change, value);
    }

    if (QGraphicsItem::ItemSelectedHasChanged == change) {
        if (false == isSelected()) {
            parent()->onGripLostFocus(this);
        }
    }

    return p;
}