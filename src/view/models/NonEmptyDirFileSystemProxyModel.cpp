#include "NonEmptyDirFileSystemProxyModel.hpp"

#include <QDir>
#include <QFileSystemModel>

namespace view {

NonEmptyDirFileSystemProxyModel::NonEmptyDirFileSystemProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {}

bool NonEmptyDirFileSystemProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    bool result = true;
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    QFileSystemModel* fsm = qobject_cast<QFileSystemModel*>(sourceModel());

    if (nullptr != fsm) {
        const QString path = fsm->filePath(index);
        const bool isDir = fsm->isDir(index);
        const bool isRoot = (source_parent == QModelIndex());

        // Always show files
        // Always show root directories
        if (true == isDir && false == isRoot) {
            QDir dir(path);
            dir.setNameFilters(fsm->nameFilters());
            dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            result = !dir.entryList().isEmpty();
        }
    }

    return result;
}

QString NonEmptyDirFileSystemProxyModel::getFilePath(const QModelIndex& proxyIndex) const {
    QString result;
    QModelIndex srcIdx = mapToSource(proxyIndex);
    const QFileSystemModel* fsm = qobject_cast<const QFileSystemModel*>(sourceModel());

    if (fsm != nullptr && srcIdx.isValid()) {
        if (fsm->isDir(srcIdx) == false) {
            result = fsm->filePath(srcIdx);
        }
    }

    return result;
}

}  // namespace view
