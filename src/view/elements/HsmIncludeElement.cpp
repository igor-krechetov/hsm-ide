#include "HsmIncludeElement.hpp"

namespace view {

HsmIncludeElement::HsmIncludeElement()
    : HsmStateElement() {
    setElementType(HsmElementType::INCLUDE);
}

HsmIncludeElement::HsmIncludeElement(const QSizeF& size)
    : HsmStateElement(size) {
    setElementType(HsmElementType::INCLUDE);
}

void HsmIncludeElement::init(const QSharedPointer<model::StateMachineEntity>& modelEntity) {
    mBackgroundBrush = QBrush(QColor("#F5F0D8"));
    HsmStateElement::init(modelEntity);
}

bool HsmIncludeElement::acceptsChildElement(const HsmElementType type) const {
    return false;
}

bool HsmIncludeElement::canBeTopLevel() const {
    return true;
}

void HsmIncludeElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmStateElement::paint(painter, option, widget);
}

};  // namespace view