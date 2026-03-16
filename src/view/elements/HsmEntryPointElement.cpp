#include "HsmEntryPointElement.hpp"

#include <QColor>
#include <QPainter>

#include "view/theme/ThemeManager.hpp"

namespace view {

HsmEntryPointElement::HsmEntryPointElement()
    : HsmRoundElement(HsmElementType::ENTRY_POINT) {}

bool HsmEntryPointElement::acceptsConnections() const {
    return false;
}

void HsmEntryPointElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRoundElement::paint(painter, option, widget);

    const qreal r = radius();
    const QPointF c = center();

    // Draw the circle
    painter->setBrush(ThemeManager::instance().theme().node.backgroundBrush);
    painter->drawEllipse(c, r, r);
}

};  // namespace view