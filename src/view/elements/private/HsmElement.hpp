#ifndef HSMELEMENT_HPP
#define HSMELEMENT_HPP

#include <QPen>
#include <QPoint>
#include <QString>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QGraphicsObject>
#include <functional>

#include "model/ModelTypes.hpp"

namespace model {
class StateMachineEntity;
}

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
    constexpr static int DEPTH_INFINITE = -1;
    enum class DragMode { NONE, SINGLE, GROUP, GROUP_NONE };

    enum class DragState { NONE, PREPARE, DRAGGING };

public:
    explicit HsmElement(const HsmElementType elementType, const QSizeF& size);
    explicit HsmElement(const HsmElementType elementType, const QSharedPointer<model::StateMachineEntity>& modelElement, const QSizeF& size);
    virtual ~HsmElement();

    virtual void init(const QSharedPointer<model::StateMachineEntity>& modelElement);

    // Slot for model data changes
public slots:
    virtual void onModelDataChanged();

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
    // sent when element's size of position is changed
    void geometryChanged(HsmElement* element);

protected:
    HsmGraphicsView* hsmView() const;
    void forEachChildElement(std::function<void(HsmElement*)> callback, const int depth = DEPTH_INFINITE);

    virtual void updateBoundingRect(const QRectF& newRect = QRectF());
    void notifyGeometryChanged();

    virtual void resizeParentToFitChildItem();

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
    QWeakPointer<model::StateMachineEntity> mModelElement;
    DragState mDragState = DragState::NONE;
    DragMode mDragMode = DragMode::NONE;
    bool mHightlight = false;
};

};  // namespace view

#endif  // HSMELEMENT_HPP
