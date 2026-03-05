#include <QtTest>

#include <QSignalSpy>

#include "model/StateMachineEntity.hpp"

class DummyEntity : public model::StateMachineEntity {
public:
    explicit DummyEntity(Type type)
        : StateMachineEntity(type) {}

    void accept(model::IModelVisitor* visitor) override {
        Q_UNUSED(visitor);
    }

    void exposeRegisterChild(const QSharedPointer<model::StateMachineEntity>& child) {
        registerNewChild(child);
    }
};

class StateMachineEntityTest : public QObject {
    Q_OBJECT

private slots:
    void MetadataAndGeometryAccessors();
    void ChildSignalsPropagate();
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

int runStateMachineEntityTest(int argc, char** argv) {
    StateMachineEntityTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateMachineEntityTest.moc"
