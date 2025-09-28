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
    TransitionType type() const;

    // Setters
    void setType(TransitionType type);

private:
    TransitionType mType = TransitionType::EXTERNAL;
};

};  // namespace model

#endif  // SELFTRANSITION_HPP
