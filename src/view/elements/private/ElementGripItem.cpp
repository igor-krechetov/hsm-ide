#include "ElementGripItem.hpp"

#include <QCursor>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QMetaMethod>
#include <QPainter>

#include "HsmElement.hpp"

namespace view {

ElementGripItem::ElementGripItem(HsmElement* annotationElement)
    : QGraphicsObject(annotationElement)
    , mGripRect(-4, -4, 8, 8)
    , mGripColor("green")
    , mAnnotationElement(annotationElement) {
    qDebug() << "CREATE: ElementGripItem: " << this;
}

ElementGripItem::~ElementGripItem() {
    qDebug() << "DELETE: ElementGripItem: " << this;
}

void ElementGripItem::init() {
    tryConnectSignal("onGripDoubleClick(ElementGripItem*)", parentObject(), "onGripDoubleClick(ElementGripItem*)");
    tryConnectSignal("onGripLostFocus(ElementGripItem*)", parentObject(), "onGripLostFocus(ElementGripItem*)");

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
            res = pos();
        } else {
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

void ElementGripItem::tryConnectSignal(const char* signalName, QObject* object, const char* slotName) {
    auto signalMethod = metaObject()->method(metaObject()->indexOfSignal(signalName));
    auto meta = object->metaObject();
    int index = object->metaObject()->indexOfSlot(slotName);
    auto slotMethod = object->metaObject()->method(object->metaObject()->indexOfSlot(slotName));

    // qDebug() << index;

    // qDebug() << signalMethod.name();
    // qDebug() << signalMethod.methodSignature();

    // qDebug() << slotMethod.name();
    // qDebug() << slotMethod.methodSignature();

    if (slotMethod.isValid()) {
        QObject::connect(this, signalMethod, object, slotMethod);
    }
}

}; // namespace view