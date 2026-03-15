#include "HsmIncludeElement.hpp"

#include "ui/theme/ThemeManager.hpp"

namespace view {

HsmIncludeElement::HsmIncludeElement()
    : HsmStateElement() {
    setElementType(HsmElementType::INCLUDE);
}

HsmIncludeElement::HsmIncludeElement(const QSizeF& size)
    : HsmStateElement(size) {
    setElementType(HsmElementType::INCLUDE);
}

bool HsmIncludeElement::acceptsChildElement(const HsmElementType type) const {
    return false;
}

bool HsmIncludeElement::canBeTopLevel() const {
    return true;
}

void HsmIncludeElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    const auto& theme = ThemeManager::instance().theme();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(theme.node.includeBackgroundBrush);

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
