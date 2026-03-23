#ifndef ELEMENTBOUNDARYGRIPITEM_HPP
#define ELEMENTBOUNDARYGRIPITEM_HPP

#include "ElementGripItem.hpp"

namespace view {

class HsmResizableElement;

enum class GripDirectionType { Horizontal = 1, Vertical = 2, Diagonal = 3 };

class ElementBoundaryGripItem : public ElementGripItem {
    Q_OBJECT
public:
    ElementBoundaryGripItem(HsmResizableElement* annotationElement, const GripDirection type);
    virtual ~ElementBoundaryGripItem() = default;

    HsmResizableElement* annotationElement() const;

    GripDirectionType directionType() const;

    int type() const override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    GripDirectionType mGripDirectionType;
};

};  // namespace view

#endif  // ELEMENTBOUNDARYGRIPITEM_HPP
