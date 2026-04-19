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
    constexpr static int cGripSize = 16;

public:
    explicit ElementGripItem(HsmElement* annotationElement, const GripDirection type = GripDirection::FreeMove);
    // virtual ~ElementGripItem() = default;
    virtual ~ElementGripItem();

    virtual void init();

    HsmElement* annotationElement() const;
    QRectF boundingRect() const override;

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
    QPointF mLastPos;
    bool mDragging = false;
};

};  // namespace view

#endif  // ELEMENTGRIPITEM_HPP
