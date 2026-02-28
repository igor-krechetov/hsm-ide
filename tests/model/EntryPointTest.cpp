#include <QtTest>

#include "model/EntryPoint.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

class EntryPointTest : public QObject {
    Q_OBJECT

private slots:
    void ManagesOutgoingTransitions();
};

/**
 * @brief Verify entry point supports multiple transitions and lookup APIs.
 *
 * Use-case: Entry point dispatches to different targets based on event/guards.
 */
void EntryPointTest::ManagesOutgoingTransitions() {
    auto source = QSharedPointer<model::RegularState>::create("Src");
    auto targetA = QSharedPointer<model::RegularState>::create("A");
    auto targetB = QSharedPointer<model::RegularState>::create("B");
    auto entry = QSharedPointer<model::EntryPoint>::create("EP");

    auto t1 = QSharedPointer<model::Transition>::create(source, targetA, "E1");
    auto t2 = QSharedPointer<model::Transition>::create(source, targetB, "E2");

    QVERIFY(entry->addChild(t1));
    QVERIFY(entry->addChild(t2));
    QCOMPARE(2, entry->transitions().size());
    QCOMPARE(entry, entry->findParentState(t1->id()));
    QCOMPARE(t2, entry->findChild(t2->id()).dynamicCast<model::Transition>());

    entry->deleteChild(t1->id());
    QCOMPARE(1, entry->transitions().size());
}

int runEntryPointTest(int argc, char** argv) {
    EntryPointTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "EntryPointTest.moc"
