#include "NonEmptyDirFileSystemProxyModel.hpp"

#include <QFileInfo>
#include <QFileSystemModel>

namespace view {

NonEmptyDirFileSystemProxyModel::NonEmptyDirFileSystemProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {}

void NonEmptyDirFileSystemProxyModel::setShowEmptyFolders(bool enabled) {
    mShowEmptyFolders = enabled;
    invalidateFilter();
}

bool NonEmptyDirFileSystemProxyModel::isShowEmptyFolders() const {
    return mShowEmptyFolders;
}

bool NonEmptyDirFileSystemProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    bool result = false;
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    const QFileSystemModel* fsm = qobject_cast<const QFileSystemModel*>(sourceModel());

    if ((nullptr != fsm) && index.isValid()) {
        if (true == fsm->isDir(index)) {
            result = mShowEmptyFolders || hasScxmlRecursively(index);
        } else {
            result = isScxmlFile(fsm->filePath(index));
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

bool NonEmptyDirFileSystemProxyModel::isDir(const QModelIndex& proxyIndex) const {
    bool result = false;
    QModelIndex srcIdx = mapToSource(proxyIndex);
    const QFileSystemModel* fsm = qobject_cast<const QFileSystemModel*>(sourceModel());

    if ((nullptr != fsm) && srcIdx.isValid()) {
        result = fsm->isDir(srcIdx);
    }

    return result;
}

bool NonEmptyDirFileSystemProxyModel::hasScxmlRecursively(const QModelIndex& sourceIndex) const {
    bool result = false;
    const QFileSystemModel* fsm = qobject_cast<const QFileSystemModel*>(sourceModel());

    if ((nullptr != fsm) && sourceIndex.isValid()) {
        const int rowCount = fsm->rowCount(sourceIndex);
        for (int row = 0; (row < rowCount) && (result == false); ++row) {
            QModelIndex childIndex = fsm->index(row, 0, sourceIndex);

            if (fsm->isDir(childIndex)) {
                result = hasScxmlRecursively(childIndex);
            } else {
                result = isScxmlFile(fsm->filePath(childIndex));
            }
        }
    }

    return result;
}

bool NonEmptyDirFileSystemProxyModel::isScxmlFile(const QString& filePath) const {
    const QFileInfo fileInfo(filePath);
    return (fileInfo.suffix().compare("scxml", Qt::CaseInsensitive) == 0);
}

}  // namespace view
