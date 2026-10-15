#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

// Test-linkage hooks exposed by WorkspaceView.cpp (added by task 5.3). These are
// external-linkage forwarders to the file-local pure collision helpers; we
// forward-declare the namespace here and call through it (mirrors tests/view
// practice for reaching pure view helpers).
namespace wsview_test_hooks {

bool nameCollides(const QString& targetDir, const QString& fileName);
QString makeNonCollidingName(const QString& targetDir, const QString& fileName);

}  // namespace wsview_test_hooks

// ---------------------------------------------------------------------------
// Property-based tests for the Workspace View collision helpers.
//
// Property 2: Collision detection (Requirement 2.1)
//   nameCollides(dir, name) is true iff a file with that name already exists in dir.
//
// Property 3: Rename produces a non-colliding name (Requirement 2.3)
//   makeNonCollidingName(dir, name) returns a name that is absent in dir and
//   preserves the original file extension.
//
// Each property runs a minimum of 100 generated iterations and is tagged with
// its design-document property per the feature's testing strategy.
// ---------------------------------------------------------------------------
class WorkspaceViewCollisionPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyCollisionDetection();
    void PropertyRenameProducesNonCollidingName();

private:
    static QString randomBaseName(QRandomGenerator& rng);
    static QString randomFileName(QRandomGenerator& rng);
    static bool createEmptyFile(const QString& dir, const QString& fileName);
};

QString WorkspaceViewCollisionPropertyTest::randomBaseName(QRandomGenerator& rng) {
    // Build a short, filesystem-safe base name from a constrained alphabet so the
    // generated names stay inside the valid input space (no path separators, no
    // reserved characters).
    static const QString alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
    const int length = static_cast<int>(rng.bounded(1, 10));

    QString result;
    for (int i = 0; i < length; ++i) {
        result.append(alphabet.at(static_cast<int>(rng.bounded(alphabet.size()))));
    }

    return result;
}

QString WorkspaceViewCollisionPropertyTest::randomFileName(QRandomGenerator& rng) {
    static const QStringList extensions = {"scxml", "txt", "xml", "json", "cpp", ""};
    const QString baseName = randomBaseName(rng);
    const QString suffix = extensions.at(static_cast<int>(rng.bounded(extensions.size())));

    QString result;
    if (suffix.isEmpty()) {
        result = baseName;
    } else {
        result = QString("%1.%2").arg(baseName, suffix);
    }

    return result;
}

bool WorkspaceViewCollisionPropertyTest::createEmptyFile(const QString& dir, const QString& fileName) {
    QFile file(QDir(dir).filePath(fileName));
    bool created = false;
    if (file.open(QIODevice::WriteOnly)) {
        file.close();
        created = true;
    }

    return created;
}

/**
 * @brief Property 2: Collision detection.
 *
 * For any target folder contents and any dropped file name, nameCollides is true
 * if and only if a file with that name already exists in the target folder.
 *
 * Each iteration populates a temp dir with a random set of files, then checks a
 * probe name that is either known-present (drawn from the created set) or
 * known-absent (verified not to be in the created set). The predicate must agree
 * with ground truth in both directions.
 *
 * Validates: Requirements 2.1
 */
