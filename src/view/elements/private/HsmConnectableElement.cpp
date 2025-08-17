#include "HsmConnectableElement.hpp"

#include <QGraphicsScene>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>

#include "ElementBoundaryGripItem.hpp"

namespace view {

// HsmConnectableElementEventFilter::HsmConnectableElementEventFilter(HsmConnectableElement* parent) : QObject(parent), mElement(parent) {}

HsmConnectableElement::HsmConnectableElement(const HsmElementType elementType)
    : HsmResizableElement(elementType) {
    qDebug() << "CREATE: HsmConnectableElement: " << this;
}

HsmConnectableElement::~HsmConnectableElement() {
    qDebug() << "DELETE " << this;

    for (const auto& direction : mArrows.keys()) {
        scene()->removeItem(mArrows[direction]);
        mArrows[direction]->deleteLater();
    }

    mArrows.clear();
}

void HsmConnectableElement::init(const model::EntityID_t modelElementId) {
    HsmResizableElement::init(modelElementId);
    updateHoverRect();
    setAcceptHoverEvents(true);
}

bool HsmConnectableElement::isConnectable() const {
    return true;
}

// void HsmConnectableElement::addTransition(std::shared_ptr<HsmTransition>& transition, HsmConnectableElement* target) {
//     if (scene()->items().contains(transition.get()) == false) {
//         scene()->addItem(transition.get());
//     }

//     transition->connectElements(this, target);
//     mTransitions.append(transition);
// }

void HsmConnectableElement::resizeElement(const QRectF& newRect) {
    HsmResizableElement::resizeElement(newRect);
    updateHoverRect();
    updateConnectionArrowsPos();
}

// QRectF HsmConnectableElement::boundingRect() const {
//     return mHoverRect;
// }

// TODO: try to implement same logic using qgraphicsitemgroup
void HsmConnectableElement::updateHoverRect() {
    qreal arrowOffset = 0.0;

    // update();// TODO: why wasw this needed?
    // prepareGeometryChange();

    // if (true == expandArea) {
        arrowOffset = ElementConnectionArrow::mW;
    // }

    mHoverRect = QRectF(mOuterRect.left() - arrowOffset,
                        mOuterRect.top() - arrowOffset,
                        mOuterRect.width() + 2 * arrowOffset,
                        mOuterRect.height() + 2 * arrowOffset);
}

// TODO: delete every time or change visibility?
void HsmConnectableElement::createConnectionArrows() {
    if (true == mArrows.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "CREATE";
        updateHoverRect();

        for (const auto direction : {ElementConnectionArrow::Direction::North,
                                     ElementConnectionArrow::Direction::East,
                                     ElementConnectionArrow::Direction::South,
                                     ElementConnectionArrow::Direction::West}) {
            ElementConnectionArrow* newArrow = new ElementConnectionArrow(nullptr, direction);
            scene()->addItem(newArrow);

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
    } else {
        qDebug() << Q_FUNC_INFO << "REATTACH";
        for (const auto& direction : mArrows.keys()) {
            // TODO: find then use
            // mArrows[direction]->setParentItem(nullptr);
            // qDebug() << Q_FUNC_INFO << __LINE__;
            mArrows[direction]->setPos(getArrowPos(direction));
            scene()->addItem(mArrows[direction]);
            mArrows[direction]->show();
            // qDebug() << Q_FUNC_INFO << __LINE__;
            // mArrows[direction]->deleteLater();
        }
    }

    if (false == mEventFilterInstalled) {
        // mEventFilter = new HsmConnectableElementEventFilter(this);
        mEventFilterInstalled = true;
        scene()->installEventFilter(this);
    }
}

void HsmConnectableElement::removeConnectionArrows() {
    if (hasVisibleArrows() == true) {
        for (const auto& direction : mArrows.keys()) {
            // TODO: find then use
            // mArrows[direction]->setParentItem(nullptr);
            // qDebug() << Q_FUNC_INFO << __LINE__;
            mArrows[direction]->hide();
            scene()->removeItem(mArrows[direction]);
            // qDebug() << Q_FUNC_INFO << __LINE__;
            // mArrows[direction]->deleteLater();
        }

        // mArrows.clear();
        updateHoverRect();
    }
}

void HsmConnectableElement::updateConnectionArrowsPos() {
    for (const auto& direction : mArrows.keys()) {
        mArrows[direction]->setPos(getArrowPos(direction));
    }
}

bool HsmConnectableElement::hasVisibleArrows() const {
    return (mArrows.isEmpty() == false && mArrows.first()->isVisible() == true);
}

bool HsmConnectableElement::onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos) {
    const bool canResize = HsmResizableElement::onGripMoved(selectedGrip, pos);

    if (true == canResize) {
        updateHoverRect();
        updateConnectionArrowsPos();
    }

    return canResize;
}

QPointF HsmConnectableElement::getArrowPos(ElementConnectionArrow::Direction arrowDirection) const {
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
    return mapToScene(positions[arrowDirection]);
    // return QPointF(xCenter, yCenter);
}

QSizeF HsmConnectableElement::getArrowSize(ElementConnectionArrow::Direction arrowDirection) const {
    QSizeF res;

    if (mArrows.empty() == false) {
        res = mArrows[arrowDirection]->boundingRect().size();
    }

    return res;
}

void HsmConnectableElement::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (hasVisibleArrows() == false) {
        const QRectF sceneRect = mapRectToScene(mOuterRect);
        const QPointF sceneMousePos = mapToScene(event->pos());

        if (sceneRect.contains(sceneMousePos)) {
            createConnectionArrows();
        }

        auto parentConnectable = qgraphicsitem_cast<HsmConnectableElement*>(parentItem());
        if (parentConnectable) {
            parentConnectable->removeConnectionArrows();
        }
    }

