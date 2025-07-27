#include "HsmExitPointElement.hpp"

#include <QColor>
#include <QPainter>

HsmExitPointElement::HsmExitPointElement()
    : HsmConnectableElement(HsmElementType::EXIT_POINT) {}

void HsmExitPointElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    painter->setBrush(QColor("#e28743"));
    mItemRect = QRectF(mOuterRect);
    painter->drawRoundedRect(mItemRect, 5, 5);

    HsmConnectableElement::paint(painter, option, widget);
}
