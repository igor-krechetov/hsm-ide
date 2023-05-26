#ifndef HSMCONNECTABLEELEMENT_HPP
#define HSMCONNECTABLEELEMENT_HPP

#include "HsmResizableElement.hpp"
#include "ElementConnectionArrow.hpp"
#include "HsmTransition.hpp"

#include <QGraphicsSceneMouseEvent>
#include <QMap>

class HsmConnectableElement : public HsmResizableElement {
    Q_OBJECT

public:
    HsmConnectableElement();
    virtual ~HsmConnectableElement() = default;

    void addTransition(HsmTransition* transition, HsmConnectableElement* target);

    QRectF boundingRect() const override;
    void updateHoverRect(bool expendArea);
    void createConnectionArrows();
    void removeConnectionArrows();
    void updateConnectionArrowsPos();

protected:
    void onGripMoved(GripPosition selectedGrip, const QPointF& pos) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QPointF getArrowPos(ElementConnectionArrow::Direction arrowDirection);
    void beginConnection(ElementConnectionArrow* arrow, const QPointF& pos);
    void finishConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos);
    void updateConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos);

private:
    // TODO: use smart pointers
    QMap<ElementConnectionArrow::Direction, ElementConnectionArrow*> mArrows;
    QList<HsmTransition*> mTransitions;
    QRectF mHoverRect;
    bool mDrawConnectionLine = false;
    HsmTransition* mConnection = nullptr;
};

#endif // HSMCONNECTABLEELEMENT_HPP
