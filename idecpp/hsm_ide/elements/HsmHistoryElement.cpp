#include "HsmHistoryElement.hpp"

#include <QColor>
#include <QPainter>

HsmHistoryElement::HsmHistoryElement()
    : HsmConnectableElement(HsmElementType::HISTORY) {}

void HsmHistoryElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    painter->setBrush(QColor("#b36a59"));
    mItemRect = QRectF(mOuterRect);
    painter->drawRoundedRect(mItemRect, 5, 5);

    HsmConnectableElement::paint(painter, option, widget);
}
