#include <QtTest>

#include "model/ModelRootState.hpp"

class ModelRootStateTest : public QObject {
    Q_OBJECT

private slots:
    void NameOnlyProperties();
};

/**
 * @brief Verify model root exposes only the name property.
 *
 * Use-case: Root metadata editor allows renaming state machine only.
 */
void ModelRootStateTest::NameOnlyProperties() {
    auto root = QSharedPointer<model::ModelRootState>::create("Root");

    QCOMPARE(model::StateType::MODEL_ROOT, root->stateType());
    QCOMPARE(QStringList({"name"}), root->properties());

    QVERIFY(root->setProperty("name", "Renamed"));
    QCOMPARE(QString("Renamed"), root->getProperty("name").toString());
}

int runModelRootStateTest(int argc, char** argv) {
    ModelRootStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ModelRootStateTest.moc"
