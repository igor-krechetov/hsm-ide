#include "ElementGripItem.hpp"

ElementGripItem::ElementGripItem(const QGraphicsObject* annotationElement)
    : QGraphicsObject(annotationElement)
    , mGripRect(-4, -4, 8, 8)
    , mGripColor("green");
{
    tryConnectSignal(&onGripDoubleClick, parent(), "onGripDoubleClick");
    // # self.tryConnectSignal(self.onGripMoved, annotation_element, "onGripMoved")
    tryConnectSignal(&onGripLostFocus, parent(), "onGripLostFocus");

    // setFlag(ItemIgnoresTransformations, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
    setZValue(11);
    setCursor(Qt::PointingHandCursor);
}

QRectF ElementGripItem::boundingRect() const override {
    return mGripRect;
}

void ElementGripItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setBrush(mGripColor);
    painter->setPen(Qt::NoPen);
    painter->drawRect(mGripRect);
}

void ElementGripItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) override {
    mGripColor = QColor("red");
    QGraphicsObject::hoverEnterEvent(event);
}

void ElementGripItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override {
    mGripColor = QColor("green");
    QGraphicsObject::hoverLeaveEvent(event);
}

void ElementGripItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override {
    emit onGripDoubleClick(this);
    QGraphicsObject::mouseDoubleClickEvent(event);
}

QVariant ElementGripItem::itemChange(GraphicsItemChange change, const QVariant& value) override {
    QPointF p;

    if ((QGraphicsItem::ItemPositionChange == change) && isEnabled()) {
        p = value.toPointF();

        if (false == parent()->onGripMoved(this, p)) {
            p = pos();
        }
    } else {
        p = QGraphicsObject::itemChange(change, value).toPointF();
    }

    if (change == QGraphicsItem::ItemSelectedHasChanged) {
        if (false == isSelected()) {
            emit onGripLostFocus(this);
        }
    }

    return p;
}

void ElementGripItem::tryConnectSignal(PointerToMemberFunction signal, QObject* object, const char* functionName) {
    auto functionObject = object->metaObject()->method(object->metaObject()->indexOfMethod(functionName));

    if (functionObject.isValid()) {
        QObject::connect(this, signal, functionObject);
    }
}