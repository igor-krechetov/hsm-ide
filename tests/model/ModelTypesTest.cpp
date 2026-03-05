#include <QtTest>

#include "model/ModelTypes.hpp"

class ModelTypesTest : public QObject {
    Q_OBJECT

private slots:
    void TransitionTypeConversions();
    void HistoryTypeConversions();
};

/**
 * @brief Verify transition type conversion helpers round-trip supported values.
 *
 * Use-case: Persist transition type in model properties and restore from integer values.
 */
void ModelTypesTest::TransitionTypeConversions() {
    QCOMPARE(model::TransitionType::EXTERNAL, model::transitionTypeFromInt(static_cast<int>(model::TransitionType::EXTERNAL)));
    QCOMPARE(model::TransitionType::INTERNAL, model::transitionTypeFromInt(static_cast<int>(model::TransitionType::INTERNAL)));
    QCOMPARE(model::TransitionType::INVALID, model::transitionTypeFromInt(777));

    QCOMPARE(QString("External"), model::transitionTypeToString(model::TransitionType::EXTERNAL));
    QCOMPARE(QString("Internal"), model::transitionTypeToString(model::TransitionType::INTERNAL));
    QCOMPARE(QString(""), model::transitionTypeToString(model::TransitionType::INVALID));
}

/**
 * @brief Verify history type conversion helpers round-trip supported values.
 *
 * Use-case: Persist history type in model properties and restore from integer values.
 */
void ModelTypesTest::HistoryTypeConversions() {
    QCOMPARE(model::HistoryType::SHALLOW, model::historyTypeFromInt(static_cast<int>(model::HistoryType::SHALLOW)));
    QCOMPARE(model::HistoryType::DEEP, model::historyTypeFromInt(static_cast<int>(model::HistoryType::DEEP)));
    QCOMPARE(model::HistoryType::INVALID, model::historyTypeFromInt(-8));

    QCOMPARE(QString("Shallow"), model::historyTypeToString(model::HistoryType::SHALLOW));
    QCOMPARE(QString("Deep"), model::historyTypeToString(model::HistoryType::DEEP));
    QCOMPARE(QString(""), model::historyTypeToString(model::HistoryType::INVALID));
}

int runModelTypesTest(int argc, char** argv) {
    ModelTypesTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ModelTypesTest.moc"
