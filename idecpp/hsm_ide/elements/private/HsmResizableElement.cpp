#include "HsmResizableElement.hpp"

#include <QPainter>

HsmResizableElement::HsmResizableElement(QObject* parent)
    : QObject(parent)
    , QGraphicsItem()
    , mPenSelectedBorder(QColor("lightblue"));
{
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    createBoundaryGrips();
    setGripVisibility(false);
    mPenSelectedBorder.setStyle(Qt::DotLine);
}

void HsmResizableElement::createBoundaryGrips() {
    for (const auto& direction : {ElementBoundaryGripDirection::North,
                                  ElementBoundaryGripDirection::NorthEast,
                                  ElementBoundaryGripDirection::East,
                                  ElementBoundaryGripDirection::SouthEast,
                                  ElementBoundaryGripDirection::South,
                                  ElementBoundaryGripDirection::SouthWest,
                                  ElementBoundaryGripDirection::West,
                                  ElementBoundaryGripDirection::NorthWest}) {
        QGraphicsEllipseItem* newGrip = createGrip(direction);  // TODO: smart pointers

        newGrip->setEnabled(false);
        newGrip->setPos(gripPoint(direction));
        newGrip->setEnabled(true);
        mGrips[direction] = newGrip;
    }
}

void HsmResizableElement::updateGripsPosition(const QList<ElementBoundaryGripDirection>& directionsList) {
    for (const auto& direction : directionsList) {
        QGraphicsEllipseItem* g = mGrips[direction];  // TODO: add safety logic

        g->setEnabled(false);
        g->setPos(gripPoint(direction));
        g->setEnabled(true);
    }
}

void HsmResizableElement::setGripVisibility(bool visible) {
    mResizeMode = visible;

    for (QGraphicsEllipseItem* grip : mGrips) {
        grip->setVisible(visible);
    }

    update();
}

void HsmResizableElement::onGripLostFocus(QGraphicsEllipseItem* grip) {
    mGripSelected = isSelected();

    if (false == mGripSelected) {
        for (QGraphicsEllipseItem* direction : mGrips) {
            if (direction->isUnderMouse()) {
                mGripSelected = true;
                break;
            }
        }
    }

    setGripVisibility(isSelected() || mGripSelected);
}

bool HsmResizableElement::onGripMoved(ElementBoundaryGripItem* selectedGrip, const QPointF& pos) {
    QRectF newOuterRect = mOuterRect;
    QList<ElementBoundaryGripDirection> updateGrips = {ElementBoundaryGripDirection::North,
                                                       ElementBoundaryGripDirection::NorthEast,
                                                       ElementBoundaryGripDirection::East,
                                                       ElementBoundaryGripDirection::SouthEast,
                                                       ElementBoundaryGripDirection::South,
                                                       ElementBoundaryGripDirection::SouthWest,
                                                       ElementBoundaryGripDirection::West,
                                                       ElementBoundaryGripDirection::NorthWest};
    updateGrips.removeOne(selectedGrip->direction());

    switch (selectedGrip->direction()) {
        case ElementBoundaryGripDirection::North:
            newOuterRect.setTop(pos.y());
            updateGrips.removeOne(ElementBoundaryGripDirection::South);
            updateGrips.removeOne(ElementBoundaryGripDirection::SouthEast);
            updateGrips.removeOne(ElementBoundaryGripDirection::SouthWest);
            break;
        case ElementBoundaryGripDirection::NorthEast:
            newOuterRect.setTopRight(pos);
            updateGrips.removeOne(ElementBoundaryGripDirection::SouthWest);
            break;
        case ElementBoundaryGripDirection::East:
            newOuterRect.setRight(pos.x());
            updateGrips.removeOne(ElementBoundaryGripDirection::West);
            updateGrips.removeOne(ElementBoundaryGripDirection::NorthWest);
            updateGrips.removeOne(ElementBoundaryGripDirection::SouthWest);
            break;
        case ElementBoundaryGripDirection::SouthEast:
            newOuterRect.setBottomRight(pos);
            updateGrips.removeOne(ElementBoundaryGripDirection::NorthWest);
            break;
        case ElementBoundaryGripDirection::South:
            newOuterRect.setBottom(pos.y());
            updateGrips.removeOne(ElementBoundaryGripDirection::North);
            updateGrips.removeOne(ElementBoundaryGripDirection::NorthWest);
            updateGrips.removeOne(ElementBoundaryGripDirection::NorthEast);
            break;
        case ElementBoundaryGripDirection::SouthWest:
            newOuterRect.setBottomLeft(pos);
            updateGrips.removeOne(ElementBoundaryGripDirection::NorthEast);
            break;
        case ElementBoundaryGripDirection::West:
            newOuterRect.setLeft(pos.x());
            updateGrips.removeOne(ElementBoundaryGripDirection::East);
            updateGrips.removeOne(ElementBoundaryGripDirection::SouthEast);
            updateGrips.removeOne(ElementBoundaryGripDirection::NorthEast);
            break;
        case ElementBoundaryGripDirection::NorthWest:
            newOuterRect.setTopLeft(pos);
            updateGrips.removeOne(ElementBoundaryGripDirection::SouthEast);
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
int HsmResizableElement::indexOf(const QPointF& p) {
    for (int i = 0; i < 4; ++i) {
        if (p == point(i)) {
            return i;
        }
    }

    return -1;
}

QPointF HsmResizableElement::gripPoint(ElementBoundaryGripDirection gripDirection) {
    qreal xCenter = mOuterRect.center().x();
    qreal yCenter = mOuterRect.center().y();
    QMap<ElementBoundaryGripDirection, QPointF> positions = {
        {ElementBoundaryGripDirection::North, QPointF(xCenter, mOuterRect.top())},
        {ElementBoundaryGripDirection::NorthEast, mOuterRect.topRight()},
        {ElementBoundaryGripDirection::East, QPointF(mOuterRect.right(), yCenter)},
        {ElementBoundaryGripDirection::SouthEast, mOuterRect.bottomRight()},
        {ElementBoundaryGripDirection::South, QPointF(xCenter, mOuterRect.bottom())},
        {ElementBoundaryGripDirection::SouthWest, mOuterRect.bottomLeft()},
        {ElementBoundaryGripDirection::West, QPointF(mOuterRect.left(), yCenter)},
        {ElementBoundaryGripDirection::NorthWest, mOuterRect.topLeft()}};
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
            for (QGraphicsEllipseItem* grip : mGrips) {
                if (grip->isUnderMouse()) {
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

QGraphicsEllipseItem* HsmResizableElement::createGrip(ElementBoundaryGripDirection direction) {
    // TODO: use smart pointers
    QGraphicsEllipseItem* grip = new QGraphicsEllipseItem(this);

    grip->setRect(-2.5, -2.5, 5, 5);
    grip->setData(0, QVariant(static_cast<int>(direction)));

    return grip;
}
