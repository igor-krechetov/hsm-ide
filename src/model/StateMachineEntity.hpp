#ifndef STATEMACHINEENTITY_HPP
#define STATEMACHINEENTITY_HPP

#include <QObject>
#include <QSharedPointer>
#include <vector>

#include "model/ModelTypes.hpp"

// TODO: move StateMachineElement::ID_t to a common header

namespace model {

class StateMachineEntity : public QObject, public QEnableSharedFromThis<StateMachineEntity> {
    Q_OBJECT
public:
    enum class Type { Invalid, State, Transition };  // namespace model

public:
    explicit StateMachineEntity(const Type type);
    virtual ~StateMachineEntity() = default;

    EntityID_t id() const;
    Type type() const;

signals:
    void modelDataChanged();

private:
    EntityID_t mId = 0;
    Type mElementType = Type::Invalid;
};

};  // namespace model

#endif  // STATEMACHINEENTITY_HPP
