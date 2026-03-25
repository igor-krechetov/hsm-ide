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

namespace {

QString sanitizeArgument(const QString& value) {
    QString sanitized = value;

    if (sanitized.contains(',') || sanitized.contains(' ')) {
        sanitized = QString("\"%1\"").arg(sanitized.replace('"', "\\\""));
    }

    return sanitized;
}

}  // namespace

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

bool StateMachineEntityViewModel::isActionProperty(const QString& key) const {
    bool res = false;

    if (key.endsWith("Callback") || key.endsWith("Action") || key == "action") {
        res = true;
    }

    return res;
}

StateMachineEntityViewModel::ComplexPropertyType StateMachineEntityViewModel::complexTypeForProperty(const QString& key) const {
    ComplexPropertyType res = ComplexPropertyType::None;

    if (isActionProperty(key)) {
        res = ComplexPropertyType::Action;
    }

    return res;
}

QStringList StateMachineEntityViewModel::selectedEntityProperties() const {
    QStringList res;

    if (hasSelectedEntity()) {
        res = mSelectedEntity->properties();
    }

    return res;
}

void StateMachineEntityViewModel::rebuildNodes() {
    mTopNodes.clear();

    if (hasSelectedEntity()) {
        const QStringList props = selectedEntityProperties();

        for (int i = 0; i < props.size(); ++i) {
            auto node = std::make_unique<PropertyNode>();
            node->row = i;
            node->propertyRow = i;
            node->type = NodeType::Property;
            node->key = props.at(i);
            node->label = props.at(i);
            node->complexType = complexTypeForProperty(node->key);

            if (node->complexType == ComplexPropertyType::Action) {
                const ActionDescriptor descriptor = parseActionDescriptor(mSelectedEntity->getProperty(node->key).toString());
                const QStringList attrs = actionAttributesForSubtype(descriptor.subtype);

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

QVariant StateMachineEntityViewModel::propertyRoleData(const PropertyNode& node, int role) const {
    QVariant res;

    if (role == PropertyKeyRole) {
        res = node.key;
    } else if (role == PropertyPathRole) {
        res = node.key;

        if (node.type == NodeType::ActionAttribute) {
            res = QString("%1.%2").arg(node.key, node.actionAttribute);
        }
    }

    return res;
}

QVariant StateMachineEntityViewModel::formatPropertyValueForRole(const PropertyNode& node, int role) const {
    QVariant res;

    if (hasSelectedEntity()) {
        const QVariant propertyValue = mSelectedEntity->getProperty(node.key);

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (node.complexType == ComplexPropertyType::Action) {
                const ActionDescriptor descriptor = parseActionDescriptor(propertyValue.toString());
                if (propertyValue.toString().trimmed().isEmpty()) {
                    res = QString("None");
                } else {
                    res = actionSubtypeToDisplayString(descriptor.subtype);
                }
            } else {
                res = propertyValue;

                if ((role == Qt::DisplayRole) && (node.key == model::Transition::cKeyTransitionType)) {
                    res = model::transitionTypeToString(model::transitionTypeFromInt(res.toInt()));
                } else if ((role == Qt::DisplayRole) && (node.key == model::HistoryState::cKeyHistoryType)) {
                    res = model::historyTypeToString(model::historyTypeFromInt(res.toInt()));
                }
            }
        } else if (role == ActionSubtypeRole && node.complexType == ComplexPropertyType::Action) {
            const ActionDescriptor descriptor = parseActionDescriptor(propertyValue.toString());
            res = actionSubtypeToDisplayString(descriptor.subtype);
        }
    }

    return res;
}

QVariant StateMachineEntityViewModel::formatActionAttributeValue(const PropertyNode& node, int role) const {
    QVariant res;

    if (hasSelectedEntity()) {
        const ActionDescriptor descriptor = parseActionDescriptor(mSelectedEntity->getProperty(node.key).toString());
        const QStringList attrs = actionAttributesForSubtype(descriptor.subtype);
        const int idx = attrs.indexOf(node.actionAttribute);

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (idx >= 0 && idx < descriptor.args.size()) {
                res = descriptor.args.at(idx);
            } else {
                res = QString();
            }
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
            res = propertyRoleData(*node, role);
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

StateMachineEntityViewModel::ActionDescriptor StateMachineEntityViewModel::parseActionDescriptor(
    const QString& rawValue) const {
    ActionDescriptor descriptor;
    auto action = model::createModelActionFromData(rawValue, model::ModelAction::CALLBACK);

    if (action) {
        if (action->type() == model::ModelAction::TIMER_START) {
            descriptor.subtype = model::ModelAction::TIMER_START;
        } else if (action->type() == model::ModelAction::TIMER_STOP) {
            descriptor.subtype = model::ModelAction::TIMER_STOP;
        } else if (action->type() == model::ModelAction::TIMER_RESTART) {
            descriptor.subtype = model::ModelAction::TIMER_RESTART;
        } else if (action->type() == model::ModelAction::SEND_EVENT) {
            descriptor.subtype = model::ModelAction::SEND_EVENT;
        } else {
            descriptor.subtype = model::ModelAction::CALLBACK;
        }

        const QStringList attrs = action->properties();

        for (const auto& attr : attrs) {
            descriptor.args.push_back(action->getProperty(attr).toString());
        }
    }

    return descriptor;
}

QString StateMachineEntityViewModel::buildActionValue(const ActionDescriptor& descriptor) const {
    QString res = descriptor.args.value(0).trimmed();
    model::ModelAction modelActionType = model::ModelAction::CALLBACK;

    if (descriptor.subtype == model::ModelAction::TIMER_START) {
        modelActionType = model::ModelAction::TIMER_START;
    } else if (descriptor.subtype == model::ModelAction::TIMER_STOP) {
        modelActionType = model::ModelAction::TIMER_STOP;
    } else if (descriptor.subtype == model::ModelAction::TIMER_RESTART) {
        modelActionType = model::ModelAction::TIMER_RESTART;
    } else if (descriptor.subtype == model::ModelAction::SEND_EVENT) {
        modelActionType = model::ModelAction::SEND_EVENT;
    }

    auto action = model::createModelAction(modelActionType);

    if (action) {
        const QStringList attrs = action->properties();

        for (int i = 0; i < attrs.size(); ++i) {
            action->setProperty(attrs.at(i), descriptor.args.value(i));
        }

        res = action->serialize();
    }

    return res;
}

QString StateMachineEntityViewModel::actionSubtypeToDisplayString(model::ModelAction subtype) const {
    return model::actionName(subtype);
}

model::ModelAction StateMachineEntityViewModel::actionSubtypeFromDisplayString(const QString& text) const {
    return model::actionTypeByName(text);
}

QStringList StateMachineEntityViewModel::actionAttributesForSubtype(model::ModelAction subtype) const {
    QStringList res;
    auto action = model::createModelAction(subtype);

    if (action) {
        res = action->properties();
    }

    return res;
}

QStringList StateMachineEntityViewModel::defaultArgumentsForSubtype(model::ModelAction subtype) const {
    QStringList res;
    ActionDescriptor descriptor;
    descriptor.subtype = subtype;

    if (subtype == model::ModelAction::TIMER_START) {
        descriptor.args = {"timer_id", "1000", "false"};
    } else if (subtype == model::ModelAction::TIMER_STOP || subtype == model::ModelAction::TIMER_RESTART) {
        descriptor.args = {"timer_id"};
    } else if (subtype == model::ModelAction::SEND_EVENT) {
        descriptor.args = {"event_id", "arguments"};
    } else {
        descriptor.args = {"function"};
    }

    const auto parsedDescriptor = parseActionDescriptor(buildActionValue(descriptor));
    res = parsedDescriptor.args;

    if (res.isEmpty()) {
        res = descriptor.args;
    }

    return res;
}

QStringList StateMachineEntityViewModel::parseActionArguments(const QString& rawArgs) const {
    QStringList args;
    QString current;
    bool insideQuotes = false;

    for (const QChar ch : rawArgs) {
        if (ch == '"') {
            insideQuotes = !insideQuotes;
            current.push_back(ch);
        } else if (ch == ',' && !insideQuotes) {
            args.push_back(current.trimmed().remove(QRegularExpression(R"(^"|"$)")));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    if (!current.isNull()) {
        args.push_back(current.trimmed().remove(QRegularExpression(R"(^"|"$)")));
    }

    return args;
}

QString StateMachineEntityViewModel::normalizeActionName(const QString& text) const {
    QString res = text.trimmed().toLower();
    res.replace('-', '_');
    res.replace(' ', '_');

    return res;
}

bool StateMachineEntityViewModel::applyPropertyValue(const QString& propName, const QVariant& newValue) {
    bool res = false;

    if (mBeginHistoryTransaction) {
        mBeginHistoryTransaction(QString("Edit property: %1").arg(propName));
    }

    res = mSelectedEntity->setProperty(propName, newValue);

    if (mCommitHistoryTransaction) {
        mCommitHistoryTransaction();
    }

    qDebug() << "setProperty -> " << res;

    return res;
}

bool StateMachineEntityViewModel::updatePropertyByNode(const PropertyNode& node, const QVariant& value, int role) {
    bool res = false;

    if (hasSelectedEntity() && role == Qt::EditRole) {
        if (node.type == NodeType::Property) {
            QVariant newValue = value;

            if (node.complexType == ComplexPropertyType::Action) {
                if (value.toString() == "None") {
                    newValue = QString();
                } else {
                    ActionDescriptor descriptor = parseActionDescriptor(mSelectedEntity->getProperty(node.key).toString());
                    descriptor.subtype = actionSubtypeFromDisplayString(value.toString());
                    descriptor.args = defaultArgumentsForSubtype(descriptor.subtype);
                    newValue = buildActionValue(descriptor);
                }
            }

            res = applyPropertyValue(node.key, newValue);
        } else if (node.type == NodeType::ActionAttribute) {
            ActionDescriptor descriptor = parseActionDescriptor(mSelectedEntity->getProperty(node.key).toString());
            const QStringList attrs = actionAttributesForSubtype(descriptor.subtype);
            const int idx = attrs.indexOf(node.actionAttribute);

            if (idx >= 0) {
                while (descriptor.args.size() <= idx) {
                    descriptor.args.push_back(QString());
                }

                descriptor.args[idx] = value.toString();
                res = applyPropertyValue(node.key, buildActionValue(descriptor));
            }
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

QStringList StateMachineEntityViewModel::valueOptions(int row) const {
    Q_UNUSED(row);
    return {};
}

void StateMachineEntityViewModel::setHistoryTransactionCallbacks(std::function<void(const QString&)> beginCallback,
                                                                 std::function<void()> commitCallback) {
    mBeginHistoryTransaction = std::move(beginCallback);
    mCommitHistoryTransaction = std::move(commitCallback);
}

}  // namespace view
