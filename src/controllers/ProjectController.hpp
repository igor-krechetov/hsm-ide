#ifndef PROJECTCONTROLLER_HPP
#define PROJECTCONTROLLER_HPP

#include <QObject>
#include <QPointer>

#include "model/StateMachineModel.hpp"

class MainWindow;
class QDropEvent;

namespace model {
class State;
};
namespace view {
class HsmElement;
};

class ProjectController : public QObject {
    Q_OBJECT
public:
    explicit ProjectController(QPointer<MainWindow> mainWindow, QObject* parent = nullptr);

    void handleViewDropEvent(QDropEvent* event);
    void handleDeleteElements(const QList<model::EntityID_t>& elementIDs);

public slots:
    void connectElements(const model::EntityID_t fromElementId, const model::EntityID_t toElementId);
    void reconnectElements(const model::EntityID_t transitionId,
                           const model::EntityID_t newFromElementId,
                           const model::EntityID_t newToElementId);

private:
    void createElement(const QString& elementTypeId, const QPoint& pos);
    void createTransition(const QSharedPointer<model::State>& fromElement, const QSharedPointer<model::State>& toElement);

private:
    QPointer<MainWindow> mMainWindow;
    QSharedPointer<model::StateMachineModel> mModel;
};

#endif  // PROJECTCONTROLLER_HPP
