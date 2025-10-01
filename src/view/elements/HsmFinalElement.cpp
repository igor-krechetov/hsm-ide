#include "HsmFinalElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

// TODO: block outgoing connections

HsmFinalElement::HsmFinalElement()
    : HsmRoundElement(HsmElementType::FINAL) {}

bool HsmFinalElement::isConnectable() const {
    return false;
}

void HsmFinalElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRoundElement::paint(painter, option, widget);

    const qreal r = radius();
    const QPointF c = center();
    const qreal outlineRadius = r * 0.7;

    // Draw the filled circle
    painter->setBrush(mBackgroundBrush);
    painter->drawEllipse(c, r, r);

    // Draw the inner circle
    painter->setBrush(mMainBrush);
    painter->drawEllipse(c, outlineRadius, outlineRadius);
}

};  // namespace view