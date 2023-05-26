#ifndef HSMTRANSITION_HPP
#define HSMTRANSITION_HPP

#include <QGraphicsItem>
#include <QLineF>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <cmath>

#include "HsmElement.hpp"

class QPainter;
class QWidget;

class HsmTransition : public HsmElement {
public:
    enum class Consts { SELECTION_DISTANCE = 2 };

    HsmTransition();

    virtual ~HsmTransition() = default;

protected:
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    virtual void geometryChangedEvent() override;

protected:
    void updateBoundingRect() override;

    QPolygonF calculateLinePolygon(int selectionOffset, const QLineF& line);
    void updateSelectionPolygon();
    void beginConnection(HsmElement* fromElement, const QPointF& pos);
    bool isConnecting() const;
    void moveConnectionTo(const QPointF& pos);
    void connectElements(HsmElement* fromElement, HsmElement* toElement);

    void removeConnection();
    QPointF findStartingPointFromPoint(const QRectF& rectFrom, const QPointF& pointTo);
    QPointF findStartingPointFromRect(const QRectF& rectFrom, const QRectF& rectTo);
    void recalculateLine(QObject* element = nullptr);
    bool onGripMoved(QObject* grip, const QPointF& pos);
    void onGripDoubleClick(QObject* grip);
    int findGripIndex(QObject* grip);
    void mouseDoubleClickEvent(QMouseEvent* event);
    std::tuple<bool, QPointF, int> isPointOnTheLine(const QPointF& pos);

    void recalculateLine();

private:
    constexpr qreal SELECTION_DISTANCE = 5.0;

    bool mDebugShowSelectionPolygon = false;

    // TODO: use smart pointers
    HsmElement* mFromElement = nullptr;
    HsmElement* mToElement = nullptr;
    QPainterPath mSelectionPath;
    QPolygonF mLinePath;
    QPolygonF mLineGrips;
    QVector<QPointF> mLinePath;
    QVector<QObject*> mLineGrips;
    QPointF mCurrentMovePos;
};

#endif  // HSMTRANSITION_HPP
