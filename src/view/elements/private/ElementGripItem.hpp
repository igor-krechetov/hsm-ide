#ifndef ELEMENTGRIPITEM_HPP
#define ELEMENTGRIPITEM_HPP

#include <QGraphicsObject>

namespace view {

class HsmElement;

class ElementGripItem : public QGraphicsObject {
    Q_OBJECT

public:
    explicit ElementGripItem(HsmElement* annotationElement);
    // virtual ~ElementGripItem() = default;
    virtual ~ElementGripItem();

    virtual void init();

    HsmElement* annotationElement() const;
    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

signals:
    void onGripDoubleClick(ElementGripItem* gripItem);
    void onGripLostFocus(ElementGripItem* gripItem);

private:
    void tryConnectSignal(const char* signal, QObject* object, const char* functionName);

private:
    HsmElement* mAnnotationElement = nullptr;
    QRectF mGripRect;
    QColor mGripColor;
    QPointF mLastPos;
};

}; // namespace view

#endif  // ELEMENTGRIPITEM_HPP
