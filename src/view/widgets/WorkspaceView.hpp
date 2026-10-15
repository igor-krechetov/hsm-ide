#ifndef WORKSPACEVIEW_HPP
#define WORKSPACEVIEW_HPP

#include <QTreeView>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QFileSystemModel;

namespace view {
class FilteredFileSystemProxyModel;
}

class WorkspaceView;

// Test-linkage hook: external-linkage forwarder to WorkspaceView's private
// resolveDropTargetFolder, used by the view property tests (see tests/view).
// Mirrors the wsview_test_hooks pattern used for the pure collision helpers;
// production code never calls this.
namespace wsview_test_hooks {
QString resolveDropTargetFolder(const WorkspaceView& view, const QModelIndex& dropProxyIndex);
QModelIndex targetFolderProxyIndex(const WorkspaceView& view, const QPoint& viewportPos);
void copyDroppedFileIntoFolder(WorkspaceView& view, const QString& sourcePath, const QString& targetDir);
}  // namespace wsview_test_hooks

class WorkspaceView : public QTreeView {
    Q_OBJECT

    friend QString wsview_test_hooks::resolveDropTargetFolder(const WorkspaceView& view, const QModelIndex& dropProxyIndex);
    friend QModelIndex wsview_test_hooks::targetFolderProxyIndex(const WorkspaceView& view, const QPoint& viewportPos);
    friend void wsview_test_hooks::copyDroppedFileIntoFolder(WorkspaceView& view,
                                                             const QString& sourcePath,
                                                             const QString& targetDir);

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
    void workspacePathRenamed(const QString& oldPath, const QString& newPath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onContextMenuRequested(const QPoint& pos);
    void onFileRenamed(const QString& path, const QString& oldName, const QString& newName);
    void onEditorClosed(QWidget* editor, QAbstractItemDelegate::EndEditHint hint);
    void onDoubleClickedItem(const QModelIndex& index);

private:
    void applyColumnVisibility();
    QModelIndex selectedWorkspaceIndex() const;
    QString selectedWorkspaceDir() const;
    QString resolveDropTargetFolder(const QModelIndex& dropProxyIndex) const;
    QModelIndex targetFolderProxyIndex(const QPoint& viewportPos) const;
    void highlightTargetFolder(const QPoint& viewportPos);
    void clearDragHighlight();
    void copyDroppedFileIntoFolder(const QString& sourcePath, const QString& targetDir);
    void createWorkspaceFile(const QModelIndex& parentIndex);
    void createWorkspaceFolder(const QModelIndex& parentIndex);
    void renameWorkspaceItem(const QModelIndex& targetIndex);
    void deleteWorkspaceItem(const QModelIndex& targetIndex);
    void finalizePendingWorkspaceFile();

private:
    QString mWorkspaceRootDir;
    QString mPendingWorkspaceFilePath;
    QFileSystemModel* mFsModel = nullptr;
    view::FilteredFileSystemProxyModel* mProxyModel = nullptr;
    bool mDragHighlightActive = false;
};

#endif  // WORKSPACEVIEW_HPP
