#include <gtest/gtest.h>

#include <QByteArray>
#include <QtGlobal>

int main(int argc, char** argv) {
    if (qEnvironmentVariableIsEmpty("HSM_IDE_TEST_SCXML_ROOT")) {
        qputenv("HSM_IDE_TEST_SCXML_ROOT", QByteArray("tests/data/scxml"));
    }

    ::testing::InitGoogleTest(&argc, argv);
    const int result = RUN_ALL_TESTS();

    return result;
}
