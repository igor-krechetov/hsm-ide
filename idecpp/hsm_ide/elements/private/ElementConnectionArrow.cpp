#include "ElementConnectionArrow.hpp"
#include <QCursor>
#include <QTransform>
#include <QGraphicsSceneMouseEvent>

ElementConnectionArrow::ElementConnectionArrow(QGraphicsObject* annotationElement, Direction direction)
    : QGraphicsObject(annotationElement)
    , mDirection(direction)
    , mOuterRect(-mW / 2, -mW / 2, mW, mW)
    , mShapeArrow(initShape(direction))
    , mArrowColor("green") {
    setAcceptHoverEvents(true);
    setZValue(11);
    setCursor(QCursor(Qt::PointingHandCursor));
}

void ElementConnectionArrow::setPos(const QPointF& pos) {
    constexpr qreal offset = 5.0;

    switch (mDirection) {
        case Direction::North:
            mPos.setX(pos.x());
            mPos.setY(pos.y() - mW / 2 - offset);
            break;
        case Direction::East:
            mPos.setX(pos.x() + mW / 2 + offset);
            mPos.setY(pos.y());
            break;
        case Direction::South:
            mPos.setX(pos.x());
            mPos.setY(pos.y() + mW / 2 + offset);
            break;
        case Direction::West:
            mPos.setX(pos.x() - mW / 2 - offset);
            mPos.setY(pos.y());
            break;
        default:
            // do nothing
            break;
    }

    QGraphicsObject::setPos(mPos);
}

QRectF ElementConnectionArrow::boundingRect() const {
    return mOuterRect;
}

void ElementConnectionArrow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setBrush(mArrowColor);
    painter->setPen(Qt::NoPen);
    painter->drawPath(mShapeArrow);
}

QPainterPath ElementConnectionArrow::initShape(const Direction direction) const {
    QTransform t;
    QPainterPath shape;
    const qreal base = 0.55 * mB;
    const qreal offset = (mW - mB) / 2;

    shape.addRect(-base / 2, -2.0 * mA, base, 2.0 * mA);
    shape.moveTo(-mW / 2 + offset, -2.0 * mA);
    shape.lineTo(mW / 2 - offset, -2.0 * mA);
    shape.lineTo(0, -mW);
    shape.lineTo(-mW / 2 + offset, -2.0 * mA);

    switch (direction) {
        case Direction::North:
            t.translate(0, mW / 2);
            break;
        case Direction::East:
            t.translate(-mW / 2, 0);
            t.rotate(90.0);
            break;
        case Direction::South:
            t.translate(0, -mW / 2);
            t.rotate(180.0);
            break;
        case Direction::West:
            t.translate(mW / 2, 0);
            t.rotate(-90.0);
            break;
        default:
            // do nothing
            break;
    }

    return t.map(shape);
}

void ElementConnectionArrow::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    mArrowColor = QColor("red");
    QGraphicsObject::hoverEnterEvent(event);
}

void ElementConnectionArrow::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    mArrowColor = QColor("green");
    QGraphicsObject::hoverLeaveEvent(event);
}

void ElementConnectionArrow::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    qDebug("PRESS - arrow");
    emit connectionStarted(this, mapToScene(event->pos()));
    event->accept();
}

void ElementConnectionArrow::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    qDebug("RELEASE - arrow");
    emit connectionFinished(this, mapToScene(event->pos()));
    QGraphicsObject::mouseReleaseEvent(event);
}

void ElementConnectionArrow::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    emit connectionPositionChanged(this, mapToScene(event->pos()));
    QGraphicsObject::mouseMoveEvent(event);
}
