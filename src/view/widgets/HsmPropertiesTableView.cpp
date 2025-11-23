#include "HsmPropertiesTableView.hpp"

#include "private/HsmEntityPropertyDelegate.hpp"

HsmPropertiesTableView::HsmPropertiesTableView(QWidget* parent)
    : QTableView(parent) {
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setItemDelegateForColumn(1, new view::HsmEntityPropertyDelegate(this));
}

void HsmPropertiesTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    // TODO: there is still a weird bug of focus getting stuck if you press up/down while editor for "path" property is open

    if (deselected.isEmpty() == false) {
        // we always have 2 columns and select full row, so editor will be in the last element
        if (QWidget* editor = indexWidget(deselected.indexes().last())) {
            // If an editor exists on the previously selected row → close it
            commitData(editor);
            closeEditor(editor, QAbstractItemDelegate::NoHint);
        }
    }

    QTableView::selectionChanged(selected, deselected);
}

bool HsmPropertiesTableView::edit(const QModelIndex& index, QAbstractItemView::EditTrigger trigger, QEvent* event) {
    const int currentRow = currentIndex().row();
    const int newRow = index.row();

    // Close current editor manually if it exists
    QModelIndex currentIndexSibling = currentIndex().sibling(currentRow, 1);
    QWidget* currentEditor = indexWidget(currentIndexSibling);

    if (nullptr != currentEditor) {
        closeEditor(currentEditor, QAbstractItemDelegate::NoHint);
    }

    // This widget always handles 2-X tables. Editing is possible only in the second column
    // Edit requests in the first column must be redirected
    QModelIndex editableIndex = index.sibling(newRow, 1);  // always column 1

    return QTableView::edit(editableIndex, trigger, event);
}