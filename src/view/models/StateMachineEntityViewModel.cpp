#include "StateMachineEntityViewModel.hpp"

#include <QDebug>
#include <QRegularExpression>
#include <QStringList>
#include <QVariant>
#include <utility>

#include "model/actions/ModelActionFactory.hpp"
#include "model/elements/HistoryState.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/Transition.hpp"

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

    if (key == "onEnteringAction" || key == "onExitingAction" || key == "transitionAction") {
        res = ComplexPropertyType::ActionList;
    } else if (key.endsWith("Action") || key == "action") {
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
                buildActionItemChildren(node.get(), ptrAction);
            } else if (node->complexType == ComplexPropertyType::ActionList) {
                const model::ModelActionList actions = actionListForKey(node->key);

                for (int actionIdx = 0; actionIdx < actions.size(); ++actionIdx) {
                    auto itemNode = std::make_unique<PropertyNode>();
                    itemNode->row = actionIdx;
                    itemNode->propertyRow = i;
                    itemNode->actionIndex = actionIdx;
                    itemNode->type = NodeType::ActionItem;
                    itemNode->key = node->key;
                    itemNode->label = QString("action %1").arg(actionIdx + 1);
                    itemNode->parent = node.get();

                    buildActionItemChildren(itemNode.get(), actions.at(actionIdx));
                    node->children.push_back(std::move(itemNode));
                }
                // The "+" add affordance is painted on this slot row itself (see delegate),
                // so no dedicated ActionAdd child row is created.
            }

            mTopNodes.push_back(std::move(node));
        }
    }
}

void StateMachineEntityViewModel::buildActionItemChildren(PropertyNode* actionNode,
                                                          const QSharedPointer<model::IModelAction>& action) {
    if ((nullptr != actionNode) && action) {
        const QStringList attrs = action->properties();

        for (int childRow = 0; childRow < attrs.size(); ++childRow) {
            auto child = std::make_unique<PropertyNode>();

            child->row = childRow;
            child->propertyRow = actionNode->propertyRow;
            child->actionIndex = actionNode->actionIndex;
            child->type = NodeType::ActionAttribute;
            child->key = actionNode->key;
            child->label = attrs.at(childRow);
            child->actionAttribute = attrs.at(childRow);
            child->parent = actionNode;
            actionNode->children.push_back(std::move(child));
        }
    }
}

model::ModelActionList StateMachineEntityViewModel::actionListForKey(const QString& key) const {
    model::ModelActionList res;

    if (hasSelectedEntity()) {
        const QVariant value = mSelectedEntity->getProperty(key);

        if (value.canConvert<model::ModelActionList>()) {
            res = value.value<model::ModelActionList>();
        }
    }

    return res;
}

QSharedPointer<model::IModelAction> StateMachineEntityViewModel::actionAtIndex(const QString& key, const int index) const {
    QSharedPointer<model::IModelAction> res;
    const model::ModelActionList actions = actionListForKey(key);

    if ((index >= 0) && (index < actions.size())) {
        res = actions.at(index);
    }

    return res;
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
        if (node.complexType == ComplexPropertyType::ActionList) {
            // Slot summary row: "N actions" / "1 action" / "none"
            const int count = actionListForKey(node.key).size();

            if (role == Qt::DisplayRole) {
                if (count == 0) {
                    res = QObject::tr("none");
                } else if (count == 1) {
                    res = QObject::tr("1 action");
                } else {
                    res = QObject::tr("%1 actions").arg(count);
                }
            }
        } else if (node.complexType == ComplexPropertyType::Action) {
            const QVariant propertyValue = mSelectedEntity->getProperty(node.key);

            if (role == Qt::DisplayRole || role == Qt::EditRole || role == ActionSubtypeRole) {
                auto ptrAction = actionFromVariant(propertyValue);
                res = model::ModelActionFactory::actionName(ptrAction->type());
            }
        } else {
            const QVariant propertyValue = mSelectedEntity->getProperty(node.key);

            if (role == Qt::DisplayRole || role == Qt::EditRole) {
                res = propertyValue;

                if ((role == Qt::DisplayRole) && (node.key == model::Transition::cKeyTransitionType)) {
                    res = model::transitionTypeToString(model::transitionTypeFromInt(res.toInt()));
                } else if ((role == Qt::DisplayRole) && (node.key == model::HistoryState::cKeyHistoryType)) {
                    res = model::historyTypeToString(model::historyTypeFromInt(res.toInt()));
                }
            }
        }
    }

    return res;
}

