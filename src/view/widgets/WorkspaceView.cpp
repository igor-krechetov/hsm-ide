#include "WorkspaceView.hpp"

#include <QAbstractItemDelegate>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>

#include "view/models/FilteredFileSystemProxyModel.hpp"

namespace {

// Pure: a file name collides iff a file with that name already exists in targetDir.
bool nameCollides(const QString& targetDir, const QString& fileName) {
    return QFileInfo::exists(QDir(targetDir).filePath(fileName));
}

// Pure: produce a name that does not collide in targetDir, preserving the extension.
// Mirrors the counter pattern used by createWorkspaceFile: name.ext, name (1).ext, name (2).ext, ...
QString makeNonCollidingName(const QString& targetDir, const QString& fileName) {
    const QFileInfo fileInfo(fileName);
    const QString baseName = fileInfo.completeBaseName();
    const QString suffix = fileInfo.suffix();

    QString candidate = fileName;
    int counter = 1;
    while (nameCollides(targetDir, candidate)) {
        if (suffix.isEmpty()) {
            candidate = QString("%1 (%2)").arg(baseName).arg(counter);
        } else {
            candidate = QString("%1 (%2).%3").arg(baseName).arg(counter).arg(suffix);
        }
        counter += 1;
    }

    return candidate;
}

}  // namespace

// Test-linkage hooks: external-linkage forwarders to the file-local pure helpers above,
// mirroring how view tests reach pure view helpers (see tests/view). Not part of the
// public widget API; declared only in the test translation units.
namespace wsview_test_hooks {

bool nameCollides(const QString& targetDir, const QString& fileName) {
    return ::nameCollides(targetDir, fileName);
}

QString makeNonCollidingName(const QString& targetDir, const QString& fileName) {
    return ::makeNonCollidingName(targetDir, fileName);
}

QString resolveDropTargetFolder(const WorkspaceView& view, const QModelIndex& dropProxyIndex) {
    return view.resolveDropTargetFolder(dropProxyIndex);
}

QModelIndex targetFolderProxyIndex(const WorkspaceView& view, const QPoint& viewportPos) {
    return view.targetFolderProxyIndex(viewportPos);
}

void copyDroppedFileIntoFolder(WorkspaceView& view, const QString& sourcePath, const QString& targetDir) {
    view.copyDroppedFileIntoFolder(sourcePath, targetDir);
}

}  // namespace wsview_test_hooks

WorkspaceView::WorkspaceView(QWidget* parent)
    : QTreeView(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);

    applyColumnVisibility();

    connect(this, &QWidget::customContextMenuRequested, this, &WorkspaceView::onContextMenuRequested);
    connect(this, &QTreeView::doubleClicked, this, &WorkspaceView::onDoubleClickedItem);
    connect(itemDelegate(), &QAbstractItemDelegate::closeEditor, this, &WorkspaceView::onEditorClosed);
}

void WorkspaceView::setWorkspaceRoot(const QString& rootDir) {
    finalizePendingWorkspaceFile();

    mWorkspaceRootDir = rootDir;

    if (nullptr != mProxyModel) {
        mProxyModel->deleteLater();
        mProxyModel = nullptr;
    }

    if (nullptr != mFsModel) {
        mFsModel->deleteLater();
        mFsModel = nullptr;
    }

    mFsModel = new QFileSystemModel(this);
    mFsModel->setRootPath(rootDir);
    mFsModel->setReadOnly(false);

    mProxyModel = new view::FilteredFileSystemProxyModel(this);
    mProxyModel->setSourceModel(mFsModel);
    mProxyModel->setRecursiveFilteringEnabled(true);

    setModel(mProxyModel);
    setRootIndex(mProxyModel->mapFromSource(mFsModel->index(rootDir)));
    applyColumnVisibility();

    connect(mFsModel, &QFileSystemModel::fileRenamed, this, &WorkspaceView::onFileRenamed);
}

void WorkspaceView::clearWorkspace() {
    finalizePendingWorkspaceFile();
    setModel(nullptr);
    mWorkspaceRootDir.clear();
    mProxyModel = nullptr;
    mFsModel = nullptr;
}

void WorkspaceView::setShowEmptyFolders(bool enabled) {
    if (nullptr != mProxyModel) {
        mProxyModel->setShowEmptyFolders(enabled);
    }
}

