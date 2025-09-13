#include "HsmEntryPointElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmEntryPointElement::HsmEntryPointElement()
    : HsmConnectableElement(HsmElementType::ENTRY_POINT) {}

void HsmEntryPointElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    painter->setBrush(QColor("#2596be"));
    mItemRect = QRectF(mOuterRect);
    painter->drawRoundedRect(mItemRect, 5, 5);
}

};  // namespace view