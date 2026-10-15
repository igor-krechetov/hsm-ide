#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QModelIndex>
#include <QRandomGenerator>
#include <QSet>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

#include "view/widgets/WorkspaceView.hpp"

// ---------------------------------------------------------------------------
// Property-based test for Property 1: Target folder resolution.
//
// WorkspaceView::resolveDropTargetFolder is a private const member, so the test
// reaches it through the wsview_test_hooks forwarder (friend of WorkspaceView),
// mirroring the hook pattern used for the pure collision helpers in
// WorkspaceViewCollisionPropertyTest.
//
// Property 1 (design.md): for any Workspace View drop index, the resolved target
// folder is
//   - the folder itself when the index is a folder item,
//   - the containing folder when the index is a file item,
//   - the workspace root when the index is invalid,
// and when no workspace is loaded the resolution yields no target (empty string)
// so the drop is ignored.
//
// The run uses a minimum of 100 generated iterations.
//
// Validates: Requirements 1.1, 1.2, 1.3
// ---------------------------------------------------------------------------
class WorkspaceViewDropPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyTargetFolderResolution();
    void PropertyTargetFolderRowKindRule();
    void PropertyNoHighlightWithoutWorkspace();

private:
    static QString randomBaseName(QRandomGenerator& rng);
    static bool createEmptyFile(const QString& path);
    // Resolve a proxy index for an existing filesystem path by going through the
    // source QFileSystemModel and mapping into the proxy (the same path the
    // widget itself uses internally). Spins the event loop until the source index
    // is valid, matching how QFileSystemModel populates asynchronously.
    static QModelIndex proxyIndexForPath(QSortFilterProxyModel* proxy, QFileSystemModel* fsModel, const QString& path);
};

QString WorkspaceViewDropPropertyTest::randomBaseName(QRandomGenerator& rng) {
    static const QString alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
    const int length = static_cast<int>(rng.bounded(3, 10));

    QString result;
    for (int i = 0; i < length; ++i) {
        result.append(alphabet.at(static_cast<int>(rng.bounded(alphabet.size()))));
    }

    return result;
}

bool WorkspaceViewDropPropertyTest::createEmptyFile(const QString& path) {
    QFile file(path);
    bool created = false;
    if (file.open(QIODevice::WriteOnly)) {
        file.close();
        created = true;
    }

    return created;
}

QModelIndex WorkspaceViewDropPropertyTest::proxyIndexForPath(QSortFilterProxyModel* proxy,
                                                             QFileSystemModel* fsModel,
                                                             const QString& path) {
    QModelIndex sourceIndex = fsModel->index(path);

    // QFileSystemModel resolves entries asynchronously; give it a bounded number
    // of event-loop spins to produce a valid source index for the path.
    int spins = 0;
    while ((false == sourceIndex.isValid()) && (spins < 200)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QTest::qWait(5);
        sourceIndex = fsModel->index(path);
        spins += 1;
    }

    QModelIndex result;
    if (sourceIndex.isValid()) {
        result = proxy->mapFromSource(sourceIndex);
    }

    return result;
}

/**
 * @brief Property 1: Target folder resolution.
 *
 * For a freshly populated workspace, resolveDropTargetFolder must map:
 *   - a folder index -> that folder's path,
 *   - a file index   -> the folder that contains the file (the workspace root
 *                        here, since probed files live at the top level),
 *   - an invalid index -> the workspace root,
 * and after the workspace is cleared (no workspace loaded) any index -> empty.
 *
 * Each iteration builds a temp workspace with a random set of top-level folders
 * and top-level .scxml files, loads it into a real WorkspaceView, resolves the
 * probe indices through the source QFileSystemModel, then checks each branch.
 * Top-level entries are used so the FilteredFileSystemProxyModel shows them
 * (top-level folders are always shown; .scxml files are shown) and so file items
 * resolve to the root folder.
 *
 * Validates: Requirements 1.1, 1.2, 1.3
 */
