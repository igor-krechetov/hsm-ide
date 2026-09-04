#include "ModelActionUtils.hpp"

#include "ModelActionFactory.hpp"

namespace model {

ModelActionList ModelActionUtils::cloneActionList(const ModelActionList& source) {
    ModelActionList result;

    for (const auto& action : source) {
        if (action && (action->type() != ModelAction::NONE)) {
            result.append(ModelActionFactory::createModelActionFromData(action->serialize(), action->type()));
        }
    }

    return result;
}

ModelActionList ModelActionUtils::singleActionList(const QSharedPointer<IModelAction>& action) {
    ModelActionList result;

    if (action && (action->type() != ModelAction::NONE)) {
        result.append(action);
    }

    return result;
}

bool ModelActionUtils::setActions(ModelActionList& target, const ModelActionList& source) {
    target = cloneActionList(source);
    return true;
}

bool ModelActionUtils::addAction(ModelActionList& target, const QSharedPointer<IModelAction>& action) {
    bool changed = false;

    if (action && (action->type() != ModelAction::NONE)) {
        target.append(action);
        changed = true;
    }

    return changed;
}

bool ModelActionUtils::insertAction(ModelActionList& target, const int index, const QSharedPointer<IModelAction>& action) {
    bool changed = false;

    if (action && (action->type() != ModelAction::NONE) && (index >= 0) && (index <= target.size())) {
        target.insert(index, action);
        changed = true;
    }

    return changed;
}

bool ModelActionUtils::removeAction(ModelActionList& target, const int index) {
    bool changed = false;

    if ((index >= 0) && (index < target.size())) {
        target.removeAt(index);
        changed = true;
    }

    return changed;
}

bool ModelActionUtils::moveAction(ModelActionList& target, const int from, const int to) {
    bool changed = false;

    if ((from >= 0) && (from < target.size()) && (to >= 0) && (to < target.size()) && (from != to)) {
        target.move(from, to);
        changed = true;
    }

    return changed;
}

}  // namespace model
