#include "ElementConnectionArrow.hpp"

#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QTransform>

#include "view/elements/ElementTypeIds.hpp"
#include "view/theme/ThemeManager.hpp"
#include "DebugRendering.hpp"

namespace view {

ElementConnectionArrow::ElementConnectionArrow(QGraphicsObject* annotationElement, Direction direction)
    : QGraphicsObject(annotationElement)
    , mDirection(direction)
    , mShapeArrow(initShape(direction))
    , mOuterRect(mShapeArrow.boundingRect()) {
    // qDebug() << "CREATE: ElementConnectionArrow: " << this;
    setAcceptHoverEvents(true);
    setZValue(11);
    setCursor(QCursor(Qt::PointingHandCursor));
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
}

void ElementConnectionArrow::setPos(const QPointF& pos) {
    constexpr qreal offset = 5.0;

    switch (mDirection) {
        case Direction::North:
            mPos.setX(pos.x());
            mPos.setY(pos.y() - offset);
            break;
        case Direction::East:
            mPos.setX(pos.x() + offset);
            mPos.setY(pos.y());
            break;
        case Direction::South:
            mPos.setX(pos.x());
            mPos.setY(pos.y() + offset);
            break;
        case Direction::West:
            mPos.setX(pos.x() - offset);
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
    const auto& theme = ThemeManager::instance().theme();

    painter->setBrush(mHovered ? theme.connectionArrow.invalidBrush : theme.connectionArrow.validBrush);
    painter->setPen(Qt::NoPen);
    painter->drawPath(mShapeArrow);

#ifdef DEBUG_RENDERING
    debugDrawX(painter, mPos);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(mOuterRect);
#endif
}

/*

    |    W   |
--      33
       ****
      ******
B    1******2
       ****
       ****
       *PP*
--    | A  |

*/

QPainterPath ElementConnectionArrow::initShape(const Direction direction) const {
    QTransform t;
    QPainterPath shape;
    const qreal baseH = 0.55 * mB;
    const qreal baseW = 0.55 * mW;
    const qreal tipHight = mB - baseH;
    const qreal offset = (mW - mB) / 2;

    shape.addRect(mPos.x() - baseW / 2, 0, baseW, baseH);
    shape.moveTo(-mW / 2, baseH);
    shape.lineTo(mW / 2, baseH);
    shape.lineTo(0, baseH + tipHight);
    shape.lineTo(-mW / 2, baseH);

    switch (direction) {
        case Direction::North:
            t.rotate(180.0);
            break;
        case Direction::East:
            t.rotate(-90.0);
            break;
        case Direction::South:
            break;
        case Direction::West:
            t.rotate(90.0);
            break;
        default:
            // do nothing
            break;
    }

    return t.map(shape);
}

void ElementConnectionArrow::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    mHovered = true;
    update();
    QGraphicsObject::hoverEnterEvent(event);
}

void ElementConnectionArrow::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    mHovered = false;
    update();
    QGraphicsObject::hoverLeaveEvent(event);
}

void ElementConnectionArrow::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    emit connectionStarted(this, event->scenePos());
    event->accept();
}

void ElementConnectionArrow::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    emit connectionFinished(this, event->scenePos());
    QGraphicsObject::mouseReleaseEvent(event);
}

void ElementConnectionArrow::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    emit connectionPositionChanged(this, event->scenePos());
    QGraphicsObject::mouseMoveEvent(event);
}

QVariant ElementConnectionArrow::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (GraphicsItemChange::ItemVisibleHasChanged == change) {
        mHovered = false;
    }

    return QGraphicsObject::itemChange(change, value);
}

int ElementConnectionArrow::type() const {
    return view::ELEMENT_TYPE_CONNECTION_ARROW;
}

};  // namespace view
