#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/elements/IncludeEntity.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

class IncludeEntityTest : public QObject {
    Q_OBJECT

private slots:
    void PathAndAllowedChildren();
};

/**
 * @brief Verify include entity accepts transitions only and exposes path.
 *
 * Use-case: External SCXML subchart include keeps transitions and reference path.
 */
void IncludeEntityTest::PathAndAllowedChildren() {
    model::EntityIdGenerator gen;
    auto include = model::ModelElementsFactory::createUniqueState(model::StateType::INCLUDE, gen)
                       .dynamicCast<model::IncludeEntity>();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                  .dynamicCast<model::RegularState>();
    s2->setName("S2");
    auto tr = model::ModelElementsFactory::createTransitionWithId(s1, s2, "go", gen.generateNextId());

    include->setPath("sub.scxml");
    QCOMPARE(QString("sub.scxml"), include->path());

    QVERIFY(include->addChild(tr));
    QVERIFY(!include->addChild(s2));
    QCOMPARE(tr, include->findTransition(tr->id()));
}

int runIncludeEntityTest(int argc, char** argv) {
    IncludeEntityTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "IncludeEntityTest.moc"
