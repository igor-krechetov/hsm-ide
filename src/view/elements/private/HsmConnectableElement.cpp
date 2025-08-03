#include "HsmConnectableElement.hpp"

#include <QGraphicsScene>

#include "ElementBoundaryGripItem.hpp"

namespace view {

HsmConnectableElement::HsmConnectableElement(const HsmElementType elementType)
    : HsmResizableElement(elementType) {
    qDebug() << "CREATE: HsmConnectableElement: " << this;
}

HsmConnectableElement::~HsmConnectableElement() {
    qDebug() << "DELETE " << this;
}

void HsmConnectableElement::init(const model::EntityID_t modelElementId) {
    HsmResizableElement::init(modelElementId);
    updateHoverRect(false);
    setAcceptHoverEvents(true);
}

bool HsmConnectableElement::isConnectable() const {
    qDebug() << Q_FUNC_INFO << this;
    return true;
}

HsmElement* HsmConnectableElement::connectableElementAt(const QPointF& pos) const {
    HsmElement* element = nullptr;
    QGraphicsItem* targetItem = scene()->itemAt(pos, QTransform());

    if (nullptr != targetItem) {
        QVariant elementType = targetItem->data(USERDATA_HSM_ELEMENT_TYPE);

        if (elementType.isValid() && elementType.toInt() != static_cast<int>(view::HsmElementType::UNKNOWN)) {
            // NOTE: because QGraphicsObject is a child to both QObject and QGraphicsItem it's crucial
            //       to use dynamic_cast. parentItem() != parentObject()
            element = dynamic_cast<HsmElement*>(targetItem);

            if (element->isConnectable() == false) {
                qDebug() << "Target is not connectable: " << element->modelId()
                         << " | viewElementType=" << elementType << " | " << element;
                element = nullptr;
            }
        }
    }

    return element;
}

void HsmConnectableElement::addTransition(std::shared_ptr<HsmTransition>& transition, HsmConnectableElement* target) {
    if (scene()->items().contains(transition.get()) == false) {
        scene()->addItem(transition.get());
    }

    transition->connectElements(this, target);
    mTransitions.append(transition);
}

QRectF HsmConnectableElement::boundingRect() const {
    return mHoverRect;
}

void HsmConnectableElement::updateHoverRect(bool expendArea) {
    qreal arrowOffset = 0.0;

    update();
    prepareGeometryChange();

    if (true == expendArea) {
        arrowOffset = ElementConnectionArrow::mW;
    }

    mHoverRect = QRectF(mOuterRect.left() - arrowOffset,
                        mOuterRect.top() - arrowOffset,
                        mOuterRect.width() + 2 * arrowOffset,
                        mOuterRect.height() + 2 * arrowOffset);
}

void HsmConnectableElement::createConnectionArrows() {
    if (true == mArrows.isEmpty()) {
        updateHoverRect(true);

        for (const auto direction : {ElementConnectionArrow::Direction::North,
                                     ElementConnectionArrow::Direction::East,
                                     ElementConnectionArrow::Direction::South,
                                     ElementConnectionArrow::Direction::West}) {
            ElementConnectionArrow* newArrow = new ElementConnectionArrow(this, direction);

            newArrow->setPos(getArrowPos(direction));
            connect(newArrow,
                    SIGNAL(connectionPositionChanged(ElementConnectionArrow*, QPointF)),
                    this,
                    SLOT(updateConnectionLine(ElementConnectionArrow*, QPointF)));
            connect(newArrow,
                    SIGNAL(connectionStarted(ElementConnectionArrow*, QPointF)),
                    this,
                    SLOT(beginConnection(ElementConnectionArrow*, QPointF)));
            connect(newArrow,
                    SIGNAL(connectionFinished(ElementConnectionArrow*, QPointF)),
                    this,
                    SLOT(finishConnectionLine(ElementConnectionArrow*, QPointF)));
            mArrows[direction] = newArrow;
        }
    }
}

void HsmConnectableElement::removeConnectionArrows() {
    if (false == mArrows.isEmpty()) {
        // TODO: replace QMap with map
        for (const auto& direction : mArrows.keys()) {
            // TODO: find then use
            mArrows[direction]->setParentItem(nullptr);
            delete mArrows[direction];
        }

        mArrows.clear();
        updateHoverRect(false);
    }
}

void HsmConnectableElement::updateConnectionArrowsPos() {
    for (const auto& direction : mArrows.keys()) {
        mArrows[direction]->setPos(getArrowPos(direction));
    }
}

bool HsmConnectableElement::onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos) {
    HsmResizableElement::onGripMoved(selectedGrip, pos);
    updateHoverRect(true);
    updateConnectionArrowsPos();

    // TODO: check result
    return true;
}

