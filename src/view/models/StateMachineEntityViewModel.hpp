#ifndef STATEMACHINEENTITYVIEWMODEL_HPP
#define STATEMACHINEENTITYVIEWMODEL_HPP

#include <QAbstractItemModel>
#include <QSharedPointer>
#include <functional>
#include <memory>

#include "model/StateMachineModel.hpp"
#include "model/actions/IModelAction.hpp"
#include "model/elements/StateMachineEntity.hpp"

namespace view {

class StateMachineEntityViewModel : public QAbstractItemModel {
    Q_OBJECT
public:
    // Custom item-data roles shared with HsmEntityPropertyDelegate
    enum CustomRoles {
        PropertyKeyRole = Qt::UserRole,
        PropertyPathRole,
        ActionSubtypeRole,
        ActionAddRole,
        ActionRemovableRole,
        ActionMoveUpRole,
        ActionMoveDownRole,
    };

private:
    enum class NodeType {
        Property,
        ActionItem,
        ActionAttribute,
        ActionAdd,
    };

    enum class ComplexPropertyType {
        None,
        Action,
        ActionList,
    };

    struct PropertyNode {
        int row = -1;
        int propertyRow = -1;
        int actionIndex = -1;
        NodeType type = NodeType::Property;
        QString key;
        QString label;
        QString actionAttribute;
        ComplexPropertyType complexType = ComplexPropertyType::None;
        PropertyNode* parent = nullptr;
        std::vector<std::unique_ptr<PropertyNode>> children;
    };

public:
    explicit StateMachineEntityViewModel(const QSharedPointer<model::StateMachineModel>& model, QObject* parent = nullptr);
    virtual ~StateMachineEntityViewModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void selectEntityById(const model::EntityID_t id);
    void selectEntity(const QSharedPointer<model::StateMachineEntity>& entity);

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    void setHistoryTransactionCallbacks(std::function<void(const QString&)> beginCallback,
                                        std::function<void()> commitCallback);

private:
    bool hasSelectedEntity() const;
    ComplexPropertyType complexTypeForProperty(const QString& key) const;

    void rebuildNodes();
    PropertyNode* nodeFromIndex(const QModelIndex& index) const;

    QVariant formatPropertyValueForRole(const PropertyNode& node, int role) const;
    QVariant formatActionAttributeValue(const PropertyNode& node, int role) const;

    void buildActionItemChildren(PropertyNode* actionNode, const QSharedPointer<model::IModelAction>& action);
    model::ModelActionList actionListForKey(const QString& key) const;
    QSharedPointer<model::IModelAction> actionAtIndex(const QString& key, const int index) const;

    bool updatePropertyByNode(const PropertyNode& node, const QVariant& value, int role);
    bool addActionToSlot(const QString& key);
    bool removeActionFromSlot(const QString& key, const int index);
    bool moveActionInSlot(const QString& key, const int from, const int to);

    QSharedPointer<model::IModelAction> actionFromVariant(const QVariant& value) const;

private:
    QSharedPointer<model::StateMachineModel> mModel;
    QSharedPointer<model::StateMachineEntity> mSelectedEntity;
    QMetaObject::Connection mEntitySignalConnection;
    std::function<void(const QString&)> mBeginHistoryTransaction;
    std::function<void()> mCommitHistoryTransaction;
    std::vector<std::unique_ptr<PropertyNode>> mTopNodes;
};

}  // namespace view

#endif  // STATEMACHINEENTITYVIEWMODEL_HPP
