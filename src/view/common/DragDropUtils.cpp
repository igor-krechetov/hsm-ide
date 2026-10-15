#include "DragDropUtils.hpp"

#include <QUrl>

namespace view::DragDropUtils {

QStringList filterScxmlPaths(const QStringList& paths) {
    QStringList scxmlPaths;

    for (const QString& path : paths) {
        if (path.endsWith(QStringLiteral(".scxml"), Qt::CaseInsensitive)) {
            scxmlPaths.append(path);
        }
    }

    return scxmlPaths;
}

bool hasScxml(const QStringList& paths) {
    return filterScxmlPaths(paths).isEmpty() == false;
}

QStringList localPathsFromUrls(const QList<QUrl>& urls) {
    QStringList localPaths;

    for (const QUrl& url : urls) {
        localPaths.append(url.toLocalFile());
    }

    return localPaths;
}

};  // namespace view::DragDropUtils
