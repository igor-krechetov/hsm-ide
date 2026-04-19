#ifndef HSMCONNECTABLEELEMENT_HPP
#define HSMCONNECTABLEELEMENT_HPP

#include <QGraphicsSceneMouseEvent>
#include <QMap>
#include <QPointer>

#include "ElementConnectionArrow.hpp"
#include "HsmElement.hpp"
#include "view/elements/HsmTransition.hpp"

class QEvent;

namespace model {
class StateMachineEntity;
}

namespace view {

class ElementBoundaryGripItem;

class HsmConnectableElement : public HsmElement {
    Q_OBJECT

public:
    explicit HsmConnectableElement(const HsmElementType elementType, const QSizeF& size);
    // virtual ~HsmConnectableElement() = default;
    virtual ~HsmConnectableElement();

    void init(const QSharedPointer<model::StateMachineEntity>& modelElement) override;

    bool isConnectable() const override;
    bool acceptsConnections() const override;
    const QRectF& hoverRect() const;

    bool hasVisibleArrows() const;

signals:
    void elementConnected(const model::EntityID_t fromElementId, const model::EntityID_t toElementId);

public slots:
    void viewTransformChanged() override;

// from HsmElement
protected:
    bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos) override;

    void updateBoundingRect(const QRectF& newRect) override;
    void updateHoverRect();
    void updateConnectionArrowsPos();

    // from QGraphicsObject
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void handleHoverEvent(const QPointF& pos);
    void createConnectionArrows();
    void removeConnectionArrows();
    void removeConnectionArrowsForOtherElements(const QPointF& sceneMousePos);
    QPointF getArrowPos(ElementConnectionArrow::Direction arrowDirection) const;
    QSizeF getArrowSize(ElementConnectionArrow::Direction arrowDirection) const;

private slots:
    void beginConnection(ElementConnectionArrow* arrow, const QPointF& pos);
    void finishConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos);
    void updateConnectionLine(ElementConnectionArrow* arrow, const QPointF& pos);

private:
    // TODO: use smart pointers
    QMap<ElementConnectionArrow::Direction, ElementConnectionArrow*> mArrows;
    QRectF mHoverRect;// contains VIEW based rect for tracking connection arrows
    bool mDrawConnectionLine = false;

    // TODO: replace with QSharedPointer
    std::shared_ptr<HsmTransition> mConnection;

    // HsmConnectableElementEventFilter* mEventFilter = nullptr;
    bool mEventFilterInstalled = false;
};

};  // namespace view

#endif  // HSMCONNECTABLEELEMENT_HPP