    HsmResizableElement::hoverMoveEvent(event);
}

void HsmConnectableElement::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    HsmResizableElement::hoverLeaveEvent(event);
    // removeConnectionArrows();
}

// TODO: redraw transitions to subitems

QVariant HsmConnectableElement::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (QGraphicsItem::ItemPositionHasChanged == change) {
        qDebug() << Q_FUNC_INFO << this;
        removeConnectionArrows();
        // TODO: Hide grips

        // Hide arrows for child elements (if any)
        forEachChildElement([](HsmElement* child){
            if (child && child->isConnectable()) {
                qgraphicsitem_cast<HsmConnectableElement*>(child)->removeConnectionArrows();
            }
        });
    }

    return HsmResizableElement::itemChange(change, value);
}

bool HsmConnectableElement::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::GraphicsSceneMouseMove) {
        if (mDrawConnectionLine == false) {
            QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
            const QPointF scenePos = mouseEvent->scenePos();
            const QSizeF arrowSize = getArrowSize(ElementConnectionArrow::Direction::West);
            const QRectF hoverRect = boundingRect().adjusted(-arrowSize.width(), -arrowSize.width(), arrowSize.width(), arrowSize.width());
            const QRectF sceneHoverRect = mapRectToScene(mHoverRect);

            if (sceneHoverRect.contains(scenePos) == false) {
                scene()->removeEventFilter(this);
                removeConnectionArrows();
                mEventFilterInstalled = false;
            }
        }
    }

    return false;
}

void HsmConnectableElement::beginConnection(ElementConnectionArrow* arrow, const QPointF& pos) {
    qDebug() << Q_FUNC_INFO;
    mDrawConnectionLine = true;
    mConnection = std::make_shared<HsmTransition>();
    mConnection->init(model::INVALID_MODEL_ID);
    mConnection->beginConnection(this, pos);
    scene()->addItem(mConnection.get());
}

// TODO: do we still need it?
void HsmConnectableElement::finishConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos) {
    qDebug() << Q_FUNC_INFO;
    mDrawConnectionLine = false;

    if (mConnection) {
        QPointer<HsmElement> targetElement = mConnection->connectionCandidate();

        qDebug() << "FINISH - arrow. Target" << targetElement << pos;
        scene()->removeItem(mConnection.get());
        mConnection.reset();

        if (targetElement) {
            targetElement->hightlight(false);
            qDebug() << "emit elementConnected";
            emit elementConnected(this->modelId(), targetElement->modelId());
        }
    }
}

void HsmConnectableElement::updateConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos) {
    if (mConnection) {
        mConnection->moveConnectionTo(pos);
    }
}

};  // namespace view