void WorkspaceViewDropPropertyTest::PropertyTargetFolderResolution() {
    // Tag: Feature: drag-and-drop-files, Property 1: Target folder resolution
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        QTemporaryDir tempDir;
        QVERIFY2(tempDir.isValid(), qPrintable(QString("Iteration %1: failed to create temp dir").arg(iteration)));

        // QFileSystemModel resolves symlinks; compare against the canonical root
        // so expectations match what resolveDropTargetFolder returns.
        const QString rootDir = QFileInfo(tempDir.path()).canonicalFilePath();

        // Build a random, de-duplicated set of top-level folders and .scxml files.
        QSet<QString> usedNames;
        QStringList folderPaths;
        QStringList filePaths;

        const int folderCount = static_cast<int>(rng.bounded(1, 5));
        for (int i = 0; i < folderCount; ++i) {
            const QString name = randomBaseName(rng);
            if (usedNames.contains(name)) {
                continue;
            }
            usedNames.insert(name);
            const QString folderPath = QDir(rootDir).filePath(name);
            QVERIFY2(QDir().mkpath(folderPath),
                     qPrintable(QString("Iteration %1: failed to create folder '%2'").arg(iteration).arg(folderPath)));
            // The FilteredFileSystemProxyModel only shows a folder when it (or a
            // descendant) contains a .scxml file, so seed each folder with one so
            // the folder item is actually visible/selectable in the view.
            QVERIFY2(createEmptyFile(QDir(folderPath).filePath("seed.scxml")),
                     qPrintable(QString("Iteration %1: failed to seed folder '%2'").arg(iteration).arg(folderPath)));
            folderPaths.append(QFileInfo(folderPath).canonicalFilePath());
        }

        const int fileCount = static_cast<int>(rng.bounded(1, 5));
        for (int i = 0; i < fileCount; ++i) {
            const QString name = randomBaseName(rng);
            if (usedNames.contains(name)) {
                continue;
            }
            usedNames.insert(name);
            const QString filePath = QDir(rootDir).filePath(name + ".scxml");
            QVERIFY2(createEmptyFile(filePath),
                     qPrintable(QString("Iteration %1: failed to create file '%2'").arg(iteration).arg(filePath)));
            filePaths.append(QFileInfo(filePath).canonicalFilePath());
        }

        WorkspaceView view;
        view.setWorkspaceRoot(rootDir);

        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(view.model());
        QVERIFY2(nullptr != proxy, qPrintable(QString("Iteration %1: view model is not the expected proxy").arg(iteration)));
        QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(proxy->sourceModel());
        QVERIFY2(nullptr != fsModel,
                 qPrintable(QString("Iteration %1: proxy source is not a QFileSystemModel").arg(iteration)));

        // Let the model settle on the root directory before probing entries.
        QTest::qWait(20);

        // --- Case: invalid index -> workspace root (Req 1.1 default area) ---
        {
            const QString resolved = wsview_test_hooks::resolveDropTargetFolder(view, QModelIndex());
            QVERIFY2(QFileInfo(resolved).canonicalFilePath() == rootDir,
                     qPrintable(QString("Iteration %1: invalid index resolved to '%2', expected root '%3'")
                                    .arg(iteration)
                                    .arg(resolved)
                                    .arg(rootDir)));
        }

        // --- Case: folder index -> that folder (Req 1.1) ---
        {
            const QString wantedFolder = folderPaths.at(static_cast<int>(rng.bounded(folderPaths.size())));
            const QModelIndex folderProxyIndex = proxyIndexForPath(proxy, fsModel, wantedFolder);
            QVERIFY2(
                folderProxyIndex.isValid(),
                qPrintable(QString("Iteration %1: could not resolve folder index for '%2'").arg(iteration).arg(wantedFolder)));

            const QString resolved = wsview_test_hooks::resolveDropTargetFolder(view, folderProxyIndex);
            QVERIFY2(QFileInfo(resolved).canonicalFilePath() == wantedFolder,
                     qPrintable(QString("Iteration %1: folder index resolved to '%2', expected '%3'")
                                    .arg(iteration)
                                    .arg(resolved)
                                    .arg(wantedFolder)));
        }

        // --- Case: file index -> containing folder (Req 1.2) ---
        {
            const QString wantedFile = filePaths.at(static_cast<int>(rng.bounded(filePaths.size())));
            const QModelIndex fileProxyIndex = proxyIndexForPath(proxy, fsModel, wantedFile);
            QVERIFY2(fileProxyIndex.isValid(),
                     qPrintable(QString("Iteration %1: could not resolve file index for '%2'").arg(iteration).arg(wantedFile)));

            const QString resolved = wsview_test_hooks::resolveDropTargetFolder(view, fileProxyIndex);
            const QString expectedParent = QFileInfo(wantedFile).absolutePath();
            QVERIFY2(QFileInfo(resolved).canonicalFilePath() == QFileInfo(expectedParent).canonicalFilePath(),
                     qPrintable(QString("Iteration %1: file index resolved to '%2', expected containing folder '%3'")
                                    .arg(iteration)
                                    .arg(resolved)
                                    .arg(expectedParent)));
        }

        // --- Case: no workspace loaded -> empty (Req 1.3) ---
        {
            view.clearWorkspace();
            const QString resolvedInvalid = wsview_test_hooks::resolveDropTargetFolder(view, QModelIndex());
            QVERIFY2(resolvedInvalid.isEmpty(),
                     qPrintable(QString("Iteration %1: no-workspace invalid index resolved to non-empty '%2'")
                                    .arg(iteration)
                                    .arg(resolvedInvalid)));
        }
    }
}

