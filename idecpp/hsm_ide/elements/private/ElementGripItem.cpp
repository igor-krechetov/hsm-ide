#include "ElementGripItem.hpp"
#include "HsmElement.hpp"
#include <QCursor>
#include <QPainter>
#include <QMetaMethod>

ElementGripItem::ElementGripItem(HsmElement* annotationElement)
    : QGraphicsObject(dynamic_cast<QGraphicsItem*>(annotationElement))
    , mGripRect(-4, -4, 8, 8)
    , mGripColor("green")
{
}

void ElementGripItem::init() {
//    tryConnectSignal(SIGNAL(onGripDoubleClick(ElementGripItem*)), parentObject(), SLOT(onGripDoubleClick(ElementGripItem*)));
    // # self.tryConnectSignal(self.onGripMoved, annotation_element, "onGripMoved")
//    tryConnectSignal(SIGNAL(onGripLostFocus(ElementGripItem*)), parentObject(), SLOT(onGripLostFocus(ElementGripItem*)));

    tryConnectSignal("onGripDoubleClick(ElementGripItem*)", parentObject(), "onGripDoubleClick(ElementGripItem*)");
    tryConnectSignal("onGripDoubleClick(ElementGripItem*)", parentObject(), "onGripDoubleClick(ElementGripItem*)");

    // setFlag(ItemIgnoresTransformations, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
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
    QPointF p;

    if ((QGraphicsItem::ItemPositionChange == change) && isEnabled()) {
        p = value.toPointF();

        if (false == annotationElement()->onGripMoved(this, p)) {
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
