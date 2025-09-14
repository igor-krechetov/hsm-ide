#include "HsmHistoryElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmHistoryElement::HsmHistoryElement()
    : HsmConnectableElement(HsmElementType::HISTORY) {}

void HsmHistoryElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
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

    // Draw the history type indicator
    painter->setPen(Qt::white);
    QString historyType = "H"; // Default to shallow history
    // If it's deep history, we would set historyType to "H*"

    // Calculate text position
    QRectF textRect(x, y, diameter, diameter);
    painter->drawText(textRect, Qt::AlignCenter, historyType);
}

};  // namespace view