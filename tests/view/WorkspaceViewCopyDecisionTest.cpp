#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMessageBox>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>
#include <QWidget>
#include <QtTest>

#include "view/widgets/WorkspaceView.hpp"

// Test-linkage hook exposed by WorkspaceView (friend forwarder). copyDroppedFileIntoFolder
// is a private, non-const member that shows a modal Overwrite/Rename/Cancel QMessageBox on
// a name collision, so the test reaches it through the wsview_test_hooks forwarder
// (declared in WorkspaceView.hpp), mirroring the hook pattern used for resolveDropTargetFolder.

// ---------------------------------------------------------------------------
// Example tests for WorkspaceView::copyDroppedFileIntoFolder collision decisions.
//
// The copy is driven end-to-end against a temp directory. Because the collision
// prompt is a modal QMessageBox, each decision is simulated by scheduling (via
// QTimer::singleShot) a handler that locates the active modal QMessageBox through
// QApplication::activeModalWidget() and clicks the Overwrite / Rename / Cancel
// button before the modal exec() returns. The tests then assert the resulting
// filesystem state.
//
//   - Overwrite (Req 2.2): destination content is replaced by the source content.
//   - Rename    (Req 2.3): a new non-colliding file is created, original unchanged.
//   - Cancel    (Req 2.4): existing file is unchanged and no new file is created.
// ---------------------------------------------------------------------------
class WorkspaceViewCopyDecisionTest : public QObject {
    Q_OBJECT

private slots:
    void OverwriteReplacesExistingFile();
    void RenameCreatesNonCollidingFileLeavingOriginal();
    void CancelLeavesExistingFileUnchangedAndSkips();

private:
    enum class Decision { Overwrite, Rename, Cancel };

    static bool writeFile(const QString& path, const QString& content);
    static QString readFile(const QString& path);
    static void scheduleModalDecision(Decision decision);
    static QAbstractButton* findButtonByText(QMessageBox* box, const QString& text);
};

bool WorkspaceViewCopyDecisionTest::writeFile(const QString& path, const QString& content) {
    QFile file(path);
    bool written = false;
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(content.toUtf8());
        file.close();
        written = true;
    }

    return written;
}

QString WorkspaceViewCopyDecisionTest::readFile(const QString& path) {
    QFile file(path);
    QString content;
    if (file.open(QIODevice::ReadOnly)) {
        content = QString::fromUtf8(file.readAll());
        file.close();
    }

    return content;
}

QAbstractButton* WorkspaceViewCopyDecisionTest::findButtonByText(QMessageBox* box, const QString& text) {
    QAbstractButton* result = nullptr;

    const QList<QAbstractButton*> buttons = box->buttons();
    for (QAbstractButton* button : buttons) {
        // Qt may add a mnemonic ampersand to button text; strip it before comparing.
        const QString buttonText = button->text().remove('&');
        if (buttonText == text) {
            result = button;
            break;
        }
    }

    return result;
}

void WorkspaceViewCopyDecisionTest::scheduleModalDecision(Decision decision) {
    // Poll for the modal prompt on the event loop and act once it appears. The
    // collision prompt is created and exec()'d synchronously inside
    // copyDroppedFileIntoFolder, so this handler runs while that exec() blocks.
    QString wantedText;
    switch (decision) {
        case Decision::Overwrite:
            wantedText = QStringLiteral("Overwrite");
            break;
        case Decision::Rename:
            wantedText = QStringLiteral("Rename");
            break;
        case Decision::Cancel:
            wantedText = QStringLiteral("Cancel");
            break;
    }

    QTimer::singleShot(0, [wantedText]() {
        QMessageBox* box = nullptr;

        // Give the modal a bounded number of event-loop spins to appear.
        for (int spins = 0; (nullptr == box) && (spins < 200); ++spins) {
            box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (nullptr == box) {
                QTest::qWait(5);
            }
        }

        if (nullptr != box) {
            QAbstractButton* button = findButtonByText(box, wantedText);
            if (nullptr != button) {
                button->click();
            } else {
                box->reject();
            }
        }
    });
}

/**
 * @brief Overwrite replaces the existing file with the dropped source (Req 2.2).
 */
