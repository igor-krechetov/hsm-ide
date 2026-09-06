#include "ModelRootState.hpp"

#include <QDebug>

#include "private/IModelVisitor.hpp"

namespace model {

ModelRootState::ModelRootState(const QString& name)
    : RegularState(name) {
    setStateType(StateType::MODEL_ROOT);
}

// Properties
QStringList ModelRootState::properties() const {
    return {"name"};
}

bool ModelRootState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "name") {
        setName(value.toString());
    }

    return handled;
}

QVariant ModelRootState::getProperty(const QString& key) const {
    if (key == "name") {
        return State::name();
    }

    return QVariant();
}

bool ModelRootState::hasInitialState() const {
    bool found = false;

    for (const QSharedPointer<StateMachineEntity>& child : childrenEntities()) {
        if (child && (child->type() == StateMachineEntity::Type::State)) {
            const QSharedPointer<State> childState = child.dynamicCast<State>();

            if (childState && (childState->stateType() == StateType::INITIAL)) {
                found = true;
                break;
            }
        }
    }

    return found;
}

};  // namespace model
