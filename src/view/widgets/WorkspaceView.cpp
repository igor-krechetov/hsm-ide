#include "WorkspaceView.hpp"

#include <QAbstractItemDelegate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>

#include "view/models/NonEmptyDirFileSystemProxyModel.hpp"

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

    setColumnHidden(1, true);  // Size
    setColumnHidden(2, true);  // Type
    setColumnHidden(3, true);  // Date Modified
    setHeaderHidden(true);

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

    mProxyModel = new view::NonEmptyDirFileSystemProxyModel(this);
    mProxyModel->setSourceModel(mFsModel);
    mProxyModel->setRecursiveFilteringEnabled(true);

    setModel(mProxyModel);
    setRootIndex(mProxyModel->mapFromSource(mFsModel->index(rootDir)));

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
    Q_UNUSED(path);

    if (false == mPendingWorkspaceFilePath.isEmpty()) {
        const QFileInfo oldInfo(mPendingWorkspaceFilePath);
        if (oldInfo.fileName() == oldName) {
            mPendingWorkspaceFilePath = oldInfo.absoluteDir().filePath(newName);
        }
    }
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
