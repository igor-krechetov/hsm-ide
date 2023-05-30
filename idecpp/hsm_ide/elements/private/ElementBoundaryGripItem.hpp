#ifndef ELEMENTBOUNDARYGRIPITEM_HPP
#define ELEMENTBOUNDARYGRIPITEM_HPP

#include "ElementGripItem.hpp"

class HsmResizableElement;

enum class GripDirection {
    North = 1,
    NorthEast = 2,
    East = 3,
    SouthEast = 4,
    South = 5,
    SouthWest = 6,
    West = 7,
    NorthWest = 8,
    DirectionPos = 9
};

enum class GripDirectionType { Horizontal = 1, Vertical = 2, Diagonal = 3 };

class ElementBoundaryGripItem : public ElementGripItem {
    Q_OBJECT
public:
    ElementBoundaryGripItem(HsmResizableElement* annotationElement, const GripDirection direction);
    virtual ~ElementBoundaryGripItem() = default;

    HsmResizableElement* annotationElement() const;

    GripDirection direction() const;
    GripDirectionType directionType() const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    GripDirection mGripDirection;
    GripDirectionType mGripDirectionType;
};

#endif  // ELEMENTBOUNDARYGRIPITEM_HPP
