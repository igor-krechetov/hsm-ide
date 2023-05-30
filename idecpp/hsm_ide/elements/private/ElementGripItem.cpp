#include "ElementGripItem.hpp"
#include "HsmElement.hpp"
#include <QCursor>
#include <QPainter>
#include <QMetaMethod>

ElementGripItem::ElementGripItem(HsmElement* annotationElement)
    : QGraphicsObject(annotationElement)
    , mGripRect(-4, -4, 8, 8)
    , mGripColor("green")
{
}

void ElementGripItem::init() {
    tryConnectSignal("onGripDoubleClick(ElementGripItem*)", parentObject(), "onGripDoubleClick(ElementGripItem*)");
    tryConnectSignal("onGripLostFocus(ElementGripItem*)", parentObject(), "onGripLostFocus(ElementGripItem*)");

    // setFlag(ItemIgnoresTransformations, true);
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setZValue(11);
    setCursor(QCursor(Qt::PointingHandCursor));
}

HsmElement* ElementGripItem::annotationElement() const {
    return dynamic_cast<HsmElement*>(parentItem());
}

QRectF ElementGripItem::boundingRect() const {
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
}

void ElementGripItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    mGripColor = QColor("green");
    QGraphicsObject::hoverLeaveEvent(event);
}

void ElementGripItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    emit onGripDoubleClick(this);
    QGraphicsObject::mouseDoubleClickEvent(event);
}

QVariant ElementGripItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    QVariant res;


    if ((QGraphicsItem::ItemPositionChange == change) && isEnabled()) {
        QPointF p = value.toPointF();

        if (false == annotationElement()->onGripMoved(this, p)) {
            p = pos();
        }

        res = p;
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

void ElementGripItem::tryConnectSignal(const char *signalName, QObject* object, const char* slotName) {
    auto signalMethod = metaObject()->method(metaObject()->indexOfSignal(signalName));
    auto meta = object->metaObject();
    int index = object->metaObject()->indexOfSlot(slotName);
    auto slotMethod = object->metaObject()->method(object->metaObject()->indexOfSlot(slotName));

    qDebug() << index;

    qDebug() << signalMethod.name();
    qDebug() << signalMethod.methodSignature();

    qDebug() << slotMethod.name();
    qDebug() << slotMethod.methodSignature();

    if (slotMethod.isValid()) {
        QObject::connect(this, signalMethod, object, slotMethod);
    }
}
