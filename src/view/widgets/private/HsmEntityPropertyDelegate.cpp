#include "HsmEntityPropertyDelegate.hpp"

#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QVariant>

#include "model/actions/ModelActionFactory.hpp"
#include "model/elements/HistoryState.hpp"
#include "model/elements/Transition.hpp"
#include "view/models/StateMachineEntityViewModel.hpp"

namespace view {

namespace {
constexpr int cPropertyKeyRole = StateMachineEntityViewModel::PropertyKeyRole;
constexpr int cPropertyPathRole = StateMachineEntityViewModel::PropertyPathRole;
constexpr int cActionSubtypeRole = StateMachineEntityViewModel::ActionSubtypeRole;
constexpr int cActionAddRole = StateMachineEntityViewModel::ActionAddRole;
constexpr int cActionRemovableRole = StateMachineEntityViewModel::ActionRemovableRole;
constexpr int cActionMoveUpRole = StateMachineEntityViewModel::ActionMoveUpRole;
constexpr int cActionMoveDownRole = StateMachineEntityViewModel::ActionMoveDownRole;
constexpr int cRemoveButtonSize = 16;
}  // namespace

HsmEntityPropertyDelegate::HsmEntityPropertyDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
    connect(this, &HsmEntityPropertyDelegate::finishEditing, this, &HsmEntityPropertyDelegate::onEditingFinished);
}

