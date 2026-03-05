#include <QtTest>

#include "model/HistoryState.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

class HistoryStateTest : public QObject {
    Q_OBJECT

private slots:
    void DefaultTransitionAndProperties();
};

/**
 * @brief Verify history state stores a default transition and history type property.
 *
 * Use-case: SCXML history node has default fallback transition and configurable depth.
 */
void HistoryStateTest::DefaultTransitionAndProperties() {
    auto source = QSharedPointer<model::RegularState>::create("S");
    auto target = QSharedPointer<model::RegularState>::create("T");
    auto history = QSharedPointer<model::HistoryState>::create("H", model::HistoryType::SHALLOW);
    auto tr = QSharedPointer<model::Transition>::create(source, target, "resume");

    QVERIFY(history->addChild(tr));
    QCOMPARE(tr, history->defaultTransition());

    QVERIFY(history->setProperty(model::HistoryState::cKeyHistoryType, static_cast<int>(model::HistoryType::DEEP)));
    QCOMPARE(model::HistoryType::DEEP, history->historyType());

    history->deleteChild(tr->id());
    QCOMPARE(QSharedPointer<model::Transition>(), history->defaultTransition());
}

int runHistoryStateTest(int argc, char** argv) {
    HistoryStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HistoryStateTest.moc"
