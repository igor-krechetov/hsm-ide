#include "StateMachineEntity.hpp"

namespace model {

StateMachineEntity::StateMachineEntity(Type type)
    : mElementType(type) {
    // TODO: reset state index per model
    static EntityID_t sNextId = 1;
    mId = sNextId;
    ++sNextId;
}

StateMachineEntity::Type StateMachineEntity::type() const {
    return mElementType;
}

StateMachineEntity& StateMachineEntity::operator=(const StateMachineEntity& other) {
    if (this != &other) {
        copyEntityData(other);
    }

    return *this;
}

void StateMachineEntity::copyEntityData(const StateMachineEntity& other) {
    mMetadata = other.mMetadata;

    const QStringList props = other.properties();

    for (const QString& key : props) {
        setProperty(key, other.getProperty(key));
    }
}

EntityID_t StateMachineEntity::id() const {
    return mId;
}

void StateMachineEntity::setMetadata(const MetadataKey key, const QVariant& value) {
    mMetadata[key] = value;
}

QVariant StateMachineEntity::getMetadata(const MetadataKey key) const {
    QVariant result;
    auto it = mMetadata.find(key);

    if (it != mMetadata.end()) {
        result = it.value();
    }

    return result;
}

void StateMachineEntity::setPos(const QPointF& pos) {
    setMetadata(MetadataKey::POSITION_X, pos.x());
    setMetadata(MetadataKey::POSITION_Y, pos.y());
}

QPointF StateMachineEntity::getPos() const {
    const double x = getMetadata(MetadataKey::POSITION_X).toDouble();
    const double y = getMetadata(MetadataKey::POSITION_Y).toDouble();
    return QPointF(x, y);
}

void StateMachineEntity::setSize(const QSizeF& size) {
    setMetadata(MetadataKey::WIDTH, size.width());
    setMetadata(MetadataKey::HEIGHT, size.height());
}

QSizeF StateMachineEntity::getSize() const {
    const double width = getMetadata(MetadataKey::WIDTH).toDouble();
    const double height = getMetadata(MetadataKey::HEIGHT).toDouble();
    return QSizeF(width, height);
}

bool StateMachineEntity::addChild(const QSharedPointer<StateMachineEntity>& child) {
    // do nothing
    return false;
}

void StateMachineEntity::deleteChild(const EntityID_t id) {
    // do nothing
}

void StateMachineEntity::deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) {
    // do nothing
}

QSharedPointer<StateMachineEntity> StateMachineEntity::findParentState(const EntityID_t childId) {
    // do nothing
    return nullptr;
}

QSharedPointer<StateMachineEntity> StateMachineEntity::findChild(const EntityID_t id,
                                                                 const StateMachineEntity::Type type) const {
    // do nothing
    return nullptr;
}

QStringList StateMachineEntity::properties() const {
    return {};
}

bool StateMachineEntity::setProperty(const QString& key, const QVariant& value) {
    Q_UNUSED(key);
    Q_UNUSED(value);
    return false;
}

QVariant StateMachineEntity::getProperty(const QString& key) const {
    Q_UNUSED(key);
    return {};
}

void StateMachineEntity::registerNewChild(const QSharedPointer<StateMachineEntity>& child) {
    // Propagate subscription: connect child's modelEntityAdded to this
    QObject::connect(child.get(), &StateMachineEntity::childAdded, this, &StateMachineEntity::childAdded);
    QObject::connect(child.get(), &StateMachineEntity::childRemoved, this, &StateMachineEntity::childRemoved);
    QObject::connect(child.get(), &StateMachineEntity::modelDataChanged, this, &StateMachineEntity::modelDataChanged);

    emit childAdded(sharedFromThis().toWeakRef(), child.toWeakRef());
}

void StateMachineEntity::unregisterChild(const QSharedPointer<StateMachineEntity>& child) {
    emit childRemoved(sharedFromThis().toWeakRef(), child.toWeakRef());
}

bool StateMachineEntity::forEachChildElement(
    std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
    const int depth,
    const bool postOrderTraversal) {
    // do nothing
    return true;
}

};  // namespace model
