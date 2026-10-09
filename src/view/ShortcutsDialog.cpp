#include "ShortcutsDialog.hpp"

#include <QFont>
#include <QHeaderView>
#include <QTableWidgetItem>

#include "./ui/ui_shortcuts.h"

ShortcutsDialog::ShortcutsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_ShortcutsDialog) {
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMaximizeButtonHint);

    struct ShortcutEntry {
        const char* action;
        const char* shortcut;
    };

    struct ShortcutGroup {
        const char* title;
        QList<ShortcutEntry> entries;
    };

    const QList<ShortcutGroup> groups = {
        {"File", {{"New HSM", "Ctrl+N"}, {"Open HSM", "Ctrl+O"}, {"Save", "Ctrl+S"}, {"Close Current", "Ctrl+W"}}},
        {"Edit",
         {{"Undo", "Ctrl+Z"},
          {"Redo", "Ctrl+Shift+Z"},
          {"Cut", "Ctrl+X"},
          {"Copy", "Ctrl+C"},
          {"Paste", "Ctrl+V"},
          {"Delete", "Del"},
          {"Duplicate", "Ctrl+D"},
          {"Edit / Rename", "F2"},
          {"Find", "Ctrl+F"},
          {"Select All", "Ctrl+A"}}},
        {"View & Zoom",
         {{"Zoom In", "Ctrl+="}, {"Zoom Out", "Ctrl+-"}, {"Reset Zoom", "Ctrl+0"}, {"Fit to View", "Ctrl+Shift+0"}}},
        {"Panels",
         {{"Switch to Design panel", "Ctrl+Shift+E"},
          {"Switch to Workspace panel", "Ctrl+Shift+B"},
          {"Switch to Debug panel", "Ctrl+Shift+D"}}},
    };

    QTableWidget* table = ui->shortcutsTable;
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({tr("Action"), tr("Shortcut")});
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    int row = 0;
    for (const ShortcutGroup& group : groups) {
        table->insertRow(row);
        QTableWidgetItem* header = new QTableWidgetItem(group.title);
        QFont headerFont = header->font();
        headerFont.setBold(true);
        header->setFont(headerFont);
        header->setFlags(Qt::NoItemFlags);
        table->setItem(row, 0, header);
        table->setSpan(row, 0, 1, 2);
        ++row;

        for (const ShortcutEntry& entry : group.entries) {
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(entry.action));
            table->setItem(row, 1, new QTableWidgetItem(entry.shortcut));
            ++row;
        }
    }
}

ShortcutsDialog::~ShortcutsDialog() {
    delete ui;
}
