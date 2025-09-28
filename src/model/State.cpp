#include "State.hpp"

#include <QDebug>
#include <QStringList>

#include "Transition.hpp"

namespace model {

State::State(const QString& name, const StateType type)
    : StateMachineEntity(StateMachineEntity::Type::State)
    , mName(name)
    , mStateType(type) {}

// const QString& State::id() const {
//     return mId;
// }

State::StateType State::stateType() const {
    return mStateType;
}

const QString& State::name() const {
    return mName;
}

void State::setName(const QString& name) {
    mName = name;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

bool State::setProperty(const QString& key, const QVariant& value) {
    bool handled = false;

    if (key == "name") {
        setName(value.toString());
        handled = true;
    }

    return handled;
}

QVariant State::getProperty(const QString& key) const {
    if (key == "name") {
        return mName;
    }
    return {};
}

QStringList State::properties() const {
    return {"name"};
}

};  // namespace model
