#include "HsmTreeView.hpp"

#include <QAbstractItemModel>
#include <QDebug>

HsmTreeView::HsmTreeView(QWidget* parent)
    : QTreeView(parent) {}

void HsmTreeView::setModel(QAbstractItemModel* model) {
    // Disconnect from old model
    if (QAbstractItemModel* oldModel = this->model()) {
        disconnect(oldModel, &QAbstractItemModel::modelReset, this, &HsmTreeView::onModelReset);
        disconnect(oldModel, &QAbstractItemModel::rowsInserted, this, &HsmTreeView::onModelRowsInserted);
        disconnect(oldModel, &QAbstractItemModel::modelAboutToBeReset, this, &HsmTreeView::onModelAboutToBeReset);
    }

    // Set new model
    QTreeView::setModel(model);

    // Connect to new model signals
    if (model) {
        connect(model, &QAbstractItemModel::modelReset, this, &HsmTreeView::onModelReset);
        connect(model, &QAbstractItemModel::rowsInserted, this, &HsmTreeView::onModelRowsInserted);
        connect(model, &QAbstractItemModel::modelAboutToBeReset, this, &HsmTreeView::onModelAboutToBeReset);
    }
}

void HsmTreeView::onModelReset() {
    qDebug() << "------------- HsmTreeView::onModelReset";
    restoreExpandedState();
}

void HsmTreeView::onModelAboutToBeReset() {
    qDebug() << "------------- HsmTreeView::onModelAboutToBeReset";
    saveExpandedState();
}

void HsmTreeView::onModelRowsInserted(const QModelIndex& parent, int first, int last) {
    Q_UNUSED(first);
    Q_UNUSED(last);

    // When new rows are inserted, check if we need to expand the parent
    if (!mRestoringState && mExpandedPaths.contains(getIndexPath(parent))) {
        expand(parent);
    }
}

void HsmTreeView::saveExpandedState() {
    qDebug() << Q_FUNC_INFO;
    mExpandedPaths.clear();

    if (!model()) {
        return;
    }

    // Recursively save expanded state for all items
    std::function<void(const QModelIndex&)> saveExpanded;
    saveExpanded = [&](const QModelIndex& index) {
        if (isExpanded(index)) {
            mExpandedPaths.insert(getIndexPath(index));
        }

        // Recursively process children
        int rowCount = model()->rowCount(index);
        for (int row = 0; row < rowCount; ++row) {
            QModelIndex childIndex = model()->index(row, 0, index);
            if (childIndex.isValid()) {
                saveExpanded(childIndex);
            }
        }
    };

    // Start from root
    saveExpanded(QModelIndex());
}

void HsmTreeView::restoreExpandedState() {
    qDebug() << Q_FUNC_INFO;
    if (!model() || mExpandedPaths.isEmpty() || mRestoringState) {
        return;
    }

    mRestoringState = true;

    // Recursively restore expanded state
    std::function<void(const QModelIndex&)> restoreExpanded;
    restoreExpanded = [&](const QModelIndex& index) {
        QString path = getIndexPath(index);
        if (mExpandedPaths.contains(path)) {
            expand(index);
        }

        // Recursively process children
        int rowCount = model()->rowCount(index);
        for (int row = 0; row < rowCount; ++row) {
            QModelIndex childIndex = model()->index(row, 0, index);
            if (childIndex.isValid()) {
                restoreExpanded(childIndex);
            }
        }
    };

    // Start from root
    restoreExpanded(QModelIndex());

    mRestoringState = false;
}

QString HsmTreeView::getIndexPath(const QModelIndex& index) const {
    if (!model() || !index.isValid()) {
        return "root";
    }

    QString path;
    QModelIndex current = index;

    while (current.isValid()) {
        path.prepend(QString::number(current.row()));
        path.prepend("/");
        current = current.parent();
    }

    return path;
}

QModelIndex HsmTreeView::findIndexByPath(const QString& path) const {
    if (!model() || path == "root") {
        return QModelIndex();
    }

    QStringList parts = path.split('/', Qt::SkipEmptyParts);
    QModelIndex currentIndex;

    for (const QString& rowStr : parts) {
        bool ok;
        int row = rowStr.toInt(&ok);
        if (!ok || row < 0) {
            return QModelIndex();
        }

        currentIndex = model()->index(row, 0, currentIndex);
        if (!currentIndex.isValid()) {
            return QModelIndex();
        }
    }

    return currentIndex;
}
