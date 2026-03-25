#include "IncludeEntity.hpp"

#include <QDebug>
#include <QStringList>

#include "ModelUtils.hpp"
#include "actions/ModelActionFactory.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

IncludeEntity::IncludeEntity(const QString& name)
    : RegularState(name) {
    setStateType(StateType::INCLUDE);
}

void IncludeEntity::promoteFrom(const QSharedPointer<RegularState>& state) {
    if (nullptr != state) {
        mName = state->name();

        mOnStateChangedAction = createModelActionFromData(state->onStateChangedCallback(), ModelAction::CALLBACK);
        mOnEnteringAction = createModelActionFromData(state->onEnteringCallback(), ModelAction::CALLBACK);
        mOnExitingAction = createModelActionFromData(state->onExitingCallback(), ModelAction::CALLBACK);
        mChildren = state->childrenEntities();

        for (auto& child : mChildren) {
            QSharedPointer<Transition> transition = hsmDynamicCast<Transition>(child);

            registerNewChild(child);

            if (transition) {
                transition->setSource(sharedFromThis().dynamicCast<State>());
            } else {
                qWarning() << "Non-Transition found inside xi:include element";
            }
        }
    }
}

void IncludeEntity::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitIncludeEntity(this);
    }
}

const QString& IncludeEntity::path() const {
    return mPath;
}

void IncludeEntity::setPath(const QString& newPath) {
    mPath = newPath;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

bool IncludeEntity::addChild(const QSharedPointer<StateMachineEntity>& child) {
    bool res = false;

    if (child && (child->type() == StateMachineEntity::Type::Transition)) {
        res = RegularState::addChild(child);
    }

    return res;
}

QStringList IncludeEntity::properties() const {
    return RegularState::properties() + QStringList{"path"};
}

bool IncludeEntity::setProperty(const QString& key, const QVariant& value) {
    qDebug() << "--------------- setProperty" << key << value;
    bool handled = true;

    if (key == "path") {
        setPath(value.toString());
    } else {
        handled = RegularState::setProperty(key, value);
    }

    return handled;
}

QVariant IncludeEntity::getProperty(const QString& key) const {
    if (key == "path") {
        return mPath;
    }

    return RegularState::getProperty(key);
}

};  // namespace model
