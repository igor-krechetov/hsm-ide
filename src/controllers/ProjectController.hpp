#ifndef PROJECTCONTROLLER_HPP
#define PROJECTCONTROLLER_HPP

#include <QObject>
#include <QPointer>
#include <QWeakPointer>

#include "model/StateMachineModel.hpp"

class MainWindow;

namespace model {
class State;
class StateMachineEntity;
};
namespace view {
class HsmElement;
};

class ProjectController : public QObject {
    Q_OBJECT
public:
    explicit ProjectController(QPointer<MainWindow> mainWindow, QObject* parent = nullptr);

    void handleViewDropEvent(const QString& elementTypeId, const QPoint& pos, const model::EntityID_t targetElementId);
    void handleViewMoveEvent(const model::EntityID_t draggedElementId, const model::EntityID_t targetElementId);
    void handleDeleteElements(const QList<model::EntityID_t>& elementIDs);

private slots:
    void connectElements(const model::EntityID_t fromElementId, const model::EntityID_t toElementId);
    void reconnectElements(const model::EntityID_t transitionId,
                           const model::EntityID_t newFromElementId,
                           const model::EntityID_t newToElementId);
    void modelEntityDeleted(QWeakPointer<model::StateMachineEntity> entity);

private:
    void createElement(const QString& elementTypeId,
                       const QPoint& pos,
                       const model::EntityID_t parentElementId = model::INVALID_MODEL_ID);
    void createTransition(const QSharedPointer<model::State>& fromElement, const QSharedPointer<model::State>& toElement);

private:
    QPointer<MainWindow> mMainWindow;
    QSharedPointer<model::StateMachineModel> mModel;
};

#endif  // PROJECTCONTROLLER_HPP
