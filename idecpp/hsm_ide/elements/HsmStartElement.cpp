#include "HsmStartElement.hpp"

#include <QColor>
#include <QPainter>

HsmStartElement::HsmStartElement()
    : HsmConnectableElement() {}

void HsmStartElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    painter->setBrush(QColor("#b3b259"));
    mItemRect = QRectF(outerRect());
    painter->drawRoundedRect(mItemRect, 5, 5);

    HsmConnectableElement::paint(painter, option, widget);
}
