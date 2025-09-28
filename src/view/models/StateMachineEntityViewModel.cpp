#include "StateMachineEntityViewModel.hpp"
#include "model/RegularState.hpp"

#include <QStringList>
#include <QVariant>
#include <QDebug>

namespace view {

StateMachineEntityViewModel::StateMachineEntityViewModel(const QSharedPointer<model::StateMachineModel>& model, QObject* parent)
    : QAbstractTableModel(parent)
    , mModel(model) {}

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
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();
    if (section == 0)
        return QStringLiteral("Property");
    if (section == 1)
        return QStringLiteral("Value");
    return QVariant();
}

void StateMachineEntityViewModel::selectEntityById(const model::EntityID_t id) {
    if (mModel && mModel->root()) {
        auto entity = mModel->root()->findChild(id);
        selectEntity(entity);
    }
}

void StateMachineEntityViewModel::selectEntity(const QSharedPointer<model::StateMachineEntity>& entity) {
    beginResetModel();
    mSelectedEntity = entity;
    // TODO: handle empty element
    endResetModel();
}

Qt::ItemFlags StateMachineEntityViewModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    if (index.column() == 1) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool StateMachineEntityViewModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    qDebug() << "------ StateMachineEntityViewModel::setData";

    if (!mSelectedEntity || !index.isValid() || index.column() != 1 || role != Qt::EditRole) {
        return false;
    }
    const auto& props = mSelectedEntity->properties();
    if (index.row() < 0 || index.row() >= props.size()) {
        return false;
    }
    QString propName = props.at(index.row());
    bool res = mSelectedEntity->setProperty(propName, value);

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

}  // namespace view