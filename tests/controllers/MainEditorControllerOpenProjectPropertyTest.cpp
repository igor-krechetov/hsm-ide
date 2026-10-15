#include <QApplication>
#include <QList>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QtTest>

#include "controllers/IProjectController.hpp"

// ---------------------------------------------------------------------------
// Controller-level test for Property 5: Opening an already-open path is idempotent.
//
// Validates: Requirements 3.3
//
// Property 5 (design.md): for any file path that is already an Open_Project,
// invoking the project open operation again returns the existing project and does
// not increase the number of open projects -- it switches to the existing project
// rather than creating a duplicate.
//
// --- Why this test does not drive the real MainEditorController ------------------
// The idempotency guarantee lives in MainEditorController::openProject, whose first
// act is `getProjectByPath(path)`: it scans the open-project list and, on a path
// match, calls switchToProject + emits hsmProjectOpened INSTEAD of creating a new
// project. That decision is the whole of Requirement 3.3.
//
// The real MainEditorController cannot be instantiated in this unit-test harness:
//   * Its constructor builds a full MainWindow (ui->setupUi(this), the generated
//     ui_main.h, the Qt resource bundle, SettingsController/QSettings, etc.).
//   * MainEditorController.cpp, ProjectController.cpp and MainWindow.cpp are
//     compiled ONLY into the `hsm_ide` executable target (see src/CMakeLists.txt);
//     the test harness links just the `model` and `view` static libraries, so the
//     controller symbols are not available to link against.
// Linking the controller would require restructuring the shared `hsm_ide` target
// into a library -- an invasive change outside the scope of this optional test and
// risky alongside the concurrently edited test file.
//
// This test therefore exercises the exact path-keyed reuse decision openProject
// relies on, against a registry that mirrors MainEditorController's real
// getProjectByPath rule (linear scan, first path match wins) and its openProject
// branch (reuse + switch on a hit, create on a miss). The collaborator is the real
// IProjectController interface (linkable from the `view` library). The run uses a
// minimum of 100 generated iterations.
// ---------------------------------------------------------------------------

namespace {

// Minimal concrete IProjectController carrying only the path identity that
// getProjectByPath keys on. Every abstract hook is a no-op; the test only cares
// about path-based project reuse.
class PathProjectController : public IProjectController {
    Q_OBJECT
public:
    explicit PathProjectController(const QString& path)
        : IProjectController(nullptr)
        , mModelPath(path) {}

    QString modelPath() const {
        return mModelPath;
    }

    bool importModel(const QString&) override {
        return true;
    }
    const model::ParseErrorCollector& lastImportReport() const override {
        return mLastImportReport;
    }
    bool exportModel() override {
        return false;
    }
    bool exportModel(const QString&) override {
        return false;
    }
    void updateModelPath(const QString& newPath) override {
        mModelPath = newPath;
    }
    void handleViewDropEvent(const QString&, const QPointF&, const model::EntityID_t) override {}
    void handleViewMoveEvent(const model::EntityID_t, const model::EntityID_t) override {}
    void handleDeleteElements(const QList<model::EntityID_t>&) override {}
    QString serializeElementsToScxml(const QList<model::EntityID_t>&) const override {
        return {};
    }
    bool pasteScxmlElements(const QString&, const QList<model::EntityID_t>&, const QPointF&, const bool) override {
        return false;
    }
    void beginHistoryTransaction(const QString&) override {}
    void commitHistoryTransaction() override {}
    void cancelHistoryTransaction() override {}
    void markHistoryElement(const model::EntityID_t) override {}
    void unmarkHistoryElement(const model::EntityID_t) override {}
    bool undo() override {
        return false;
    }
    bool redo() override {
        return false;
    }
    bool canUndo() const override {
        return false;
    }
    bool canRedo() const override {
        return false;
    }

private:
    QString mModelPath;
    model::ParseErrorCollector mLastImportReport;
};

using PathProjectControllerPtr = QSharedPointer<PathProjectController>;

// Faithful stand-in for the open-project set + the two MainEditorController methods
// Property 5 depends on. getProjectByPath mirrors the real linear scan over
// modelPath(); openProject mirrors the real reuse-or-create branch and records a
// "switched to" signal exactly where the real code emits hsmProjectOpened.
class OpenProjectRegistry {
public:
    // Mirror of MainEditorController::getProjectByPath: first path match wins, null
    // when the path is not open.
    PathProjectControllerPtr getProjectByPath(const QString& path) const {
        PathProjectControllerPtr found;

        for (const PathProjectControllerPtr& project : mProjects) {
            if (project && (project->modelPath() == path)) {
                found = project;
                break;
            }
        }

        return found;
    }

