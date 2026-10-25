#include "ModelUtils.hpp"

#include <QDir>
#include <QFileInfo>

namespace model {

QString resolveIncludePath(const QString& includingFilePath, const QString& includePath) {
    QString resolved;

    if (includePath.isEmpty()) {
        // Nothing to resolve.
        resolved = includePath;
    } else if (QDir::isAbsolutePath(includePath)) {
        // Absolute include path passes through unchanged (mirrors os.path.join with an
        // absolute second argument).
        resolved = QDir::cleanPath(includePath);
    } else if (includingFilePath.isEmpty()) {
        // No including file to resolve against (e.g. untitled project): leave as-is.
        resolved = includePath;
    } else {
        // Relative include path resolves against the directory of the including file.
        const QString includingDir = QFileInfo(includingFilePath).absolutePath();
        resolved = QFileInfo(QDir(includingDir), includePath).absoluteFilePath();
    }

    return resolved;
}

QString sanitiseIdentifier(const QString& input) {
    // remove all characters to make "input" compliant with C++ identifier name rules
    QString out;
    bool firstValidCharFound = false;

    out.reserve(input.length());

    for (const QChar& c : input) {
        if (false == firstValidCharFound) {
            // First valid character must be a letter or underscore
            if (c.isLetter() || c == '_') {
                out.append(c);
                firstValidCharFound = true;
            }
            // Otherwise skip character
        } else {
            // Subsequent characters: letters, digits, underscore
            if (c.isLetterOrNumber() || c == '_') {
                out.append(c);
            }
        }
    }

    return out;
}

void reassignImportedIds(const QSharedPointer<State>& root, EntityIdGenerator& idGenerator) {
    if (root) {
        root->setId(idGenerator.generateNextId());

        root->forEachChildElement(
            [&idGenerator](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> entity) {
                Q_UNUSED(parent);

                if (entity) {
                    entity->setId(idGenerator.generateNextId());
                }

                return true;
            },
            StateMachineEntity::DEPTH_INFINITE,
            false);
    }
}

};  // namespace model