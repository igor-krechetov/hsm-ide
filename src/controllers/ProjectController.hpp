#ifndef PROJECTCONTROLLER_HPP
#define PROJECTCONTROLLER_HPP

#include <QObject>
#include <QSharedPointer>

#include "model/StateMachineModel.hpp"

class MainWindow;
class QDropEvent;
class State;

class ProjectController : public QObject {
    Q_OBJECT
public:
    explicit ProjectController(QSharedPointer<MainWindow> mainWindow, QObject* parent = nullptr);

    void handleViewDropEvent(QDropEvent* event);

private:
    void createElement(const QString& elementId, const QPoint& pos);
    void createTransition(const QSharedPointer<State>& fromElement, const QSharedPointer<State>& toElement);

private:
    QSharedPointer<MainWindow> mMainWindow;
    QSharedPointer<StateMachineModel> mModel;
};

#endif  // PROJECTCONTROLLER_HPP
