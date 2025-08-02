#ifndef HSMTRANSITION_HPP
#define HSMTRANSITION_HPP

#include <QLineF>
#include <QPen>

#include "private/HsmElement.hpp"

class QPainter;
class QWidget;
class QGraphicsSceneMouseEvent;

namespace view {

class ElementGripItem;

class HsmTransition : public HsmElement {
    Q_OBJECT

public:
    HsmTransition();
    // virtual ~HsmTransition() = default;
    virtual ~HsmTransition();

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    void beginConnection(const HsmElement* fromElement, const QPointF& pos);
    bool isConnecting() const;
    void moveConnectionTo(const QPointF& pos);
    void connectElements(const HsmElement* fromElement, const HsmElement* toElement);
    void removeConnection();

protected:
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void updateBoundingRect(const QRectF& newRect = QRectF()) override;

    QPolygonF calculateLinePolygon(int selectionOffset, const QLineF& line);
    void updateSelectionPolygon();

    QPointF findStartingPointFromPoint(const QRectF& rectFrom, const QPointF& pointTo);
    QPointF findStartingPointFromRect(const QRectF& rectFrom, const QRectF& rectTo);
    int findGripIndex(const ElementGripItem* grip);

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    std::tuple<bool, QPointF, int> isPointOnTheLine(const QPointF& pos);

    bool onGripMoved(const ElementGripItem* grip, const QPointF& pos) override;

protected slots:
    void onGripDoubleClick(ElementGripItem* grip);

protected slots:
    void recalculateLine();

private:
    static constexpr qreal SELECTION_DISTANCE = 2.0;

    bool mDebugShowSelectionPolygon = false;

    // TODO: use smart pointers
    const HsmElement* mFromElement = nullptr;
    const HsmElement* mToElement = nullptr;
    QPainterPath mSelectionPath;
    QPolygonF mLinePath;
    // QPolygonF mLineGrips;
    // QVector<QPointF> mLinePath;
    // TODO: use smart pointers
    // TODO: use list ??
    std::vector<ElementGripItem*> mLineGrips = {nullptr, nullptr};
    QPointF mCurrentMovePos;
};

}; // namespace view

#endif  // HSMTRANSITION_HPP
