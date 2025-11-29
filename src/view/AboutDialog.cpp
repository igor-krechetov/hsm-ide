#include "AboutDialog.hpp"

#include "./ui/ui_about.h"
#include "version.hpp"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_AboutDialog) {
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(size());

    ui->title->setText(QString("HSM IDE %1").arg(HSM_IDE_VERSION));
    ui->description->setText(QString("Build Date: %1\n\nRevision: %2\n\nCopyright 2025 Ihor Krechetov\n\n"
                                     "The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING "
                                     "THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.")
                                 .arg(HSM_IDE_BUILD_DATE)
                                 .arg(HSM_IDE_GIT_COMMIT));
}

AboutDialog::~AboutDialog() {
    delete ui;
}
