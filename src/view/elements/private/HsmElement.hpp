#ifndef HSMELEMENT_HPP
#define HSMELEMENT_HPP

#include <QGraphicsObject>
#include <QPoint>
#include <QString>

namespace view {

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

class HsmElement : public QGraphicsObject {
    Q_OBJECT

public:
    explicit HsmElement(const HsmElementType elementType);
    explicit HsmElement(const HsmElementType elementType, const uint32_t modelElementId);
    virtual ~HsmElement();

    uint32_t modelId() const;
    void setModelId(const uint32_t modelElementId);

    HsmElementType elementType() const;
    QRectF elementRect() const;

    virtual void init();

    virtual bool isConnectable() const;

    // TODO: why is this here?
    virtual bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos);

    // QGraphicsItem interface
public:
    QRectF boundingRect() const override;

protected:
    virtual void updateBoundingRect(const QRectF& newRect = QRectF());

    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;

protected:
    uint32_t mModelElementId;
    QSizeF mSize;
    QRectF mOuterRect;

private:
    HsmElementType mType = HsmElementType::UNKNOWN;
};

}; // namespace view

#endif  // HSMELEMENT_HPP
