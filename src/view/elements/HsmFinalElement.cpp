#include "HsmFinalElement.hpp"

#include <QColor>
#include <QPainter>

#include "view/theme/ThemeManager.hpp"

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
    const qreal outlineRadius = r * ThemeManager::instance().theme().node.finalInnerRadiusFactor;

    // Draw the filled circle
    painter->setBrush(ThemeManager::instance().theme().node.backgroundBrush);
    painter->drawEllipse(c, r, r);

    // Draw the inner circle
    painter->setBrush(ThemeManager::instance().theme().node.mainBrush);
    painter->drawEllipse(c, outlineRadius, outlineRadius);
}

};  // namespace view
