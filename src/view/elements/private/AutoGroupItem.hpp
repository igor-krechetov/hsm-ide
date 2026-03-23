#ifndef AUTOGROUPITEM_HPP
#define AUTOGROUPITEM_HPP

#include <QGraphicsObject>
#include <QRectF>
#include <QVariant>

class QGraphicsItem;
class QChildEvent;
class QMetaObject;
class QPainter;

namespace view {

    class AutoGroupItem : public QGraphicsObject {
    Q_OBJECT
public:
    AutoGroupItem(QGraphicsItem* parent = nullptr);

    void makeMovable(const bool enable);

    QRectF boundingRect() const override;
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;
    int type() const override;

signals:
    void geometryChanged() const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void childEvent(QChildEvent* event) override;

private slots:
    void relayout();

private:
    // needs to be mutable so we can update it in boundingRect()
    mutable QRectF mLastRect;
};

};  // namespace view

#endif  // AUTOGROUPITEM_HPP