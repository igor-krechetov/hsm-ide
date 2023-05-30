#include "HsmFinalElement.hpp"

#include <QColor>
#include <QPainter>

HsmFinalElement::HsmFinalElement()
    : HsmConnectableElement(HsmElementType::FINAL) {}

void HsmFinalElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    painter->setBrush(QColor("#B259b3"));
    mItemRect = QRectF(mOuterRect);
    painter->drawRoundedRect(mItemRect, 5, 5);

    HsmConnectableElement::paint(painter, option, widget);
}
