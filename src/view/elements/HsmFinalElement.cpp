#include "HsmFinalElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmFinalElement::HsmFinalElement()
    : HsmConnectableElement(HsmElementType::FINAL) {}

void HsmFinalElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);
    mItemRect = QRectF(mOuterRect);

    // Calculate the diameter of the circle based on the rectangle size
    qreal diameter = qMin(mItemRect.width(), mItemRect.height());

    // Calculate the position to center the circle within the rectangle
    qreal x = mItemRect.x() + (mItemRect.width() - diameter) / 2;
    qreal y = mItemRect.y() + (mItemRect.height() - diameter) / 2;

    // Draw the black filled circle
    painter->setBrush(Qt::black);
    painter->drawEllipse(QRectF(x, y, diameter, diameter));

    // Draw the white outline circle
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::white);
    qreal outlineDiameter = diameter * 0.8;
    qreal outlineX = x + (diameter - outlineDiameter) / 2;
    qreal outlineY = y + (diameter - outlineDiameter) / 2;
    painter->drawEllipse(QRectF(outlineX, outlineY, outlineDiameter, outlineDiameter));
}

};  // namespace view