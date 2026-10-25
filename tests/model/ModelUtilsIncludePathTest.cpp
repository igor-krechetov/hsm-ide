#include <QtTest>

#include "model/ModelUtils.hpp"

class ModelUtilsIncludePathTest : public QObject {
    Q_OBJECT

private slots:
    void RelativeIncludeResolvesAgainstIncludingDir();
    void AbsoluteIncludePassesThroughUnchanged();
    void NestedRelativeIncludeNormalizesParentDir();
    void NestedIncludesChainPerFileDirectory();
    void EmptyIncludingPathLeavesRelativeIncludeUnchanged();
    void EmptyIncludePathPassesThroughUnchanged();
};

/**
 * @brief A relative include path resolves against the including file's directory, not CWD.
 *
 * Use-case: parent SCXML includes a child via a relative href while opened from another CWD.
 */
void ModelUtilsIncludePathTest::RelativeIncludeResolvesAgainstIncludingDir() {
    const QString resolved = model::resolveIncludePath("/proj/dir/main.scxml", "sub/child.scxml");
    QCOMPARE(resolved, QString("/proj/dir/sub/child.scxml"));
}

/**
 * @brief An absolute include path is used as-is.
 *
 * Use-case: include references an SCXML file by absolute path.
 */
void ModelUtilsIncludePathTest::AbsoluteIncludePassesThroughUnchanged() {
    const QString resolved = model::resolveIncludePath("/proj/dir/main.scxml", "/abs/other.scxml");
    QCOMPARE(resolved, QString("/abs/other.scxml"));
}

/**
 * @brief A relative include containing ".." normalizes against the including file's directory.
 *
 * Use-case: sibling-directory include referenced via a parent-relative href.
 */
void ModelUtilsIncludePathTest::NestedRelativeIncludeNormalizesParentDir() {
    const QString resolved = model::resolveIncludePath("/proj/dir/main.scxml", "../shared/x.scxml");
    QCOMPARE(resolved, QString("/proj/shared/x.scxml"));
}

/**
 * @brief Nested includes chain: each file's includes resolve relative to that file's own dir.
 *
 * Use-case: parent includes child in a subdir; child includes grandchild via its own relative href.
 */
void ModelUtilsIncludePathTest::NestedIncludesChainPerFileDirectory() {
    const QString child = model::resolveIncludePath("/proj/dir/main.scxml", "sub/child.scxml");
    QCOMPARE(child, QString("/proj/dir/sub/child.scxml"));

    const QString grandchild = model::resolveIncludePath(child, "grandchild.scxml");
    QCOMPARE(grandchild, QString("/proj/dir/sub/grandchild.scxml"));
}

/**
 * @brief An empty including-file path leaves a relative include unchanged (nothing to resolve against).
 *
 * Use-case: include double-clicked in an untitled project with no file path yet.
 */
void ModelUtilsIncludePathTest::EmptyIncludingPathLeavesRelativeIncludeUnchanged() {
    const QString resolved = model::resolveIncludePath(QString(), "sub/child.scxml");
    QCOMPARE(resolved, QString("sub/child.scxml"));
}

/**
 * @brief An empty include path is returned unchanged.
 *
 * Use-case: include element with no href set.
 */
void ModelUtilsIncludePathTest::EmptyIncludePathPassesThroughUnchanged() {
    const QString resolved = model::resolveIncludePath("/proj/dir/main.scxml", QString());
    QCOMPARE(resolved, QString());
}

int runModelUtilsIncludePathTest(int argc, char** argv) {
    ModelUtilsIncludePathTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ModelUtilsIncludePathTest.moc"
