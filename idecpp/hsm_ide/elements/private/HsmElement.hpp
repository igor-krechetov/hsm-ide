#ifndef HSMELEMENT_HPP
#define HSMELEMENT_HPP

#include <QGraphicsObject>
#include <QPoint>

class ElementGripItem;

enum class HsmElementType {
    UNKNOWN,

    START,
    FINAL,
    ENTRY_POINT,
    EXIT_POINT,
    STATE,
    TRANSITION,
    HISTORY
};

constexpr int USERDATA_HSM_ELEMENT_TYPE = 1;

class HsmElement : public QGraphicsObject
{
    Q_OBJECT

public:
    HsmElement(const HsmElementType elementType);

    HsmElementType elementType() const;
    QRectF elementRect() const;

    virtual void init();
    virtual bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos);

// QGraphicsItem interface
public:
    QRectF boundingRect() const override;

protected:
    virtual void updateBoundingRect(const QRectF& newRect = QRectF());

    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

protected:
    QSizeF mSize;
    QRectF mOuterRect;

private:
    HsmElementType mType = HsmElementType::UNKNOWN;
};

#endif // HSMELEMENT_HPP
