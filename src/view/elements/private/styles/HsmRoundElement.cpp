#include "HsmRoundElement.hpp"

#include <QColor>
#include <QPainter>

namespace view {

HsmRoundElement::HsmRoundElement(const HsmElementType elementType)
    : HsmConnectableElement(elementType, QSizeF(40, 40))
    , mBackgroundBrush(Qt::white)
    , mMainBrush(Qt::darkGray) {}

void HsmRoundElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    // Configure painter
    QPen pen = QPen(Qt::SolidLine);

    pen.setWidth(2);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing, true);
}

qreal HsmRoundElement::radius() const {
    // Calculate the diameter of the circle based on the rectangle size
    // 1px to account for edge size
    return qMin(mOuterRect.width(), mOuterRect.height()) / 2.0 - 1;
}

QPointF HsmRoundElement::center() const {
    return QPointF(mOuterRect.width() / 2, mOuterRect.height() / 2);
}

};  // namespace view