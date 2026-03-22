#ifndef STATEMACHINEENTITY_HPP
#define STATEMACHINEENTITY_HPP

#include <QMap>
#include <QObject>
#include <QPointF>
#include <QSharedPointer>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "model/ModelTypes.hpp"

// TODO: move StateMachineElement::ID_t to a common header

namespace model {

class IModelVisitor;

class StateMachineEntity : public QObject, public QEnableSharedFromThis<StateMachineEntity> {
    Q_OBJECT

public:
    constexpr static int DEPTH_INFINITE = -1;

public:
    enum class MetadataKey { POSITION_X, POSITION_Y, WIDTH, HEIGHT, GEOMETRY };

public:
    enum class Type { Invalid, State, Transition };  // namespace model

public:
    explicit StateMachineEntity(const Type type);
    virtual ~StateMachineEntity();
    virtual StateMachineEntity& operator=(const StateMachineEntity& other);
    virtual void copyEntityData(const StateMachineEntity& other);

    EntityID_t id() const;
    Type type() const;

    void setMetadata(const MetadataKey key, const QVariant& value);
    QVariant getMetadata(const MetadataKey key) const;

    void setPos(const QPointF& pos);
    QPointF getPos() const;

    void setSize(const QSizeF& size);
    QSizeF getSize() const;

    virtual void accept(class IModelVisitor* visitor) = 0;

    virtual bool addChild(const QSharedPointer<StateMachineEntity>& child);
    virtual void deleteChild(const EntityID_t id);
    virtual void deleteDirectChild(const QSharedPointer<StateMachineEntity>& child);
    virtual void deleteAllChildren();

    virtual QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId);
    virtual QSharedPointer<StateMachineEntity> findChild(
        const EntityID_t id,
        const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const;

    virtual QStringList properties() const;
    virtual bool setProperty(const QString& key, const QVariant& value);
    virtual QVariant getProperty(const QString& key) const;

    // postOrderTraversal - if true, traverse children before calling the callback on the parent
    //                      if false, call the callback on the parent before traversing children
    virtual bool forEachChildElement(
        std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
        const int depth = DEPTH_INFINITE,
        const bool postOrderTraversal = true);

protected:
    void registerNewChild(const QSharedPointer<StateMachineEntity>& child);
    void unregisterChild(const QSharedPointer<StateMachineEntity>& child);

signals:
    // TODO: use id instead?
    void childAdded(QWeakPointer<StateMachineEntity> parent, QWeakPointer<StateMachineEntity> entity);
    void childRemoved(QWeakPointer<StateMachineEntity> parent, QWeakPointer<StateMachineEntity> entity);
    void modelDataChanged(QWeakPointer<StateMachineEntity> entity);

private:
    EntityID_t mId = 0;
    Type mElementType = Type::Invalid;

    // x, y, width, height, geomerty
    QMap<MetadataKey, QVariant> mMetadata;
};

};  // namespace model

#endif  // STATEMACHINEENTITY_HPP