bool WorkspaceView::showEmptyFolders() const {
    bool result = false;

    if (nullptr != mProxyModel) {
        result = mProxyModel->isShowEmptyFolders();
    }

    return result;
}

void WorkspaceView::createWorkspaceFileInSelection() {
    createWorkspaceFile(selectedWorkspaceIndex());
}

void WorkspaceView::createWorkspaceFolderInSelection() {
    createWorkspaceFolder(selectedWorkspaceIndex());
}

void WorkspaceView::collapseWorkspaceTree() {
    collapseAll();
}

void WorkspaceView::onContextMenuRequested(const QPoint& pos) {
    const QModelIndex clickedIndex = indexAt(pos);
    QMenu menu(this);

    QAction* actionNewFile = menu.addAction(tr("New File"));
    QAction* actionNewFolder = menu.addAction(tr("New Folder"));
    QAction* actionRename = menu.addAction(tr("Rename"));
    QAction* actionDelete = menu.addAction(tr("Delete"));

    if (false == clickedIndex.isValid()) {
        actionRename->setEnabled(false);
        actionDelete->setEnabled(false);
    }

    QAction* selectedAction = menu.exec(viewport()->mapToGlobal(pos));
    if (selectedAction == actionNewFile) {
        createWorkspaceFile(clickedIndex);
    } else if (selectedAction == actionNewFolder) {
        createWorkspaceFolder(clickedIndex);
    } else if (selectedAction == actionRename) {
        renameWorkspaceItem(clickedIndex);
    } else if (selectedAction == actionDelete) {
        deleteWorkspaceItem(clickedIndex);
    }
}

void WorkspaceView::onFileRenamed(const QString& path, const QString& oldName, const QString& newName) {
    const QString oldPath = QDir(path).absoluteFilePath(oldName);
    const QString newPath = QDir(path).absoluteFilePath(newName);

    if (false == mPendingWorkspaceFilePath.isEmpty()) {
        const QFileInfo oldInfo(mPendingWorkspaceFilePath);
        if (oldInfo.fileName() == oldName) {
            mPendingWorkspaceFilePath = oldInfo.absoluteDir().filePath(newName);
        }
    }

    emit workspacePathRenamed(oldPath, newPath);
}

void WorkspaceView::onEditorClosed(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) {
    Q_UNUSED(editor);
    Q_UNUSED(hint);
    finalizePendingWorkspaceFile();
}

void WorkspaceView::onDoubleClickedItem(const QModelIndex& index) {
    QString filePath;

    if ((nullptr != mProxyModel) && index.isValid()) {
        filePath = mProxyModel->getFilePath(index);
    }

    if (false == filePath.isEmpty()) {
        emit workspaceFileActivated(filePath);
    }
}

QModelIndex WorkspaceView::selectedWorkspaceIndex() const {
    QModelIndex result;
    const QItemSelectionModel* selectionModel = this->selectionModel();

    if (nullptr != selectionModel) {
        result = selectionModel->currentIndex();
    }

    return result;
}

QString WorkspaceView::selectedWorkspaceDir() const {
    QString result = mWorkspaceRootDir;
    const QModelIndex selectedIndex = selectedWorkspaceIndex();

    if ((nullptr != mProxyModel) && (nullptr != mFsModel) && selectedIndex.isValid()) {
        const QModelIndex sourceIndex = mProxyModel->mapToSource(selectedIndex);
        if (mFsModel->isDir(sourceIndex)) {
            result = mFsModel->filePath(sourceIndex);
        } else {
            result = QFileInfo(mFsModel->filePath(sourceIndex)).absolutePath();
        }
    }

    return result;
}

QString WorkspaceView::resolveDropTargetFolder(const QModelIndex& dropProxyIndex) const {
    QString result;

    if ((nullptr != mProxyModel) && (nullptr != mFsModel) && (false == mWorkspaceRootDir.isEmpty())) {
        if (false == dropProxyIndex.isValid()) {
            result = mWorkspaceRootDir;
        } else {
            const QModelIndex sourceIndex = mProxyModel->mapToSource(dropProxyIndex);
            if (mFsModel->isDir(sourceIndex)) {
                result = mFsModel->filePath(sourceIndex);
            } else {
                result = QFileInfo(mFsModel->filePath(sourceIndex)).absolutePath();
            }
        }
    }

    return result;
}

