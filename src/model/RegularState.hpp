#ifndef REGULARSTATE_HPP
#define REGULARSTATE_HPP

#include "State.hpp"
#include "Transition.hpp"

namespace model {

class RegularState : public State {
public:
    explicit RegularState(const QString& name);
    virtual ~RegularState() = default;

    // Getters
    const QString& onStateChangedCallback() const;
    const QString& onEnteringCallback() const;
    const QString& onExitingCallback() const;

    // Setters
    void setOnStateChangedCallback(const QString& callback);
    void setOnEnteringCallback(const QString& callback);
    void setOnExitingCallback(const QString& callback);

    void addChild(const QSharedPointer<StateMachineEntity>& child) override;
    void addChildState(const QSharedPointer<State>& child);
    void addTransition(const QSharedPointer<Transition>& child);
    const QList<QSharedPointer<StateMachineEntity>>& children() const;

    void deleteChild(const EntityID_t id) override;
    void deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) override;

    QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId) override;
    QSharedPointer<StateMachineEntity> findChild(const EntityID_t id,
                                                 const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const override;
    QSharedPointer<State> findState(const EntityID_t id) const;
    QSharedPointer<RegularState> findRegularState(const EntityID_t id) const;
    QSharedPointer<Transition> findTransition(const EntityID_t id) const;

    QStringList properties() const override;
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;

protected:
    void forEachChildElement(std::function<void(QSharedPointer<StateMachineEntity>)> callback, const int depth = DEPTH_INFINITE) override;

private:
    QString mOnStateChangedCallback;
    QString mOnEnteringCallback;
    QString mOnExitingCallback;

    QList<QSharedPointer<StateMachineEntity>> mChildren;
};

};  // namespace model

#endif  // REGULARSTATE_HPP
