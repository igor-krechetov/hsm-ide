#ifndef STATEMACHINETREEMODEL_HPP
#define STATEMACHINETREEMODEL_HPP

#include <QAbstractItemModel>
#include <QSharedPointer>

#include "model/StateMachineEntity.hpp"

namespace model {
class StateMachineModel;
class RegularState;
class EntryPoint;
class InitialState;
class HistoryState;
};  // namespace model

namespace view {
class StateMachineTreeModel : public QAbstractItemModel {
    Q_OBJECT

private:
    struct TreeNode {
        enum NodeType {
            Entity,  // Regular entity (state, transition)
            Root,    // Root state of the model
            Invalid
        };

        TreeNode* parent = nullptr;
        QList<TreeNode*> children;
        QWeakPointer<model::StateMachineEntity> entity;
        NodeType nodeType = NodeType::Entity;

        ~TreeNode() {
            qDeleteAll(children);
        }

        QSharedPointer<model::StateMachineEntity> getEntity() {
            return entity.lock();
        }

        model::StateMachineEntity::Type type() const {
            auto ptrEntity = entity.lock();
            return (nullptr != ptrEntity ? ptrEntity->type() : model::StateMachineEntity::Type::Invalid);
        }

        bool isRoot() const {
            return (NodeType::Root == nodeType);
        }

        int row() const {
            if (nullptr == parent) {
                return 0;
            }

            return parent->children.indexOf(const_cast<TreeNode*>(this));
        }
    };

public:
    explicit StateMachineTreeModel(QSharedPointer<model::StateMachineModel> model, QObject* parent = nullptr);
    virtual ~StateMachineTreeModel() override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex findModelEntity(const model::EntityID_t id) const;
    QSharedPointer<model::StateMachineEntity> entiryFromIndex(const QModelIndex& index) const;

    bool removeRows(int row, int count, const QModelIndex& parent) override;

public slots:
    void onModelChanged();
    void onModelDataChanged(QWeakPointer<model::StateMachineEntity> entity);

private:
    void setupModelData();
    TreeNode* nodeFromIndex(const QModelIndex& index) const;
    void addModelEntity(TreeNode* parentNode, const QSharedPointer<model::StateMachineEntity>& entity);
    void addRegularState(TreeNode* parentNode, const QSharedPointer<model::RegularState>& state);
    void addEntryPoint(TreeNode* parentNode, const QSharedPointer<model::EntryPoint>& state);
    void addInitialState(TreeNode* parentNode, const QSharedPointer<model::InitialState>& state);
    void addHistoryState(TreeNode* parentNode, const QSharedPointer<model::HistoryState>& state);

private:
    // TODO: make shared ptr
    TreeNode* mRootNode = nullptr;

    QSharedPointer<model::StateMachineModel> mModel;
    bool mUpdatingModel = false;
    // You may want to add a tree node structure here for mapping QModelIndex to model elements
};
}  // namespace view

#endif  // STATEMACHINETREEMODEL_HPP
