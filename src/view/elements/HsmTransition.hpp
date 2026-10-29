#ifndef HSMTRANSITION_HPP
#define HSMTRANSITION_HPP

#include <QLineF>
#include <QPen>
#include <QPointer>

#include "private/HsmElement.hpp"
#include "private/HsmStateTextItem.hpp"

class QPainter;
class QWidget;
class QGraphicsSceneMouseEvent;

namespace view {

class AutoGroupItem;
class ElementGripItem;

class HsmTransition : public HsmElement {
    Q_OBJECT

public:
    static constexpr qreal SELECTION_DISTANCE = 2.0;
    static constexpr qreal ARROW_SIZE = 10.0;
    // Stacking order: a transition normally sits at DEFAULT_Z. While its grips are shown (hover,
    // selection or connecting) the whole transition is raised to ACTIVE_Z so its grips - and the
    // grips' child stacking - render above neighbouring transitions and their labels, which makes
    // closely packed transitions grabbable. ACTIVE_Z stays below a dragged element (z=100).
    static constexpr qreal DEFAULT_Z = 5.0;
    static constexpr qreal ACTIVE_Z = 50.0;
    static constexpr qreal SELFTRANSITION_X_OFFSET = 10.0;
    static constexpr qreal SELFTRANSITION_Y_OFFSET = 15.0;
    static constexpr qreal SELFTRANSITION_LENGTH = 40.0;

public:
    HsmTransition();
    // virtual ~HsmTransition() = default;
    virtual ~HsmTransition();

    void init(const QSharedPointer<model::StateMachineEntity>& modelEntity) override;

    QList<QGraphicsItem*> hsmChildItems() const override;
    void beginConnection(HsmElement* fromElement, const QPointF& pos);
    bool isConnecting() const;
    void moveConnectionTo(const QPointF& pos);
    void connectElements(HsmElement* fromElement, HsmElement* toElement);
    void disconnectElements();
    void beginEventEditMode();
    void beginEventTypingMode(const QString& newText);

    inline bool isSelfTransition() const;

    // Returns true when this transition is structurally valid, i.e. its endpoints live on the
    // same hierarchy level (same parent) - or, for a transition into a history state, on
    // different levels. An invalid transition is drawn in red. See StateHierarchyRules.
    bool isValid() const;

    QPointer<HsmElement> connectionCandidate() const;

signals:
    void transitionReconnected(const model::EntityID_t transitionId,
                               const model::EntityID_t fromElementId,
                               const model::EntityID_t toElementId);

protected:
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void updateBoundingRect(const QRectF& newRect = QRectF()) override;

    QPolygonF calculateLinePolygon(int selectionOffset, const QLineF& line);
    void updateSelectionPolygon();

    QPointF findStartingPointFromPoint(const QRectF& rectFrom, const QPointF& pointTo);
    QPointF findStartingPointFromRect(const QRectF& rectFrom, const QRectF& rectTo);
    QPointF findPerpendicularEdgePoint(const QRectF& rect, const QPointF& neighbourPoint);

    // When two elements face each other with overlapping edges and the transition has no
    // intermediate grips, a straight perpendicular segment can join them. Returns true and sets
    // outFrom/outTo to the centers of the overlapping edge spans; returns false when no such
    // straight perpendicular connection exists (the rects are diagonal or overlapping).
    bool findStraightPerpendicularConnection(const QRectF& rectFrom, const QRectF& rectTo, QPointF& outFrom, QPointF& outTo);
    int findGripIndex(const ElementGripItem* grip);

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    std::tuple<bool, QPointF, int> isPointOnTheLine(const QPointF& pos);

protected slots:
    bool onGripMoved(ElementGripItem* selectedGrip, const QPointF& delta) override;

    void onGripDoubleClick(ElementGripItem* grip);
    void onGripMoveEnterEvent(ElementGripItem* gripItem);
    void onGripMoveLeaveEvent(ElementGripItem* gripItem);

    void onEventEditFinished();
    void onConditionEditFinished();
    void onEventLabelPositionChanged();
    void onModelDataChanged() override;

    void recalculateLabelPosition();

public slots:
    void recalculateLine();

private:
    void setAllGripsVisibility(const bool visible);

    // Recomputes mIsValid from the current source/target endpoints and their parents,
    // requesting a repaint when the result changes.
    void updateValidity();

private:
    QPointer<HsmElement> mFromElement;
    QPointer<HsmElement> mToElement;
    QPainterPath mSelectionPath;
    QPolygonF mLinePath;
    std::vector<ElementGripItem*> mLineGrips;

    bool mConnecting = false;
    QPointer<HsmElement> mPrevConnectedElement;
    bool mIsValid = true;

    QPointer<ElementGripItem> mSrcGrip;
    QPointer<ElementGripItem> mDestGrip;
    QPointer<HsmElement> mLastConnectionTarget;
    QPointer<HsmStateTextItem> mLabelEvent;
    QPointer<HsmStateTextItem> mLabelCondition;
    AutoGroupItem* mLabel = nullptr;
    QPointF mLabelOffset = QPointF(0, 0);
};

inline bool HsmTransition::isSelfTransition() const {
    return ((nullptr != mFromElement) && (mFromElement == mToElement));
}

};  // namespace view

#endif  // HSMTRANSITION_HPP
