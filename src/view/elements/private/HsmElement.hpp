#ifndef HSMELEMENT_HPP
#define HSMELEMENT_HPP

#include <QPen>
#include <QPoint>
#include <QPointer>
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

// TODO: subscribe to model data chages and redraw HsmElement

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

    virtual void init(const QSharedPointer<model::StateMachineEntity>& modelEntity);

    model::EntityID_t modelId() const;
    // void setModelId(const model::EntityID_t modelElementId);

    void hightlight(const bool enable);
    bool isHighligted() const;

    void setDragMode(const bool dragging);
    void setGroupDragMode(const bool enableGroup);
    bool isDragged() const;

    HsmElementType elementType() const;
    QRectF elementRect() const;

    virtual QPointF mapFromSceneToBody(const QPointF &point) const;

    template <typename T>
    QSharedPointer<T> modelElement();

    virtual bool isConnectable() const;
    virtual bool acceptsConnections() const;
    virtual bool isResizable() const;
    // HsmElement* connectableElementAt(const QPointF& pos) const;

    virtual bool acceptsChildren() const;
    virtual QList<QGraphicsItem*> hsmChildItems() const;
    bool isDirectChild(HsmElement* item) const;
    QRectF childrenRect() const;

    // By default just calls setParentItem for the child, but can be overriden for custom logic
    // Only use setParentItem(nullptr) to disconnect child elements
    virtual void addChildItem(HsmElement* child);

    // Sets items as a child of parent item both logically and physically (QObject relationship)
    // manages pointer to a logical item parent (which is could be different from Qt child-parent hierarchy)
    void setHsmParentItem(HsmElement* parent);
    inline QPointer<HsmElement> hsmParentItem() const;

    // TODO: move to an interface
    virtual bool onGripMoved(const ElementGripItem* selectedGrip, const QPointF& pos);

// Slot for model data changes
public slots:
    virtual void onModelDataChanged();

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

    QPen mPenNormalMode;
    QPen mPenHighlightMode;
    QPen mPenSelectedBorder;
    QBrush mBackgroundBrush;
    QBrush mMainBrush;

private:
    QPointer<HsmElement> mHsmParent;
    HsmElementType mType = HsmElementType::UNKNOWN;
    QWeakPointer<model::StateMachineEntity> mModelElement;
    DragState mDragState = DragState::NONE;
    DragMode mDragMode = DragMode::NONE;
    bool mHightlight = false;
};

inline QPointer<HsmElement> HsmElement::hsmParentItem() const {
    return mHsmParent;
}

template <typename T>
QSharedPointer<T> HsmElement::modelElement() {
    QSharedPointer<T> res;
    auto entitySharedPtr = mModelElement.lock();

    if (entitySharedPtr) {
        res = entitySharedPtr.dynamicCast<T>();
    }

    return res;
}

};  // namespace view

#endif  // HSMELEMENT_HPP
