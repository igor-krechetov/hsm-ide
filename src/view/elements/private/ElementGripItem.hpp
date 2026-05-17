#ifndef ELEMENTGRIPITEM_HPP
#define ELEMENTGRIPITEM_HPP

#include <QGraphicsObject>

namespace view {

class HsmElement;

enum class GripDirection {
    North = 1,
    NorthEast = 2,
    East = 3,
    SouthEast = 4,
    South = 5,
    SouthWest = 6,
    West = 7,
    NorthWest = 8,
    FreeMove = 9
};

class ElementGripItem : public QGraphicsObject {
    Q_OBJECT

public:
    constexpr static int cGripSize = 10;// TODO: move to theme

public:
    explicit ElementGripItem(HsmElement* annotationElement, const GripDirection type = GripDirection::FreeMove);
    // virtual ~ElementGripItem() = default;
    virtual ~ElementGripItem();

    virtual void init();

    HsmElement* annotationElement() const;
    QRectF boundingRect() const override;
    // Visual position of the grip. Normallly same as pos() unless grip is being dragged.
    // During dragging physical position doesnt change till dragging is finished, but 
    // rendering position changes
    QPointF renderingPos() const;
    QPointF renderingPosScene() const;


    GripDirection direction() const;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    int type() const override;

signals:
    void gripMoved(ElementGripItem* grip, const QPointF& delta);
    void onGripDoubleClick(ElementGripItem* gripItem);
    void onGripLostFocus(ElementGripItem* gripItem);
    // void onGripMoved(ElementGripItem* gripItem, const QPointF& pos);
    void onGripMoveEnterEvent(ElementGripItem* gripItem);
    void onGripMoveLeaveEvent(ElementGripItem* gripItem);

protected:
    QPointF alignToGrid(const QPointF& localPos) const;

private:
    HsmElement* mAnnotationElement = nullptr;
    GripDirection mGripDirection = GripDirection::FreeMove;
    QRectF mGripRect;
    bool mHovered = false;
    bool mDragging = false;

protected:
    QPointF mLastDragPos;
    QPointF mLastScenePos;
    QPointF mLastSceneDelta;
    QPointF mRenderingOffset;

    QPointF mDragStartScenePos;
    QPointF mStartGripScenePos;
    QPointF mRenderingOffsetAtDragStart;
};

};  // namespace view

#endif  // ELEMENTGRIPITEM_HPP
