#ifndef HSMGRAPHICSVIEW_HPP
#define HSMGRAPHICSVIEW_HPP

#include <QGraphicsView>
#include <QPointer>
#include <QSize>
#include <QWeakPointer>

#include "model/ModelTypes.hpp"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class ProjectController;

namespace model {
class StateMachineEntity;
class IncludeEntity;
}  // namespace model

namespace view {
class HsmElement;
class HsmResizableElement;
class HsmTransition;
};  // namespace view

class HsmGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    enum KeyboardModifier {
        NoModifier = 0x00000000,
        ShiftModifier = 0x02000000,
        ControlModifier = 0x04000000,
        AltModifier = 0x08000000,
        SpaceModifier = 0x10000000,
        R_Modifier = 0x20000000,
    };

public:
    HsmGraphicsView(QWidget* parent = nullptr);
    virtual ~HsmGraphicsView();

    QWeakPointer<ProjectController> projectController() const;
    void setProjectController(const QWeakPointer<ProjectController>& controller);

    view::HsmElement* createHsmElement(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                       const QString& elementTypeId,
                                       const QPointF& pos,
                                       const QSizeF& size,
                                       const model::EntityID_t parentElementId);
    view::HsmTransition* createHsmTransition(const QSharedPointer<model::StateMachineEntity>& transition,
                                             const model::EntityID_t fromElementId,
                                             const model::EntityID_t toElementId);
    void deleteHsmElement(const model::EntityID_t modelElementId);
    void clearAllHsmElements();
    void moveHsmElement(const model::EntityID_t elementId, const model::EntityID_t newParentId);

    void reconnectHsmTransition(const model::EntityID_t transitionId,
                                const model::EntityID_t fromElementId,
                                const model::EntityID_t toElementId);

    void selectHsmElement(const model::EntityID_t id);
    void selectAllHsmElements();
    QList<model::EntityID_t> getSelectedElements() const;
    void deleteSelectedItems();
    void clearSelection();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitSceneToView();

    bool keyboardShiftPressed() const;
    bool keyboardSpacePressed() const;
    bool keyboardCtrlPressed() const;
    bool keyboardAltPressed() const;
    bool keyboardModifierPressed(const KeyboardModifier modifier) const;
    bool keyboardReparentModifierPressed() const;

    QPointer<view::HsmElement> findHsmElement(const model::EntityID_t id) const;
    QPointer<view::HsmTransition> findHsmTransition(const model::EntityID_t id) const;

signals:
    void hsmElementDoubleClickEvent(QWeakPointer<model::StateMachineEntity> entity);
    void transformChanged();

#ifdef DEBUG_RENDERING
    void mouseMoved(const QPointF& scenePos);
#endif

protected:
    void focusOutEvent(QFocusEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    // void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    // from HsmElement
public slots:
    void dragElementBegin(view::HsmElement* element, const QPointF& scenePos);
    void dragElementEvent(view::HsmElement* element, const QPointF& scenePos);
    void dropElementEvent(view::HsmElement* element, const QPointF& scenePos);

private:
    void applyScale(const double scaleFactor);

    // only element or mimetype is expected to be provided
    bool handleElementDragEvent(const QPointF& scenePos, view::HsmElement* element, const QString& mimetype = "");
    bool handleElementDropEvent(view::HsmElement* element, const QPointF& scenePos);

    void setPanningMode(const bool enable);

    view::HsmElement* itemToHsmElement(QGraphicsItem* item) const;
    // view::HsmResizableElement* itemToHsmResizableElement(QGraphicsItem* item) const;
    view::HsmResizableElement* elementToHsmResizableElement(view::HsmElement* element) const;

    void forEachSelectedElement(std::function<void(view::HsmElement*)> callback);

private:
    // Weak cache of all elements attached to the view. Flat structure will allow easy search for elements
    QMap<model::EntityID_t, QPointer<view::HsmElement>> mElements;
    QWeakPointer<ProjectController> mProjectController;
    QPoint mLastPanPoint;
    bool mPanning = false;
    KeyboardModifier mKeyboardModifiers = KeyboardModifier::NoModifier;

    QPointer<view::HsmElement> mDraggedElement;
    QPointer<view::HsmElement> mDragTargetElement;
    // list of elements that are deselected during dragging because their parent is already selected
    QList<view::HsmElement*> mDraggedChildElements;

    QMap<view::HsmElement*, QPointF> mDragRevertPositions;
};

#endif  // HSMGRAPHICSVIEW_HPP
