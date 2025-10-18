#include "ProjectController.hpp"

#include <QDebug>
#include <QFile>
#include <QLoggingCategory>
#include <QMimeData>
#include <QTextStream>

#include "ObjectUtils.hpp"
#include "model/EntryPoint.hpp"
#include "model/InitialState.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/Transition.hpp"
#include "view/MainWindow.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/HsmElement.hpp"  // TODO: move out from private
#include "view/widgets/HsmGraphicsView.hpp"

Q_DECLARE_LOGGING_CATEGORY(ProjectController)

ProjectController::ProjectController(QPointer<MainWindow> mainWindow, QObject* parent)
    : QObject(parent)
    , mMainWindow(mainWindow)
    , mModel(new model::StateMachineModel("Default Model", this)) {
    // ---------------------
    // TODO: debug
    createElement("state", QPoint(10, 10));
    createElement("state", QPoint(-150, 170));
    createElement("state", QPoint(250, 170));

    createElement("initial", QPoint(-150, -120));
    createElement("entrypoint", QPoint(-100, -120));
    createElement("exitpoint", QPoint(-20, -120));
    createElement("final", QPoint(75, -120));
    createElement("history", QPoint(175, -120));

    // createTransition("e1", "e2");

    QObject::connect(mModel.get(), &model::StateMachineModel::modelEntityDeleted, this, &ProjectController::modelEntityDeleted);

    // std::shared_ptr<HsmTransition> t = std::make_shared<HsmTransition>();
    // t->init();
    // e1->addTransition(t, e2);
    mMainWindow->setModel(mModel);
}

bool ProjectController::importModel(const QString& path) {
    return false;
}

bool ProjectController::exportModel(const QString& path) {
    bool res = false;
    model::StateMachineSerializer serializer;
    const QString scxmlContent = serializer.serializeToScxml(mModel);

    if (scxmlContent.isEmpty() == false) {
        QFile file(path);

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&file);

            out << scxmlContent;
            file.close();
            res = true;
        }
    }

    return res;
}

void ProjectController::handleViewDropEvent(const QString& elementTypeId,
                                            const QPoint& pos,
                                            const model::EntityID_t targetElementId) {
    qDebug() << Q_FUNC_INFO << elementTypeId << targetElementId << pos;

    createElement(elementTypeId, pos, targetElementId);
}

void ProjectController::handleViewMoveEvent(const model::EntityID_t draggedElementId, const model::EntityID_t targetElementId) {
    qDebug() << Q_FUNC_INFO << draggedElementId << targetElementId;
    bool updateUi = false;

    // TODO: handle top level moves
    if (model::INVALID_MODEL_ID != targetElementId) {
        // change parent in model
        updateUi = mModel->moveElement(draggedElementId, targetElementId);
    } else {
        updateUi = mModel->moveElement(draggedElementId, mModel->root()->id());
    }

    qDebug() << "UPDATE UI: " << updateUi;

    if (true == updateUi) {
        // change parent in view
        mMainWindow->view()->moveHsmElement(draggedElementId, targetElementId);
    }
}

void ProjectController::handleDeleteElements(const QList<model::EntityID_t>& elementIDs) {
    qDebug() << Q_FUNC_INFO << elementIDs;

    for (model::EntityID_t id : elementIDs) {
        mModel->root()->deleteChild(id);
        // NOTE: model will send signal which will trigger view update
    }
}

void ProjectController::connectElements(const model::EntityID_t fromElementId, const model::EntityID_t toElementId) {
    qDebug() << Q_FUNC_INFO << fromElementId << " -> " << toElementId;
    auto source = mModel->root()->findState(fromElementId);
    auto target = mModel->root()->findState(toElementId);
    auto newTransition = model::ModelElementsFactory::createUniqueTransition(source, target);

    if (newTransition) {
        view::HsmTransition* newViewTransition =
            mMainWindow->view()->createHsmTransition(newTransition, fromElementId, toElementId);

        tryConnectSignal(newViewTransition,
                         "transitionReconnected(model::EntityID_t,model::EntityID_t,model::EntityID_t)",
                         this,
                         "reconnectElements(model::EntityID_t,model::EntityID_t,model::EntityID_t)");
    } else {
        qCritical() << "Failed to create new transition";
    }
}

void ProjectController::reconnectElements(const model::EntityID_t transitionId,
                                          const model::EntityID_t newFromElementId,
                                          const model::EntityID_t newToElementId) {
    qDebug() << Q_FUNC_INFO << transitionId << ": " << newFromElementId << " -> " << newToElementId;

    if (mModel->reconnectElements(transitionId, newFromElementId, newToElementId) == true) {
        mMainWindow->view()->reconnectHsmTransition(transitionId, newFromElementId, newToElementId);
    } else {
        qCritical() << "Failed to reconnect transition with id " << transitionId << newFromElementId << newToElementId;
    }
}

void ProjectController::modelEntityDeleted(QWeakPointer<model::StateMachineEntity> entity) {
    qDebug() << Q_FUNC_INFO;
    auto ptr = entity.lock();

    if (ptr) {
        mMainWindow->view()->deleteHsmElement(ptr->id());
    } else {
        qCritical() << "StateMachineEntity doesnt exist!";
    }
}

void ProjectController::createElement(const QString& elementTypeId,
                                      const QPoint& pos,
                                      const model::EntityID_t parentElementId) {
    qDebug() << Q_FUNC_INFO << elementTypeId << parentElementId << pos;

    static std::map<QString, model::StateType> sElementTypes = {// TODO: decide what to do with start type
                                                                {"initial", model::StateType::INITIAL},
                                                                {"final", model::StateType::FINAL},
                                                                {"state", model::StateType::REGULAR},
                                                                {"entrypoint", model::StateType::ENTRYPOINT},
                                                                {"exitpoint", model::StateType::EXITPOINT},
                                                                {"history", model::StateType::HISTORY}};

    auto it = sElementTypes.find(elementTypeId);

    if (sElementTypes.end() != it) {
        auto newModelElement = model::ModelElementsFactory::createUniqueState(it->second);
        auto parentState = mModel->root()->findRegularState(parentElementId);
        view::HsmElement* newViewElement =
            mMainWindow->view()->createHsmElement(newModelElement, elementTypeId, pos, parentElementId);

        qDebug() << Q_FUNC_INFO << parentState;

        if (parentState.isNull()) {
            parentState = mModel->root();
        }

        parentState->addChildState(newModelElement);

        tryConnectSignal(newViewElement,
                         "elementConnected(model::EntityID_t,model::EntityID_t)",
                         this,
                         "connectElements(model::EntityID_t,model::EntityID_t)");
    } else {
        qCritical() << Q_FUNC_INFO << "Unsupported element type:" << elementTypeId;
    }
}

void ProjectController::createTransition(const QSharedPointer<model::State>& fromElement,
                                         const QSharedPointer<model::State>& toElement) {}
