#include "HsmEntryPointElement.hpp"

#include <QColor>
#include <QPainter>

HsmEntryPointElement::HsmEntryPointElement()
    : HsmConnectableElement() {}

void HsmEntryPointElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    painter->setBrush(QColor("#2596be"));
    mItemRect = QRectF(outerRect());
    painter->drawRoundedRect(mItemRect, 5, 5);

    HsmConnectableElement::paint(painter, option, widget);
}