QVariant StateMachineEntityViewModel::formatActionAttributeValue(const PropertyNode& node, int role) const {
    QVariant res;

    if ((role == Qt::DisplayRole || role == Qt::EditRole) && hasSelectedEntity()) {
        QSharedPointer<model::IModelAction> ptrAction;

        if (node.actionIndex >= 0) {
            ptrAction = actionAtIndex(node.key, node.actionIndex);
        } else {
            ptrAction = actionFromVariant(mSelectedEntity->getProperty(node.key));
        }

        if (ptrAction) {
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
            } else if (node->type == NodeType::ActionItem) {
                if ((role == Qt::DisplayRole) || (role == Qt::EditRole) || (role == ActionSubtypeRole)) {
                    auto ptrAction = actionAtIndex(node->key, node->actionIndex);

                    if (ptrAction) {
                        res = model::ModelActionFactory::actionName(ptrAction->type());
                    }
                }
            } else if (node->type == NodeType::ActionAttribute) {
                res = formatActionAttributeValue(*node, role);
            }
        }

        if (!res.isValid()) {
            if (role == PropertyKeyRole) {
                res = node->key;
            } else if (role == ActionAddRole && node->type == NodeType::Property &&
                       node->complexType == ComplexPropertyType::ActionList) {
                res = true;
            } else if (role == ActionRemovableRole && node->type == NodeType::ActionItem) {
                res = true;
            } else if (role == ActionMoveUpRole && node->type == NodeType::ActionItem) {
                res = (node->actionIndex > 0);
            } else if (role == ActionMoveDownRole && node->type == NodeType::ActionItem) {
                const int count = actionListForKey(node->key).size();
                res = (node->actionIndex >= 0) && (node->actionIndex < (count - 1));
            } else if (role == PropertyPathRole) {
                res = node->key;

                if (node->type == NodeType::ActionItem) {
                    res = QString("%1[%2]").arg(node->key).arg(node->actionIndex);
                } else if (node->type == NodeType::ActionAttribute) {
                    if (node->actionIndex >= 0) {
                        res = QString("%1[%2].%3").arg(node->key).arg(node->actionIndex).arg(node->actionAttribute);
                    } else {
                        res = QString("%1.%2").arg(node->key, node->actionAttribute);
                    }
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

        const PropertyNode* node = nodeFromIndex(index);

        // The ActionList slot summary row (e.g. "3 actions") is not directly editable;
        // its children (ActionItem / ActionAttribute) and the ActionAdd row carry the interaction.
        const bool isListSummary =
            (node && (node->type == NodeType::Property) && (node->complexType == ComplexPropertyType::ActionList));

        if ((index.column() == 1) && (isListSummary == false)) {
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
        } else if (node.type == NodeType::ActionItem) {
            // Change the type of one action within the list
            model::ModelActionList actions = actionListForKey(node.key);

            if ((node.actionIndex >= 0) && (node.actionIndex < actions.size())) {
                const auto newActionType = model::ModelActionFactory::actionTypeByName(value.toString());

                if (newActionType != actions.at(node.actionIndex)->type()) {
                    actions[node.actionIndex] = model::ModelActionFactory::createModelAction(newActionType);
                    res = mSelectedEntity->setProperty(node.key, QVariant::fromValue(actions));
                }
            }
        } else if (node.type == NodeType::ActionAttribute) {
            Q_ASSERT(node.parent);

            if (nullptr != node.parent) {
                if (node.actionIndex >= 0) {
                    // Attribute of one action within a list slot
                    model::ModelActionList actions = actionListForKey(node.key);

                    if ((node.actionIndex >= 0) && (node.actionIndex < actions.size())) {
                        actions.at(node.actionIndex)->setProperty(node.actionAttribute, value);
                        res = mSelectedEntity->setProperty(node.key, QVariant::fromValue(actions));
                    }
                } else {
                    // Attribute of a single-action slot (e.g. onStateChangedAction)
                    auto ptrCurrentAction = actionFromVariant(mSelectedEntity->getProperty(node.key));

                    ptrCurrentAction->setProperty(node.actionAttribute, value);
                    res = mSelectedEntity->setProperty(node.parent->key, QVariant::fromValue(ptrCurrentAction));
                }
            }
        }

        if (mCommitHistoryTransaction) {
            mCommitHistoryTransaction();
        }
    }

    return res;
}

bool StateMachineEntityViewModel::addActionToSlot(const QString& key) {
    bool res = false;

    if (hasSelectedEntity()) {
        if (mBeginHistoryTransaction) {
            mBeginHistoryTransaction(QString("Add action: %1").arg(key));
        }

        model::ModelActionList actions = actionListForKey(key);
        actions.append(model::ModelActionFactory::createModelAction(model::ModelAction::CALLBACK));
        res = mSelectedEntity->setProperty(key, QVariant::fromValue(actions));

        if (mCommitHistoryTransaction) {
            mCommitHistoryTransaction();
        }
    }

    return res;
}

bool StateMachineEntityViewModel::removeActionFromSlot(const QString& key, const int index) {
    bool res = false;

    if (hasSelectedEntity()) {
        model::ModelActionList actions = actionListForKey(key);

        if ((index >= 0) && (index < actions.size())) {
            if (mBeginHistoryTransaction) {
                mBeginHistoryTransaction(QString("Remove action: %1").arg(key));
            }

            actions.removeAt(index);
            res = mSelectedEntity->setProperty(key, QVariant::fromValue(actions));

            if (mCommitHistoryTransaction) {
                mCommitHistoryTransaction();
            }
        }
    }

    return res;
}

bool StateMachineEntityViewModel::moveActionInSlot(const QString& key, const int from, const int to) {
    bool res = false;

    if (hasSelectedEntity()) {
        model::ModelActionList actions = actionListForKey(key);

        if ((from >= 0) && (from < actions.size()) && (to >= 0) && (to < actions.size()) && (from != to)) {
            if (mBeginHistoryTransaction) {
                mBeginHistoryTransaction(QString("Reorder action: %1").arg(key));
            }

            actions.move(from, to);
            res = mSelectedEntity->setProperty(key, QVariant::fromValue(actions));

            if (mCommitHistoryTransaction) {
                mCommitHistoryTransaction();
            }
        }
    }

    return res;
}

bool StateMachineEntityViewModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    bool res = false;
    const PropertyNode* node = nodeFromIndex(index);

    if (node && index.column() == 1) {
        if ((node->type == NodeType::Property) && (node->complexType == ComplexPropertyType::ActionList) &&
            (role == ActionAddRole)) {
            res = addActionToSlot(node->key);
        } else if ((node->type == NodeType::ActionItem) && (role == ActionRemovableRole)) {
            res = removeActionFromSlot(node->key, node->actionIndex);
        } else if ((node->type == NodeType::ActionItem) && (role == ActionMoveUpRole)) {
            res = moveActionInSlot(node->key, node->actionIndex, node->actionIndex - 1);
        } else if ((node->type == NodeType::ActionItem) && (role == ActionMoveDownRole)) {
            res = moveActionInSlot(node->key, node->actionIndex, node->actionIndex + 1);
        } else {
            res = updatePropertyByNode(*node, value, role);
        }

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
