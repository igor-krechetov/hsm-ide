#include <QByteArray>
#include <QtGlobal>

#include "QtTestCompat.hpp"

int main(int argc, char** argv) {
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (qEnvironmentVariableIsEmpty("HSM_IDE_TEST_SCXML_ROOT")) {
        qputenv("HSM_IDE_TEST_SCXML_ROOT", QByteArray("tests/data/scxml"));
    }

    const int result = qttest_compat::runAllTests();

    return result;
}