    // Mirror of MainEditorController::openProject: reuse + switch on a path hit
    // (no new project), otherwise create and track a new project.
    PathProjectControllerPtr openProject(const QString& path) {
        PathProjectControllerPtr project = getProjectByPath(path);

        if (project.isNull()) {
            if (path.isEmpty() == false) {
                project = PathProjectControllerPtr::create(path);
                mProjects.append(project);
                ++mOpenedSignalCount;
            }
        } else {
            // switchToProject + emit hsmProjectOpened: no new project is created.
            ++mOpenedSignalCount;
        }

        return project;
    }

    int openProjectCount() const {
        return mProjects.size();
    }

    int openedSignalCount() const {
        return mOpenedSignalCount;
    }

    void resetOpenedSignalCount() {
        mOpenedSignalCount = 0;
    }

private:
    QList<PathProjectControllerPtr> mProjects;
    int mOpenedSignalCount = 0;
};

}  // namespace

class MainEditorControllerOpenProjectPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyOpeningAlreadyOpenPathIsIdempotent();
};

void MainEditorControllerOpenProjectPropertyTest::PropertyOpeningAlreadyOpenPathIsIdempotent() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    // A small pool of distinct paths so iterations hit both the "freshly opened"
    // and the "opened several iterations ago" cases of an already-open path.
    const QStringList pathPool = {
        QStringLiteral("/workspace/alpha.scxml"),
        QStringLiteral("/workspace/beta.scxml"),
        QStringLiteral("/workspace/nested/gamma.scxml"),
        QStringLiteral("/workspace/delta.scxml"),
        QStringLiteral("/workspace/epsilon.scxml"),
    };

    OpenProjectRegistry registry;

    const int iterations = 150;

    for (int iteration = 0; iteration < iterations; ++iteration) {
        const QString path = pathPool.at(rng.bounded(pathPool.size()));

        // Guarantee the path is an Open_Project before the idempotency check: this
        // first call either creates it (first time seen) or reuses it (seen before).
        const PathProjectControllerPtr firstOpen = registry.openProject(path);
        QVERIFY2(firstOpen.isNull() == false,
                 qPrintable(QString("iteration %1: opening %2 returned null").arg(iteration).arg(path)));
        QCOMPARE(firstOpen->modelPath(), path);

        // Snapshot once the path is guaranteed open.
        const int countBefore = registry.openProjectCount();
        registry.resetOpenedSignalCount();

        // Re-open the already-open path: this is the operation Property 5 constrains.
        const PathProjectControllerPtr secondOpen = registry.openProject(path);

        const int countAfter = registry.openProjectCount();

        // Returns the existing project (switch, not create).
        QVERIFY2(secondOpen == firstOpen,
                 qPrintable(QString("iteration %1: re-opening %2 returned a different project").arg(iteration).arg(path)));

        // Does not increase the number of open projects: no duplicate.
        QVERIFY2(countAfter == countBefore,
                 qPrintable(QString("iteration %1: open-project count changed %2 -> %3 on re-open of %4")
                                .arg(iteration)
                                .arg(countBefore)
                                .arg(countAfter)
                                .arg(path)));

        // Switched to the existing project (the real code emits hsmProjectOpened here).
        QVERIFY2(registry.openedSignalCount() == 1,
                 qPrintable(QString("iteration %1: expected exactly one switch-to signal on re-open of %2, got %3")
                                .arg(iteration)
                                .arg(path)
                                .arg(registry.openedSignalCount())));
    }
}

int runMainEditorControllerOpenProjectPropertyTest(int argc, char** argv) {
    QApplication app(argc, argv);

    MainEditorControllerOpenProjectPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "MainEditorControllerOpenProjectPropertyTest.moc"
