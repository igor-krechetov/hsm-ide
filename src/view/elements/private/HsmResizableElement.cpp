#include "HsmResizableElement.hpp"

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
// #include <QCursor>
// #include <QGraphicsView>
namespace view {

HsmResizableElement::HsmResizableElement(const HsmElementType elementType, const QSizeF& size)
    : HsmConnectableElement(elementType, size) {
    qDebug() << "CREATE: HsmResizableElement: " << this;
}

HsmResizableElement::~HsmResizableElement() {
    qDebug() << "DELETE " << this;
}

void HsmResizableElement::init(const model::EntityID_t modelElementId) {
    HsmConnectableElement::init(modelElementId);
    qDebug() << Q_FUNC_INFO << this << modelId();

    createBoundaryGrips();
    setGripVisibility(false);
}

bool HsmResizableElement::isResizable() const {
    return true;
}

void HsmResizableElement::updateBoundingRect(const QRectF& newRect) {
    HsmConnectableElement::updateBoundingRect(newRect);
    // Resize parent to fit new size of current item
    resizeParentToFitChildItem();
}

void HsmResizableElement::resizeElement(const QRectF& newRect) {
    QList<GripDirection> updateGrips = {GripDirection::North,
                                        GripDirection::NorthEast,
                                        GripDirection::East,
                                        GripDirection::SouthEast,
                                        GripDirection::South,
                                        GripDirection::SouthWest,
                                        GripDirection::West,
                                        GripDirection::NorthWest};

    qDebug() << Q_FUNC_INFO << boundingRect() << mOuterRect << " --> " << newRect;
    updateBoundingRect(newRect);
    qDebug() << Q_FUNC_INFO << boundingRect() << mOuterRect << " --> " << newRect;
    updateGripsPosition(updateGrips);
    notifyGeometryChanged();

    // Need to update parts of connectable element
    HsmConnectableElement::updateHoverRect();
    HsmConnectableElement::updateConnectionArrowsPos();
}

void HsmResizableElement::resizeToFitChildItem(HsmElement* child) {
    if (isDirectChild(child) == true) {
        // Get child bounding rectangle in parent coordinates
        QRectF childRect = child->mapRectToParent(child->elementRect());
        // Get parent's current rectangle
        QRectF parentRect = elementRect();

        // qDebug() << Q_FUNC_INFO << newElement->boundingRect() << " --> ELEM: " << newElement->childrenBoundingRect();
        // qDebug() << Q_FUNC_INFO << parentRect << resizableParent->childrenBoundingRect();

        // Calculate required size to fit child
        // qreal requiredWidth = qMax(parentRect.width(), childRect.right() - parentRect.left());
        // qreal requiredHeight = qMax(parentRect.height(), childRect.bottom() - parentRect.top());

        // parentRect.setLeft(qMin(parentRect.left(), childRect.left() - 5));
        // parentRect.setTop(qMin(parentRect.top(), childRect.top() - 5));
        // parentRect.setRight(qMax(parentRect.right(), childRect.right() + 5));
        // parentRect.setBottom(qMax(parentRect.bottom(), childRect.bottom() + 5));
        QRectF parentNewRect = parentRect.united(childRect);

        // parentRect = resizableParent->childrenBoundingRect();

        if (parentRect != parentNewRect) {
            if (parentRect.left() != parentNewRect.left()) {
                parentNewRect.adjust(-cChildPadding, 0, 0, 0);
            }
            if (parentRect.top() != parentNewRect.top()) {
                parentNewRect.adjust(0, -cChildPadding, 0, 0);
            }
            if (parentRect.right() != parentNewRect.right()) {
                parentNewRect.adjust(0, 0, cChildPadding, 0);
            }
            if (parentRect.bottom() != parentNewRect.bottom()) {
                parentNewRect.adjust(0, 0, 0, cChildPadding);
            }

            // Expand parent if needed
            // if (requiredWidth > parentRect.width() || requiredHeight > parentRect.height()) {
            // Update parent size by moving the bottom-right grip
            // QPointF newSizePoint(parentRect.left() + requiredWidth, parentRect.top() + requiredHeight);

            // parentRect.setWidth(requiredWidth);
            // parentRect.setHeight(requiredHeight);
            resizeElement(parentNewRect);
            // parentElement->onGripMoved(
            //     nullptr, // No specific grip selected
            //     newSizePoint
            // );

            // Emit geometry changed signal
            // emit resizableParent->geometryChanged(resizableParent);
            // }
            resizeParentToFitChildItem();
        }
    }
}

void HsmResizableElement::normalizeElementRect() {
    QList<GripDirection> updateGrips = {GripDirection::North,
                                        GripDirection::NorthEast,
                                        GripDirection::East,
                                        GripDirection::SouthEast,
                                        GripDirection::South,
                                        GripDirection::SouthWest,
                                        GripDirection::West,
                                        GripDirection::NorthWest};
    QPointF newPositionDelta = mOuterRect.topLeft();
    QRectF newOuterRect = mOuterRect;

    newOuterRect.moveTo(0, 0);

    setPos(pos() + newPositionDelta);
    updateBoundingRect(newOuterRect);

    updateGripsPosition(updateGrips);

    // Update position of child items
    forEachChildElement(
        [&](HsmElement* child) {
            qDebug() << "child:" << child->pos() << " -> " << (child->pos() - newPositionDelta);
            child->setPos(child->pos() - newPositionDelta);
        },
        1);

    update();
    notifyGeometryChanged();
}

void HsmResizableElement::createBoundaryGrips() {
    QList<GripDirection> directions = {GripDirection::North,
                                       GripDirection::NorthEast,
                                       GripDirection::East,
                                       GripDirection::SouthEast,
                                       GripDirection::South,
                                       GripDirection::SouthWest,
                                       GripDirection::West,
                                       GripDirection::NorthWest};

    for (const auto& direction : directions) {
        ElementBoundaryGripItem* newGrip = createGrip(direction);  // TODO: smart pointers

        mGrips[direction] = newGrip;
    }
}

void HsmResizableElement::updateGripsPosition(const QList<GripDirection>& directionsList) {
    for (const auto& direction : directionsList) {
        ElementBoundaryGripItem* g = mGrips[direction];  // TODO: add safety logic

        g->setEnabled(false);
        g->setPos(gripPoint(direction));
        g->setEnabled(true);
    }
}

void HsmResizableElement::setGripVisibility(bool visible) {
    mResizeMode = visible;

    for (const auto& grip : mGrips) {
        grip.second->setVisible(visible);

        // if (grip.second->scene() == nullptr) {
        //     qDebug() << "ERROR: grip is not attached to sceen";
        // }
    }

    update();
}

void HsmResizableElement::onGripLostFocus(ElementBoundaryGripItem* grip) {
    mGripSelected = isSelected();

    if (false == mGripSelected) {
        for (const auto& curGrip : mGrips) {
            if (curGrip.second->isUnderMouse()) {
                mGripSelected = true;
                break;
            }
        }
    }

    setGripVisibility(isSelected() || mGripSelected);
}

bool HsmResizableElement::onGripMoved(const ElementGripItem* selectedGrip, const QPointF& gripPos) {
    if (gripPos.isNull()) {
        qDebug() << "SKIP NULL DELTA";
        return false;
    }

    // QPointF newPosition = mapFromParent(pos());
    QPointF newPositionDelta;
    QRectF newOuterRect = mOuterRect;
    QList<GripDirection> updateGrips = {GripDirection::North,
                                        GripDirection::NorthEast,
                                        GripDirection::East,
                                        GripDirection::SouthEast,
                                        GripDirection::South,
                                        GripDirection::SouthWest,
                                        GripDirection::West,
                                        GripDirection::NorthWest};
    // TODO: is there a better option instead of casting?
    const GripDirection gripDirection = reinterpret_cast<const ElementBoundaryGripItem*>(selectedGrip)->direction();

    updateGrips.removeOne(gripDirection);

    switch (gripDirection) {
        case GripDirection::SouthWest:
            newPositionDelta.setX(gripPos.x());
            // newOuterRect.setBottomLeft(gripPos);
            newOuterRect.adjust(0, 0, -gripPos.x(), gripPos.y());
            // updateGrips.removeOne(GripDirection::NorthEast);
            break;
        case GripDirection::West:
            newPositionDelta.setX(gripPos.x());
            newOuterRect.setLeft(gripPos.x());
            // updateGrips.removeOne(GripDirection::East);
            // updateGrips.removeOne(GripDirection::SouthEast);
            // updateGrips.removeOne(GripDirection::NorthEast);
            break;
        case GripDirection::NorthWest:
            newPositionDelta = gripPos;
            newOuterRect.setTopLeft(gripPos);
            // updateGrips.removeOne(GripDirection::SouthEast);
            break;
        case GripDirection::North:
            newPositionDelta.setY(gripPos.y());
            newOuterRect.setTop(gripPos.y());
            // updateGrips.removeOne(GripDirection::South);
            // updateGrips.removeOne(GripDirection::SouthEast);
            // updateGrips.removeOne(GripDirection::SouthWest);
            break;
        case GripDirection::NorthEast:
            newPositionDelta.setY(gripPos.y());
            newOuterRect.adjust(0, 0, gripPos.x(), -gripPos.y());
            // updateGrips.removeOne(GripDirection::SouthWest);
            break;

        case GripDirection::East:
            newOuterRect.adjust(0, 0, gripPos.x(), 0);
            updateGrips.removeOne(GripDirection::West);
            updateGrips.removeOne(GripDirection::NorthWest);
            updateGrips.removeOne(GripDirection::SouthWest);
            break;
        case GripDirection::SouthEast:
            // newOuterRect.setBottomRight(gripPos);
            newOuterRect.adjust(0, 0, gripPos.x(), gripPos.y());
            updateGrips.removeOne(GripDirection::NorthWest);
            break;
        case GripDirection::South:
            // newOuterRect.setBottom(gripPos.y());
            newOuterRect.adjust(0, 0, 0, gripPos.y());
            updateGrips.removeOne(GripDirection::North);
            updateGrips.removeOne(GripDirection::NorthWest);
            updateGrips.removeOne(GripDirection::NorthEast);
            break;

        default:
            // do nothing
            break;
    }

    // qDebug() << "GRIP MOVED: " << (int)gripDirection << " POS: " << gripPos << " -> " << newPositionDelta;
    // qDebug() << "NEW POS: " << pos() << " -> " << pos() + newPositionDelta << " : " << newOuterRect;

    newOuterRect.moveTo(0, 0);

    bool canResize = true;
    QRectF childrenSize = childrenRect();

    // qDebug() << childrenSize << newOuterRect << elementRect();
    if (childrenSize.isNull() == false) {
        childrenSize.adjust(-newPositionDelta.x(), -newPositionDelta.y(), -newPositionDelta.x(), -newPositionDelta.y());
        childrenSize.adjust(-cChildPadding, -cChildPadding, cChildPadding, cChildPadding);
    }

    // qDebug() << "CHILD RECT:" << childrenSize << "NEW RECT:" << newOuterRect << "DELTA:" << newPositionDelta;
    // qDebug() << childrenSize.topLeft() << childrenSize.bottomRight();

    // qDebug() << (newOuterRect.width() > childrenSize.width())
    //          << (newOuterRect.height() > childrenSize.height())
    //          << (childrenSize.bottom() < newOuterRect.bottom())
    //          << (childrenSize.right() < newOuterRect.right())
    //          << (childrenSize.top() > newOuterRect.top())
    //          << (childrenSize.left() > newOuterRect.left());

    if (childrenSize.isNull() ||
        (newOuterRect.width() > childrenSize.width() && newOuterRect.height() > childrenSize.height() &&
         childrenSize.bottom() < newOuterRect.bottom() && childrenSize.right() < newOuterRect.right() &&
         childrenSize.top() > newOuterRect.top() && childrenSize.left() > newOuterRect.left())) {
        // TODO: implement min size concept
        if ((newOuterRect.height() > 0) && (newOuterRect.width() > 0)) {
            setPos(pos() + newPositionDelta);
            updateBoundingRect(newOuterRect);

            // mOuterRect = newOuterRect;
            updateGripsPosition(updateGrips);

            // Update position of child items
            forEachChildElement([&](HsmElement* child) { child->setPos(child->pos() - newPositionDelta); }, 1);

            update();
            // self.scene().update()
            notifyGeometryChanged();
        } else {
            canResize = false;
        }
    } else {
        qDebug() << "can't resize because child items wont fit";
        canResize = false;
    }

    // TODO: would be nice to resume resizing only when cursor reaches the grip again

    // if (canResize == false) {
    //     QPointF viewPos = mapFromScene(pos);
    //     QPointF screenPos = scene()->views().first()->viewport()->mapToGlobal(viewPos);
    //     qDebug() << Q_FUNC_INFO << pos << viewPos << screenPos;
    //     QCursor::setPos(screenPos.toPoint());
    // }

    return canResize;
}

// TODO: check
// int HsmResizableElement::indexOf(const QPointF& p) {
//     for (int i = 0; i < 4; ++i) {
//         if (p == point(i)) {
//             return i;
//         }
//     }

//     return -1;
// }

QPointF HsmResizableElement::gripPoint(GripDirection gripDirection) {
    const QPointF elementCenter = mOuterRect.center();
    qreal xCenter = elementCenter.x();
    qreal yCenter = elementCenter.y();
    QMap<GripDirection, QPointF> positions = {{GripDirection::North, QPointF(xCenter, mOuterRect.top())},
                                              {GripDirection::NorthEast, mOuterRect.topRight()},
                                              {GripDirection::East, QPointF(mOuterRect.right(), yCenter)},
                                              {GripDirection::SouthEast, mOuterRect.bottomRight()},
                                              {GripDirection::South, QPointF(xCenter, mOuterRect.bottom())},
                                              {GripDirection::SouthWest, mOuterRect.bottomLeft()},
                                              {GripDirection::West, QPointF(mOuterRect.left(), yCenter)},
                                              {GripDirection::NorthWest, mOuterRect.topLeft()}};
    return positions[gripDirection];
}

// QRectF HsmResizableElement::boundingRect() const {
//     return mOuterRect;
// }

// void HsmResizableElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
//     Q_UNUSED(option)
//     Q_UNUSED(widget)

//     if (true == mResizeMode) {
//         painter->setPen(mPenSelectedBorder);
//         painter->drawRoundedRect(mOuterRect, 5, 5);

//         painter->drawEllipse(QPointF(0, 0), 5, 5);
//         painter->drawPoint(0, 0);
//     }
// }

QVariant HsmResizableElement::itemChange(const GraphicsItemChange change, const QVariant& value) {
    if (QGraphicsItem::ItemSelectedHasChanged == change) {
        mGripSelected = isSelected();

        if (false == mGripSelected) {
            for (const auto& curGrip : mGrips) {
                if (curGrip.second->isUnderMouse()) {
                    mGripSelected = true;
                    break;
                }
            }
        }

        setGripVisibility(isSelected() || mGripSelected);
    }

    return HsmConnectableElement::itemChange(change, value);
}

// TODO: remove function?
ElementBoundaryGripItem* HsmResizableElement::createGrip(GripDirection direction) {
    // TODO: use smart pointers
    ElementBoundaryGripItem* grip = new ElementBoundaryGripItem(this, direction);

    // grip->setRect(-2.5, -2.5, 5, 5);
    // grip->setData(0, QVariant(static_cast<int>(direction)));
    grip->init();
    grip->setEnabled(false);
    grip->setPos(gripPoint(direction));
    grip->setEnabled(true);

    return grip;
}

};  // namespace view