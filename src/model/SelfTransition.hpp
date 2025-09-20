#ifndef SELFTRANSITION_HPP
#define SELFTRANSITION_HPP

#include <QSharedPointer>
#include <QString>
#include <QWeakPointer>

#include "Transition.hpp"

namespace model {

// Forward declaration to break circular dependency
class State;

class SelfTransition : public Transition {
public:
    SelfTransition(QSharedPointer<State> state, const QString& event);
    virtual ~SelfTransition() = default;

    // Getters
    Type type() const;

    // Setters
    void setType(Type type);

private:
    Type mType = Type::EXTERNAL;
};

};  // namespace model

#endif  // SELFTRANSITION_HPP
