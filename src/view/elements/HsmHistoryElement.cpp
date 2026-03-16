#include "HsmHistoryElement.hpp"

#include <QColor>
#include <QPainter>

#include "view/theme/ThemeManager.hpp"

namespace view {

HsmHistoryElement::HsmHistoryElement()
    : HsmRoundElement(HsmElementType::HISTORY) {}

bool HsmHistoryElement::isConnectable() const {
    return true;
}

bool HsmHistoryElement::canBeTopLevel() const {
    return false;
}

void HsmHistoryElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRoundElement::paint(painter, option, widget);

    const auto& theme = ThemeManager::instance().theme();
    const qreal r = radius();
    const QPointF c = center();
    painter->setFont(theme.node.historyFont);

    // Draw the history type indicator
    const QString historyType = "H";  // Default to shallow history
    // If it's deep history, we would set historyType to "H*"

    // Draw the white filled circle
    painter->setBrush(theme.node.backgroundBrush);
    painter->drawEllipse(c, r, r);

    // Draw text centered in the ellipse
    painter->drawText(mOuterRect, Qt::AlignCenter, historyType);
}

};  // namespace view
