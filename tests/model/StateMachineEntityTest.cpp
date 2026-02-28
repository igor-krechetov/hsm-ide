#include <gtest/gtest.h>

#include <QSignalSpy>

#include "model/StateMachineEntity.hpp"

namespace {

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

/**
 * @brief Verify entity metadata helpers for position/size and custom keys.
 *
 * Use-case: UI stores geometry metadata and restores it after reload.
 *
 */
TEST(StateMachineEntityTest, MetadataAndGeometryAccessors) {
    auto entity = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);

    entity->setPos(QPointF(10.5, -3.0));
    entity->setSize(QSizeF(88.0, 11.0));
    entity->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, QString("poly"));

    EXPECT_EQ(QPointF(10.5, -3.0), entity->getPos());
    EXPECT_EQ(QSizeF(88.0, 11.0), entity->getSize());
    EXPECT_EQ(QString("poly"), entity->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).toString());
}

/**
 * @brief Verify child-added signal propagation through parent registration.
 *
 * Use-case: Model emits notifications when nested entities are attached.
 *
 */
TEST(StateMachineEntityTest, ChildSignalsPropagate) {
    auto parent = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);
    auto child = QSharedPointer<DummyEntity>::create(model::StateMachineEntity::Type::State);

    QSignalSpy spy(parent.get(), &model::StateMachineEntity::childAdded);

    parent->exposeRegisterChild(child);

    EXPECT_EQ(1, spy.count());
}

}  // namespace
