#ifndef PROJECTCONTROLLER_HPP
#define PROJECTCONTROLLER_HPP

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QWeakPointer>

#include "model/StateMachineModel.hpp"

class HsmGraphicsView;

namespace model {
class State;
class StateMachineEntity;
};  // namespace model
namespace view {
class HsmElement;
class StateMachineTreeModel;
class StateMachineEntityViewModel;
};  // namespace view

class ModificationHistoryController;

class ProjectController : public QObject, public QEnableSharedFromThis<ProjectController> {
    Q_OBJECT
public:
    explicit ProjectController(const QString& id, QObject* parent = nullptr);
    virtual ~ProjectController() override;

    void registerView(QPointer<HsmGraphicsView> view);
    inline QPointer<HsmGraphicsView> view() const;
    inline view::StateMachineTreeModel* hsmStructureModel();
    inline view::StateMachineEntityViewModel* hsmEntityViewModel();

    inline QString id() const;
    inline QString name() const;
    inline QString modelPath() const;
    inline bool isModified() const;

    bool importModel(const QString& path);
    bool exportModel();
    bool exportModel(const QString& path);

    void handleViewDropEvent(const QString& elementTypeId, const QPointF& parentPos, const model::EntityID_t targetElementId);
    void handleViewMoveEvent(const model::EntityID_t draggedElementId, const model::EntityID_t targetElementId);
    void handleDeleteElements(const QList<model::EntityID_t>& elementIDs);
    QString serializeElementsToScxml(const QList<model::EntityID_t>& elementIDs) const;
    bool pasteScxmlElements(const QString& scxmlContent,
                            const QList<model::EntityID_t>& selectedElementIDs = {},
                            const QPointF& cursorScenePos = QPointF(),
                            const bool useCursorPosition = false);
    void beginHistoryTransaction(const QString& label);
    void commitHistoryTransaction();
    void cancelHistoryTransaction();
    void markHistoryElement(const model::EntityID_t elementId);
    void unmarkHistoryElement(const model::EntityID_t elementId);
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

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
    void refreshViewFromModel();

    QSharedPointer<model::State> resolvePasteTargetParent(const QList<model::EntityID_t>& selectedElementIDs) const;
    // Find state's position within the sceene
    QPointF calculateStateScenePos(const QSharedPointer<model::State>& state) const;
    void handleModelEntityAdded(QSharedPointer<model::StateMachineEntity> parent,
                                QSharedPointer<model::StateMachineEntity> entity,
                                const bool addChildren);

    // posParent - position in parent coordinate system
    void createElement(const QString& elementTypeId,
                       const QPointF& posParent,
                       const model::EntityID_t parentElementId = model::INVALID_MODEL_ID);
    void createTransition(const QSharedPointer<model::State>& fromElement, const QSharedPointer<model::State>& toElement);

    void projectModified();

private:
    QString mId;
    // path to a file from which the model was loaded or empty
    QString mModelPath;
    QPointer<HsmGraphicsView> mView;
    QSharedPointer<model::StateMachineModel> mModel;
    QSharedPointer<ModificationHistoryController> mHistoryController;

    view::StateMachineTreeModel* mHsmStructureViewModel = nullptr;
    view::StateMachineEntityViewModel* mHsmEntityViewModel = nullptr;

    // true if the model was modified since last save/loading
    bool mModified = false;
    // flag to block new model entities from being added to the view
    bool mIgnoreAddedModelEntities = false;
};

inline QPointer<HsmGraphicsView> ProjectController::view() const {
    return mView;
}

inline view::StateMachineTreeModel* ProjectController::hsmStructureModel() {
    return mHsmStructureViewModel;
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

inline QString ProjectController::modelPath() const {
    return mModelPath;
}

inline bool ProjectController::isModified() const {
    return mModified;
}

#endif  // PROJECTCONTROLLER_HPP
