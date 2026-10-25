#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"

// Joint end-to-end test between hsm-ide and the hsmcpp code generator.
//
// Flow under test:
//   1. Load and deserialize the hand-authored manual fixture all_elements.scxml.
//   2. Serialize the resulting model back out to a temporary SCXML file.
//   3. Run hsmcpp's tools/scxml2gen/scxml2gen.py (vendored as a git submodule under
//      thirdparty/hsmcpp) against that serialized file and assert the generator
//      produced C++ with no ERROR and no WARNING diagnostics.
//
// NOTE: this test exercises the full round-trip against the real downstream
// generator. It is expected to surface gaps where a feature drawn in hsm-ide is
// not yet understood by scxml2gen; those are not fixed here - the test simply
// makes the current behaviour visible.
class AllElementsScxml2genJointTest : public QObject {
    Q_OBJECT

private slots:
    void LoadSerializeGenerateHasNoErrorsOrWarnings();

private:
    static QString sourceDir();
    static QString scxml2genScript();
};

QString AllElementsScxml2genJointTest::sourceDir() {
#ifdef HSM_IDE_SOURCE_DIR
    return QStringLiteral(HSM_IDE_SOURCE_DIR);
#else
    return QDir::currentPath();
#endif
}

QString AllElementsScxml2genJointTest::scxml2genScript() {
#ifdef HSM_IDE_SCXML2GEN_SCRIPT
    return QStringLiteral(HSM_IDE_SCXML2GEN_SCRIPT);
#else
    return sourceDir() + "/thirdparty/hsmcpp/tools/scxml2gen/scxml2gen.py";
#endif
}

/**
 * @brief Load all_elements.scxml, re-serialize it, and generate C++ with scxml2gen.
 *
 * Use-case: a model built from every supported hsm-ide element must round-trip
 * through serialization and then be consumable by the hsmcpp generator without
 * the generator reporting any ERROR or WARNING.
 */
void AllElementsScxml2genJointTest::LoadSerializeGenerateHasNoErrorsOrWarnings() {
    const QString manualTestsDir = sourceDir() + "/tests/data/manual_tests";
    const QString fixturePath = manualTestsDir + "/all_elements.scxml";

    // --- Step 1: load + deserialize --------------------------------------------------
    QFile fixtureFile(fixturePath);
    QVERIFY2(fixtureFile.open(QIODevice::ReadOnly | QIODevice::Text),
             qPrintable(QString("cannot open fixture: %1").arg(fixturePath)));
    const QString scxmlIn = QString::fromUtf8(fixtureFile.readAll());
    fixtureFile.close();
    QVERIFY(!scxmlIn.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxmlIn);
    QVERIFY(model);
    QVERIFY(model->root());

    // --- Step 2: serialize to a temporary file ---------------------------------------
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString serializedPath = tempDir.path() + "/all_elements.generated.scxml";
    const QString serialized = serializer.serializeToScxml(model);
    QVERIFY(!serialized.isEmpty());

    QFile outFile(serializedPath);
    QVERIFY(outFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(outFile.write(serialized.toUtf8()) > 0);
    outFile.close();

    // all_elements.scxml references transitions.scxml via <xi:include>; scxml2gen
    // resolves that href relative to the SCXML file's own directory, so the included
    // fixture must sit next to the serialized file for generation to succeed.
    const QString includeSrc = manualTestsDir + "/transitions.scxml";
    if (QFileInfo::exists(includeSrc)) {
        QVERIFY(QFile::copy(includeSrc, tempDir.path() + "/transitions.scxml"));
    }

    // --- Step 3: generate code with scxml2gen ----------------------------------------
    const QString script = scxml2genScript();
    QVERIFY2(QFileInfo::exists(script),
             qPrintable(QString("scxml2gen not found (is the hsmcpp submodule initialized?): %1").arg(script)));

    const QString scriptDir = QFileInfo(script).absolutePath();
    const QString genHpp = tempDir.path() + "/GeneratedHsm.hpp";
    const QString genCpp = tempDir.path() + "/GeneratedHsm.cpp";

    QProcess proc;
    proc.setWorkingDirectory(scriptDir);
    proc.setProcessChannelMode(QProcess::MergedChannels);

    const QStringList args = {
        script,
        "-code",
        "-scxml", serializedPath,
        "-class_name", "GeneratedHsm",
        "-template_hpp", scriptDir + "/template.hpp",
        "-template_cpp", scriptDir + "/template.cpp",
        "-dest_hpp", genHpp,
        "-dest_cpp", genCpp,
    };

    proc.start("python3", args);
    QVERIFY2(proc.waitForStarted(10000), "failed to start python3 for scxml2gen");
    QVERIFY2(proc.waitForFinished(60000), "scxml2gen did not finish in time");

    const QString output = QString::fromUtf8(proc.readAll());
    qInfo().noquote() << "scxml2gen output:\n" << output;

    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QCOMPARE(proc.exitCode(), 0);

    // The generator prints ERROR:/WARNING: lines on unsupported or malformed input.
    QVERIFY2(!output.contains("ERROR", Qt::CaseSensitive),
             qPrintable(QString("scxml2gen reported an ERROR:\n%1").arg(output)));
    QVERIFY2(!output.contains("WARNING", Qt::CaseSensitive),
             qPrintable(QString("scxml2gen reported a WARNING:\n%1").arg(output)));

    QVERIFY2(QFileInfo::exists(genHpp), "generated .hpp was not produced");
    QVERIFY2(QFileInfo::exists(genCpp), "generated .cpp was not produced");
}

int runAllElementsScxml2genJointTest(int argc, char** argv) {
    AllElementsScxml2genJointTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "AllElementsScxml2genJointTest.moc"