QPointF HsmConnectableElement::getArrowPos(ElementConnectionArrow::Direction arrowDirection) {
    const qreal xCenter = mOuterRect.center().x();
    const qreal yCenter = mOuterRect.center().y();
    // TODO: no need to create a map with all items
    QMap<ElementConnectionArrow::Direction, QPointF> positions{
        {ElementConnectionArrow::Direction::North, QPointF(xCenter, mOuterRect.top())},
        {ElementConnectionArrow::Direction::East, QPointF(mOuterRect.right(), yCenter)},
        {ElementConnectionArrow::Direction::South, QPointF(xCenter, mOuterRect.bottom())},
        {ElementConnectionArrow::Direction::West, QPointF(mOuterRect.left(), yCenter)}};
    // qDebug() << "getArrowPos: " << xCenter << "\\" << yCenter << "   " <<  mOuterRect << "---- " <<
    // positions[arrowDirection];
    return positions[arrowDirection];
    // return QPointF(xCenter, yCenter);
}

void HsmConnectableElement::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (mArrows.isEmpty()) {
        const QRectF sceneRect = mapRectToScene(mOuterRect);
        const QPointF sceneMousePos = mapToScene(event->pos());

        if (sceneRect.contains(sceneMousePos)) {
            createConnectionArrows();
        }
    }

    HsmResizableElement::hoverMoveEvent(event);
}

void HsmConnectableElement::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    removeConnectionArrows();
    HsmResizableElement::hoverLeaveEvent(event);
}

QVariant HsmConnectableElement::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (QGraphicsItem::ItemPositionHasChanged == change) {
        removeConnectionArrows();
        // TODO: Hide grips
    }

    return HsmResizableElement::itemChange(change, value);
}

void HsmConnectableElement::beginConnection(ElementConnectionArrow* arrow, const QPointF& pos) {
    mDrawConnectionLine = false;
    mConnection = std::make_shared<HsmTransition>();
    mConnection->init(0);
    mConnection->beginConnection(this, pos);
    scene()->addItem(mConnection.get());
    if (mConnection->parent()) {
        qDebug() << "ERROR: parent is null";
    }
}

void HsmConnectableElement::finishConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos) {
    qDebug() << "FINISH - arrow. Target" << mLastConnectionTarget << pos;

    mDrawConnectionLine = false;

    if (mConnection) {
        scene()->removeItem(mConnection.get());
        mConnection.reset();
    }

    if (mLastConnectionTarget) {
        mLastConnectionTarget->hightlight(false);
        qDebug() << "emit elementConnected";
        emit elementConnected(this->modelId(), mLastConnectionTarget->modelId());
        mLastConnectionTarget.clear();
    }
}

void HsmConnectableElement::updateConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos) {
    if (mConnection) {
        mConnection->moveConnectionTo(pos);

        // Check if there is an element under cursor and highlight it
        QPointer<HsmElement> element = connectableElementAt(pos);

        if (mLastConnectionTarget != element) {
            if (mLastConnectionTarget) {
                mLastConnectionTarget->hightlight(false);
            }

            if (element) {
                element->hightlight(true);
            }

            mLastConnectionTarget = element;
        }
    }
}

};  // namespace view