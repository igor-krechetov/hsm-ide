#ifndef HSMCONNECTABLEELEMENT_HPP
#define HSMCONNECTABLEELEMENT_HPP

#include <QGraphicsSceneMouseEvent>
#include <QMap>
#include <QPointer>

#include "ElementConnectionArrow.hpp"
#include "HsmResizableElement.hpp"
#include "view/elements/HsmTransition.hpp"

namespace view {

class ElementBoundaryGripItem;

class HsmConnectableElement : public HsmResizableElement {
    Q_OBJECT

public:
    explicit HsmConnectableElement(const HsmElementType elementType);
    // virtual ~HsmConnectableElement() = default;
    virtual ~HsmConnectableElement();

    void init(const model::EntityID_t modelElementId) override;

    bool isConnectable() const override;

    // void addTransition(std::shared_ptr<HsmTransition>& transition, HsmConnectableElement* target);

    QRectF boundingRect() const override;
    void updateHoverRect(bool expendArea);
    void createConnectionArrows();
    void removeConnectionArrows();
    void updateConnectionArrowsPos();

signals:
    void elementConnected(const model::EntityID_t fromElementId, const model::EntityID_t toElementId);

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
    QRectF mHoverRect;
    bool mDrawConnectionLine = false;

    // TODO: replace with QSharedPointer
    std::shared_ptr<HsmTransition> mConnection;
};

};  // namespace view

#endif  // HSMCONNECTABLEELEMENT_HPP
