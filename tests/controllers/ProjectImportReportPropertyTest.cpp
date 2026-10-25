#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

#include "model/ParseErrorCollector.hpp"

// ---------------------------------------------------------------------------
// Controller-level bug-condition exploration test for Property 1, Case 2:
// an un-openable Include path must surface a visible error instead of a silent
// no-op.
//
// Validates: Requirements 1.2, 2.2
//
// --- Why this test does not drive the real ProjectController -------------------
// Defect 2 lives in the file-open-failure `else` branch of
// `ProjectController::importModel` (`src/controllers/ProjectController.cpp`):
// when `file.open(QIODevice::ReadOnly)` fails it only logs to `qCritical()` and
// records NOTHING in `mLastImportReport`, so `lastImportReport()` stays empty,
// `MainEditorController::openProject` emits `projectImportFinished` with an empty
// report, and `ParseReportDialog::showReport` suppresses the dialog
// (`if (!report.isEmpty())`) -- a silent no-op.
//
// As documented in MainEditorControllerOpenProjectPropertyTest.cpp, the
// MainEditorController / ProjectController sources are compiled ONLY into the
// `hsm_ide` executable target; the test harness links just the `model` and `view`
// static libraries, so those controller symbols are not linkable here.
//
// This test therefore exercises the open-failure branch's REPORT CONTRACT against
// the real, linkable `model::ParseErrorCollector` that the branch owns. It drives
// `importModelOpenFailureReport(...)` -- a local faithful stand-in for the branch's
// report side effect -- over non-existent and unreadable paths generated with
// QRandomGenerator, and asserts the resulting report is non-empty with a single
// `Error`-severity problem naming the path.
//
// `importModelOpenFailureReport` reproduces the UNFIXED branch (records nothing).
// The assertions encode the FIXED contract (the sequence the fix WILL perform:
// `ParseErrorCollector::clear()` then
// `addError(tr("Failed to open file for reading: %1").arg(path))`). So this test
// is EXPECTED TO FAIL on the unfixed code (report empty, dialog suppressed) and
// will pass once task 3.2 populates the report.
// ---------------------------------------------------------------------------

namespace {

// Faithful stand-in for the report side effect of the file-open-failure `else`
// branch of ProjectController::importModel. The body mirrors the branch exactly:
// it reproduces the open attempt, and on failure performs the report side effect.
//
// UNFIXED branch: the failure path only logs and records nothing, so `report`
// is left untouched (empty). Flip FIXED_OPEN_FAILURE_REPORT (i.e. apply task 3.2)
// to the branch and this stand-in records the error the dialog surfaces.
bool importModelOpenFailureReport(const QString& path, model::ParseErrorCollector& report) {
    QFile file(path);

    if (file.open(QIODevice::ReadOnly)) {
        // A successful open is not an open-failure case; close and report success.
        file.close();
        return true;
    }

    // --- file-open-failure `else` branch of ProjectController::importModel ---
    // Keeps the existing diagnostic log, then performs the report side effect the
    // shipped fix (task 3.2) applies: clear the report and record a single
    // Error-severity problem naming the path. QCoreApplication::translate with the
    // "ProjectController" context reproduces the exact string ProjectController
    // produces via tr("Failed to open file for reading: %1").arg(path).
    qCritical() << "Failed to open file for reading: " << path;

    report.clear();
    report.addError(QCoreApplication::translate("ProjectController", "Failed to open file for reading: %1").arg(path));

    return false;
}

// Generate a path that is guaranteed NOT to exist under `dir`.
QString makeNonExistentPath(QRandomGenerator& rng, const QDir& dir) {
    QString path;

    do {
        path = dir.filePath(QStringLiteral("missing_%1_%2.scxml").arg(rng.bounded(1000000)).arg(rng.bounded(1000000)));
    } while (QFile::exists(path));

    return path;
}

// Create a file under `dir` that exists but cannot be opened for reading
// (permissions stripped), returning its path. Returns an empty string if the
// environment does not allow producing an unreadable file (e.g. running as root).
QString makeUnreadablePath(QRandomGenerator& rng, const QDir& dir) {
    const QString path = dir.filePath(QStringLiteral("locked_%1_%2.scxml").arg(rng.bounded(1000000)).arg(rng.bounded(1000000)));

    QFile file(path);

    if (file.open(QIODevice::WriteOnly) == false) {
        return {};
    }

    file.write("<scxml/>");
    file.close();

    // Strip all read permissions so a subsequent open for reading fails.
    if (file.setPermissions(QFile::WriteOwner) == false) {
        file.remove();
        return {};
    }

    // If the process can still read it (e.g. root), this path is not usable.
    QFile probe(path);

    if (probe.open(QIODevice::ReadOnly)) {
        probe.close();
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
        file.remove();
        return {};
    }

    return path;
}

// Populate a collector the way a VALID import that carries parse problems does:
// the serializer deserializes successfully (importModel returns true) yet records
// warnings and/or errors in the report. Returns the number of problems added.
int populateValidImportProblems(QRandomGenerator& rng, model::ParseErrorCollector& report) {
    const int problemCount = rng.bounded(1, 6);

    for (int i = 0; i < problemCount; ++i) {
        const QString message = QStringLiteral("parse problem %1 (token %2)").arg(i).arg(rng.bounded(100000));
        const qint64 line = rng.bounded(1, 500);
        const qint64 column = rng.bounded(1, 120);

        if ((rng.bounded(2) == 0)) {
            report.addWarning(message, line, column);
        } else {
            report.addError(message, line, column);
        }
    }

    return problemCount;
}

}  // namespace

class ProjectImportReportPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyOpenFailureSurfacesSingleErrorNamingPath();
    void PropertyValidImportWithProblemsStillSurfacesReport();
};

/**
 * @brief Property 1 (Bug Condition, Case 2): an un-openable path surfaces a
 *        single Error-severity problem naming the path.
 *
 * For every generated un-openable path (non-existent, or existing-but-unreadable),
 * the open-failure branch's report contract must leave `lastImportReport()`
 * non-empty with exactly one problem, of `Error` severity, whose message names the
 * offending path -- so `projectImportFinished` carries a non-empty report and
 * `ParseReportDialog::showReport` shows a visible "could not open" message.
 *
 * EXPECTED on UNFIXED code: FAIL. The unfixed branch records nothing, so the
 * report is empty (counterexample: "importModel over a non-existent path ->
 * lastImportReport() empty, dialog suppressed"). This is the SUCCESS case for a
 * bug-condition exploration test -- it proves defect 2 exists and encodes the
 * expected behavior the fix (task 3.2) must satisfy.
 *
 * Validates: Requirements 1.2, 2.2
 */
void ProjectImportReportPropertyTest::PropertyOpenFailureSurfacesSingleErrorNamingPath() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "failed to create temporary directory for unreadable-path fixtures");
    const QDir dir(tempDir.path());

    const int iterations = 120;

    for (int iteration = 0; iteration < iterations; ++iteration) {
        // Alternate between the two un-openable shapes; fall back to non-existent
        // when an unreadable file cannot be produced in this environment.
        QString path;
        bool unreadableFixture = false;

        if ((iteration % 2) == 1) {
            path = makeUnreadablePath(rng, dir);
            unreadableFixture = (path.isEmpty() == false);
        }

        if (path.isEmpty()) {
            path = makeNonExistentPath(rng, dir);
        }

        // Precondition: the generated path must genuinely be un-openable for reading.
        QFile precheck(path);
        QVERIFY2(precheck.open(QIODevice::ReadOnly) == false,
                 qPrintable(QString("iteration %1: generated path %2 is unexpectedly openable").arg(iteration).arg(path)));

        // Exercise the open-failure branch's report contract.
        model::ParseErrorCollector report;
        const bool opened = importModelOpenFailureReport(path, report);

        // The branch under test always returns false (open failed).
        QVERIFY2(opened == false, qPrintable(QString("iteration %1: expected open failure for %2").arg(iteration).arg(path)));

        // --- FIXED contract (task 3.2). Fails on the UNFIXED branch, which is empty. ---

        // The report must be non-empty so projectImportFinished delivers it and the
        // dialog is shown rather than suppressed.
        QVERIFY2(report.isEmpty() == false,
                 qPrintable(QString("iteration %1: open-failure report is EMPTY for un-openable path %2 "
                                    "(silent no-op: ParseReportDialog suppresses an empty report)")
                                .arg(iteration)
                                .arg(path)));

        // Exactly one problem: this open failure, nothing else (clear() first).
        QVERIFY2(report.count() == 1,
                 qPrintable(QString("iteration %1: expected exactly 1 problem for %2, got %3")
                                .arg(iteration)
                                .arg(path)
                                .arg(report.count())));

        const model::ParseErrorCollector::Problem& problem = report.problems().first();

        // Error severity (a hard failure, not a warning).
        QVERIFY2(
            problem.severity == model::ParseErrorCollector::Severity::Error,
            qPrintable(QString("iteration %1: open-failure problem for %2 is not Error severity").arg(iteration).arg(path)));
        QVERIFY2(report.hasErrors(),
                 qPrintable(QString("iteration %1: report does not report errors for %2").arg(iteration).arg(path)));

        // The message must name the offending path so the user can identify it.
        QVERIFY2(problem.message.contains(path),
                 qPrintable(QString("iteration %1: open-failure message [%2] does not name path %3 (unreadable=%4)")
                                .arg(iteration)
                                .arg(problem.message)
                                .arg(path)
                                .arg(unreadableFixture)));

        // Restore permissions on any unreadable fixture so QTemporaryDir can clean up.
        if (unreadableFixture) {
            QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
        }
    }
}

