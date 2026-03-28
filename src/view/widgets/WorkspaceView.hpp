#ifndef WORKSPACEVIEW_HPP
#define WORKSPACEVIEW_HPP

#include <QTreeView>

class QFileSystemModel;

namespace view {
class NonEmptyDirFileSystemProxyModel;
}

class WorkspaceView : public QTreeView {
    Q_OBJECT

public:
    explicit WorkspaceView(QWidget* parent = nullptr);

    void setWorkspaceRoot(const QString& rootDir);
    void clearWorkspace();

    void setShowEmptyFolders(bool enabled);
    bool showEmptyFolders() const;

    void createWorkspaceFileInSelection();
    void createWorkspaceFolderInSelection();
    void collapseWorkspaceTree();

signals:
    void workspaceFileActivated(const QString& path);

private slots:
    void onContextMenuRequested(const QPoint& pos);
    void onFileRenamed(const QString& path, const QString& oldName, const QString& newName);
    void onEditorClosed(QWidget* editor, QAbstractItemDelegate::EndEditHint hint);
    void onDoubleClickedItem(const QModelIndex& index);

private:
    QModelIndex selectedWorkspaceIndex() const;
    QString selectedWorkspaceDir() const;
    void createWorkspaceFile(const QModelIndex& parentIndex);
    void createWorkspaceFolder(const QModelIndex& parentIndex);
    void renameWorkspaceItem(const QModelIndex& targetIndex);
    void deleteWorkspaceItem(const QModelIndex& targetIndex);
    void finalizePendingWorkspaceFile();

private:
    QString mWorkspaceRootDir;
    QString mPendingWorkspaceFilePath;
    QFileSystemModel* mFsModel = nullptr;
    view::NonEmptyDirFileSystemProxyModel* mProxyModel = nullptr;
};

#endif  // WORKSPACEVIEW_HPP
