#include "HsmRectangularElement.hpp"

#include <QColor>
#include <QPainter>

#include "view/theme/ThemeManager.hpp"
#include "view/theme/ThemeUtils.hpp"

namespace view {

HsmRectangularElement::HsmRectangularElement(const HsmElementType elementType)
    : HsmRectangularElement(elementType, QSizeF(theme::snapToGrid(240), theme::snapToGrid(140))) {}

HsmRectangularElement::HsmRectangularElement(const HsmElementType elementType, const QSizeF& size)
    : HsmResizableElement(elementType, size) {
    setMinSize(theme::snapToGrid(200), theme::snapToGrid(100));
}

void HsmRectangularElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    const auto& theme = ThemeManager::instance().theme();

    paintRectangularBody(painter, theme.node.backgroundBrush);
}

void HsmRectangularElement::paintRectangularBody(QPainter* painter, const QBrush& backgroundBrush) const {
    // Configure painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    const auto& theme = ThemeManager::instance().theme();

    // Set the brush color
    painter->setBrush(backgroundBrush);

    // Draw the rounded rectangle with fully rounded corners
    if (hasOverlapWarning() == true) {
        painter->setPen(theme.node.overlapWarningPen);
    } else if (isHighligted() == true) {
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