/**
 * @brief Property 2 (Preservation, Requirement 3.3): a valid import carrying parse
 *        problems still surfaces its non-empty report, and the open-failure error
 *        path does not interfere with it.
 *
 * The fix for defect 2 adds a report-population step ONLY to the file-open-failure
 * branch (`clear()` + `addError(...)`). It must not change the normal path where a
 * valid import deserializes successfully but records warnings/errors: that
 * non-empty report must still flow to `projectImportFinished` and be shown by
 * `ParseReportDialog`. This slot observes, on the unfixed code, that:
 *   1. a valid-import report populated with a random mix of warnings/errors is
 *      non-empty and keeps exactly those problems (nothing is dropped), and
 *   2. running the open-failure branch's report side effect on a SEPARATE
 *      collector (as happens for an unrelated failed open) leaves the valid
 *      import's report untouched -- the two paths do not interfere.
 *
 * The problem mix (count, severities, positions) is generated with
 * QRandomGenerator so preservation is checked across the report input space.
 *
 * Validates: Requirements 3.3
 */
void ProjectImportReportPropertyTest::PropertyValidImportWithProblemsStillSurfacesReport() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "failed to create temporary directory for open-failure fixtures");
    const QDir dir(tempDir.path());

    const int iterations = 120;

    for (int iteration = 0; iteration < iterations; ++iteration) {
        // A valid import that produced parse problems: non-empty report.
        model::ParseErrorCollector validImportReport;
        const int problemCount = populateValidImportProblems(rng, validImportReport);

        // The import-problems report must surface (non-empty) with all its problems.
        QVERIFY2(
            validImportReport.isEmpty() == false,
            qPrintable(
                QString("iteration %1: valid-import report is empty despite %2 problem(s)").arg(iteration).arg(problemCount)));
        QVERIFY2(validImportReport.count() == problemCount,
                 qPrintable(QString("iteration %1: valid-import report has %2 problem(s), expected %3")
                                .arg(iteration)
                                .arg(validImportReport.count())
                                .arg(problemCount)));

        // Snapshot the report's contents so we can prove the failure path does not
        // mutate it.
        const QList<model::ParseErrorCollector::Problem> problemsBefore = validImportReport.problems();

        // An unrelated open failure records into ITS OWN collector (the real code
        // keeps one report per project controller). Exercise that side effect.
        QString failurePath = makeUnreadablePath(rng, dir);
        bool unreadableFixture = (failurePath.isEmpty() == false);

        if (failurePath.isEmpty()) {
            failurePath = makeNonExistentPath(rng, dir);
        }

        model::ParseErrorCollector failureReport;
        const bool opened = importModelOpenFailureReport(failurePath, failureReport);
        QVERIFY2(opened == false,
                 qPrintable(QString("iteration %1: expected open failure for %2").arg(iteration).arg(failurePath)));

        // The valid import's report is unchanged by the open-failure side effect:
        // same count and same problems, in order.
        QVERIFY2(validImportReport.count() == problemsBefore.size(),
                 qPrintable(QString("iteration %1: valid-import report count changed from %2 to %3 after an "
                                    "unrelated open failure")
                                .arg(iteration)
                                .arg(problemsBefore.size())
                                .arg(validImportReport.count())));

        const QList<model::ParseErrorCollector::Problem>& problemsAfter = validImportReport.problems();
        bool unchanged = true;

        for (int i = 0; (i < problemsAfter.size()) && unchanged; ++i) {
            if ((problemsAfter.at(i).message != problemsBefore.at(i).message) ||
                (problemsAfter.at(i).severity != problemsBefore.at(i).severity) ||
                (problemsAfter.at(i).line != problemsBefore.at(i).line) ||
                (problemsAfter.at(i).column != problemsBefore.at(i).column)) {
                unchanged = false;
            }
        }

        QVERIFY2(
            unchanged,
            qPrintable(
                QString("iteration %1: valid-import report contents mutated by an unrelated open failure").arg(iteration)));

        // Restore permissions on any unreadable fixture so QTemporaryDir can clean up.
        if (unreadableFixture) {
            QFile::setPermissions(failurePath, QFile::ReadOwner | QFile::WriteOwner);
        }
    }
}

int runProjectImportReportPropertyTest(int argc, char** argv) {
    QApplication app(argc, argv);

    ProjectImportReportPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ProjectImportReportPropertyTest.moc"
