#include "HsmIncludeElement.hpp"

#include "view/theme/ThemeManager.hpp"

namespace view {

HsmIncludeElement::HsmIncludeElement()
    : HsmStateElement() {
    setElementType(HsmElementType::INCLUDE);
}

HsmIncludeElement::HsmIncludeElement(const QSizeF& size)
    : HsmStateElement(size) {
    setElementType(HsmElementType::INCLUDE);
}

void HsmIncludeElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    const auto& theme = ThemeManager::instance().theme();

    paintRectangularBody(painter, theme.node.includeBackgroundBrush);
}

};  // namespace view
