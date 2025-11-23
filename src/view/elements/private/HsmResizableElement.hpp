#ifndef HSMRESIZABLEELEMENT_HPP
#define HSMRESIZABLEELEMENT_HPP

#include <QMap>
#include <QObject>
#include <QPen>

#include "ElementBoundaryGripItem.hpp"
#include "HsmConnectableElement.hpp"

class QPainter;

namespace model {
class StateMachineEntity;
}

namespace view {

class HsmResizableElement : public HsmConnectableElement {
    Q_OBJECT

public:
    explicit HsmResizableElement(const HsmElementType elementType, const QSizeF& size);
    // virtual ~HsmResizableElement() = default;
    virtual ~HsmResizableElement();

    void init(const QSharedPointer<model::StateMachineEntity>& modelElement) override;
    virtual bool isResizable() const;

    // Resizes current element and updates the bounding rect. New position can be genative.
    // Clients must call normalizeElementRect() to tormalize the coordinates
    virtual void resizeElement(const QRectF& newRect);
    virtual void resizeToFitChildItem(HsmElement* child);
    // Normalizes bounding rect to start at 0,0 position
    virtual void normalizeElementRect();

    void createBoundaryGrips();
    void updateGripsPosition(const QList<GripDirection>& directionsList);
    void setGripVisibility(bool visible);
    QPointF gripPoint(GripDirection gripDirection);

    void setMinSize(qreal minWidth, qreal minHeight);
    void setMinWidth(qreal minWidth);
    void setMinHeight(qreal minHeight);
    QSizeF minSize() const;

public slots:
    bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos);
    void onGripLostFocus(ElementBoundaryGripItem* grip);

protected:
    void updateBoundingRect(const QRectF& newRect = QRectF()) override;
    QVariant itemChange(const GraphicsItemChange change, const QVariant& value) override;

    inline bool isResizing() const;

private:
    ElementBoundaryGripItem* createGrip(GripDirection direction);

protected:
    static constexpr int cChildPadding = 5;

private:
    QMap<GripDirection, ElementBoundaryGripItem*> mGrips;
    bool mResizeMode = false;
    bool mGripSelected = false;
    qreal mMinWidth = 10.0;
    qreal mMinHeight = 10.0;
};

inline bool HsmResizableElement::isResizing() const {
    return mResizeMode;
}

};  // namespace view

#endif  // HSMRESIZABLEELEMENT_HPP
