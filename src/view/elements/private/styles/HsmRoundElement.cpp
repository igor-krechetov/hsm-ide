#include "HsmRoundElement.hpp"

#include <QPainter>

#include "view/theme/ThemeManager.hpp"

namespace view {

HsmRoundElement::HsmRoundElement(const HsmElementType elementType)
    : HsmConnectableElement(elementType, QSizeF(40, 40)) {}

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
}

qreal HsmRoundElement::radius() const {
    // Calculate the diameter of the circle based on the rectangle size
    // 1px to account for edge size
    return qMin(mOuterRect.width(), mOuterRect.height()) / 2.0 - 1;
}

QPointF HsmRoundElement::center() const {
    return QPointF(mOuterRect.width() / 2, mOuterRect.height() / 2);
}

};  // namespace view
