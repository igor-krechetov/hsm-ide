#include <QtTest>

#include "model/InitialState.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

class InitialStateTest : public QObject {
    Q_OBJECT

private slots:
    void StoresSingleTransition();
};

/**
 * @brief Verify initial state keeps a single outgoing transition.
 *
 * Use-case: Initial pseudostate in SCXML points to startup target.
 */
void InitialStateTest::StoresSingleTransition() {
    auto root = QSharedPointer<model::RegularState>::create("Root");
    auto target = QSharedPointer<model::RegularState>::create("S1");
    auto initial = QSharedPointer<model::InitialState>::create();
    auto tr = QSharedPointer<model::Transition>::create(root, target, "");

    QVERIFY(initial->addChild(tr));
    QCOMPARE(tr, initial->transition());

    initial->deleteDirectChild(tr);
    QCOMPARE(QSharedPointer<model::Transition>(), initial->transition());
    QVERIFY(initial->properties().isEmpty());
}

int runInitialStateTest(int argc, char** argv) {
    InitialStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "InitialStateTest.moc"
