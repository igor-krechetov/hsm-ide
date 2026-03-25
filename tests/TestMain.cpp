#include <QByteArray>
#include <QtGlobal>

int runModelTypesTest(int argc, char** argv);
int runStateMachineEntityTest(int argc, char** argv);
int runRegularStateTest(int argc, char** argv);
int runHistoryStateTest(int argc, char** argv);
int runInitialStateTest(int argc, char** argv);
int runFinalStateTest(int argc, char** argv);
int runEntryPointTest(int argc, char** argv);
int runExitPointTest(int argc, char** argv);
int runTransitionTest(int argc, char** argv);
int runModelActionsTest(int argc, char** argv);
int runIncludeEntityTest(int argc, char** argv);
int runModelRootStateTest(int argc, char** argv);
int runModelElementsFactoryTest(int argc, char** argv);
int runStateMachineModelTest(int argc, char** argv);
int runStateHierarchyRulesTest(int argc, char** argv);
int runStateMachineSerializerSerializationTest(int argc, char** argv);
int runStateMachineSerializerDeserializationTest(int argc, char** argv);

// void silentMessageHandler(QtMsgType, const QMessageLogContext &, const QString &) {
//     // Do nothing → suppress all messages
// }

int main(int argc, char** argv) {
    // qInstallMessageHandler(silentMessageHandler);
    qputenv("QT_LOGGING_RULES", "*.debug=false;*.warning=false;");

    if (qEnvironmentVariableIsEmpty("HSM_IDE_TEST_SCXML_ROOT")) {
#ifdef HSM_IDE_TEST_SCXML_ROOT_DEFAULT
        qputenv("HSM_IDE_TEST_SCXML_ROOT", QByteArray(HSM_IDE_TEST_SCXML_ROOT_DEFAULT));
#else
        qputenv("HSM_IDE_TEST_SCXML_ROOT", QByteArray("tests/data/scxml"));
#endif
    }

    int result = 0;

    result |= runModelTypesTest(argc, argv);
    result |= runStateMachineEntityTest(argc, argv);
    result |= runRegularStateTest(argc, argv);
    result |= runHistoryStateTest(argc, argv);
    result |= runInitialStateTest(argc, argv);
    result |= runFinalStateTest(argc, argv);
    result |= runEntryPointTest(argc, argv);
    result |= runExitPointTest(argc, argv);
    result |= runTransitionTest(argc, argv);
    result |= runModelActionsTest(argc, argv);
    result |= runIncludeEntityTest(argc, argv);
    result |= runModelRootStateTest(argc, argv);
    result |= runModelElementsFactoryTest(argc, argv);
    result |= runStateMachineModelTest(argc, argv);
    result |= runStateHierarchyRulesTest(argc, argv);
    result |= runStateMachineSerializerSerializationTest(argc, argv);
    result |= runStateMachineSerializerDeserializationTest(argc, argv);

    return result;
}
