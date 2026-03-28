#include "FilteredFileSystemProxyModel.hpp"

#include <QFileInfo>
#include <QFileSystemModel>

namespace view {

FilteredFileSystemProxyModel::FilteredFileSystemProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {}

void FilteredFileSystemProxyModel::setShowEmptyFolders(bool enabled) {
    mShowEmptyFolders = enabled;
    invalidateFilter();
}

bool FilteredFileSystemProxyModel::isShowEmptyFolders() const {
    return mShowEmptyFolders;
}

bool FilteredFileSystemProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    bool result = false;
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    const QFileSystemModel* fsm = qobject_cast<const QFileSystemModel*>(sourceModel());
    const bool isRoot = (source_parent.isValid() == false);

    if ((nullptr != fsm) && index.isValid()) {
        if ((true == fsm->isDir(index)) && (true == isRoot)) {
            result = true;
        } else if (true == fsm->isDir(index)) {
            result = mShowEmptyFolders || hasScxmlRecursively(index);
        } else {
            result = isScxmlFile(fsm->filePath(index));
        }
    }

    return result;
}

QString FilteredFileSystemProxyModel::getFilePath(const QModelIndex& proxyIndex) const {
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

bool FilteredFileSystemProxyModel::isDir(const QModelIndex& proxyIndex) const {
    bool result = false;
    QModelIndex srcIdx = mapToSource(proxyIndex);
    const QFileSystemModel* fsm = qobject_cast<const QFileSystemModel*>(sourceModel());

    if ((nullptr != fsm) && srcIdx.isValid()) {
        result = fsm->isDir(srcIdx);
    }

    return result;
}

bool FilteredFileSystemProxyModel::hasScxmlRecursively(const QModelIndex& sourceIndex) const {
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

bool FilteredFileSystemProxyModel::isScxmlFile(const QString& filePath) const {
    const QFileInfo fileInfo(filePath);
    return (fileInfo.suffix().compare("scxml", Qt::CaseInsensitive) == 0);
}

}  // namespace view
