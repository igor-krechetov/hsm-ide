#include "FilteredFileSystemProxyModel.hpp"

#include <QStack>
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
    const QString entryPath = fsm->filePath(index);

    // Filter out anything not under the root path
    if (false == entryPath.startsWith(fsm->rootPath())) {
        return false;
    }

    if (entryPath == fsm->rootPath()) {
        return true;
    }

    const bool isRoot = (source_parent.isValid() == false);

    if ((nullptr != fsm) && index.isValid()) {
        if ((true == fsm->isDir(index)) && (true == isRoot)) {
            result = true;
        } else if (true == fsm->isDir(index)) {
            result = mShowEmptyFolders || hasScxmlFiles(entryPath);
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

bool FilteredFileSystemProxyModel::hasScxmlFiles(const QString& path) const {
    bool found = false;
    QDir dir(path);

    if (dir.exists()) {
        QStack<QDir> stack;

        stack.push(dir);

        while (!stack.isEmpty() && !found) {
            QDir currentDir = stack.pop();

            // Check for files in the current directory
            QFileInfoList files = currentDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
            for (const QFileInfo& fileInfo : files) {
                if (isScxmlFile(fileInfo.absoluteFilePath())) {
                    found = true;
                    break;
                }
            }

            // Add subdirectories to the stack
            if (false == found) {
                QFileInfoList subdirs = currentDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QFileInfo& subdirInfo : subdirs) {
                    stack.push(QDir(subdirInfo.absoluteFilePath()));
                }
            } else {
                break;
            }
        }
    }

    return found;
}

bool FilteredFileSystemProxyModel::isScxmlFile(const QString& filePath) const {
    const QFileInfo fileInfo(filePath);
    return (fileInfo.suffix().compare("scxml", Qt::CaseInsensitive) == 0);
}

}  // namespace view
