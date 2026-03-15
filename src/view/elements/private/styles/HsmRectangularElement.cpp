#include "HsmRectangularElement.hpp"

#include <QColor>
#include <QPainter>

#include "ui/theme/ThemeManager.hpp"

namespace view {

HsmRectangularElement::HsmRectangularElement(const HsmElementType elementType)
    : HsmRectangularElement(elementType, QSizeF(240, 140)) {}

HsmRectangularElement::HsmRectangularElement(const HsmElementType elementType, const QSizeF& size)
    : HsmResizableElement(elementType, size) {
    setMinSize(200, 100);
}

void HsmRectangularElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    // Configure painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    const auto& theme = ThemeManager::instance().theme();

    // Set the brush color
    painter->setBrush(theme.node.backgroundBrush);

    // Draw the rounded rectangle with fully rounded corners
    if (isHighligted() == true) {
        painter->setPen(theme.node.highlightBorderPen);
    } else if (isSelected() == true) {
        painter->setPen(theme.node.selectedBorderPen);
    } else {
        painter->setPen(theme.node.borderPen);
    }

    painter->drawRoundedRect(
        mOuterRect.adjusted(cOuterBorderAdjustment, cOuterBorderAdjustment, -cOuterBorderAdjustment, -cOuterBorderAdjustment),
        theme.node.cornerRadius,
        theme.node.cornerRadius);

#ifdef DEBUG_RENDERING
    painter->drawEllipse(QPointF(0, 0), 5, 5);
    painter->drawPoint(0, 0);
#endif  // DEBUG_RENDERING
}

};  // namespace view
