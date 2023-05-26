#ifndef ELEMENTGRIPITEM_HPP
#define ELEMENTGRIPITEM_HPP

#include <QGraphicsObject>

class ElementGripItem : public QGraphicsObject {
    Q_OBJECT

public:
    explicit ElementGripItem(const QGraphicsObject* annotationElement);
    virtual ~ElementGripItem() = default;

    QRectF boundingRect();

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

signals:
    void onGripDoubleClick(ElementGripItem* gripItem);
    void onGripLostFocus(ElementGripItem* gripItem);

private:
    void tryConnectSignal(PointerToMemberFunction signal, QObject* object, const char* functionName);

private:
    QRectF mGripRect;
    QColor mGripColor;
};

#endif  // ELEMENTGRIPITEM_HPP
