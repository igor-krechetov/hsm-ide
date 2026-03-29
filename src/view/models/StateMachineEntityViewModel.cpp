#include "StateMachineEntityViewModel.hpp"

#include <QDebug>
#include <QRegularExpression>
#include <QStringList>
#include <QVariant>
#include <utility>

#include "model/HistoryState.hpp"
#include "model/ModelRootState.hpp"
#include "model/Transition.hpp"
#include "model/actions/ModelActionFactory.hpp"

namespace view {
StateMachineEntityViewModel::StateMachineEntityViewModel(const QSharedPointer<model::StateMachineModel>& model, QObject* parent)
    : QAbstractItemModel(parent)
    , mModel(model) {}

StateMachineEntityViewModel::~StateMachineEntityViewModel() {
    if (mEntitySignalConnection) {
        QObject::disconnect(mEntitySignalConnection);
    }
}

bool StateMachineEntityViewModel::hasSelectedEntity() const {
    bool res = false;

    if (mSelectedEntity) {
        res = true;
    }

    return res;
}

StateMachineEntityViewModel::ComplexPropertyType StateMachineEntityViewModel::complexTypeForProperty(const QString& key) const {
    ComplexPropertyType res = ComplexPropertyType::None;

    if (key.endsWith("Action") || key == "action") {
        res = ComplexPropertyType::Action;
    }

    return res;
}
void StateMachineEntityViewModel::rebuildNodes() {
    mTopNodes.clear();

    if (hasSelectedEntity()) {
        const QStringList props = mSelectedEntity->properties();

        for (int i = 0; i < props.size(); ++i) {
            auto node = std::make_unique<PropertyNode>();
            node->row = i;
            node->propertyRow = i;
            node->type = NodeType::Property;
            node->key = props.at(i);
            node->label = props.at(i);
            node->complexType = complexTypeForProperty(node->key);

            if (node->complexType == ComplexPropertyType::Action) {
                auto ptrAction = actionFromVariant(mSelectedEntity->getProperty(node->key));
                const QStringList attrs = ptrAction->properties();

                for (int childRow = 0; childRow < attrs.size(); ++childRow) {
                    auto child = std::make_unique<PropertyNode>();

                    child->row = childRow;
                    child->propertyRow = i;
                    child->type = NodeType::ActionAttribute;
                    child->key = node->key;
                    child->label = attrs.at(childRow);
                    child->actionAttribute = attrs.at(childRow);
                    child->parent = node.get();
                    node->children.push_back(std::move(child));
                }
            }

            mTopNodes.push_back(std::move(node));
        }
    }
}

StateMachineEntityViewModel::PropertyNode* StateMachineEntityViewModel::nodeFromIndex(const QModelIndex& index) const {
    PropertyNode* node = nullptr;

    if (index.isValid()) {
        node = static_cast<PropertyNode*>(index.internalPointer());
    }

    return node;
}

int StateMachineEntityViewModel::rowCount(const QModelIndex& parent) const {
    int res = 0;

    if (!parent.isValid()) {
        res = static_cast<int>(mTopNodes.size());
    } else {
        const PropertyNode* node = nodeFromIndex(parent);

        if (node) {
            res = static_cast<int>(node->children.size());
        }
    }

    return res;
}

int StateMachineEntityViewModel::columnCount(const QModelIndex& /*parent*/) const {
    return 2;
}

QModelIndex StateMachineEntityViewModel::index(int row, int column, const QModelIndex& parent) const {
    QModelIndex res;

    if (column >= 0 && column < columnCount(parent)) {
        if (!parent.isValid()) {
            if (row >= 0 && row < static_cast<int>(mTopNodes.size())) {
                res = createIndex(row, column, mTopNodes.at(row).get());
            }
        } else {
            const PropertyNode* parentNode = nodeFromIndex(parent);

            if (parentNode && row >= 0 && row < static_cast<int>(parentNode->children.size())) {
                res = createIndex(row, column, parentNode->children.at(row).get());
            }
        }
    }

    return res;
}

QModelIndex StateMachineEntityViewModel::parent(const QModelIndex& index) const {
    QModelIndex res;
    const PropertyNode* node = nodeFromIndex(index);

    if (node && node->parent) {
        const PropertyNode* parentNode = node->parent;
        res = createIndex(parentNode->row, 0, const_cast<PropertyNode*>(parentNode));
    }

    return res;
}

QVariant StateMachineEntityViewModel::formatPropertyValueForRole(const PropertyNode& node, int role) const {
    QVariant res;

    if (hasSelectedEntity()) {
        const QVariant propertyValue = mSelectedEntity->getProperty(node.key);

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (node.complexType == ComplexPropertyType::Action) {
                auto ptrAction = actionFromVariant(propertyValue);

                res = model::ModelActionFactory::actionName(ptrAction->type());
            } else {
                res = propertyValue;

                if ((role == Qt::DisplayRole) && (node.key == model::Transition::cKeyTransitionType)) {
                    res = model::transitionTypeToString(model::transitionTypeFromInt(res.toInt()));
                } else if ((role == Qt::DisplayRole) && (node.key == model::HistoryState::cKeyHistoryType)) {
                    res = model::historyTypeToString(model::historyTypeFromInt(res.toInt()));
                }
            }
        } else if (role == ActionSubtypeRole && node.complexType == ComplexPropertyType::Action) {
            auto ptrAction = actionFromVariant(propertyValue);

            res = model::ModelActionFactory::actionName(ptrAction->type());
        }
    }

