#include <QApplication>
#include <QList>
#include <QRandomGenerator>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtTest>

// view::DragDropUtils exposes pure free functions (see
// src/view/common/DragDropUtils.hpp), compiled into the `view` library this
// test links against. No private-access tricks are needed; the functions are
// called directly.
#include "view/common/DragDropUtils.hpp"

// ---------------------------------------------------------------------------
// Property-based tests for the pure drag-and-drop helpers in view::DragDropUtils.
//
// Property 1: SCXML filtering is an order-preserving partition (Req 2.1, 2.3,
//   2.5, 3.6) — filterScxmlPaths returns exactly the subsequence of inputs that
//   end with .scxml (case-insensitive), in their original relative order.
//
// Property 2: Overlay/accept gate matches SCXML presence (Req 3.1, 3.2) —
//   hasScxml(paths) is true iff filterScxmlPaths(paths) is non-empty.
//
// Property 3: URL-to-path conversion preserves order and locality (Req 2.1,
//   3.6) — localPathsFromUrls yields one local path per input URL in the same
//   order.
//
// Each property runs a minimum of 100 generated iterations and is tagged with
// its design-document property per the feature's testing strategy.
// ---------------------------------------------------------------------------
class DragDropUtilsPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyScxmlFilteringIsOrderPreservingPartition();
    void PropertyOverlayGateMatchesScxmlPresence();
    void PropertyUrlToPathConversionPreservesOrderAndLocality();

private:
    static QString makeScxmlPath(QRandomGenerator& rng, int index);
    static QString makeNonScxmlPath(QRandomGenerator& rng, int index);
    static QString makeAnyPath(QRandomGenerator& rng, int index, bool& isScxmlOut);
};

// Build a genuine .scxml path with the extension in a varied letter case so
// case-insensitivity is exercised.
QString DragDropUtilsPropertyTest::makeScxmlPath(QRandomGenerator& rng, int index) {
    static const QStringList scxmlExtensionCases = {
        QStringLiteral(".scxml"),
        QStringLiteral(".SCXML"),
        QStringLiteral(".ScXmL"),
        QStringLiteral(".sCxMl"),
        QStringLiteral(".Scxml"),
        QStringLiteral(".scxmL"),
    };

    const QString ext = scxmlExtensionCases.at(static_cast<int>(rng.bounded(scxmlExtensionCases.size())));

    return QStringLiteral("/tmp/dir%1/machine%2%3").arg(rng.bounded(10)).arg(index).arg(ext);
}

// Build a path that must NOT be treated as .scxml: near-misses that merely
// contain "scxml", end in a different extension, or are empty/plain.
QString DragDropUtilsPropertyTest::makeNonScxmlPath(QRandomGenerator& rng, int index) {
    QString result;

    switch (rng.bounded(7)) {
        case 0:
            result = QStringLiteral("/tmp/notes%1.xml").arg(index);
            break;
        case 1:
            result = QStringLiteral("/tmp/trunc%1.scxm").arg(index);
            break;
        case 2:
            result = QStringLiteral("/tmp/backup%1.scxml.bak").arg(index);
            break;
        case 3:
            result = QStringLiteral("/tmp/scxml%1").arg(index);  // contains the word, no extension
            break;
        case 4:
            result = QStringLiteral("/tmp/scxml_folder%1/readme").arg(index);
            break;
        case 5:
            result = QStringLiteral("/tmp/doc%1.txt").arg(index);
            break;
        default:
            result = QString();  // empty path
            break;
    }

    return result;
}

// Build an arbitrary path (scxml or not) and report which kind it is, so the
// caller can compute the expected .scxml subset independently of the helper.
QString DragDropUtilsPropertyTest::makeAnyPath(QRandomGenerator& rng, int index, bool& isScxmlOut) {
    QString result;

    if (rng.bounded(3) == 0) {
        result = makeScxmlPath(rng, index);
        isScxmlOut = true;
    } else {
        result = makeNonScxmlPath(rng, index);
        isScxmlOut = false;
    }

    return result;
}

/**
 * @brief Property 1: SCXML filtering is an order-preserving partition.
 *
 * For any list of file paths, filterScxmlPaths returns exactly the subsequence
 * of inputs that end with .scxml (case-insensitive), in their original relative
 * order: every returned path is an .scxml path, no .scxml input is dropped, and
 * no non-.scxml input is included.
 *
 * A fresh random path list is generated each iteration from a mixed alphabet of
 * path kinds (mixed-case .scxml, near-miss, plain non-matching, and empty
 * paths), with list length varied from 0 up to a few dozen so the partition is
 * checked regardless of count. The expected .scxml subset is recomputed
 * independently of the production helper (by remembering, at generation time,
 * which paths were built as .scxml) and compared for exact equality including
 * order.
 *
 * Feature: drag-and-drop-visual-feedback, Property 1: SCXML filtering is an order-preserving partition
 *
 * Validates: Requirements 2.1, 2.3, 2.5, 3.6
 */
