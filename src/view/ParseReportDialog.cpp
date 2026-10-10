#include "ParseReportDialog.hpp"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QString severityText(const model::ParseErrorCollector::Severity severity) {
    QString result;

    if (model::ParseErrorCollector::Severity::Error == severity) {
        result = QObject::tr("Error");
    } else {
        result = QObject::tr("Warning");
    }

    return result;
}

QString locationText(const qint64 value) {
    QString result;

    if (value >= 0) {
        result = QString::number(value);
    }

    return result;
}

}  // namespace

ParseReportDialog::ParseReportDialog(const QString& fileName, const model::ParseErrorCollector& report, QWidget* parent)
    : QDialog(parent) {
    buildUi(fileName, report);
}

void ParseReportDialog::buildUi(const QString& fileName, const model::ParseErrorCollector& report) {
    setWindowTitle(tr("Import Problems"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(640, 360);

    int errorCount = 0;
    int warningCount = 0;

    for (const model::ParseErrorCollector::Problem& problem : report.problems()) {
        if (model::ParseErrorCollector::Severity::Error == problem.severity) {
            ++errorCount;
        } else {
            ++warningCount;
        }
    }

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* summary = new QLabel(this);
    summary->setText(tr("Problems while importing %1: %2 error(s), %3 warning(s)")
                         .arg(fileName.isEmpty() ? tr("the file") : fileName)
                         .arg(errorCount)
                         .arg(warningCount));
    summary->setWordWrap(true);
    layout->addWidget(summary);

    QTableWidget* table = new QTableWidget(report.count(), 4, this);
    table->setHorizontalHeaderLabels({tr("Severity"), tr("Line"), tr("Column"), tr("Message")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    const QColor errorColor(0xB0, 0x00, 0x20);
    const QColor warningColor(0x9A, 0x6A, 0x00);
    int row = 0;

    for (const model::ParseErrorCollector::Problem& problem : report.problems()) {
        const bool isError = (model::ParseErrorCollector::Severity::Error == problem.severity);

        QTableWidgetItem* severityItem = new QTableWidgetItem(severityText(problem.severity));
        severityItem->setForeground(isError ? errorColor : warningColor);

        QTableWidgetItem* lineItem = new QTableWidgetItem(locationText(problem.line));
        QTableWidgetItem* columnItem = new QTableWidgetItem(locationText(problem.column));
        QTableWidgetItem* messageItem = new QTableWidgetItem(problem.message);

        table->setItem(row, 0, severityItem);
        table->setItem(row, 1, lineItem);
        table->setItem(row, 2, columnItem);
        table->setItem(row, 3, messageItem);
        ++row;
    }

    layout->addWidget(table);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}

void ParseReportDialog::showReport(QWidget* parent, const QString& fileName, const model::ParseErrorCollector& report) {
    if (!report.isEmpty()) {
        ParseReportDialog dialog(fileName, report, parent);
        dialog.exec();
    }
}
