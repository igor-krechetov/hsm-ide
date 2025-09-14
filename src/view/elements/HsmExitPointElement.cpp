#include "HsmExitPointElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmExitPointElement::HsmExitPointElement()
    : HsmConnectableElement(HsmElementType::EXIT_POINT) {}

void HsmExitPointElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    mItemRect = QRectF(mOuterRect);

    // Calculate the diameter of the circle based on the rectangle size
    qreal diameter = qMin(mItemRect.width(), mItemRect.height());

    // Calculate the position to center the circle within the rectangle
    qreal x = mItemRect.x() + (mItemRect.width() - diameter) / 2;
    qreal y = mItemRect.y() + (mItemRect.height() - diameter) / 2;

    // Draw the circle
    painter->setBrush(Qt::black);
    painter->drawEllipse(QRectF(x, y, diameter, diameter));

    // Draw the cross (×) inside the circle
    painter->setPen(Qt::white);

    // Calculate the cross size
    qreal crossSize = diameter * 0.6;
    qreal crossWidth = diameter * 0.2;

    // Calculate the cross position
    qreal crossX = x + (diameter - crossSize) / 2;
    qreal crossY = y + (diameter - crossSize) / 2;

    // Draw the cross
    QPointF p1(crossX, crossY + crossSize / 2);
    QPointF p2(crossX + crossSize, crossY + crossSize / 2);
    QPointF p3(crossX + crossSize / 2, crossY);
    QPointF p4(crossX + crossSize / 2, crossY + crossSize);

    painter->drawLine(p1, p2);
    painter->drawLine(p3, p4);
}

};  // namespace view