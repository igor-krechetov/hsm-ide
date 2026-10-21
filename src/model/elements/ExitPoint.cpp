#include "ExitPoint.hpp"

#include "private/IModelVisitor.hpp"

namespace model {

ExitPoint::ExitPoint(const QString& name)
    : StateWithActions(name, StateType::EXITPOINT) {}

void ExitPoint::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitExitPoint(this);
    }
}

const QString& ExitPoint::event() const {
    return mEvent;
}

void ExitPoint::setEvent(const QString& event) {
    mEvent = event;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

QStringList ExitPoint::properties() const {
    return StateWithActions::properties() + QStringList{"event"};
}

bool ExitPoint::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "event") {
        setEvent(value.toString());
    } else {
        handled = StateWithActions::setProperty(key, value);
    }

    return handled;
}

QVariant ExitPoint::getProperty(const QString& key) const {
    QVariant res;

    if (key == "event") {
        res = mEvent;
    } else {
        res = StateWithActions::getProperty(key);
    }

    return res;
}

void ExitPoint::copyEntityData(const StateMachineEntity& other) {
    StateWithActions::copyEntityData(other);

    if (const ExitPoint* eOther = dynamic_cast<const ExitPoint*>(&other)) {
        mEvent = eOther->mEvent;
    }
}

};  // namespace model
