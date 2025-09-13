#ifndef HSMRESIZABLEELEMENT_HPP
#define HSMRESIZABLEELEMENT_HPP

#include <QObject>
#include <QPen>
#include <map>

#include "ElementBoundaryGripItem.hpp"
#include "HsmConnectableElement.hpp"

class QPainter;

namespace view {

class HsmResizableElement : public HsmConnectableElement {
    Q_OBJECT

public:
    explicit HsmResizableElement(const HsmElementType elementType);
    // virtual ~HsmResizableElement() = default;
    virtual ~HsmResizableElement();

    void init(const model::EntityID_t modelElementId) override;
    virtual bool isResizable() const;

    void updateBoundingRect(const QRectF& newRect = QRectF()) override;

    // Resizes current element and updates the bounding rect. New position can be genative.
    // Clients must call normalizeElementRect() to tormalize the coordinates
    virtual void resizeElement(const QRectF& newRect);
    void resizeToFitChildItem(HsmElement* child);
    // Normalizes bounding rect to start at 0,0 position
    void normalizeElementRect();

    void createBoundaryGrips();
    void updateGripsPosition(const QList<GripDirection>& directionsList);
    void setGripVisibility(bool visible);
    QPointF gripPoint(GripDirection gripDirection);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

public slots:
    bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos);
    void onGripLostFocus(ElementBoundaryGripItem* grip);

protected:
    QVariant itemChange(const GraphicsItemChange change, const QVariant& value) override;

private:
    ElementBoundaryGripItem* createGrip(GripDirection direction);

private:
    static constexpr int cChildPadding = 5;
    // public:
    // TODO: replace with QMap
    std::map<GripDirection, ElementBoundaryGripItem*> mGrips;
    QPen mPenSelectedBorder = QColor("lightblue");
    bool mResizeMode = false;
    bool mGripSelected = false;
};

};  // namespace view

#endif  // HSMRESIZABLEELEMENT_HPP
