#include "StateMachineEntityViewModel.hpp"

#include <QDebug>
#include <QStringList>
#include <QVariant>
#include <utility>

#include "model/HistoryState.hpp"
#include "model/ModelRootState.hpp"
#include "model/Transition.hpp"

namespace view {

StateMachineEntityViewModel::StateMachineEntityViewModel(const QSharedPointer<model::StateMachineModel>& model, QObject* parent)
    : QAbstractTableModel(parent)
    , mModel(model) {}

StateMachineEntityViewModel::~StateMachineEntityViewModel() {
    // Disconnect previous connection if any
    if (mEntitySignalConnection) {
        QObject::disconnect(mEntitySignalConnection);
    }
}

int StateMachineEntityViewModel::rowCount(const QModelIndex& /*parent*/) const {
    int res = 0;

    if (mSelectedEntity) {
        res = mSelectedEntity->properties().size();
    }

    return res;
}

int StateMachineEntityViewModel::columnCount(const QModelIndex& /*parent*/) const {
    return 2;  // Property name and value
}

QVariant StateMachineEntityViewModel::data(const QModelIndex& index, int role) const {
    QVariant res;

    if (mSelectedEntity && index.isValid()) {
        const auto& props = mSelectedEntity->properties();

        if ((index.row() >= 0) && (index.row() < props.size())) {
            const auto& key = props.at(index.row());

            if (index.column() == 0) {
                if (role == Qt::DisplayRole) {
                    res = key;
                }
            } else if (index.column() == 1) {
                if (role == Qt::DisplayRole || role == Qt::EditRole) {
                    res = mSelectedEntity->getProperty(key);

                    if ((role == Qt::DisplayRole) && (key == model::Transition::cKeyTransitionType)) {
                        res = model::transitionTypeToString(model::transitionTypeFromInt(res.toInt()));
                    } else if ((role == Qt::DisplayRole) && (key == model::HistoryState::cKeyHistoryType)) {
                        res = model::historyTypeToString(model::historyTypeFromInt(res.toInt()));
                    }
                } else if (role == Qt::UserRole) {
                    res = key;
                }
            }
        }
    }

    return res;
}

QVariant StateMachineEntityViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    } else if (section == 0) {
        return QStringLiteral("Property");
    } else if (section == 1) {
        return QStringLiteral("Value");
    }

    return QVariant();
}

void StateMachineEntityViewModel::selectEntityById(const model::EntityID_t id) {
    if (mModel && mModel->root()) {
        // Check if id matches root entity
        if (mModel->root()->id() == id) {
            selectEntity(mModel->root());
        } else {
            selectEntity(mModel->root()->findChild(id));
        }
    }
}

void StateMachineEntityViewModel::selectEntity(const QSharedPointer<model::StateMachineEntity>& entity) {
    beginResetModel();
    mSelectedEntity = entity;

    // Disconnect previous connection if any
    if (mEntitySignalConnection) {
        QObject::disconnect(mEntitySignalConnection);
    }

    if (mSelectedEntity) {
        // Connect to entity changes
        mEntitySignalConnection = QObject::connect(mSelectedEntity.get(),
                                                   &model::StateMachineEntity::modelDataChanged,
                                                   this,
                                                   [this](QWeakPointer<model::StateMachineEntity> changedEntity) {
                                                       emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
                                                   });
    }

    endResetModel();
}

Qt::ItemFlags StateMachineEntityViewModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    if (index.column() == 1) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

    // column 0
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool StateMachineEntityViewModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!mSelectedEntity || !index.isValid() || index.column() != 1 || role != Qt::EditRole) {
        return false;
    }

    const auto& props = mSelectedEntity->properties();

    if (index.row() < 0 || index.row() >= props.size()) {
        return false;
    }

    QString propName = props.at(index.row());
    QVariant newValue = value;

    // // Convert absolute path to relative if property is "path"
    // if (propName == "path" && mModel && !value.toString().isEmpty()) {
    //     QString absPath = value.toString();
    //     QString rootFile = mModel->filePath(); // You may need to add this getter if not present
    //     QFileInfo rootInfo(rootFile);
    //     QDir rootDir = rootInfo.absoluteDir();
    //     newValue = rootDir.relativeFilePath(absPath);
    // }

    if (mBeginHistoryTransaction) {
        mBeginHistoryTransaction(QString("Edit property: %1").arg(propName));
    }

    bool res = mSelectedEntity->setProperty(propName, newValue);

    if (mCommitHistoryTransaction) {
        mCommitHistoryTransaction();
    }

    qDebug() << "setProperty -> " << res;

    if (false == res) {
        return false;
    }

    emit dataChanged(index, index, {role});
    return true;
}

QStringList StateMachineEntityViewModel::valueOptions(int row) const {
    // // Example: for drop-downs, return options for specific properties
    // if (!mSelectedEntity) return {};
    // const auto& props = mSelectedEntity->properties();
    // if (row < 0 || row >= props.size()) return {};
    // QString propName = props.at(row);
    // if (auto transition = qobject_cast<model::Transition*>(mSelectedEntity.data())) {
    //     if (propName == "transitionType") {
    //         return {"EXTERNAL", "INTERNAL"};
    //     } else if (propName == "expectedConditionValue") {
    //         return {"false", "true"};
    //     }
    // }
    return {};
}

void StateMachineEntityViewModel::setHistoryTransactionCallbacks(std::function<void(const QString&)> beginCallback,
                                                                 std::function<void()> commitCallback) {
    mBeginHistoryTransaction = std::move(beginCallback);
    mCommitHistoryTransaction = std::move(commitCallback);
}

}  // namespace view
