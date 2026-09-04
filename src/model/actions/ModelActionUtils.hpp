#ifndef MODELACTIONUTILS_HPP
#define MODELACTIONUTILS_HPP

#include <QSharedPointer>

#include "IModelAction.hpp"

namespace model {

class ModelActionUtils {
public:
    // Deep-copies each action in a list by re-parsing its serialized form, preserving each
    // action's concrete type. NONE actions are dropped so an empty list means "no action".
    static ModelActionList cloneActionList(const ModelActionList& source);

    // Builds a normalized single-element list from one action (empty if the action is null or NONE).
    static ModelActionList singleActionList(const QSharedPointer<IModelAction>& action);

    // Pure list mutators (no signalling). Each normalizes away NONE actions and validates
    // indices; callers emit their change notification after invoking these. Every mutator
    // returns true when it modified the target, false otherwise.
    static bool setActions(ModelActionList& target, const ModelActionList& source);
    static bool addAction(ModelActionList& target, const QSharedPointer<IModelAction>& action);
    static bool insertAction(ModelActionList& target, const int index, const QSharedPointer<IModelAction>& action);
    static bool removeAction(ModelActionList& target, const int index);
    static bool moveAction(ModelActionList& target, const int from, const int to);
};

}  // namespace model

#endif  // MODELACTIONUTILS_HPP
