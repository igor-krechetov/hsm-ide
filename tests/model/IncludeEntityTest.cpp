#include <QtTest>

#include "model/IncludeEntity.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

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
    auto include = QSharedPointer<model::IncludeEntity>::create("Inc");
    auto s1 = QSharedPointer<model::RegularState>::create("S1");
    auto s2 = QSharedPointer<model::RegularState>::create("S2");
    auto tr = QSharedPointer<model::Transition>::create(s1, s2, "go");

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