void DragDropUtilsPropertyTest::PropertyScxmlFilteringIsOrderPreservingPartition() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        const int pathCount = static_cast<int>(rng.bounded(0, 32));

        QStringList inputPaths;
        QStringList expectedScxmlPaths;

        for (int i = 0; i < pathCount; ++i) {
            bool isScxml = false;
            const QString path = makeAnyPath(rng, i, isScxml);

            inputPaths.append(path);
            if (isScxml) {
                expectedScxmlPaths.append(path);  // order preserved by appending in input order
            }
        }

        const QStringList filteredPaths = view::DragDropUtils::filterScxmlPaths(inputPaths);

        // The filtered set must equal exactly the .scxml subset, order preserved.
        QVERIFY2(filteredPaths == expectedScxmlPaths,
                 qPrintable(QString("Iteration %1 (count %2): filtered paths [%3] != expected .scxml subset [%4] "
                                    "for input [%5]")
                                .arg(iteration)
                                .arg(pathCount)
                                .arg(filteredPaths.join(QStringLiteral(", ")))
                                .arg(expectedScxmlPaths.join(QStringLiteral(", ")))
                                .arg(inputPaths.join(QStringLiteral(", ")))));

        // Every returned path must itself be an .scxml path (no non-.scxml leaks in).
        for (const QString& path : filteredPaths) {
            QVERIFY2(
                path.endsWith(QStringLiteral(".scxml"), Qt::CaseInsensitive),
                qPrintable(QString("Iteration %1: non-.scxml path [%2] present in filtered output").arg(iteration).arg(path)));
        }
    }
}

/**
 * @brief Property 2: Overlay/accept gate matches SCXML presence.
 *
 * For any list of dragged paths, hasScxml(paths) is true if and only if at
 * least one path is an .scxml path, i.e. hasScxml(paths) ==
 * (filterScxmlPaths(paths).isEmpty() == false). This is the single predicate
 * that gates the editor drop overlay (show/accept vs. hide/decline).
 *
 * The input space is covered by varying the list length (including the empty
 * list, which must gate to false) and the mix of .scxml / non-.scxml / empty
 * paths, so both the true and false arms of the gate are exercised.
 *
 * Feature: drag-and-drop-visual-feedback, Property 2: Overlay/accept gate matches SCXML presence
 *
 * Validates: Requirements 3.1, 3.2
 */
void DragDropUtilsPropertyTest::PropertyOverlayGateMatchesScxmlPresence() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        const int pathCount = static_cast<int>(rng.bounded(0, 32));

        QStringList inputPaths;
        bool anyScxml = false;

        for (int i = 0; i < pathCount; ++i) {
            bool isScxml = false;
            const QString path = makeAnyPath(rng, i, isScxml);

            inputPaths.append(path);
            anyScxml = anyScxml || isScxml;
        }

        const bool gate = view::DragDropUtils::hasScxml(inputPaths);
        const bool filterNonEmpty = view::DragDropUtils::filterScxmlPaths(inputPaths).isEmpty() == false;

        // The gate must agree with the filter emptiness and with the independent
        // record of whether any .scxml path was generated.
        QVERIFY2(gate == filterNonEmpty,
                 qPrintable(QString("Iteration %1 (count %2): hasScxml (%3) != (filter non-empty) (%4)")
                                .arg(iteration)
                                .arg(pathCount)
                                .arg(gate)
                                .arg(filterNonEmpty)));

        QVERIFY2(gate == anyScxml,
                 qPrintable(QString("Iteration %1 (count %2): hasScxml (%3) != any-.scxml-generated (%4)")
                                .arg(iteration)
                                .arg(pathCount)
                                .arg(gate)
                                .arg(anyScxml)));
    }
}

/**
 * @brief Property 3: URL-to-path conversion preserves order and locality.
 *
 * For any list of URLs, localPathsFromUrls returns one local-file path per
 * input URL in the same order, so a drag's dropped set is partitioned
 * downstream solely by filterScxmlPaths.
 *
 * Each iteration builds a QList<QUrl> from randomly generated local file paths
 * via QUrl::fromLocalFile, varying the count from 0 up to a few dozen. The
 * expected list is the original local paths in input order; the output must
 * match it exactly (same length, same order, round-trip-equal per entry).
 *
 * Feature: drag-and-drop-visual-feedback, Property 3: URL-to-path conversion preserves order and locality
 *
 * Validates: Requirements 2.1, 3.6
 */
void DragDropUtilsPropertyTest::PropertyUrlToPathConversionPreservesOrderAndLocality() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        const int urlCount = static_cast<int>(rng.bounded(0, 32));

        QList<QUrl> inputUrls;
        QStringList expectedPaths;

        for (int i = 0; i < urlCount; ++i) {
            bool isScxml = false;
            const QString localPath = makeAnyPath(rng, i, isScxml);
            const QUrl url = QUrl::fromLocalFile(localPath);

            inputUrls.append(url);
            // The expected local path is the round-trip through QUrl, which is
            // exactly what localPathsFromUrls computes per entry.
            expectedPaths.append(url.toLocalFile());
        }

        const QStringList converted = view::DragDropUtils::localPathsFromUrls(inputUrls);

        // One local path per input URL (locality), same order.
        QVERIFY2(converted.size() == inputUrls.size(),
                 qPrintable(QString("Iteration %1: converted count (%2) != input URL count (%3)")
                                .arg(iteration)
                                .arg(converted.size())
                                .arg(inputUrls.size())));

        QVERIFY2(converted == expectedPaths,
                 qPrintable(QString("Iteration %1 (count %2): converted paths [%3] != expected [%4]")
                                .arg(iteration)
                                .arg(urlCount)
                                .arg(converted.join(QStringLiteral(", ")))
                                .arg(expectedPaths.join(QStringLiteral(", ")))));
    }
}

int runDragDropUtilsPropertyTest(int argc, char** argv) {
    QApplication app(argc, argv);

    DragDropUtilsPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "DragDropUtilsPropertyTest.moc"