    return res;
}

QVariant StateMachineEntityViewModel::formatActionAttributeValue(const PropertyNode& node, int role) const {
    QVariant res;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (hasSelectedEntity()) {
            auto ptrAction = actionFromVariant(mSelectedEntity->getProperty(node.key));
            res = ptrAction->getProperty(node.actionAttribute).toString();
        }
    }

    return res;
}

QVariant StateMachineEntityViewModel::data(const QModelIndex& index, int role) const {
    QVariant res;
    const PropertyNode* node = nodeFromIndex(index);

    if (node) {
        if (index.column() == 0) {
            if (role == Qt::DisplayRole) {
                res = node->label;
            }
        } else if (index.column() == 1) {
            if (node->type == NodeType::Property) {
                res = formatPropertyValueForRole(*node, role);
            } else if (node->type == NodeType::ActionAttribute) {
                res = formatActionAttributeValue(*node, role);
            }
        }

        if (!res.isValid()) {
            if (role == PropertyKeyRole) {
                res = node->key;
            } else if (role == PropertyPathRole) {
                res = node->key;

                if (node->type == NodeType::ActionAttribute) {
                    res = QString("%1.%2").arg(node->key, node->actionAttribute);
                }
            }
        }
    }

    return res;
}

QVariant StateMachineEntityViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    QVariant res;

    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0) {
            res = QStringLiteral("Property");
        } else if (section == 1) {
            res = QStringLiteral("Value");
        }
    }

    return res;
}

void StateMachineEntityViewModel::selectEntityById(const model::EntityID_t id) {
    if (mModel && mModel->root()) {
        if (mModel->root()->id() == id) {
            selectEntity(qSharedPointerCast<model::StateMachineEntity>(mModel->root()));
        } else {
            selectEntity(mModel->root()->findChild(id));
        }
    }
}

void StateMachineEntityViewModel::selectEntity(const QSharedPointer<model::StateMachineEntity>& entity) {
    beginResetModel();
    mSelectedEntity = entity;

    if (mEntitySignalConnection) {
        QObject::disconnect(mEntitySignalConnection);
    }

    if (mSelectedEntity) {
        mEntitySignalConnection = QObject::connect(mSelectedEntity.get(),
                                                   &model::StateMachineEntity::modelDataChanged,
                                                   this,
                                                   [this](QWeakPointer<model::StateMachineEntity> /*changedEntity*/) {
                                                       beginResetModel();
                                                       rebuildNodes();
                                                       endResetModel();
                                                   });
    }

    rebuildNodes();
    endResetModel();
}

Qt::ItemFlags StateMachineEntityViewModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags res = Qt::NoItemFlags;

    if (index.isValid()) {
        res = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        if (index.column() == 1) {
            res |= Qt::ItemIsEditable;
        }
    }

    return res;
}

bool StateMachineEntityViewModel::updatePropertyByNode(const PropertyNode& node, const QVariant& value, int role) {
    bool res = false;

    if (hasSelectedEntity() && role == Qt::EditRole) {
        if (mBeginHistoryTransaction) {
            mBeginHistoryTransaction(QString("Edit property: %1").arg(node.key));
        }

        if (node.type == NodeType::Property) {
            QVariant newValue = value;

            if (node.complexType == ComplexPropertyType::Action) {
                auto ptrCurrentAction = actionFromVariant(mSelectedEntity->getProperty(node.key));
                auto newActionType = model::ModelActionFactory::actionTypeByName(value.toString());

                if (newActionType != ptrCurrentAction->type()) {

                    newValue = QVariant::fromValue(model::ModelActionFactory::createModelAction(newActionType));
                }
            }

            res = mSelectedEntity->setProperty(node.key, newValue);
        } else if (node.type == NodeType::ActionAttribute) {
            Q_ASSERT(node.parent);

            if (nullptr != node.parent) {
                auto ptrCurrentAction = actionFromVariant(mSelectedEntity->getProperty(node.key));

                ptrCurrentAction->setProperty(node.actionAttribute, value);
                res = mSelectedEntity->setProperty(node.parent->key, QVariant::fromValue(ptrCurrentAction));
            }
        }

        if (mCommitHistoryTransaction) {
            mCommitHistoryTransaction();
        }
    }

    return res;
}

bool StateMachineEntityViewModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    bool res = false;
    const PropertyNode* node = nodeFromIndex(index);

    if (node && index.column() == 1) {
        res = updatePropertyByNode(*node, value, role);

        if (res) {
            beginResetModel();
            rebuildNodes();
            endResetModel();
        }
    }

    return res;
}

void StateMachineEntityViewModel::setHistoryTransactionCallbacks(std::function<void(const QString&)> beginCallback,
                                                                 std::function<void()> commitCallback) {
    mBeginHistoryTransaction = std::move(beginCallback);
    mCommitHistoryTransaction = std::move(commitCallback);
}

QSharedPointer<model::IModelAction> StateMachineEntityViewModel::actionFromVariant(const QVariant& value) const {
    QSharedPointer<model::IModelAction> res;

    if (value.canConvert<QSharedPointer<model::IModelAction>>()) {
        res = value.value<QSharedPointer<model::IModelAction>>();
    } else {
        res = model::ModelActionFactory::createModelActionFromData(value.toString(), model::ModelAction::NONE);
    }

    Q_ASSERT(res);

    return res;
}

}  // namespace view