QWidget* HsmEntityPropertyDelegate::createEditor(QWidget* parent,
                                                 const QStyleOptionViewItem& option,
                                                 const QModelIndex& index) const {
    if (index.data(cActionSubtypeRole).isValid()) {
        QComboBox* combo = new QComboBox(parent);

        for (const auto& actionName : model::ModelActionFactory::supportedActionNames()) {
            combo->addItem(actionName);
        }

        return combo;
    } else if (index.data(cPropertyKeyRole).toString() == model::Transition::cKeyTransitionType) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem(model::transitionTypeToString(model::TransitionType::EXTERNAL),
                       static_cast<int>(model::TransitionType::EXTERNAL));
        combo->addItem(model::transitionTypeToString(model::TransitionType::INTERNAL),
                       static_cast<int>(model::TransitionType::INTERNAL));

        return combo;
    } else if (index.data(cPropertyKeyRole).toString() == model::Transition::cKeyExpectedConditionValue) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem(tr("true"), true);
        combo->addItem(tr("false"), false);

        return combo;
    } else if (index.data(cPropertyPathRole).toString().endsWith(".singleshot")) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem(tr("true"), true);
        combo->addItem(tr("false"), false);

        return combo;
    } else if (index.data(cPropertyKeyRole).toString() == model::HistoryState::cKeyHistoryType) {
        QComboBox* combo = new QComboBox(parent);

        combo->addItem(model::historyTypeToString(model::HistoryType::SHALLOW), static_cast<int>(model::HistoryType::SHALLOW));
        combo->addItem(model::historyTypeToString(model::HistoryType::DEEP), static_cast<int>(model::HistoryType::DEEP));

        return combo;
    } else if (index.data(cPropertyKeyRole).toString() == "path") {
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
    if (index.data(cPropertyKeyRole).toString() == "path") {
        QLineEdit* line = editor->findChild<QLineEdit*>();

        if (line) {
            line->setText(index.data(Qt::EditRole).toString());
            line->setFocus();
        } else {
            qFatal("HsmEntityPropertyDelegate: unexpected editor widget");
        }
    } else if (QComboBox* combo = qobject_cast<QComboBox*>(editor)) {
        if (index.data(cActionSubtypeRole).isValid()) {
            int idx = combo->findText(index.data(Qt::EditRole).toString());

            if (idx >= 0) {
                combo->setCurrentIndex(idx);
            } else {
                combo->setCurrentIndex(0);
            }
        } else if (index.data(cPropertyPathRole).toString().endsWith(".singleshot")) {
            const bool boolValue = index.data(Qt::EditRole).toBool();
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
    if (index.data(cPropertyKeyRole).toString() == "path") {
        QLineEdit* line = editor->findChild<QLineEdit*>();

        if (line) {
            model->setData(index, line->text(), Qt::EditRole);
        } else {
            qFatal("HsmEntityPropertyDelegate: unexpected editor widget");
        }
    } else if (QComboBox* combo = qobject_cast<QComboBox*>(editor)) {
        if (index.data(cActionSubtypeRole).isValid()) {
            model->setData(index, combo->currentText(), Qt::EditRole);
        } else if (index.data(cPropertyPathRole).toString().endsWith(".singleshot")) {
            model->setData(index, combo->currentData(Qt::UserRole).toBool(), Qt::EditRole);
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

QRect HsmEntityPropertyDelegate::removeButtonRect(const QStyleOptionViewItem& option) {
    const int y = option.rect.top() + (option.rect.height() - cRemoveButtonSize) / 2;

    return QRect(option.rect.right() - cRemoveButtonSize - 2, y, cRemoveButtonSize, cRemoveButtonSize);
}

QRect HsmEntityPropertyDelegate::moveDownButtonRect(const QStyleOptionViewItem& option) {
    const int y = option.rect.top() + (option.rect.height() - cRemoveButtonSize) / 2;

    return QRect(option.rect.right() - 2 * (cRemoveButtonSize + 2), y, cRemoveButtonSize, cRemoveButtonSize);
}

QRect HsmEntityPropertyDelegate::moveUpButtonRect(const QStyleOptionViewItem& option) {
    const int y = option.rect.top() + (option.rect.height() - cRemoveButtonSize) / 2;

    return QRect(option.rect.right() - 3 * (cRemoveButtonSize + 2), y, cRemoveButtonSize, cRemoveButtonSize);
}

QRect HsmEntityPropertyDelegate::addButtonRect(const QStyleOptionViewItem& option) {
    const int y = option.rect.top() + (option.rect.height() - cRemoveButtonSize) / 2;

    return QRect(option.rect.right() - cRemoveButtonSize - 2, y, cRemoveButtonSize, cRemoveButtonSize);
}

void HsmEntityPropertyDelegate::drawButton(QPainter* painter,
                                           const QStyleOptionViewItem& option,
                                           const QRect& rect,
                                           const QString& glyph,
                                           const bool hovered) const {
    painter->save();

    if (hovered) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(option.palette.color(QPalette::Highlight));
        painter->drawRoundedRect(rect, 3, 3);
        painter->setPen(option.palette.color(QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(QPalette::ButtonText));
    }

    painter->drawText(rect, Qt::AlignCenter, glyph);
    painter->restore();
}

void HsmEntityPropertyDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyledItemDelegate::paint(painter, option, index);

    if (index.column() != 1) {
        return;
    }

    const bool rowHovered = (mHoverIndex == index);

    if (index.data(cActionAddRole).toBool()) {
        // "+" affordance on the action-list slot row itself
        const QRect addRect = addButtonRect(option);
        drawButton(painter, option, addRect, QStringLiteral("+"), rowHovered && (mHoverButtonRect == addRect));
    } else if (index.data(cActionRemovableRole).toBool()) {
        const QRect rmRect = removeButtonRect(option);
        drawButton(painter, option, rmRect, QStringLiteral("\u2715"), rowHovered && (mHoverButtonRect == rmRect));

        if (index.data(cActionMoveUpRole).toBool()) {
            const QRect upRect = moveUpButtonRect(option);
            drawButton(painter, option, upRect, QStringLiteral("\u25B2"), rowHovered && (mHoverButtonRect == upRect));
        }
        if (index.data(cActionMoveDownRole).toBool()) {
            const QRect downRect = moveDownButtonRect(option);
            drawButton(painter, option, downRect, QStringLiteral("\u25BC"), rowHovered && (mHoverButtonRect == downRect));
        }
    }
}

bool HsmEntityPropertyDelegate::editorEvent(QEvent* event,
                                            QAbstractItemModel* model,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) {
    bool handled = false;

    if ((nullptr != event) && (index.column() == 1) &&
        ((event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::MouseMove))) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const QPoint pos = mouseEvent->pos();

        // Determine which sub-button (if any) the pointer is over on this row.
        QRect hoveredRect;

        if (index.data(cActionAddRole).toBool()) {
            if (addButtonRect(option).contains(pos)) {
                hoveredRect = addButtonRect(option);
            }
        } else if (index.data(cActionRemovableRole).toBool()) {
            if (removeButtonRect(option).contains(pos)) {
                hoveredRect = removeButtonRect(option);
            } else if (index.data(cActionMoveUpRole).toBool() && moveUpButtonRect(option).contains(pos)) {
                hoveredRect = moveUpButtonRect(option);
            } else if (index.data(cActionMoveDownRole).toBool() && moveDownButtonRect(option).contains(pos)) {
                hoveredRect = moveDownButtonRect(option);
            }
        }

        // Update hover state; the view repaints its viewport on mouse move (mouse
        // tracking enabled in HsmPropertiesTableView), so the highlight follows.
        mHoverIndex = index;
        mHoverButtonRect = hoveredRect;

        if (event->type() == QEvent::MouseButtonRelease) {
            if (index.data(cActionAddRole).toBool() && addButtonRect(option).contains(pos)) {
                model->setData(index, true, cActionAddRole);
                handled = true;
            } else if (index.data(cActionRemovableRole).toBool()) {
                if (removeButtonRect(option).contains(pos)) {
                    model->setData(index, true, cActionRemovableRole);
                    handled = true;
                } else if (index.data(cActionMoveUpRole).toBool() && moveUpButtonRect(option).contains(pos)) {
                    model->setData(index, true, cActionMoveUpRole);
                    handled = true;
                } else if (index.data(cActionMoveDownRole).toBool() && moveDownButtonRect(option).contains(pos)) {
                    model->setData(index, true, cActionMoveDownRole);
                    handled = true;
                }
            }
        }
    }

    if (handled == false) {
        handled = QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    return handled;
}

};  // namespace view
