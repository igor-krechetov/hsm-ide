#ifndef TESTS_MODEL_TESTPATHS_HPP
#define TESTS_MODEL_TESTPATHS_HPP

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QDebug>

namespace test {

inline QString scxmlDataRoot() {
    static const QString kRoot = []() {
        QString root;
        const QByteArray envValue = qgetenv("HSM_IDE_TEST_SCXML_ROOT");

        if (envValue.isEmpty() == false) {
            root = QString::fromUtf8(envValue);
        } else {
#ifdef HSM_IDE_TEST_SCXML_ROOT_DEFAULT
            root = QStringLiteral(HSM_IDE_TEST_SCXML_ROOT_DEFAULT);
#else
            root = QStringLiteral("tests/data/scxml");
#endif
        }

        return root;
    }();

    return kRoot;
}

inline QString loadScxmlFixture(const QString& filename) {
    QString content;
    const QString fullPath = scxmlDataRoot() + "/" + filename;
    QFile file(fullPath);

    qDebug() << "loadScxmlFixture: " << fullPath;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        content = QString::fromUtf8(file.readAll());
        file.close();
    }

    return content;
}

}  // namespace test

#endif  // TESTS_MODEL_TESTPATHS_HPP
