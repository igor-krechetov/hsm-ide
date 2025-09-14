#include "HsmEntryPointElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmEntryPointElement::HsmEntryPointElement()
    : HsmRoundElement(HsmElementType::ENTRY_POINT) {}

void HsmEntryPointElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRoundElement::paint(painter, option, widget);

    const qreal r = radius();
    const QPointF c = center();

    // Draw the circle
    painter->setBrush(mBackgroundBrush);
    painter->drawEllipse(c, r, r);
}

};  // namespace view