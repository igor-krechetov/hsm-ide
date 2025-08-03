#include "HsmStateElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmStateElement::HsmStateElement()
    : HsmConnectableElement(HsmElementType::STATE) {}

void HsmStateElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    if (mHightlight) {
        painter->setPen(mPenHighlightMode);
    } else {
        painter->setPen(Qt::SolidLine);
    }

    painter->setBrush(QColor("#8bb359"));
    mItemRect = QRectF(mOuterRect);
    painter->drawRoundedRect(mItemRect, 5, 5);

    HsmConnectableElement::paint(painter, option, widget);
}

}; // namespace view