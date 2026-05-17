#include <QByteArray>
#include <QtGlobal>

using TestRunner = int (*)(int, char**);

int runModelTypesTest(int argc, char** argv) __attribute__((weak));
int runStateMachineEntityTest(int argc, char** argv) __attribute__((weak));
int runRegularStateTest(int argc, char** argv) __attribute__((weak));
int runHistoryStateTest(int argc, char** argv) __attribute__((weak));
int runInitialStateTest(int argc, char** argv) __attribute__((weak));
int runFinalStateTest(int argc, char** argv) __attribute__((weak));
int runEntryPointTest(int argc, char** argv) __attribute__((weak));
int runExitPointTest(int argc, char** argv) __attribute__((weak));
int runTransitionTest(int argc, char** argv) __attribute__((weak));
int runModelActionsTest(int argc, char** argv) __attribute__((weak));
int runIncludeEntityTest(int argc, char** argv) __attribute__((weak));
int runModelRootStateTest(int argc, char** argv) __attribute__((weak));
int runModelElementsFactoryTest(int argc, char** argv) __attribute__((weak));
int runStateMachineModelTest(int argc, char** argv) __attribute__((weak));
int runStateHierarchyRulesTest(int argc, char** argv) __attribute__((weak));
int runStateMachineSerializerSerializationTest(int argc, char** argv) __attribute__((weak));
int runStateMachineSerializerDeserializationTest(int argc, char** argv) __attribute__((weak));
int runHsmGraphicsViewTest(int argc, char** argv) __attribute__((weak));

int main(int argc, char** argv) {
    qputenv("QT_LOGGING_RULES", "*.debug=false;*.warning=false;");

    if (qEnvironmentVariableIsEmpty("HSM_IDE_TEST_SCXML_ROOT")) {
#ifdef HSM_IDE_TEST_SCXML_ROOT_DEFAULT
        qputenv("HSM_IDE_TEST_SCXML_ROOT", QByteArray(HSM_IDE_TEST_SCXML_ROOT_DEFAULT));
#else
        qputenv("HSM_IDE_TEST_SCXML_ROOT", QByteArray("tests/data/scxml"));
#endif
    }

    int result = 0;

    const TestRunner tests[] = {
        runModelTypesTest,
        runStateMachineEntityTest,
        runRegularStateTest,
        runHistoryStateTest,
        runInitialStateTest,
        runFinalStateTest,
        runEntryPointTest,
        runExitPointTest,
        runTransitionTest,
        runModelActionsTest,
        runIncludeEntityTest,
        runModelRootStateTest,
        runModelElementsFactoryTest,
        runStateMachineModelTest,
        runStateHierarchyRulesTest,
        runStateMachineSerializerSerializationTest,
        runStateMachineSerializerDeserializationTest,
        runHsmGraphicsViewTest,
    };

    for (const auto test : tests) {
        if (test != nullptr) {
            result |= test(argc, argv);
        }
    }

    return result;
}
