#include "HsmResizableElement.hpp"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QDebug>
// #include <QCursor>
// #include <QGraphicsView>
namespace view {

HsmResizableElement::HsmResizableElement(const HsmElementType elementType)
    : HsmElement(elementType) {
    qDebug() << "CREATE: HsmResizableElement: " << this;
}

HsmResizableElement::~HsmResizableElement() {
    qDebug() << "DELETE " << this;
}

void HsmResizableElement::init(const model::EntityID_t modelElementId) {
    HsmElement::init(modelElementId);
    qDebug() << Q_FUNC_INFO << this << modelId();

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    createBoundaryGrips();
    setGripVisibility(false);
    mPenSelectedBorder.setStyle(Qt::DotLine);
}

bool HsmResizableElement::isResizable() const {
    return true;
}

void HsmResizableElement::updateBoundingRect(const QRectF& newRect) {
    HsmElement::updateBoundingRect(newRect);
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
    // update();
    // scene()->update();
    // qDebug() << Q_FUNC_INFO << boundingRect() << " --> " << newRect;
    notifyGeometryChanged();
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
            qDebug() << Q_FUNC_INFO << elementRect() << " --> " << parentRect;

            if ( parentRect.left() != parentNewRect.left() ) {
                parentNewRect.adjust(-5, 0, 0, 0);
            }
            if ( parentRect.top() != parentNewRect.top() ) {
                parentNewRect.adjust(0, -5, 0, 0);
            }
            if ( parentRect.right() != parentNewRect.right() ) {
                parentNewRect.adjust(0, 0, 5, 0);
            }
            if ( parentRect.bottom() != parentNewRect.bottom() ) {
                parentNewRect.adjust(0, 0, 0, 5);
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

void HsmResizableElement::resizeParentToFitChildItem() {
    auto parent = dynamic_cast<HsmResizableElement*>(parentItem());

    // TODO: check if parent is also resizable
    if (parent != nullptr) {
        qDebug() << Q_FUNC_INFO << "RESIZING PARENT";
        parent->resizeToFitChildItem(this);
    }
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

        if (grip.second->scene() == nullptr) {
            qDebug() << "ERROR: grip is not attached to sceen";
        }
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

bool HsmResizableElement::onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos) {
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
        case GripDirection::North:
            newOuterRect.setTop(pos.y());
            updateGrips.removeOne(GripDirection::South);
            updateGrips.removeOne(GripDirection::SouthEast);
            updateGrips.removeOne(GripDirection::SouthWest);
            break;
        case GripDirection::NorthEast:
            newOuterRect.setTopRight(pos);
            updateGrips.removeOne(GripDirection::SouthWest);
            break;
        case GripDirection::East:
            newOuterRect.setRight(pos.x());
            updateGrips.removeOne(GripDirection::West);
            updateGrips.removeOne(GripDirection::NorthWest);
            updateGrips.removeOne(GripDirection::SouthWest);
            break;
        case GripDirection::SouthEast:
            newOuterRect.setBottomRight(pos);
            updateGrips.removeOne(GripDirection::NorthWest);
            break;
        case GripDirection::South:
            newOuterRect.setBottom(pos.y());
            updateGrips.removeOne(GripDirection::North);
            updateGrips.removeOne(GripDirection::NorthWest);
            updateGrips.removeOne(GripDirection::NorthEast);
            break;
        case GripDirection::SouthWest:
            newOuterRect.setBottomLeft(pos);
            updateGrips.removeOne(GripDirection::NorthEast);
            break;
        case GripDirection::West:
            newOuterRect.setLeft(pos.x());
            updateGrips.removeOne(GripDirection::East);
            updateGrips.removeOne(GripDirection::SouthEast);
            updateGrips.removeOne(GripDirection::NorthEast);
            break;
        case GripDirection::NorthWest:
            newOuterRect.setTopLeft(pos);
            updateGrips.removeOne(GripDirection::SouthEast);
            break;
        default:
            // do nothing
            break;
    }

    bool canResize = true;
    QRectF childrenSize = childrenRect();

    qDebug() << childrenSize << newOuterRect << elementRect();

    if (childrenSize.bottom() < newOuterRect.bottom() &&
        childrenSize.right() < newOuterRect.right() &&
        childrenSize.top() > newOuterRect.top() &&
        childrenSize.left() > newOuterRect.left()) {
        // TODO: implement min size concept
        if ((newOuterRect.height() > 0) && (newOuterRect.width() > 0)) {
            updateBoundingRect(newOuterRect);
            // mOuterRect = newOuterRect;
            updateGripsPosition(updateGrips);
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

void HsmResizableElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (true == mResizeMode) {
        painter->setPen(mPenSelectedBorder);
        painter->drawRoundedRect(mOuterRect, 5, 5);

        painter->drawEllipse(QPointF(0, 0), 5, 5);
        painter->drawPoint(0, 0);
    }
}

void HsmResizableElement::notifyGeometryChanged() {
    emit geometryChanged(this);
}

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
    } else if (change == QGraphicsItem::ItemPositionHasChanged) {
        notifyGeometryChanged();
        resizeParentToFitChildItem();

        forEachChildElement([&](HsmElement* child){
            if (child && child->isResizable()) {
                qgraphicsitem_cast<HsmResizableElement*>(child)->notifyGeometryChanged();
            }
        });
    }

    return QGraphicsItem::itemChange(change, value);
}

// TODO: remove function?
ElementBoundaryGripItem* HsmResizableElement::createGrip(GripDirection direction) {
    qDebug() << Q_FUNC_INFO << this << modelId();
    // TODO: use smart pointers
    ElementBoundaryGripItem* grip = new ElementBoundaryGripItem(this, direction);

    // grip->setRect(-2.5, -2.5, 5, 5);
    // grip->setData(0, QVariant(static_cast<int>(direction)));
    qDebug() << "-- before init";
    grip->init();
    grip->setEnabled(false);
    grip->setPos(gripPoint(direction));
    grip->setEnabled(true);
    qDebug() << "-- after init";

    return grip;
}

};  // namespace view