#include "ElementGripItem.hpp"

#include <QCursor>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMetaMethod>
#include <QPainter>

#include "HsmElement.hpp"
#include "ObjectUtils.hpp"
#include "view/elements/ElementTypeIds.hpp"
#include "view/theme/ThemeManager.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

namespace view {

ElementGripItem::ElementGripItem(HsmElement* annotationElement, const GripDirection type)
    : QGraphicsObject(annotationElement)
    , mGripDirection(type)
    , mGripRect(-(cGripSize / 2), -(cGripSize / 2), cGripSize, cGripSize)
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

    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setAcceptHoverEvents(true);
    setZValue(11);

    switch (mGripDirection) {
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

QPointF ElementGripItem::renderingPos() const {
    return pos() + mRenderingOffset;
}

QPointF ElementGripItem::renderingPosScene() const {
    return scenePos() + mRenderingOffset;
}

void ElementGripItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (false == mDragging) {
        const auto& theme = ThemeManager::instance().theme();
        painter->setBrush(mHovered ? theme.grip.hoverColor : theme.grip.color);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(mGripRect.adjusted(mRenderingOffset.x(), mRenderingOffset.y(), mRenderingOffset.x(), mRenderingOffset.y()));

    #ifdef DEBUG_RENDERING
        // draw small X at the center of the grip
        painter->setPen(ThemeManager::instance().theme().grip.debugPen);
        painter->drawLine(QPointF(-cGripSize / 4, -cGripSize / 4), QPointF(cGripSize / 4, cGripSize / 4));
        painter->drawLine(QPointF(-cGripSize / 4, cGripSize / 4), QPointF(cGripSize / 4, -cGripSize / 4));
    #endif  // DEBUG_RENDERING
    }
}

void ElementGripItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    mHovered = true;
    update();
    QGraphicsObject::hoverEnterEvent(event);
    // setCursor(QCursor(Qt::PointingHandCursor));
}

void ElementGripItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    mHovered = false;
    update();
    QGraphicsObject::hoverLeaveEvent(event);
}

void ElementGripItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    // qDebug() << "GRIP: hoverMoveEvent";
}

void ElementGripItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    mLastDragPos = event->scenePos();
    mLastScenePos = scenePos();
    mRenderingOffset = QPointF();

    mDragStartScenePos = event->scenePos();
    mStartGripScenePos = scenePos();
    mRenderingOffsetAtDragStart = mRenderingOffset;

    event->accept();
    mDragging = true;
    emit onGripMoveEnterEvent(this);
}

void ElementGripItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    event->accept();
    mDragging = false;
    mLastDragPos = QPointF();

    if (renderingPos() != pos()) {
        setPos(renderingPos());
    }

    mRenderingOffset = QPointF();
    qDebug() << "emit onGripMoveLeaveEvent";
    emit onGripMoveLeaveEvent(this);
}

void ElementGripItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    const QPointF currentMouseScene = event->scenePos();

    // 1. Compute total drag delta from start (stable, no accumulation error)
    QPointF totalDelta = currentMouseScene - mDragStartScenePos;

    // 2. Constrain movement based on grip direction
    switch (mGripDirection) {
        case GripDirection::North:
        case GripDirection::South:
            totalDelta.setX(0);
            break;

        case GripDirection::East:
        case GripDirection::West:
            totalDelta.setY(0);
            break;
    }

    // 3. Compute intended target position in scene space
    QPointF targetScenePos = mStartGripScenePos + totalDelta;

    // 4. Snap in scene space (single source of truth)
    QPointF snappedScenePos = alignToGrid(targetScenePos);

    // 5. Convert to rendering offset relative to CURRENT position
    QPointF newRenderingOffset = mRenderingOffsetAtDragStart +
                                (snappedScenePos - mStartGripScenePos);

    // qDebug() << "----- ElementGripItem: mouse move: (1) mGripDirection=" << (int)mGripDirection
    //         << ", newRenderingOffset=" << newRenderingOffset
    //         << ", scenePos=" << scenePos();

    switch(mGripDirection) {
        case GripDirection::North:
        case GripDirection::South:
            newRenderingOffset.setX(0);
            break;

        case GripDirection::East:
        case GripDirection::West:
            newRenderingOffset.setY(0);
            break;
    }

    // 6. Compute delta to apply
    QPointF delta = newRenderingOffset - mRenderingOffset;

    // qDebug() << "----- ElementGripItem: mouse move: delta=" << delta
    //         << ", totalDelta=" << totalDelta
    //         << ", currentMouseScene=" << currentMouseScene
    //         <<", targetScenePos=" << targetScenePos
    //         << ", snappedScenePos=" << snappedScenePos
    //         << ", newRenderingOffset=" << newRenderingOffset
    //         <<", mRenderingOffsetAtDragStart=" << mRenderingOffsetAtDragStart
    //         << ", mDragStartScenePos=" << mDragStartScenePos
    //         << ", mStartGripScenePos=" << mStartGripScenePos;

    if (!delta.isNull()) {
        if (mAnnotationElement->onGripMoved(this, delta)) {
            mRenderingOffset = newRenderingOffset;
            emit gripMoved(this, delta);
        }
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void ElementGripItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    emit onGripDoubleClick(this);
    QGraphicsObject::mouseDoubleClickEvent(event);
}

QVariant ElementGripItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    QVariant res;

    res = QGraphicsObject::itemChange(change, value);

    if (change == QGraphicsItem::ItemSelectedHasChanged) {
        if (false == isSelected()) {
            emit onGripLostFocus(this);
        }
    }

    return res;
}

int ElementGripItem::type() const {
    return view::ELEMENT_TYPE_GRIP;
}

QPointF ElementGripItem::alignToGrid(const QPointF& scenePos) const {
    QPointF alignedPos = scenePos;

    if (scene() != nullptr && scene()->views().isEmpty() == false) {
        HsmGraphicsView* hsmView = dynamic_cast<HsmGraphicsView*>(scene()->views().first());

        if (hsmView != nullptr) {
            alignedPos = hsmView->snapPointToGrid(scenePos);
        }
    }

    return alignedPos;
}

};  // namespace view
