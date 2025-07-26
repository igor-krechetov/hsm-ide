#ifndef HSMCONNECTABLEELEMENT_HPP
#define HSMCONNECTABLEELEMENT_HPP

#include "HsmResizableElement.hpp"
#include "ElementConnectionArrow.hpp"
#include "elements/HsmTransition.hpp"

#include <QGraphicsSceneMouseEvent>
#include <QMap>

class ElementBoundaryGripItem;

class HsmConnectableElement : public HsmResizableElement {
    Q_OBJECT

public:
    explicit HsmConnectableElement(const HsmElementType elementType);
    virtual ~HsmConnectableElement() = default;

    void init() override;

    void addTransition(std::shared_ptr<HsmTransition>& transition, HsmConnectableElement* target);

    QRectF boundingRect() const override;
    void updateHoverRect(bool expendArea);
    void createConnectionArrows();
    void removeConnectionArrows();
    void updateConnectionArrowsPos();

protected:
    bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QPointF getArrowPos(ElementConnectionArrow::Direction arrowDirection);

private slots:
    void beginConnection(ElementConnectionArrow* arrow, const QPointF& pos);
    void finishConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos);
    void updateConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos);

private:
    // TODO: use smart pointers
    QMap<ElementConnectionArrow::Direction, ElementConnectionArrow*> mArrows;
    // TODO: does it make sence to store all transitions inside this object?
    // TODO: remove shared_ptr. Transitions belong to the sceene and will be deleted by it
    // TODO: use QPointer
    QList<std::shared_ptr<HsmTransition>> mTransitions;
    QRectF mHoverRect;
    bool mDrawConnectionLine = false;
    // TODO: replace with shared_ptr
    std::shared_ptr<HsmTransition> mConnection;
};

#endif // HSMCONNECTABLEELEMENT_HPP
