#include "HsmStateElement.hpp"

#include <QColor>
#include <QPainter>

HsmStateElement::HsmStateElement()
    : HsmConnectableElement() {}

void HsmStateElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    painter->setBrush(QColor("#8bb359"));
    mItemRect = QRectF(outerRect());
    painter->drawRoundedRect(mItemRect, 5, 5);

    HsmConnectableElement::paint(painter, option, widget);
}
