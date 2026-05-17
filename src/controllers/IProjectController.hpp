#ifndef IIPROJECTCONTROLLER_HPP
#define IIPROJECTCONTROLLER_HPP

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QWeakPointer>

#include "model/StateMachineModel.hpp"

class IProjectController : public QObject, public QEnableSharedFromThis<IProjectController> {
    Q_OBJECT
public:
    explicit IProjectController(QObject* parent = nullptr);
    virtual ~IProjectController() = default;

    virtual bool importModel(const QString& path) = 0;
    virtual bool exportModel() = 0;
    virtual bool exportModel(const QString& path) = 0;
    virtual void updateModelPath(const QString& newPath) = 0;

    virtual void handleViewDropEvent(const QString& elementTypeId, const QPointF& parentPos, const model::EntityID_t targetElementId) = 0;
    virtual void handleViewMoveEvent(const model::EntityID_t draggedElementId, const model::EntityID_t targetElementId) = 0;
    virtual void handleDeleteElements(const QList<model::EntityID_t>& elementIDs) = 0;
    virtual QString serializeElementsToScxml(const QList<model::EntityID_t>& elementIDs) const = 0;
    virtual bool pasteScxmlElements(const QString& scxmlContent,
                            const QList<model::EntityID_t>& selectedElementIDs = {},
                            const QPointF& cursorScenePos = QPointF(),
                            const bool useCursorPosition = false) = 0;
    virtual void beginHistoryTransaction(const QString& label) = 0;
    virtual void commitHistoryTransaction() = 0;
    virtual void cancelHistoryTransaction() = 0;
    virtual void markHistoryElement(const model::EntityID_t elementId) = 0;
    virtual void unmarkHistoryElement(const model::EntityID_t elementId) = 0;
    virtual bool undo() = 0;
    virtual bool redo() = 0;
    virtual bool canUndo() const = 0;
    virtual bool canRedo() const = 0;
};

#endif  // IIPROJECTCONTROLLER_HPP
