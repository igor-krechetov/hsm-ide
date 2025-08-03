#ifndef HSMGRAPHICSVIEW_HPP
#define HSMGRAPHICSVIEW_HPP

#include <QGraphicsView>
#include <QPointer>
#include "model/ModelTypes.hpp"

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

    void setProjectController(QPointer<ProjectController> controller);

    view::HsmElement* createHsmElement(const model::EntityID_t  modelElementId, const QString& elementTypeId, const QPoint& pos);
    view::HsmTransition* createHsmTransition(const model::EntityID_t transitionId, const model::EntityID_t fromElementId, const model::EntityID_t toElementId);
    void deleteHsmElement(const model::EntityID_t modelElementId);

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
    QPointer<view::HsmElement> findHsmElement(const model::EntityID_t id) const;


private:
    // Weak cache of all elements attached to the view. Flat structure will allow easy search for elements
    QMap<model::EntityID_t, QPointer<view::HsmElement>> mElements;
    QPointer<ProjectController> mProjectController;
    QPoint mLastPanPoint;
    bool mPanning = false;
    bool mSpacePressed = false;
};

#endif  // HSMGRAPHICSVIEW_HPP
