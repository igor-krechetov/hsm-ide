#ifndef HSMHSMRESIZABLEELEMENT_HPP
#define HSMHSMRESIZABLEELEMENT_HPP

#include <QObject>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

enum class ElementBoundaryGripDirection {
    North,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest
};

class HsmResizableElement : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    explicit HsmResizableElement(QObject* parent = nullptr);
    virtual ~HsmResizableElement() = default;

    void createBoundaryGrips();
    void updateGripsPosition(const QList<ElementBoundaryGripDirection>& directionsList);
    void setGripVisibility(bool visible);
    void onGripLostFocus(QGraphicsEllipseItem* grip);
    bool onGripMoved(ElementBoundaryGripItem* selectedGrip, const QPointF& pos);
    int indexOf(const QPointF& p);
    QPointF gripPoint(ElementBoundaryGripDirection gripDirection);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void geometryChanged(HsmResizableElement* element);

protected:
    QVariant itemChange(const GraphicsItemChange change, const QVariant& value) override;

private:
    QGraphicsEllipseItem* createGrip(ElementBoundaryGripDirection direction);

private:
    QRectF mOuterRect;
    QMap<ElementBoundaryGripDirection, ElementBoundaryGripItem*> mGrips;
    QPen mPenSelectedBorder;
    bool mResizeMode = false;
    bool mGripSelected = false;
};

#endif // HSMHSMRESIZABLEELEMENT_HPP
