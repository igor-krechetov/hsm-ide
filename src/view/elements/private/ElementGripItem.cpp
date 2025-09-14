#include "ElementGripItem.hpp"

#include <QCursor>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QMetaMethod>
#include <QPainter>

#include "HsmElement.hpp"
#include "ObjectUtils.hpp"

namespace view {

ElementGripItem::ElementGripItem(HsmElement* annotationElement)
    : QGraphicsObject(annotationElement)
    , mGripRect(-4, -4, cGripSize, cGripSize)
    , mGripColor("green")
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

    // setFlag(ItemIgnoresTransformations, true);
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setZValue(11);
}

HsmElement* ElementGripItem::annotationElement() const {
    // return dynamic_cast<HsmElement*>(parentItem());
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
    painter->drawRect(mGripRect);
}

void ElementGripItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    mGripColor = QColor("red");
    QGraphicsObject::hoverEnterEvent(event);
    setCursor(QCursor(Qt::PointingHandCursor));
}

void ElementGripItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    mGripColor = QColor("green");
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
            qDebug() << "GRIP MOVE:" << pos() << " -> " << value;
            res = value;
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