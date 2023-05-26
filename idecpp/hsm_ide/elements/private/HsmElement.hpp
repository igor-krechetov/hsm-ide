#ifndef HSMELEMENT_HPP
#define HSMELEMENT_HPP

#include <QGraphicsObject>

class HsmElement : public QGraphicsObject
{
    Q_OBJECT
public:
    HsmElement();

protected:
    QRectF elementRect() const;
    virtual void updateBoundingRect(const QRectF& newRect = QRectF());

// QGraphicsItem interface
public:
    QRectF boundingRect() const override;

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private:
    QSizeF mSize;
    QRectF mOuterRect;
};

#endif // HSMELEMENT_HPP
