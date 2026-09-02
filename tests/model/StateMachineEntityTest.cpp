#include <QtTest>

#include <QSignalSpy>

#include "model/private/EntityIdGenerator.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/StateMachineEntity.hpp"

class DummyEntity : public model::StateMachineEntity {
public:
    explicit DummyEntity(Type type)
        : StateMachineEntity(type) {}

    DummyEntity(Type type, model::EntityID_t restoredId)
        : StateMachineEntity(type, restoredId) {}

    void accept(model::IModelVisitor* visitor) override {
        Q_UNUSED(visitor);
    }

    void exposeRegisterChild(const QSharedPointer<model::StateMachineEntity>& child) {
        registerNewChild(child);
    }

    void exposeUnregisterChild(const QSharedPointer<model::StateMachineEntity>& child) {
        unregisterChild(child);
    }

    void exposeSetId(model::EntityID_t id) {
        setId(id);
    }
};

class StateMachineEntityTest : public QObject {
    Q_OBJECT

private slots:
    void MetadataAndGeometryAccessors();
    void ChildSignalsPropagate();
    void ChildSignalsDontPropagateAfterUnregister();
    void ConstructorWithRestoredIdAssignsCorrectly();
    void SetIdChangesEntityId();
    void RenamingStateDoesNotChangeId();
    void ReparentingPreservesId();
};

/**
 * @brief Verify entity metadata helpers for position/size and custom keys.
 *
 * Use-case: UI stores geometry metadata and restores it after reload.
 */
void StateMachineEntityTest::MetadataAndGeometryAccessors() {
    auto entity = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);

    entity->setPos(QPointF(10.5, -3.0));
    entity->setSize(QSizeF(88.0, 11.0));
    entity->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, QString("poly"));

    QCOMPARE(QPointF(10.5, -3.0), entity->getPos());
    QCOMPARE(QSizeF(88.0, 11.0), entity->getSize());
    QCOMPARE(QString("poly"), entity->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).toString());
}

/**
 * @brief Verify child-added signal propagation through parent registration.
 *
 * Use-case: Model emits notifications when nested entities are attached.
 */
void StateMachineEntityTest::ChildSignalsPropagate() {
    auto parent = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);
    auto child = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);

    QSignalSpy spy(parent.get(), &model::StateMachineEntity::childAdded);

    parent->exposeRegisterChild(child);

    QCOMPARE(1, spy.count());
}

/**
 * @brief Verify child signals stop propagation after unregister.
 *
 * Use-case: Entity moves between parents should not keep stale signal links.
 */
void StateMachineEntityTest::ChildSignalsDontPropagateAfterUnregister() {
    auto parent = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);
    auto child = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);
    auto nested = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);

    QSignalSpy spy(parent.get(), &model::StateMachineEntity::childAdded);

    parent->exposeRegisterChild(child);
    QCOMPARE(1, spy.count());

    parent->exposeUnregisterChild(child);
    child->exposeRegisterChild(nested);
    QCOMPARE(1, spy.count());
}

/**
 * @brief Verify constructor with restored ID assigns the given value.
 *
 * Use-case: Deserialization restores entity with a persisted UID.
 */
void StateMachineEntityTest::ConstructorWithRestoredIdAssignsCorrectly() {
    const model::EntityID_t restoredId = 42;
    auto entity = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State, restoredId);

    QCOMPARE(restoredId, entity->id());
}

/**
 * @brief Verify setId changes the entity's ID to the requested value.
 *
 * Use-case: Factory or serializer reassigns UID after creation.
 */
void StateMachineEntityTest::SetIdChangesEntityId() {
    auto entity = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);
    const model::EntityID_t originalId = entity->id();
    const model::EntityID_t newId = 99;

    entity->exposeSetId(newId);

    QVERIFY(newId != originalId);
    QCOMPARE(newId, entity->id());
}

/**
 * @brief Verify renaming a state does not change its UID.
 *
 * Use-case: User renames state in the editor; UID must stay stable for metadata references.
 */
void StateMachineEntityTest::RenamingStateDoesNotChangeId() {
    model::EntityIdGenerator gen;
    auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                     .dynamicCast<model::RegularState>();
    state->setName("OriginalName");
    const model::EntityID_t idBefore = state->id();

    state->setName("NewName");

    QCOMPARE(idBefore, state->id());
}

/**
 * @brief Verify reparenting a child state preserves its UID.
 *
 * Use-case: User moves a child state to a different parent; UID must stay stable.
 */
void StateMachineEntityTest::ReparentingPreservesId() {
    model::EntityIdGenerator gen;
    auto parentA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                       .dynamicCast<model::RegularState>();
    parentA->setName("ParentA");
    auto parentB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                       .dynamicCast<model::RegularState>();
    parentB->setName("ParentB");
    auto child = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                     .dynamicCast<model::RegularState>();
    child->setName("Child");

    parentA->addChildState(child);
    const model::EntityID_t idBefore = child->id();

    parentA->deleteDirectChild(child);
    parentB->addChildState(child);

    QCOMPARE(idBefore, child->id());
}

int runStateMachineEntityTest(int argc, char** argv) {
    StateMachineEntityTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateMachineEntityTest.moc"
