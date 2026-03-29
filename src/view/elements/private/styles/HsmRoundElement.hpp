#ifndef HSMROUNDELEMENT_HPP
#define HSMROUNDELEMENT_HPP

#include "view/elements/private/HsmConnectableElement.hpp"

class QPainter;
class QWidget;
class QStyleOptionGraphicsItem;

namespace view {

class HsmRoundElement : public HsmConnectableElement {
public:
    HsmRoundElement(const HsmElementType elementType);
    virtual ~HsmRoundElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    qreal radius() const;
    QPointF center() const;
};

};  // namespace view

#endif  // HSMROUNDELEMENT_HPP
