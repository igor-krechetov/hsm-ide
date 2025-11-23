#ifndef HISTORYSTATE_HPP
#define HISTORYSTATE_HPP

#include "State.hpp"

namespace model {

class Transition;

class HistoryState : public State {
public:
    explicit HistoryState(const QString& name, const HistoryType historyType);
    virtual ~HistoryState() = default;

    void accept(class IModelVisitor* visitor) override;

    // Getters
    HistoryType historyType() const {
        return mHistoryType;
    }

    // Setters
    void setHistoryType(HistoryType historyType);

    void setDefaultTransition(const QSharedPointer<Transition>& transition);
    QSharedPointer<Transition> defaultTransition() const;

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;

    bool addChild(const QSharedPointer<StateMachineEntity>& child) override;
    void deleteChild(const EntityID_t id) override;
    void deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) override;

    QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId) override;
    QSharedPointer<StateMachineEntity> findChild(
        const EntityID_t id,
        const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const override;

protected:
    bool forEachChildElement(
        std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
        const int depth = DEPTH_INFINITE,
        const bool postOrderTraversal = true) override;

public:
    static constexpr char cKeyHistoryType[] = "historyType";

private:
    HistoryType mHistoryType;
    QSharedPointer<Transition> mDefaultTransition;
};

};  // namespace model

#endif  // HISTORYSTATE_HPP
