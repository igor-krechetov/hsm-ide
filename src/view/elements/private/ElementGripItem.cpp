#include "ElementGripItem.hpp"

#include <QCursor>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QMetaMethod>
#include <QPainter>

#include "HsmElement.hpp"
#include "ObjectUtils.hpp"

namespace view {

ElementGripItem::ElementGripItem(HsmElement* annotationElement, const GripDirection type)
    : QGraphicsObject(annotationElement)
    , mGripDirection(type)
    , mGripRect(-(cGripSize/2), -(cGripSize/2), cGripSize, cGripSize)
    , mGripColor("red") // TODO: move to style object
    , mAnnotationElement(annotationElement) {
    // qDebug() << "CREATE: ElementGripItem: " << this << "parent: " << annotationElement;
    // qDebug() << "QObject parent:" << parentObject();
    // qDebug() << "QGraphicsItem parent:" << parentItem();
}

ElementGripItem::~ElementGripItem() {
    // qDebug() << "DELETE: ElementGripItem: " << this;
}

void ElementGripItem::init() {
    // For states
    tryConnectSignal(this, "onGripLostFocus(ElementGripItem*)", parentObject(), "onGripLostFocus(ElementGripItem*)");
    // tryConnectSignal("onGripMoved(ElementGripItem*,QPointF)", parentObject(), "onGripMoved(ElementGripItem*,QPointF)");

    // For transitions
    tryConnectSignal(this, "onGripDoubleClick(ElementGripItem*)", parentObject(), "onGripDoubleClick(ElementGripItem*)");
    tryConnectSignal(this, "onGripMoveEnterEvent(ElementGripItem*)", parentObject(), "onGripMoveEnterEvent(ElementGripItem*)");
    tryConnectSignal(this, "onGripMoveLeaveEvent(ElementGripItem*)", parentObject(), "onGripMoveLeaveEvent(ElementGripItem*)");

    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setAcceptHoverEvents(true);
    setZValue(11);

    switch(mGripDirection) {
        case GripDirection::North:
        case GripDirection::South:
            setCursor(QCursor(Qt::SizeVerCursor));
            break;

        case GripDirection::NorthEast:
        case GripDirection::SouthWest:
            setCursor(QCursor(Qt::SizeBDiagCursor));
            break;

        case GripDirection::East:
        case GripDirection::West:
            setCursor(QCursor(Qt::SizeHorCursor));
            break;
        case GripDirection::SouthEast:
        case GripDirection::NorthWest:
            setCursor(QCursor(Qt::SizeFDiagCursor));
            break;

        case GripDirection::FreeMove:
        default:
            setCursor(QCursor(Qt::PointingHandCursor));
            break;
    }
}

GripDirection ElementGripItem::direction() const {
    return mGripDirection;
}

HsmElement* ElementGripItem::annotationElement() const {
    return mAnnotationElement;
}

QRectF ElementGripItem::boundingRect() const {
    // printf("ElementGripItem::boundingRect: %f, %f, %f, %f\n", mGripRect
    // qDebug() << mGripRect << "\n";
    return mGripRect;
}

void ElementGripItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setBrush(mGripColor);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(mGripRect);

#ifdef DEBUG_RENDERING
    // draw small X at the center of the grip
    painter->setPen(QPen(Qt::black, 1));
    painter->drawLine(QPointF(-cGripSize/4, -cGripSize/4), QPointF(cGripSize/4, cGripSize/4));
    painter->drawLine(QPointF(-cGripSize/4, cGripSize/4), QPointF(cGripSize/4, -cGripSize/4));
#endif // DEBUG_RENDERING
}

void ElementGripItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    mGripColor = QColor("green");
    QGraphicsObject::hoverEnterEvent(event);
    // setCursor(QCursor(Qt::PointingHandCursor));
}

void ElementGripItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    mGripColor = QColor("red");
    QGraphicsObject::hoverLeaveEvent(event);
}

void ElementGripItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    // qDebug() << "GRIP: hoverMoveEvent";
}

void ElementGripItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    // printf("ElementGripItem::mousePressEvent\n");
    mLastPos = event->scenePos();
    event->accept();
    mDragging = true;
    qDebug() << "emit onGripMoveEnterEvent";
    emit onGripMoveEnterEvent(this);
}

void ElementGripItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    event->accept();
    mDragging = false;
    qDebug() << "emit onGripMoveLeaveEvent";
    emit onGripMoveLeaveEvent(this);
}

void ElementGripItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    // printf("ElementGripItem::mouseMoveEvent\n");
    QPointF delta = event->scenePos() - mLastPos;

    moveBy(delta.x(), delta.y());
    mLastPos = event->scenePos();
}

void ElementGripItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    printf("ElementGripItem::mouseDoubleClickEvent\n");
    emit onGripDoubleClick(this);
    QGraphicsObject::mouseDoubleClickEvent(event);
}

QVariant ElementGripItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    // qDebug() << "ElementGripItem::itemChange: " << change;
    QVariant res;

    if ((QGraphicsItem::ItemPositionChange == change) && isEnabled()) {
        if (false == annotationElement()->onGripMoved(this, value.toPointF())) {
            qDebug() << pos() << " -> " << value;
            res = pos();
        } else {
            res = QGraphicsObject::itemChange(change, value);
        }
    } else {
        res = QGraphicsObject::itemChange(change, value);
    }

    if (change == QGraphicsItem::ItemSelectedHasChanged) {
        if (false == isSelected()) {
            emit onGripLostFocus(this);
        }
    }

    return res;
}

};  // namespace view