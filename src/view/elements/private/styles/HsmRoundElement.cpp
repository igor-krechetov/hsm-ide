#include "HsmRoundElement.hpp"

#include <QPainter>

#include "view/theme/ThemeManager.hpp"
#include "view/theme/ThemeUtils.hpp"

namespace view {

HsmRoundElement::HsmRoundElement(const HsmElementType elementType)
    : HsmConnectableElement(elementType, QSizeF(theme::snapToGrid(40), theme::snapToGrid(40))) {}

void HsmRoundElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    const auto& theme = ThemeManager::instance().theme();

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (isHighligted() == true) {
        painter->setPen(theme.node.highlightBorderPen);
    } else if (isSelected() == true) {
        painter->setPen(theme.node.selectedBorderPen);
    } else {
        painter->setPen(theme.node.borderPen);
    }

#ifdef DEBUG_RENDERING
    painter->setPen(QPen(Qt::red, 1, Qt::DashLine));
    painter->drawRect(mOuterRect);

    painter->drawEllipse(QPointF(0, 0), 5, 5);
    painter->drawPoint(0, 0);
#endif
}

qreal HsmRoundElement::radius() const {
    // The outer rect is a grid multiple and is NOT inflated by the border pen, so the
    // circle must be inset by half the border pen width to keep the whole stroke inside
    // mOuterRect (otherwise the stroke, centered on the edge, would be clipped).
    const qreal penWidth = ThemeManager::instance().theme().node.selectedBorderPen.widthF();

    return qMin(mOuterRect.width(), mOuterRect.height()) / 2.0 - penWidth / 2.0;
}

QPointF HsmRoundElement::center() const {
    return QPointF(mOuterRect.width() / 2, mOuterRect.height() / 2);
}

};  // namespace view
