#include "HsmPropertiesTableView.hpp"

#include "private/HsmEntityPropertyDelegate.hpp"

HsmPropertiesTableView::HsmPropertiesTableView(QWidget* parent)
    : QTreeView(parent) {
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setRootIsDecorated(true);
    setItemsExpandable(true);
    setUniformRowHeights(true);

    setItemDelegateForColumn(1, new view::HsmEntityPropertyDelegate(this));
}

void HsmPropertiesTableView::setModel(QAbstractItemModel* model) {
    if (QAbstractItemModel* oldModel = QTreeView::model()) {
        disconnect(oldModel, nullptr, this, nullptr);
    }

    QTreeView::setModel(model);

    if (model) {
        connect(model, &QAbstractItemModel::modelReset, this, [this]() { expandAll(); });
        connect(model, &QAbstractItemModel::rowsInserted, this, [this]() { expandAll(); });
    }

    expandAll();
}

void HsmPropertiesTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    if (deselected.isEmpty() == false) {
        if (QWidget* editor = indexWidget(deselected.indexes().last())) {
            commitData(editor);
            closeEditor(editor, QAbstractItemDelegate::NoHint);
        }
    }

    QTreeView::selectionChanged(selected, deselected);
}

bool HsmPropertiesTableView::edit(const QModelIndex& index, QAbstractItemView::EditTrigger trigger, QEvent* event) {
    const QModelIndex currentValueIndex = currentIndex().sibling(currentIndex().row(), 1);
    QWidget* currentEditor = indexWidget(currentValueIndex);

    if (nullptr != currentEditor) {
        closeEditor(currentEditor, QAbstractItemDelegate::NoHint);
    }

    QModelIndex editableIndex = index;

    if (index.isValid()) {
        editableIndex = index.sibling(index.row(), 1);
    }

    return QTreeView::edit(editableIndex, trigger, event);
}
