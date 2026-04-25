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

enum class AutoLayoutDirection : int {
    HORIZONTAL = 0,
    VERTICAL = 1
};

class AutoGroupItem : public QGraphicsObject {
    Q_OBJECT
public:
    AutoGroupItem(QGraphicsItem* parent = nullptr);

    void setDirection(const AutoLayoutDirection direction);
    void makeMovable(const bool enable);

    QRectF boundingRect() const override;
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;
    int type() const override;

    void addItem(QGraphicsItem* item);

signals:
    void geometryChanged() const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void childEvent(QChildEvent* event) override;

public slots:
    void relayout();

private:
    void relayoutVertical();
    void relayoutHorizontal();

private:
    // needs to be mutable so we can update it in boundingRect()
    mutable QRectF mLastRect;
    AutoLayoutDirection mLayoutDirection = AutoLayoutDirection::VERTICAL;
};

};  // namespace view

#endif  // AUTOGROUPITEM_HPP