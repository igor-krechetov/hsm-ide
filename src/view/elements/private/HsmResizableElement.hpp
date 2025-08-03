#ifndef HSMRESIZABLEELEMENT_HPP
#define HSMRESIZABLEELEMENT_HPP

#include <QObject>
#include <QPen>
#include <map>

#include "ElementBoundaryGripItem.hpp"
#include "HsmElement.hpp"

class QPainter;

namespace view {

class HsmResizableElement : public HsmElement {
    Q_OBJECT

public:
    explicit HsmResizableElement(const HsmElementType elementType);
    // virtual ~HsmResizableElement() = default;
    virtual ~HsmResizableElement();

    void init(const model::EntityID_t modelElementId) override;

    void createBoundaryGrips();
    void updateGripsPosition(const QList<GripDirection>& directionsList);
    void setGripVisibility(bool visible);
    // int indexOf(const QPointF& p);
    QPointF gripPoint(GripDirection gripDirection);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

public slots:
    bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos);
    void onGripLostFocus(ElementBoundaryGripItem* grip);

signals:
    void geometryChanged(HsmResizableElement* element);

protected:
    QVariant itemChange(const GraphicsItemChange change, const QVariant& value) override;

private:
    ElementBoundaryGripItem* createGrip(GripDirection direction);

    // private:
public:
    // TODO: replace with QMap
    std::map<GripDirection, ElementBoundaryGripItem*> mGrips;
    QPen mPenSelectedBorder = QColor("lightblue");
    bool mResizeMode = false;
    bool mGripSelected = false;
};

}; // namespace view

#endif  // HSMRESIZABLEELEMENT_HPP
