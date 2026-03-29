#ifndef STATEMACHINEENTITYVIEWMODEL_HPP
#define STATEMACHINEENTITYVIEWMODEL_HPP

#include <QAbstractItemModel>
#include <QSharedPointer>
#include <functional>
#include <memory>

#include "model/StateMachineEntity.hpp"
#include "model/StateMachineModel.hpp"
#include "model/actions/IModelAction.hpp"

namespace view {

class StateMachineEntityViewModel : public QAbstractItemModel {
    Q_OBJECT
private:
    enum CustomRoles {
        PropertyKeyRole = Qt::UserRole,
        PropertyPathRole,
        ActionSubtypeRole,
    };

    enum class NodeType {
        Property,
        ActionAttribute,
    };

    enum class ComplexPropertyType {
        None,
        Action,
    };

    struct PropertyNode {
        int row = -1;
        int propertyRow = -1;
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

    bool updatePropertyByNode(const PropertyNode& node, const QVariant& value, int role);

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