/**
 * @brief Property 4: Target_Folder resolution follows the row-kind rule.
 *
 * For any pointer position over a loaded workspace, the highlighted folder equals
 * the Target_Folder resolved by resolveDropTargetFolder, and that folder maps back
 * to a VALID proxy index via targetFolderProxyIndex:
 *   - a folder row     -> itself,
 *   - a file row       -> the folder containing the file (the workspace root here),
 *   - empty tree area  -> the workspace root.
 *
 * Each iteration builds a temp workspace with random top-level folders and .scxml
 * files, loads it into a real WorkspaceView, then drives targetFolderProxyIndex via
 * real viewport positions obtained from visualRect(...). For each case the resolved
 * proxy index must be valid and point at the same filesystem path that
 * resolveDropTargetFolder reports for the row under that position.
 *
 * The run uses a minimum of 100 generated iterations.
 *
 * Validates: Requirements 1.1, 1.2, 1.5
 */
void WorkspaceViewDropPropertyTest::PropertyTargetFolderRowKindRule() {
    // Tag: Feature: drag-and-drop-visual-feedback, Property 4: Target_Folder resolution follows the row-kind rule
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        QTemporaryDir tempDir;
        QVERIFY2(tempDir.isValid(), qPrintable(QString("Iteration %1: failed to create temp dir").arg(iteration)));

        const QString rootDir = QFileInfo(tempDir.path()).canonicalFilePath();

        QSet<QString> usedNames;
        QStringList folderPaths;
        QStringList filePaths;

        const int folderCount = static_cast<int>(rng.bounded(1, 5));
        for (int i = 0; i < folderCount; ++i) {
            const QString name = randomBaseName(rng);
            if (usedNames.contains(name)) {
                continue;
            }
            usedNames.insert(name);
            const QString folderPath = QDir(rootDir).filePath(name);
            QVERIFY2(QDir().mkpath(folderPath),
                     qPrintable(QString("Iteration %1: failed to create folder '%2'").arg(iteration).arg(folderPath)));
            QVERIFY2(createEmptyFile(QDir(folderPath).filePath("seed.scxml")),
                     qPrintable(QString("Iteration %1: failed to seed folder '%2'").arg(iteration).arg(folderPath)));
            folderPaths.append(QFileInfo(folderPath).canonicalFilePath());
        }

        const int fileCount = static_cast<int>(rng.bounded(1, 5));
        for (int i = 0; i < fileCount; ++i) {
            const QString name = randomBaseName(rng);
            if (usedNames.contains(name)) {
                continue;
            }
            usedNames.insert(name);
            const QString filePath = QDir(rootDir).filePath(name + ".scxml");
            QVERIFY2(createEmptyFile(filePath),
                     qPrintable(QString("Iteration %1: failed to create file '%2'").arg(iteration).arg(filePath)));
            filePaths.append(QFileInfo(filePath).canonicalFilePath());
        }

        WorkspaceView view;
        view.setWorkspaceRoot(rootDir);
        view.resize(400, 400);
        view.show();
        QVERIFY2(QTest::qWaitForWindowExposed(&view),
                 qPrintable(QString("Iteration %1: view window was not exposed").arg(iteration)));

        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(view.model());
        QVERIFY2(nullptr != proxy, qPrintable(QString("Iteration %1: view model is not the expected proxy").arg(iteration)));
        QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(proxy->sourceModel());
        QVERIFY2(nullptr != fsModel,
                 qPrintable(QString("Iteration %1: proxy source is not a QFileSystemModel").arg(iteration)));

        // Let the model settle and the rows lay out before probing viewport geometry.
        QTest::qWait(20);

        // --- Case: folder row -> itself (Req 1.1) ---
        {
            const QString wantedFolder = folderPaths.at(static_cast<int>(rng.bounded(folderPaths.size())));
            const QModelIndex folderProxyIndex = proxyIndexForPath(proxy, fsModel, wantedFolder);
            QVERIFY2(
                folderProxyIndex.isValid(),
                qPrintable(QString("Iteration %1: could not resolve folder index for '%2'").arg(iteration).arg(wantedFolder)));

            view.scrollTo(folderProxyIndex);
            QTest::qWait(5);
            const QPoint pos = view.visualRect(folderProxyIndex).center();

            const QModelIndex targetIndex = wsview_test_hooks::targetFolderProxyIndex(view, pos);
            QVERIFY2(targetIndex.isValid(),
                     qPrintable(QString("Iteration %1: folder-row target proxy index was invalid").arg(iteration)));

            const QString resolved = wsview_test_hooks::resolveDropTargetFolder(view, view.indexAt(pos));
            const QString mappedPath = fsModel->filePath(proxy->mapToSource(targetIndex));
            QVERIFY2(QFileInfo(mappedPath).canonicalFilePath() == QFileInfo(resolved).canonicalFilePath(),
                     qPrintable(QString("Iteration %1: folder-row proxy index '%2' != resolved folder '%3'")
                                    .arg(iteration)
                                    .arg(mappedPath)
                                    .arg(resolved)));
            QVERIFY2(QFileInfo(mappedPath).canonicalFilePath() == wantedFolder,
                     qPrintable(QString("Iteration %1: folder-row resolved to '%2', expected '%3'")
                                    .arg(iteration)
                                    .arg(mappedPath)
                                    .arg(wantedFolder)));
        }

        // --- Case: file row -> containing folder (Req 1.2) ---
        {
            const QString wantedFile = filePaths.at(static_cast<int>(rng.bounded(filePaths.size())));
            const QModelIndex fileProxyIndex = proxyIndexForPath(proxy, fsModel, wantedFile);
            QVERIFY2(fileProxyIndex.isValid(),
                     qPrintable(QString("Iteration %1: could not resolve file index for '%2'").arg(iteration).arg(wantedFile)));

            view.scrollTo(fileProxyIndex);
            QTest::qWait(5);
            const QPoint pos = view.visualRect(fileProxyIndex).center();

            const QModelIndex targetIndex = wsview_test_hooks::targetFolderProxyIndex(view, pos);
            QVERIFY2(targetIndex.isValid(),
                     qPrintable(QString("Iteration %1: file-row target proxy index was invalid").arg(iteration)));

            const QString mappedPath = fsModel->filePath(proxy->mapToSource(targetIndex));
            const QString expectedParent = QFileInfo(wantedFile).absolutePath();
            QVERIFY2(QFileInfo(mappedPath).canonicalFilePath() == QFileInfo(expectedParent).canonicalFilePath(),
                     qPrintable(QString("Iteration %1: file-row proxy index '%2' != containing folder '%3'")
                                    .arg(iteration)
                                    .arg(mappedPath)
                                    .arg(expectedParent)));
        }

        // --- Case: empty tree area -> workspace root (Req 1.1 default area) ---
        {
            // A point below the last row maps to an invalid row index, which
            // resolveDropTargetFolder treats as the workspace root.
            const QPoint emptyPos(view.viewport()->width() / 2, view.viewport()->height() - 2);
            QVERIFY2(false == view.indexAt(emptyPos).isValid(),
                     qPrintable(QString("Iteration %1: empty-area probe unexpectedly hit a row").arg(iteration)));

            const QModelIndex targetIndex = wsview_test_hooks::targetFolderProxyIndex(view, emptyPos);
            QVERIFY2(targetIndex.isValid(),
                     qPrintable(QString("Iteration %1: empty-area target proxy index was invalid").arg(iteration)));

            const QString mappedPath = fsModel->filePath(proxy->mapToSource(targetIndex));
            QVERIFY2(QFileInfo(mappedPath).canonicalFilePath() == rootDir,
                     qPrintable(QString("Iteration %1: empty-area resolved to '%2', expected root '%3'")
                                    .arg(iteration)
                                    .arg(mappedPath)
                                    .arg(rootDir)));
        }
    }
}

