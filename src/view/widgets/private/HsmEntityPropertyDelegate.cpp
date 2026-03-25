#include "HsmEntityPropertyDelegate.hpp"

#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVariant>

#include "model/HistoryState.hpp"
#include "model/Transition.hpp"
#include "model/actions/ModelActionFactory.hpp"
#include "view/models/StateMachineEntityViewModel.hpp"

namespace view {

HsmEntityPropertyDelegate::HsmEntityPropertyDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
    connect(this, &HsmEntityPropertyDelegate::finishEditing, this, &HsmEntityPropertyDelegate::onEditingFinished);
}

QWidget* HsmEntityPropertyDelegate::createEditor(QWidget* parent,
                                                 const QStyleOptionViewItem& option,
                                                 const QModelIndex& index) const {
    if (index.data(view::StateMachineEntityViewModel::ActionSubtypeRole).isValid()) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem("None");

        for (const auto& actionName : model::supportedActionNames()) {
            combo->addItem(actionName);
        }

        return combo;
    } else if (index.data(view::StateMachineEntityViewModel::PropertyKeyRole).toString() ==
               model::Transition::cKeyTransitionType) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem(model::transitionTypeToString(model::TransitionType::EXTERNAL),
                       static_cast<int>(model::TransitionType::EXTERNAL));
        combo->addItem(model::transitionTypeToString(model::TransitionType::INTERNAL),
                       static_cast<int>(model::TransitionType::INTERNAL));

        return combo;
    } else if (index.data(view::StateMachineEntityViewModel::PropertyKeyRole).toString() ==
               model::Transition::cKeyExpectedConditionValue) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem("true", true);
        combo->addItem("false", false);

        return combo;
    } else if (index.data(view::StateMachineEntityViewModel::PropertyPathRole).toString().endsWith(".singleshot")) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem("true", true);
        combo->addItem("false", false);

        return combo;
    } else if (index.data(view::StateMachineEntityViewModel::PropertyKeyRole).toString() ==
               model::HistoryState::cKeyHistoryType) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem(model::historyTypeToString(model::HistoryType::SHALLOW), static_cast<int>(model::HistoryType::SHALLOW));
        combo->addItem(model::historyTypeToString(model::HistoryType::DEEP), static_cast<int>(model::HistoryType::DEEP));

        return combo;
    } else if (index.data(view::StateMachineEntityViewModel::PropertyKeyRole).toString() == "path") {
        // Use a line edit + button inside a widget
        QWidget* editor = new QWidget(parent);
        auto* layout = new QHBoxLayout(editor);
        layout->setContentsMargins(0, 0, 0, 0);

        auto* line = new QLineEdit(editor);
        auto* btn = new QPushButton("...", editor);
        btn->setFixedWidth(28);

        layout->addWidget(line);
        layout->addWidget(btn);

        // When the user clicks the button → open file dialog
        connect(btn, &QPushButton::clicked, this, [&, line, editor]() mutable {
            QString currentDirectory;
            const QString currentPath = line->text();

            if (currentPath.isEmpty() == false) {
                QFileInfo info(currentPath);

                currentDirectory = info.absolutePath();
            }

            const QString file = QFileDialog::getOpenFileName(editor, "Select File", currentDirectory);

            if (!file.isEmpty()) {
                line->setText(file);
                emit finishEditing(editor);
            }
        });

        return editor;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void HsmEntityPropertyDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    // TODO: should we rely on widget casting or UserRole from index?
    if (index.data(view::StateMachineEntityViewModel::PropertyKeyRole).toString() == "path") {
        QLineEdit* line = editor->findChild<QLineEdit*>();

        if (line) {
            line->setText(index.data(Qt::EditRole).toString());
            line->setFocus();
        } else {
            qFatal("HsmEntityPropertyDelegate: unexpected editor widget");
        }
    } else if (QComboBox* combo = qobject_cast<QComboBox*>(editor)) {
        if (index.data(view::StateMachineEntityViewModel::ActionSubtypeRole).isValid()) {
            int idx = combo->findText(index.data(Qt::EditRole).toString());

            if (idx >= 0) {
                combo->setCurrentIndex(idx);
            } else {
                combo->setCurrentIndex(0);
            }
        } else if (index.data(view::StateMachineEntityViewModel::PropertyPathRole).toString().endsWith(".singleshot")) {
            const QString valueText = index.data(Qt::EditRole).toString().trimmed().toLower();
            const bool boolValue = (valueText == "1" || valueText == "true");
            int idx = combo->findData(boolValue, Qt::UserRole);

            if (idx >= 0) {
                combo->setCurrentIndex(idx);
            } else {
                combo->setCurrentIndex(0);
            }
        } else {
            int idx = combo->findData(index.data(Qt::EditRole).toInt(), Qt::UserRole);

            if (idx >= 0) {
                combo->setCurrentIndex(idx);
            } else {
                combo->setCurrentIndex(0);
            }
        }
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void HsmEntityPropertyDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    if (index.data(view::StateMachineEntityViewModel::PropertyKeyRole).toString() == "path") {
        QLineEdit* line = editor->findChild<QLineEdit*>();

        if (line) {
            model->setData(index, line->text(), Qt::EditRole);
        } else {
            qFatal("HsmEntityPropertyDelegate: unexpected editor widget");
        }
    } else if (QComboBox* combo = qobject_cast<QComboBox*>(editor)) {
        if (index.data(view::StateMachineEntityViewModel::ActionSubtypeRole).isValid()) {
            model->setData(index, combo->currentText(), Qt::EditRole);
        } else if (index.data(view::StateMachineEntityViewModel::PropertyPathRole).toString().endsWith(".singleshot")) {
            model->setData(index, combo->currentData(Qt::UserRole).toBool() ? "true" : "false", Qt::EditRole);
        } else {
            model->setData(index, combo->currentData(Qt::UserRole), Qt::EditRole);
        }
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void HsmEntityPropertyDelegate::updateEditorGeometry(QWidget* editor,
                                                     const QStyleOptionViewItem& option,
                                                     const QModelIndex& index) const {
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);

    // Only for properties with custom QLineEdit widgets
    if (auto* line = editor->findChild<QLineEdit*>()) {
        QTimer::singleShot(0, line, [line]() { line->setFocus(); });
    }
}

void HsmEntityPropertyDelegate::onEditingFinished(QWidget* editor) {
    emit commitData(editor);   // write to model
    emit closeEditor(editor);  // close editor
}

};  // namespace view
