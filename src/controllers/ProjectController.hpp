#ifndef PROJECTCONTROLLER_HPP
#define PROJECTCONTROLLER_HPP

#include <QObject>
#include <QPointer>
#include <QWeakPointer>

#include "model/StateMachineModel.hpp"

class HsmGraphicsView;

namespace model {
class State;
class StateMachineEntity;
};
namespace view {
class HsmElement;
class StateMachineTreeModel;
class StateMachineEntityViewModel;
};

class ProjectController : public QObject {
    Q_OBJECT
public:
    explicit ProjectController(const QString& id, QObject* parent = nullptr);
    virtual ~ProjectController() override = default;

    void registerView(QPointer<HsmGraphicsView> view);
    inline QPointer<HsmGraphicsView> view() const;
    inline view::StateMachineTreeModel* hsmStructureModel();
    inline view::StateMachineEntityViewModel* hsmEntityViewModel();

    inline QString id() const;
    inline QString name() const;
    inline bool isModified() const;

    bool importModel(const QString& path);
    bool exportModel(const QString& path);

    void handleViewDropEvent(const QString& elementTypeId, const QPointF& parentPos, const model::EntityID_t targetElementId);
    void handleViewMoveEvent(const model::EntityID_t draggedElementId, const model::EntityID_t targetElementId);
    void handleDeleteElements(const QList<model::EntityID_t>& elementIDs);

signals:
    void projectModelChanged(QPointer<ProjectController> project);

private slots:
    void connectElements(const model::EntityID_t fromElementId, const model::EntityID_t toElementId);
    void reconnectElements(const model::EntityID_t transitionId,
                           const model::EntityID_t newFromElementId,
                           const model::EntityID_t newToElementId);
    void modelEntityAdded(QWeakPointer<model::StateMachineEntity> parent, QWeakPointer<model::StateMachineEntity> entity);
    void modelEntityDeleted(QWeakPointer<model::StateMachineEntity> parent, QWeakPointer<model::StateMachineEntity> entity);
    void modelDataChanged(QWeakPointer<model::StateMachineEntity> entity);

private:
    void handleModelEntityAdded(QSharedPointer<model::StateMachineEntity> parent, QSharedPointer<model::StateMachineEntity> entity);

    // posParent - position in parent coordinate system
    void createElement(const QString& elementTypeId,
                       const QPointF& posParent,
                       const model::EntityID_t parentElementId = model::INVALID_MODEL_ID);
    void createTransition(const QSharedPointer<model::State>& fromElement, const QSharedPointer<model::State>& toElement);

    void projectModified();

private:
    QString mId;
    QPointer<HsmGraphicsView> mView;
    QSharedPointer<model::StateMachineModel> mModel;

    view::StateMachineTreeModel* mHsmStrctureViewModel = nullptr;
    view::StateMachineEntityViewModel* mHsmEntityViewModel = nullptr;

    // true if the model was modified since last save/loading
    bool mModified = false;

    // TODO
    //     2. Create a scene per project
    // Each project gets its own QGraphicsScene (which encapsulates your state machine graph, connections, etc.).
    // auto* scene = new QGraphicsScene(this);
    // scene->addItem(...); // populate
    // m_openProjects.insert(projectName, scene);
    //     3. On project switch

    // Just swap the scene pointer:

    // void EditorWindow::switchToProject(const QString& name)
    // {
    //     if (!m_openProjects.contains(name))
    //         return;

    //     m_view->setScene(m_openProjects[name]);
    //     m_currentProject = name;
    // }
};

inline QPointer<HsmGraphicsView> ProjectController::view() const {
    return mView;
}

inline view::StateMachineTreeModel* ProjectController::hsmStructureModel() {
    return mHsmStrctureViewModel;
}

inline view::StateMachineEntityViewModel* ProjectController::hsmEntityViewModel() {
    return mHsmEntityViewModel;
}

inline QString ProjectController::id() const {
    return mId;
}

inline QString ProjectController::name() const {
    return (mModel != nullptr ? mModel->name() : "Untitled");
}

inline bool ProjectController::isModified() const {
    return mModified;
}

#endif  // PROJECTCONTROLLER_HPP
