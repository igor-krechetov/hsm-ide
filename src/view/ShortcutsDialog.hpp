#ifndef SHORTCUTSDIALOG_HPP
#define SHORTCUTSDIALOG_HPP

#include <QDialog>

QT_BEGIN_NAMESPACE
class Ui_ShortcutsDialog;
QT_END_NAMESPACE

class ShortcutsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShortcutsDialog(QWidget* parent = nullptr);
    ~ShortcutsDialog();

private:
    Ui_ShortcutsDialog* ui = nullptr;
};

#endif  // SHORTCUTSDIALOG_HPP
