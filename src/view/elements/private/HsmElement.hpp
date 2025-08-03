#ifndef HSMELEMENT_HPP
#define HSMELEMENT_HPP

#include <QGraphicsObject>
#include <QPoint>
#include <QPen>
#include <QString>

#include "model/ModelTypes.hpp"
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
    explicit HsmElement(const HsmElementType elementType, const model::EntityID_t modelElementId);
    virtual ~HsmElement();

    model::EntityID_t modelId() const;
    // void setModelId(const model::EntityID_t modelElementId);

    void hightlight(const bool enable);

    HsmElementType elementType() const;
    QRectF elementRect() const;

    virtual void init(const model::EntityID_t modelElementId);

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
    model::EntityID_t mModelElementId = model::INVALID_MODEL_ID;
    bool mHightlight = false;
    QSizeF mSize;
    QRectF mOuterRect;

    QPen mPenHighlightMode;

private:
    HsmElementType mType = HsmElementType::UNKNOWN;
};

};  // namespace view

#endif  // HSMELEMENT_HPP
