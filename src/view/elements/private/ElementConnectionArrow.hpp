#ifndef ELEMENTCONNECTIONARROW_HPP
#define ELEMENTCONNECTIONARROW_HPP

#include <QColor>
#include <QGraphicsObject>
#include <QPainter>
#include <QPointF>
#include <QRectF>

class QPainter;

namespace view {

class ElementConnectionArrow : public QGraphicsObject {
    Q_OBJECT

public:
    enum class Direction { North = 1, East = 3, South = 5, West = 7 };

public:
    ElementConnectionArrow(QGraphicsObject* annotationElement, Direction direction);
    virtual ~ElementConnectionArrow() = default;

    // TODO: check if it's ok to overload setPos
    void setPos(const QPointF& pos);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QPainterPath initShape(const Direction direction) const;

signals:
    void connectionPositionChanged(ElementConnectionArrow*, QPointF);
    void connectionStarted(ElementConnectionArrow*, QPointF);
    void connectionFinished(ElementConnectionArrow*, QPointF);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

public:
    static constexpr qreal mA = 10.0;
    static constexpr qreal mB = 16.0;
    static constexpr qreal mW = 3 * mA;

private:
    QPointF mPos;
    QRectF mOuterRect;
    Direction mDirection;
    QPainterPath mShapeArrow;
    QColor mArrowColor;
};

}; // namespace view

#endif  // ELEMENTCONNECTIONARROW_HPP
