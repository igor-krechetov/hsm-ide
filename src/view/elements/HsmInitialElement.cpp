#include "HsmInitialElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmInitialElement::HsmInitialElement()
    : HsmRoundElement(HsmElementType::START) {}

bool HsmInitialElement::acceptsConnections() const {
    return false;
}

void HsmInitialElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRoundElement::paint(painter, option, widget);

    const qreal r = radius();
    const QPointF c = center();

    // Draw the circle
    painter->setBrush(mMainBrush);
    painter->drawEllipse(c, r, r);
}

};  // namespace view