#include "HsmElement.hpp"

HsmElement::HsmElement()
    : mSize(200.0, 40.0) {
    setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable);
    setZValue(3);
    updateBoundingRect();
}

QRectF HsmElement::elementRect() const {
    return mOuterRect;
}

void HsmElement::updateBoundingRect(const QRectF &newRect) {
    self.prepareGeometryChange() if None == newRect : penWidth = 1.0 self.outerRect =
        QRectF((-1) * self.size.width() / 2 - penWidth / 2,
               (-1) * self.size.height() / 2 - penWidth / 2,
               self.size.width() + penWidth,
               self.size.height() + penWidth) else : self.outerRect = newRect self.size = self.outerRect.size()

                                                                                              prepareGeometryChange();
    if (newRect.isNull()) {
        const qreal penWidth = 1.0;
        mOuterRect = QRectF(-mSize.width() / 2 - penWidth / 2,
                            -mSize.height() / 2 - penWidth / 2,
                            mSize.width() + penWidth,
                            mSize.height() + penWidth);
    } else {
        mOuterRect = newRect;
        mSize = mOuterRect.size();
    }
}

QRectF HsmElement::boundingRect() const {
    return mOuterRect;
}

void HsmElement::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
    qDebug() << "ITEM: dragEnterEvent: " << event->source() << ", <" << event->mimeData()->formats() << ">";
    QGraphicsObject::dragEnterEvent(event);
}

void HsmElement::dropEvent(QGraphicsSceneDragDropEvent *event) {
    qDebug() << "ITEM: dropEvent: " << event->source() << ", <" << event->mimeData()->formats() << ">";
    QGraphicsObject::dropEvent(event);
}
