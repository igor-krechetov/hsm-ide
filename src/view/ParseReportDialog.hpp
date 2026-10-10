#ifndef PARSEREPORTDIALOG_HPP
#define PARSEREPORTDIALOG_HPP

#include <QDialog>
#include <QString>

#include "model/ParseErrorCollector.hpp"

/**
 * @brief Modal dialog that lists SCXML parse problems after an import attempt.
 *
 * Presents every problem in the report with its severity, location (line/column) and
 * message. Problems without a known location show empty location cells.
 */
class ParseReportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ParseReportDialog(const QString& fileName, const model::ParseErrorCollector& report, QWidget* parent = nullptr);

    static void showReport(QWidget* parent, const QString& fileName, const model::ParseErrorCollector& report);

private:
    void buildUi(const QString& fileName, const model::ParseErrorCollector& report);
};

#endif  // PARSEREPORTDIALOG_HPP
