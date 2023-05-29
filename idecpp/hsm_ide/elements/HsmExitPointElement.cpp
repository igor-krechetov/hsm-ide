#include "HsmExitPointElement.hpp"
#include <QPainter>
#include <QColor>

HsmExitPointElement::HsmExitPointElement()
    : HsmConnectableElement() {}

void HsmExitPointElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    painter->setBrush(QColor("#e28743"));
    mItemRect = QRectF(mOuterRect);
    painter->drawRoundedRect(mItemRect, 5, 5);

    HsmConnectableElement::paint(painter, option, widget);
}
