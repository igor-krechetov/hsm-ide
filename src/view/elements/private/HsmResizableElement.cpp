#include "HsmResizableElement.hpp"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

HsmResizableElement::HsmResizableElement(const HsmElementType elementType)
    : HsmElement(elementType) {}

void HsmResizableElement::init() {
    HsmElement::init();
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    createBoundaryGrips();
    setGripVisibility(false);
    mPenSelectedBorder.setStyle(Qt::DotLine);
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

    // TODO: implement min size concept
    if ((newOuterRect.height() > 0) && (newOuterRect.width() > 0)) {
        updateBoundingRect(newOuterRect);
        // mOuterRect = newOuterRect;
        updateGripsPosition(updateGrips);
        update();
        // self.scene().update()
        emit geometryChanged(this);
    } else {
        canResize = false;
    }

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

QRectF HsmResizableElement::boundingRect() const {
    return mOuterRect;
}

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
        emit geometryChanged(this);
    }

    return QGraphicsItem::itemChange(change, value);
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
