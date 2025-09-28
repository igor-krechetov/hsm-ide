#include "StateMachineTreeModel.hpp"

#include <QVariant>

#include "model/EntryPoint.hpp"
#include "model/InitialState.hpp"
#include "model/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/Transition.hpp"

namespace view {

StateMachineTreeModel::StateMachineTreeModel(QSharedPointer<model::StateMachineModel> model, QObject* parent)
    : QAbstractItemModel(parent)
    , mModel(model) {
    if (mModel) {
        QObject::connect(mModel.data(), &model::StateMachineModel::modelChanged, this, &StateMachineTreeModel::onModelChanged);
        QObject::connect(mModel.data(),
                         &model::StateMachineModel::modelDataChanged,
                         this,
                         &StateMachineTreeModel::onModelDataChanged);
    }
    setupModelData();
}

StateMachineTreeModel::~StateMachineTreeModel() {
    delete mRootNode;
    mRootNode = nullptr;
}

void StateMachineTreeModel::onModelChanged() {
    qDebug() << "-----------" << Q_FUNC_INFO;
    beginResetModel();

    delete mRootNode;
    mRootNode = nullptr;
    setupModelData();

    endResetModel();
}

void StateMachineTreeModel::onModelDataChanged(QWeakPointer<model::StateMachineEntity> entity) {
    qDebug() << "-----------" << Q_FUNC_INFO;
    // Find the QModelIndex for the changed entity and emit dataChanged for that row
    auto entityPtr = entity.lock();

    if (entityPtr) {
        QModelIndex idx = findModelEntity(entityPtr->id());

        if (idx.isValid()) {
            emit dataChanged(idx, idx, {Qt::DisplayRole});
        }
    }
}

void StateMachineTreeModel::setupModelData() {
    // Build tree from StateMachineModel
    if (mModel && mModel->root()) {
        addModelEntity(nullptr, mModel->root());
    }
}

StateMachineTreeModel::TreeNode* StateMachineTreeModel::nodeFromIndex(const QModelIndex& index) const {
    if (index.isValid()) {
        return static_cast<TreeNode*>(index.internalPointer());
    }

    return mRootNode;
}

QModelIndex StateMachineTreeModel::index(int row, int column, const QModelIndex& parent) const {
    TreeNode* parentNode = nodeFromIndex(parent);
    if (!parentNode || row < 0 || row >= parentNode->children.size())
        return QModelIndex();
    TreeNode* childNode = parentNode->children.at(row);
    return createIndex(row, column, childNode);
}

QModelIndex StateMachineTreeModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();
    TreeNode* node = nodeFromIndex(index);
    if (!node || !node->parent)
        return QModelIndex();
    return createIndex(node->parent->row(), 0, node->parent);
}

int StateMachineTreeModel::rowCount(const QModelIndex& parent) const {
    TreeNode* node = nodeFromIndex(parent);
    return node ? node->children.size() : 0;
}

int StateMachineTreeModel::columnCount(const QModelIndex& /*parent*/) const {
    return 1;
}

QVariant StateMachineTreeModel::data(const QModelIndex& index, int role) const {
    QVariant res;

    if (index.isValid()) {
        TreeNode* node = nodeFromIndex(index);

        if (nullptr != node) {
            if (Qt::DisplayRole == role) {
                if (model::StateMachineEntity::Type::State == node->type() && node->entity) {
                    auto state = node->entity.dynamicCast<model::State>();

                    if (state) {
                        res = state->name();
                    }
                } else if (model::StateMachineEntity::Type::Transition == node->type() && node->entity) {
                    auto transition = node->entity.dynamicCast<model::Transition>();
                    QString sourceName;
                    QString targetName;

                    if (transition) {
                        if (transition->source()) {
                            sourceName = transition->source()->name();
                        }
                        if (transition->target()) {
                            targetName = transition->target()->name();
                        }

                        res = QString("Transition %1 -> %2").arg(sourceName, targetName);
                    }
                }
            } else if (Qt::UserRole == role) {
                if (node->entity) {
                    res = node->entity->id();
                } else {
                    // TODO: error
                }
            }
        }
    }

    return res;
}

Qt::ItemFlags StateMachineTreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex StateMachineTreeModel::findModelEntity(const model::EntityID_t id) const {
    QModelIndex res;
    QModelIndex start = index(0, 0);  // start from top-left
    QModelIndexList matches = match(start,
                                    Qt::UserRole,
                                    QVariant(id),
                                    1,  // only need 1 match
                                    Qt::MatchExactly | Qt::MatchRecursive);

    if (false == matches.isEmpty()) {
        res = matches.first();
        qDebug() << "Found at row" << res.row();
    }

    return res;
}

// TODO: following entities can have children: RegularState, EntryPoint, InitialState
// modify arg to be of State
void StateMachineTreeModel::addModelEntity(TreeNode* parentNode, const QSharedPointer<model::StateMachineEntity>& entity) {
    if (entity) {
        TreeNode* childNode = new TreeNode{parentNode, {}, entity};

        if (nullptr == parentNode) {
            mRootNode = childNode;
        } else {
            parentNode->children.append(childNode);
        }

        if (entity->type() == model::State::Type::State) {
            auto state = entity.dynamicCast<model::State>();

            if (state) {
                switch (state->stateType()) {
                    case model::State::StateType::Regular:
                        addRegularState(childNode, state.dynamicCast<model::RegularState>());
                        break;
                    case model::State::StateType::EntryPoint:
                        addEntryPoint(childNode, state.dynamicCast<model::EntryPoint>());
                        break;
                    case model::State::StateType::Initial:
                        addInitialState(childNode, state.dynamicCast<model::InitialState>());
                        break;
                    default:
                        // TODO: error
                        break;
                }
            } else {
                // TODO: Critical error
            }
        }
    }
}

void StateMachineTreeModel::addRegularState(TreeNode* parentNode, const QSharedPointer<model::RegularState>& state) {
    for (const auto& child : state->children()) {
        addModelEntity(parentNode, child);

        // if (child->type() == model::StateMachineEntity::Type::State) {
        //     // auto childState = child.dynamicCast<model::RegularState>();
        //     // if (childState) {
        //         // auto childNode = new TreeNode{child->type(), childState, nullptr, parentNode, {}};
        //         // parentNode->children.append(childNode);
        //     addModelEntity(parentNode, child);
        //     // }
        // } else if (child->type() == model::StateMachineEntity::Type::Transition) {
        //     auto transition = child.dynamicCast<model::Transition>();
        //     if (transition) {
        //         auto transNode = new TreeNode{child->type(), nullptr, transition, parentNode, {}};
        //         parentNode->children.append(transNode);
        //     }
        // }
    }
}

void StateMachineTreeModel::addEntryPoint(TreeNode* parentNode, const QSharedPointer<model::EntryPoint>& state) {
    for (const auto& transition : state->transitions()) {
        addModelEntity(parentNode, transition);
    }
}

void StateMachineTreeModel::addInitialState(TreeNode* parentNode, const QSharedPointer<model::InitialState>& state) {
    addModelEntity(parentNode, state->transition());
}

}  // namespace view