QModelIndex WorkspaceView::targetFolderProxyIndex(const QPoint& viewportPos) const {
    QModelIndex result;

    const QString targetDir = resolveDropTargetFolder(indexAt(viewportPos));
    if ((nullptr != mProxyModel) && (nullptr != mFsModel) && (false == targetDir.isEmpty())) {
        result = mProxyModel->mapFromSource(mFsModel->index(targetDir));  // Req 1.1/1.2
    }

    return result;  // Req 1.5: invalid when no workspace is loaded
}

void WorkspaceView::highlightTargetFolder(const QPoint& viewportPos) {
    const QModelIndex targetIndex = targetFolderProxyIndex(viewportPos);

    if (targetIndex.isValid()) {
        setCurrentIndex(targetIndex);  // Req 1.1/1.2
        mDragHighlightActive = true;
    } else {
        clearDragHighlight();  // Req 1.5
    }
}

void WorkspaceView::clearDragHighlight() {
    if (mDragHighlightActive) {
        setCurrentIndex(QModelIndex());
        mDragHighlightActive = false;
    }
}

void WorkspaceView::dragEnterEvent(QDragEnterEvent* event) {
    if ((nullptr != event->mimeData()) && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        highlightTargetFolder(event->position().toPoint());  // Req 1.1
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

void WorkspaceView::dragMoveEvent(QDragMoveEvent* event) {
    if ((nullptr != event->mimeData()) && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        highlightTargetFolder(event->position().toPoint());  // Req 1.2
    } else {
        QTreeView::dragMoveEvent(event);
    }
}

void WorkspaceView::dragLeaveEvent(QDragLeaveEvent* event) {
    clearDragHighlight();  // Req 1.3
    QTreeView::dragLeaveEvent(event);
}

void WorkspaceView::dropEvent(QDropEvent* event) {
    if ((nullptr != event->mimeData()) && event->mimeData()->hasUrls()) {
        const QString targetDir = resolveDropTargetFolder(indexAt(event->position().toPoint()));

        if (targetDir.isEmpty()) {
            // Req 1.3: no workspace loaded -> ignore the external drop.
            clearDragHighlight();  // Req 1.4
            event->ignore();
        } else {
            const QList<QUrl> urls = event->mimeData()->urls();
            for (const QUrl& url : urls) {
                const QString localPath = url.toLocalFile();
                if ((false == localPath.isEmpty()) && QFileInfo(localPath).isFile()) {
                    copyDroppedFileIntoFolder(localPath, targetDir);  // Req 1.1/1.2, 4.1
                }
            }
            clearDragHighlight();  // Req 1.4
            event->acceptProposedAction();
        }
    } else {
        // Preserve the existing InternalMove behavior for non-URL (internal) drops.
        QTreeView::dropEvent(event);
    }
}

void WorkspaceView::copyDroppedFileIntoFolder(const QString& sourcePath, const QString& targetDir) {
    const QString destName = QFileInfo(sourcePath).fileName();
    QString destPath = QDir(targetDir).filePath(destName);
    bool skip = false;

    if (nameCollides(targetDir, destName)) {  // Req 2.1
        QMessageBox prompt(this);
        prompt.setIcon(QMessageBox::Question);
        prompt.setWindowTitle(tr("File Exists"));
        prompt.setText(tr("A file named '%1' already exists in the target folder.").arg(destName));
        
        QPushButton* overwriteButton = prompt.addButton(tr("Overwrite"), QMessageBox::AcceptRole);
        QPushButton* renameButton = prompt.addButton(tr("Rename"), QMessageBox::ActionRole);
        QPushButton* cancelButton = prompt.addButton(QMessageBox::Cancel);

        prompt.setDefaultButton(cancelButton);
        prompt.exec();

        if (prompt.clickedButton() == overwriteButton) {
            QFile::remove(destPath);  // Req 2.2: replace existing, so QFile::copy can succeed
        } else if (prompt.clickedButton() == renameButton) {
            destPath = QDir(targetDir).filePath(makeNonCollidingName(targetDir, destName));  // Req 2.3
        } else {
            skip = true;  // Req 2.4: Cancel -> leave existing untouched, skip this file
        }
    }

    if (false == skip) {
        if (false == QFile::copy(sourcePath, destPath)) {  // Req 1.1/1.2
            QMessageBox::warning(this, tr("Copy Failed"), tr("Failed to copy '%1'.").arg(destName));
        }
    }
}

void WorkspaceView::createWorkspaceFile(const QModelIndex& parentIndex) {
    QString parentDir = selectedWorkspaceDir();

    if ((nullptr != mProxyModel) && (nullptr != mFsModel) && (parentIndex.isValid())) {
        const QModelIndex sourceIndex = mProxyModel->mapToSource(parentIndex);
        if (false == mFsModel->isDir(sourceIndex)) {
            parentDir = QFileInfo(mFsModel->filePath(sourceIndex)).absolutePath();
        }
    }

    if ((nullptr != mFsModel) && (false == parentDir.isEmpty())) {
        finalizePendingWorkspaceFile();

        QString filePath = QDir(parentDir).filePath("__new_file__.scxml");
        int counter = 1;
        while (QFileInfo::exists(filePath)) {
            filePath = QDir(parentDir).filePath(QString("__new_file__%1.scxml").arg(counter));
            counter += 1;
        }

        QFile newFile(filePath);
        if (newFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            newFile.close();
            mPendingWorkspaceFilePath = filePath;
            const QModelIndex newSourceIndex = mFsModel->index(filePath);
            const QModelIndex newProxyIndex = mProxyModel->mapFromSource(newSourceIndex);
            setCurrentIndex(newProxyIndex);
            edit(newProxyIndex);
        }
    }
}

void WorkspaceView::createWorkspaceFolder(const QModelIndex& parentIndex) {
    QString folderName = QInputDialog::getText(this, tr("Create Folder"), tr("Folder Name"));
    QString parentDir = selectedWorkspaceDir();

    if ((nullptr != mProxyModel) && (nullptr != mFsModel) && parentIndex.isValid()) {
        const QModelIndex sourceIndex = mProxyModel->mapToSource(parentIndex);
        if (false == mFsModel->isDir(sourceIndex)) {
            parentDir = QFileInfo(mFsModel->filePath(sourceIndex)).absolutePath();
        }
    }

    if ((nullptr != mFsModel) && (false == parentDir.isEmpty()) && (false == folderName.trimmed().isEmpty())) {
        const QModelIndex parentSourceIndex = mFsModel->index(parentDir);
        const QModelIndex createdIndex = mFsModel->mkdir(parentSourceIndex, folderName.trimmed());

        if (createdIndex.isValid()) {
            const QModelIndex proxyIndex = mProxyModel->mapFromSource(createdIndex);
            setCurrentIndex(proxyIndex);
        }
    }
}

void WorkspaceView::renameWorkspaceItem(const QModelIndex& targetIndex) {
    if (targetIndex.isValid()) {
        setCurrentIndex(targetIndex);
        edit(targetIndex);
    }
}

void WorkspaceView::deleteWorkspaceItem(const QModelIndex& targetIndex) {
    if ((nullptr != mProxyModel) && (nullptr != mFsModel) && targetIndex.isValid()) {
        const QModelIndex sourceIndex = mProxyModel->mapToSource(targetIndex);
        const QString itemPath = mFsModel->filePath(sourceIndex);
        const QFileInfo itemInfo(itemPath);
        const auto answer = QMessageBox::question(this,
                                                  tr("Delete"),
                                                  tr("Delete '%1'?").arg(itemInfo.fileName()),
                                                  QMessageBox::Yes | QMessageBox::No);

        if (QMessageBox::Yes == answer) {
            bool isRemoved = false;

            if (itemInfo.isDir()) {
                QDir dir(itemPath);
                isRemoved = dir.removeRecursively();
            } else {
                isRemoved = mFsModel->remove(sourceIndex);
            }

            if (false == isRemoved) {
                QMessageBox::warning(this, tr("Delete Failed"), tr("Failed to delete '%1'.").arg(itemInfo.fileName()));
            }
        }
    }
}

void WorkspaceView::finalizePendingWorkspaceFile() {
    if (false == mPendingWorkspaceFilePath.isEmpty()) {
        const QFileInfo fileInfo(mPendingWorkspaceFilePath);
        const bool keepFile = fileInfo.exists() &&
                              (QRegularExpression("^__new_file__\\d*\\.scxml$").match(fileInfo.fileName()).hasMatch() == false);

        if (false == keepFile) {
            QFile::remove(mPendingWorkspaceFilePath);
        }

        mPendingWorkspaceFilePath.clear();
    }
}

void WorkspaceView::applyColumnVisibility() {
    if (nullptr != model()) {
        for (int column = 1; column < model()->columnCount(); ++column) {
            setColumnHidden(column, true);
        }
    }
    setHeaderHidden(true);
}
