#include "HsmConnectableElement.hpp"

HsmConnectableElement::HsmConnectableElement()
    : HsmResizableElement()
{
    updateHoverRect(false);
    setAcceptHoverEvents(true);
}

void HsmConnectableElement::addTransition(HsmTransition* transition, HsmConnectableElement* target)
{
    if (transition->parentItem() == nullptr) {
        scene()->addItem(transition);
    }
    transition->connectElements(this, target);
    mTransitions.append(transition);
}

QRectF HsmConnectableElement::boundingRect() const
{
    return mHoverRect;
}

void HsmConnectableElement::updateHoverRect(bool expendArea)
{
    qreal arrowOffset = 0.0;

    update();
    prepareGeometryChange();

    if (true == expendArea) {
        arrowOffset = ElementConnectionArrow::mW;
    }

    mHoverRect = QRectF(outerRect().left() - arrowOffset,
                        outerRect().top() - arrowOffset,
                        outerRect().width() + 2 * arrowOffset,
                        outerRect().height() + 2 * arrowOffset);
}

void HsmConnectableElement::createConnectionArrows()
{
    if (true == mArrows.isEmpty()) {
        updateHoverRect(true);

        for (const auto direction : {ElementConnectionArrow::Direction::North,
                               ElementConnectionArrow::Direction::East,
                               ElementConnectionArrow::Direction::South,
                               ElementConnectionArrow::Direction::West}) {
            ElementConnectionArrow* newArrow = new ElementConnectionArrow(this, direction);

            newArrow->setPos(getArrowPos(direction));
            connect(newArrow, &ElementConnectionArrow::connectionPositionChanged,
                    this, &HsmConnectableElement::updateConnectionLine);
            connect(newArrow, &ElementConnectionArrow::connectionStarted,
                    this, &HsmConnectableElement::beginConnection);
            connect(newArrow, &ElementConnectionArrow::connectionFinished,
                    this, &HsmConnectableElement::finishConnectionLine);
            mArrows[direction] = newArrow;
        }
    }
}

void HsmConnectableElement::removeConnectionArrows()
{
    if (false == mArrows.isEmpty()) {
        for (const auto direction : mArrows.keys()) {
            // TODO: find then use
            mArrows[direction]->setParentItem(nullptr);
            delete mArrows[direction];
        }

        mArrows.clear();
        updateHoverRect(false);
    }
}

void HsmConnectableElement::updateConnectionArrowsPos()
{
    for (const auto direction : mArrows.keys()) {
        mArrows[direction]->setPos(getArrowPos(direction));
    }
}

void HsmConnectableElement::onGripMoved(GripPosition selectedGrip, const QPointF& pos)
{
    HsmResizableElement::onGripMoved(selectedGrip, pos);
    updateHoverRect(true);
    updateConnectionArrowsPos();
}

QPointF HsmConnectableElement::getArrowPos(ElementConnectionArrow::Direction arrowDirection)
{
    const qreal xCenter = outerRect().center().x();
    const qreal yCenter = outerRect().center().y();
    // TODO: no need to create a map with all items
    QMap<ElementConnectionArrow::Direction, QPointF> positions {
        {ElementConnectionArrow::Direction::North, QPointF(xCenter, outerRect().top())},
        {ElementConnectionArrow::Direction::East, QPointF(outerRect().right(), yCenter)},
        {ElementConnectionArrow::Direction::South, QPointF(xCenter, outerRect().bottom())},
        {ElementConnectionArrow::Direction::West, QPointF(outerRect().left(), yCenter)}
    };
    return positions[arrowDirection];
}

void HsmConnectableElement::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (mArrows.isEmpty()) {
        const QRectF sceneRect = mapRectToScene(outerRect());
        const QPointF sceneMousePos = mapToScene(event->pos());

        if (sceneRect.contains(sceneMousePos)) {
            createConnectionArrows();
        }
    }

    HsmResizableElement::hoverMoveEvent(event);
}

void HsmConnectableElement::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    removeConnectionArrows();
    HsmResizableElement::hoverLeaveEvent(event);
}

QVariant HsmConnectableElement::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (QGraphicsItem::ItemPositionHasChanged == change) {
        removeConnectionArrows();
        // TODO: Hide grips
    }

    return HsmResizableElement::itemChange(change, value);
}

void HsmConnectableElement::beginConnection(ElementConnectionArrow* arrow, const QPointF& pos)
{
    mDrawConnectionLine = false;
    mConnection = new HsmTransition();
    mConnection->beginConnection(this, pos);
    scene()->addItem(mConnection);
}

void HsmConnectableElement::finishConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos)
{
    QGraphicsItem* targetItem = scene()->itemAt(pos, QTransform());
    // TODO: use HsmElement* instead. add types to HsmElement

    if (targetItem == arrow) {
        targetItem = nullptr;
    }

    mDrawConnectionLine = false;
    qDebug() << "FINISH - arrow. Target" << targetItem << pos;

    // TODO: fix
    // if (this == targetItem) {
    //     qDebug() << "TODO: target-self";
    //     mConnection->setParentItem(nullptr);
    // } else if (dynamic_cast<HsmConnectableElement*>(targetItem)) {
    //     qDebug() << "TODO: target-state";
    //     addTransition(mConnection, dynamic_cast<HsmConnectableElement*>(targetItem));
    // } else {
        qDebug() << "TODO: cancel connection";
        mConnection->setParentItem(nullptr);
    // }
    // TODO: target - child object
    delete mConnection;
    mConnection = nullptr;
}

void HsmConnectableElement::updateConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos)
{
    if (mConnection != nullptr) {
        mConnection->moveConnectionTo(pos);
    }
}
