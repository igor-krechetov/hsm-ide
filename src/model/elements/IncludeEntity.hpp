#ifndef INCLUDEENTITY_HPP
#define INCLUDEENTITY_HPP

#include <QSharedPointer>
#include <QString>

#include "RegularState.hpp"

namespace model {

class IncludeEntity : public RegularState {
public:
    explicit IncludeEntity(const QString& name);
    virtual ~IncludeEntity() = default;

    void promoteFrom(const QSharedPointer<RegularState>& state);

    void accept(class IModelVisitor* visitor) override;

    const QString& path() const;
    void setPath(const QString& newPath);

    virtual QStringList properties() const override;
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;

    bool addChild(const QSharedPointer<StateMachineEntity>& child) override;

    // Adding substates to include element is not allowed
    void addChildState(const QSharedPointer<State>& child) = delete;
    QSharedPointer<State> findState(const EntityID_t id) = delete;
    QSharedPointer<RegularState> findRegularState(const EntityID_t id) const = delete;

protected:
    QString mPath;
};

};  // namespace model

#endif  // INCLUDEENTITY_HPP