/**
 * @brief Property 5: No highlight without a workspace.
 *
 * For any pointer position, when no Open_Workspace is loaded, resolveDropTargetFolder
 * yields an empty result and targetFolderProxyIndex yields an invalid index, so no
 * Target_Folder row can be highlighted.
 *
 * Each iteration constructs a WorkspaceView that never loads a workspace (or that is
 * cleared after loading one) and probes a random viewport position; both the resolved
 * folder and the mapped proxy index must indicate "no target".
 *
 * The run uses a minimum of 100 generated iterations.
 *
 * Validates: Requirements 1.5
 */
void WorkspaceViewDropPropertyTest::PropertyNoHighlightWithoutWorkspace() {
    // Tag: Feature: drag-and-drop-visual-feedback, Property 5: No highlight without a workspace
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        WorkspaceView view;
        view.resize(400, 400);

        // Half of the iterations never load a workspace; the other half load one and
        // then clear it. Both must behave as "no workspace loaded".
        if (0 != (iteration % 2)) {
            QTemporaryDir tempDir;
            QVERIFY2(tempDir.isValid(), qPrintable(QString("Iteration %1: failed to create temp dir").arg(iteration)));
            const QString rootDir = QFileInfo(tempDir.path()).canonicalFilePath();
            QVERIFY2(createEmptyFile(QDir(rootDir).filePath("seed.scxml")),
                     qPrintable(QString("Iteration %1: failed to seed workspace").arg(iteration)));
            view.setWorkspaceRoot(rootDir);
            QTest::qWait(10);
            view.clearWorkspace();
        }

        const QPoint probe(static_cast<int>(rng.bounded(0, 400)), static_cast<int>(rng.bounded(0, 400)));

        const QString resolved = wsview_test_hooks::resolveDropTargetFolder(view, view.indexAt(probe));
        QVERIFY2(resolved.isEmpty(),
                 qPrintable(QString("Iteration %1: no-workspace resolved to non-empty '%2'").arg(iteration).arg(resolved)));

        const QModelIndex targetIndex = wsview_test_hooks::targetFolderProxyIndex(view, probe);
        QVERIFY2(false == targetIndex.isValid(),
                 qPrintable(QString("Iteration %1: no-workspace target proxy index was valid").arg(iteration)));
    }
}

int runWorkspaceViewDropPropertyTest(int argc, char** argv) {
    QApplication app(argc, argv);

    WorkspaceViewDropPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "WorkspaceViewDropPropertyTest.moc"
