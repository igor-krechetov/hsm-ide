#include "HsmStateElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmStateElement::HsmStateElement()
    : HsmResizableElement(HsmElementType::STATE) {}

bool HsmStateElement::acceptsChildren() const {
    return true;
}

void HsmStateElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    // if (mHightlight) {
    //     painter->setPen(mPenHighlightMode);
    // } else {
    painter->setPen(Qt::SolidLine);
    // }

    painter->setBrush(QColor("#8bb359"));
    mItemRect = QRectF(mOuterRect);
    painter->drawRoundedRect(mItemRect, 5, 5);

    if (isHighligted() == true) {
        painter->setPen(mPenHighlightMode);
        painter->drawRoundedRect(mItemRect.adjusted(2, 2, -2, -2), 5, 5);
    }

    HsmResizableElement::paint(painter, option, widget);
}

};  // namespace view