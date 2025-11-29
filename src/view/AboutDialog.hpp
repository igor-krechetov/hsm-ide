#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

#include <QDialog>
#include "version.hpp"

QT_BEGIN_NAMESPACE
class Ui_AboutDialog;
QT_END_NAMESPACE

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

private:
    Ui_AboutDialog *ui = nullptr;
};

#endif // ABOUTDIALOG_HPP
