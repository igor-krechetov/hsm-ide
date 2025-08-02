#ifndef HSMGRAPHICSVIEW_HPP
#define HSMGRAPHICSVIEW_HPP

#include <QGraphicsView>
#include <QSharedPointer>
#include "model/StateMachineEntity.hpp"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class ProjectController;

namespace view {
    class HsmElement;
    class HsmTransition;
};
class HsmGraphicsView : public QGraphicsView {
public:
    HsmGraphicsView(QWidget* parent);
    virtual ~HsmGraphicsView() = default;

    void setProjectController(QSharedPointer<ProjectController> controller);

    view::HsmElement* createHsmElement(const QString& modelElementId, const QString& elementTypeId, const QPoint& pos);
    view::HsmTransition* createHsmTransition(const model::StateMachineEntity::ID_t fromElementId, const model::StateMachineEntity::ID_t toElementId);

protected:
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

private:
    void setPanningMode(const bool enable);
    QPointer<view::HsmElement> findHsmElement(const model::StateMachineEntity::ID_t id) const;


private:
    // Weak cache of all elements attached to the view. Flat structure will allow easy search for elements
    QMap<model::StateMachineEntity::ID_t, QPointer<view::HsmElement>> mElements;
    QSharedPointer<ProjectController> mProjectController;
    QPoint mLastPanPoint;
    bool mPanning = false;
    bool mSpacePressed = false;
};

#endif  // HSMGRAPHICSVIEW_HPP