void WorkspaceViewCollisionPropertyTest::PropertyCollisionDetection() {
    // Tag: Feature: drag-and-drop-files, Property 2: Collision detection
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        QTemporaryDir tempDir;
        QVERIFY2(tempDir.isValid(),
                 qPrintable(QString("Iteration %1: failed to create temp dir").arg(iteration)));
        const QString dir = tempDir.path();

        // Populate the directory with a random, de-duplicated set of files.
        QSet<QString> present;
        const int fileCount = static_cast<int>(rng.bounded(0, 8));
        for (int i = 0; i < fileCount; ++i) {
            const QString name = randomFileName(rng);
            if (!present.contains(name)) {
                QVERIFY2(createEmptyFile(dir, name),
                         qPrintable(QString("Iteration %1: failed to create file '%2'").arg(iteration).arg(name)));
                present.insert(name);
            }
        }

        // Decide whether to probe a name that exists or one that does not.
        const bool probeExisting = (!present.isEmpty()) && (0 == rng.bounded(2));

        QString probeName;
        bool expectedCollision = false;
        if (probeExisting) {
            // Pick an arbitrary existing name from the created set.
            const QStringList presentList = present.values();
            probeName = presentList.at(static_cast<int>(rng.bounded(presentList.size())));
            expectedCollision = true;
        } else {
            // Generate a name until it is guaranteed absent from the created set.
            do {
                probeName = randomFileName(rng);
            } while (present.contains(probeName));
            expectedCollision = false;
        }

        const bool actualCollision = wsview_test_hooks::nameCollides(dir, probeName);

        QVERIFY2(actualCollision == expectedCollision,
                 qPrintable(QString("Iteration %1: nameCollides('%2') returned %3 but file %4 in dir")
                                .arg(iteration)
                                .arg(probeName)
                                .arg(actualCollision ? "true" : "false")
                                .arg(expectedCollision ? "exists" : "does not exist")));
    }
}

/**
 * @brief Property 3: Rename produces a non-colliding name.
 *
 * For any target folder contents and any dropped file name, the name generated by
 * makeNonCollidingName does not match any existing file name in the target folder
 * and preserves the original file extension.
 *
 * Each iteration seeds a temp dir, then forces a collision by creating the probe
 * name itself (plus random extra files), so the generator must actually produce a
 * distinct name. The result is asserted absent in the dir and extension-preserving.
 *
 * Validates: Requirements 2.3
 */
void WorkspaceViewCollisionPropertyTest::PropertyRenameProducesNonCollidingName() {
    // Tag: Feature: drag-and-drop-files, Property 3: Rename produces a non-colliding name
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        QTemporaryDir tempDir;
        QVERIFY2(tempDir.isValid(),
                 qPrintable(QString("Iteration %1: failed to create temp dir").arg(iteration)));
        const QString dir = tempDir.path();

        const QString probeName = randomFileName(rng);

        // Force a collision: the probe name itself exists in the directory.
        QVERIFY2(createEmptyFile(dir, probeName),
                 qPrintable(QString("Iteration %1: failed to create probe file '%2'").arg(iteration).arg(probeName)));

        // Also create a few sequentially-numbered variants to push the counter.
        const QFileInfo probeInfo(probeName);
        const QString baseName = probeInfo.completeBaseName();
        const QString suffix = probeInfo.suffix();
        const int extraVariants = static_cast<int>(rng.bounded(0, 4));
        for (int i = 1; i <= extraVariants; ++i) {
            QString variant;
            if (suffix.isEmpty()) {
                variant = QString("%1 (%2)").arg(baseName).arg(i);
            } else {
                variant = QString("%1 (%2).%3").arg(baseName).arg(i).arg(suffix);
            }
            createEmptyFile(dir, variant);
        }

        const QString generated = wsview_test_hooks::makeNonCollidingName(dir, probeName);

        // The generated name must be absent in the directory.
        QVERIFY2(!QFileInfo::exists(QDir(dir).filePath(generated)),
                 qPrintable(QString("Iteration %1: makeNonCollidingName('%2') -> '%3' already exists in dir")
                                .arg(iteration)
                                .arg(probeName)
                                .arg(generated)));

        // The generated name must also not collide per the predicate (consistency).
        QVERIFY2(!wsview_test_hooks::nameCollides(dir, generated),
                 qPrintable(QString("Iteration %1: generated name '%2' still reported as colliding")
                                .arg(iteration)
                                .arg(generated)));

        // The generated name must preserve the original extension.
        QCOMPARE(QFileInfo(generated).suffix(), suffix);
    }
}

int runWorkspaceViewCollisionPropertyTest(int argc, char** argv) {
    WorkspaceViewCollisionPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "WorkspaceViewCollisionPropertyTest.moc"
