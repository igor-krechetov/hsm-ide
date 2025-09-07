#ifndef HSMELEMENT_HPP
#define HSMELEMENT_HPP

#include <QGraphicsObject>
#include <QPen>
#include <QPoint>
#include <QString>
#include <functional>

#include "model/ModelTypes.hpp"

class HsmGraphicsView;

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
    enum class DragMode {
        NONE, SINGLE, GROUP, GROUP_NONE
    };

    enum class DragState {
        NONE, PREPARE, DRAGGING
    };

public:
    explicit HsmElement(const HsmElementType elementType);
    explicit HsmElement(const HsmElementType elementType, const model::EntityID_t modelElementId);
    virtual ~HsmElement();

    virtual void init(const model::EntityID_t modelElementId);

    model::EntityID_t modelId() const;
    // void setModelId(const model::EntityID_t modelElementId);

    void hightlight(const bool enable);
    bool isHighligted() const;

    void setDragMode(const bool dragging);
    void setGroupDragMode(const bool enableGroup);
    bool isDragged() const;

    HsmElementType elementType() const;
    QRectF elementRect() const;

    virtual bool isConnectable() const;
    virtual bool isResizable() const;
    // HsmElement* connectableElementAt(const QPointF& pos) const;

    virtual bool acceptsChildren() const;
    bool isDirectChild(HsmElement* item) const;

    QRectF childrenRect() const;

    // TODO: move to an interface
    virtual bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos);

signals:
    void dragElementBegin(HsmElement* element, const QPointF& scenePos);
    void dragElementEvent(HsmElement* element, const QPointF& scenePos);
    void dropElementEvent(HsmElement* element, const QPointF& scenePos);

protected:
    HsmGraphicsView* hsmView() const;
    void forEachChildElement(std::function<void(HsmElement*)> callback);
    virtual void updateBoundingRect(const QRectF& newRect = QRectF());

// QGraphicsItem interface
protected:
    QRectF boundingRect() const override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(const GraphicsItemChange change, const QVariant& value) override;

    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;

protected:
    // dragging - element is being moved around the view and can be assigned to a new parent
    QSizeF mSize;
    QRectF mOuterRect;

    QPen mPenHighlightMode;

private:
    HsmElementType mType = HsmElementType::UNKNOWN;
    model::EntityID_t mModelElementId = model::INVALID_MODEL_ID;
    DragState mDragState = DragState::NONE;
    DragMode mDragMode = DragMode::NONE;
    bool mHightlight = false;
};

};  // namespace view

#endif  // HSMELEMENT_HPP
