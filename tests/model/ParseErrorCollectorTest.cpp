#include <QtTest>

#include "model/ParseErrorCollector.hpp"

class ParseErrorCollectorTest : public QObject {
    Q_OBJECT

private slots:
    void EmptyByDefault();
    void PreservesOrder();
    void RetainsFields();
    void NoLocationUsesSentinel();
    void HasErrorsReflectsSeverity();
    void ClearEmptiesCollector();
};

/**
 * @brief A freshly constructed collector reports empty with no problems.
 *
 * Use-case: A clean import must start with an empty report.
 */
void ParseErrorCollectorTest::EmptyByDefault() {
    model::ParseErrorCollector collector;

    QVERIFY(collector.isEmpty());
    QVERIFY(!collector.hasErrors());
    QCOMPARE(0, collector.count());
    QVERIFY(collector.problems().isEmpty());
}

/**
 * @brief Problems are retained in insertion order.
 *
 * Use-case: Users expect the dialog to list problems in the order they were detected.
 */
void ParseErrorCollectorTest::PreservesOrder() {
    model::ParseErrorCollector collector;
    collector.addWarning("first", 1, 2);
    collector.addError("second", 3, 4);
    collector.addWarning("third");

    QCOMPARE(3, collector.count());
    QCOMPARE(QString("first"), collector.problems().at(0).message);
    QCOMPARE(QString("second"), collector.problems().at(1).message);
    QCOMPARE(QString("third"), collector.problems().at(2).message);
}

/**
 * @brief Each recorded problem retains message, severity and location.
 *
 * Use-case: The dialog needs the full problem detail per entry.
 */
void ParseErrorCollectorTest::RetainsFields() {
    model::ParseErrorCollector collector;
    collector.addError("boom", 10, 5);

    const model::ParseErrorCollector::Problem& problem = collector.problems().at(0);
    QCOMPARE(QString("boom"), problem.message);
    QCOMPARE(model::ParseErrorCollector::Severity::Error, problem.severity);
    QCOMPARE(static_cast<qint64>(10), problem.line);
    QCOMPARE(static_cast<qint64>(5), problem.column);
}

/**
 * @brief A problem without a known position stores the -1 sentinel.
 *
 * Use-case: Semantic problems detected after the reader position is lost have no location.
 */
void ParseErrorCollectorTest::NoLocationUsesSentinel() {
    model::ParseErrorCollector collector;
    collector.addWarning("no position");

    const model::ParseErrorCollector::Problem& problem = collector.problems().at(0);
    QCOMPARE(model::ParseErrorCollector::Severity::Warning, problem.severity);
    QCOMPARE(static_cast<qint64>(-1), problem.line);
    QCOMPARE(static_cast<qint64>(-1), problem.column);
}

/**
 * @brief hasErrors is true only when at least one Error-severity problem is present.
 *
 * Use-case: Distinguish fatal errors from warning-only reports.
 */
void ParseErrorCollectorTest::HasErrorsReflectsSeverity() {
    model::ParseErrorCollector warningsOnly;
    warningsOnly.addWarning("w1");
    warningsOnly.addWarning("w2");
    QVERIFY(!warningsOnly.hasErrors());

    model::ParseErrorCollector withError;
    withError.addWarning("w1");
    withError.addError("e1");
    QVERIFY(withError.hasErrors());
}

/**
 * @brief clear() removes all problems and returns the collector to empty.
 *
 * Use-case: The serializer resets the report before each import.
 */
void ParseErrorCollectorTest::ClearEmptiesCollector() {
    model::ParseErrorCollector collector;
    collector.addError("e1", 1, 1);
    collector.addWarning("w1");
    QVERIFY(!collector.isEmpty());

    collector.clear();

    QVERIFY(collector.isEmpty());
    QVERIFY(!collector.hasErrors());
    QCOMPARE(0, collector.count());
}

int runParseErrorCollectorTest(int argc, char** argv) {
    ParseErrorCollectorTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ParseErrorCollectorTest.moc"
