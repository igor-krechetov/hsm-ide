#include "HsmRectangularElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmRectangularElement::HsmRectangularElement(const HsmElementType elementType)
    : HsmRectangularElement(elementType, QSizeF(240, 140)) {}

HsmRectangularElement::HsmRectangularElement(const HsmElementType elementType, const QSizeF& size)
    : HsmResizableElement(elementType, size) {
    setMinSize(200, 100);
}

void HsmRectangularElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    constexpr qreal cEdgeRadius = 5.0;

    // Configure painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Set the brush color
    painter->setBrush(mBackgroundBrush);

    // Draw the rounded rectangle with fully rounded corners
    if (isHighligted() == true) {
        painter->setPen(mPenHighlightMode);
    } else if (isResizing() == true) {
        painter->setPen(mPenSelectedBorder);
    } else {
        painter->setPen(mPenNormalMode);
    }

    painter->drawRoundedRect(
        mOuterRect.adjusted(cOuterBorderAdjustment, cOuterBorderAdjustment, -cOuterBorderAdjustment, -cOuterBorderAdjustment),
        cEdgeRadius,
        cEdgeRadius);

#ifdef DEBUG_RENDERING
    painter->drawEllipse(QPointF(0, 0), 5, 5);
    painter->drawPoint(0, 0);
#endif  // DEBUG_RENDERING
}

};  // namespace view