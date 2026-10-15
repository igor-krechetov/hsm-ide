#ifndef DRAGDROPUTILS_HPP
#define DRAGDROPUTILS_HPP

#include <QList>
#include <QStringList>

class QUrl;

namespace view::DragDropUtils {

// Pure: keep only .scxml paths (case-insensitive), preserving order.
QStringList filterScxmlPaths(const QStringList& paths);

// Pure: true if at least one path ends with .scxml (case-insensitive).
bool hasScxml(const QStringList& paths);

// Pure: convert a drag's URLs to local file paths (preserving order).
QStringList localPathsFromUrls(const QList<QUrl>& urls);

};  // namespace view::DragDropUtils

#endif  // DRAGDROPUTILS_HPP