void WorkspaceViewCopyDecisionTest::OverwriteReplacesExistingFile() {
    QTemporaryDir sourceDir;
    QTemporaryDir targetDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(targetDir.isValid());

    const QString fileName = QStringLiteral("sample.scxml");
    const QString sourcePath = QDir(sourceDir.path()).filePath(fileName);
    const QString destPath = QDir(targetDir.path()).filePath(fileName);

    QVERIFY(writeFile(sourcePath, QStringLiteral("SOURCE-CONTENT")));
    QVERIFY(writeFile(destPath, QStringLiteral("EXISTING-CONTENT")));

    WorkspaceView view;
    scheduleModalDecision(Decision::Overwrite);
    wsview_test_hooks::copyDroppedFileIntoFolder(view, sourcePath, targetDir.path());

    // The single destination file now holds the source content.
    QVERIFY2(QFileInfo::exists(destPath), "destination file must still exist after overwrite");
    QCOMPARE(readFile(destPath), QStringLiteral("SOURCE-CONTENT"));

    // No renamed sibling was created.
    QVERIFY2(!QFileInfo::exists(QDir(targetDir.path()).filePath(QStringLiteral("sample (1).scxml"))),
             "overwrite must not create a renamed copy");
}

/**
 * @brief Rename copies under a non-colliding name and leaves the original intact (Req 2.3).
 */
void WorkspaceViewCopyDecisionTest::RenameCreatesNonCollidingFileLeavingOriginal() {
    QTemporaryDir sourceDir;
    QTemporaryDir targetDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(targetDir.isValid());

    const QString fileName = QStringLiteral("sample.scxml");
    const QString sourcePath = QDir(sourceDir.path()).filePath(fileName);
    const QString destPath = QDir(targetDir.path()).filePath(fileName);
    const QString renamedPath = QDir(targetDir.path()).filePath(QStringLiteral("sample (1).scxml"));

    QVERIFY(writeFile(sourcePath, QStringLiteral("SOURCE-CONTENT")));
    QVERIFY(writeFile(destPath, QStringLiteral("EXISTING-CONTENT")));

    WorkspaceView view;
    scheduleModalDecision(Decision::Rename);
    wsview_test_hooks::copyDroppedFileIntoFolder(view, sourcePath, targetDir.path());

    // The original existing file is untouched.
    QVERIFY2(QFileInfo::exists(destPath), "original existing file must remain after rename");
    QCOMPARE(readFile(destPath), QStringLiteral("EXISTING-CONTENT"));

    // A new, non-colliding file carrying the source content was created.
    QVERIFY2(QFileInfo::exists(renamedPath), "rename must create a non-colliding copy");
    QCOMPARE(readFile(renamedPath), QStringLiteral("SOURCE-CONTENT"));
}

/**
 * @brief Cancel leaves the existing file unchanged and skips the copy (Req 2.4).
 */
void WorkspaceViewCopyDecisionTest::CancelLeavesExistingFileUnchangedAndSkips() {
    QTemporaryDir sourceDir;
    QTemporaryDir targetDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(targetDir.isValid());

    const QString fileName = QStringLiteral("sample.scxml");
    const QString sourcePath = QDir(sourceDir.path()).filePath(fileName);
    const QString destPath = QDir(targetDir.path()).filePath(fileName);
    const QString renamedPath = QDir(targetDir.path()).filePath(QStringLiteral("sample (1).scxml"));

    QVERIFY(writeFile(sourcePath, QStringLiteral("SOURCE-CONTENT")));
    QVERIFY(writeFile(destPath, QStringLiteral("EXISTING-CONTENT")));

    WorkspaceView view;
    scheduleModalDecision(Decision::Cancel);
    wsview_test_hooks::copyDroppedFileIntoFolder(view, sourcePath, targetDir.path());

    // The existing file is unchanged.
    QVERIFY2(QFileInfo::exists(destPath), "existing file must remain after cancel");
    QCOMPARE(readFile(destPath), QStringLiteral("EXISTING-CONTENT"));

    // No new file was created by the skipped copy.
    QVERIFY2(!QFileInfo::exists(renamedPath), "cancel must not create a renamed copy");
}

int runWorkspaceViewCopyDecisionTest(int argc, char** argv) {
    QApplication app(argc, argv);

    WorkspaceViewCopyDecisionTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "WorkspaceViewCopyDecisionTest.moc"
