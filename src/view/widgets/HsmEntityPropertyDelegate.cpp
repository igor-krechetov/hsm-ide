#include "HsmEntityPropertyDelegate.hpp"

#include <QComboBox>
#include <QDebug>
#include <QVariant>

#include "model/Transition.hpp"

namespace view {

HsmEntityPropertyDelegate::HsmEntityPropertyDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

QWidget* HsmEntityPropertyDelegate::createEditor(QWidget* parent,
                                                 const QStyleOptionViewItem& option,
                                                 const QModelIndex& index) const {
    if (index.data(Qt::UserRole).toString() == model::Transition::cKeyTransitionType) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem(model::transitionTypeToString(model::TransitionType::EXTERNAL),
                       static_cast<int>(model::TransitionType::EXTERNAL));
        combo->addItem(model::transitionTypeToString(model::TransitionType::INTERNAL),
                       static_cast<int>(model::TransitionType::INTERNAL));
        return combo;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void HsmEntityPropertyDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    // TODO: should we rely on widget casting or UserRole from index?
    if (QComboBox* combo = qobject_cast<QComboBox*>(editor)) {
        int idx = combo->findData(index.data(Qt::EditRole).toInt(), Qt::UserRole);

        // TODO: use user data instead of index
        if (idx >= 0) {
            combo->setCurrentIndex(idx);
        } else {
            combo->setCurrentIndex(0);
        }
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void HsmEntityPropertyDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    if (QComboBox* combo = qobject_cast<QComboBox*>(editor)) {
        model->setData(index, combo->currentData(Qt::UserRole), Qt::EditRole);
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void HsmEntityPropertyDelegate::updateEditorGeometry(QWidget* editor,
                                                     const QStyleOptionViewItem& option,
                                                     const QModelIndex& index) const {
    editor->setGeometry(option.rect);
}

};  // namespace view