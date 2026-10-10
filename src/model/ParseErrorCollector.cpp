#include "ParseErrorCollector.hpp"

namespace model {

void ParseErrorCollector::addError(const QString& message, qint64 line, qint64 column) {
    add(Severity::Error, message, line, column);
}

void ParseErrorCollector::addWarning(const QString& message, qint64 line, qint64 column) {
    add(Severity::Warning, message, line, column);
}

void ParseErrorCollector::clear() {
    mProblems.clear();
}

bool ParseErrorCollector::isEmpty() const {
    return mProblems.isEmpty();
}

bool ParseErrorCollector::hasErrors() const {
    bool result = false;

    for (const Problem& problem : mProblems) {
        if (Severity::Error == problem.severity) {
            result = true;
            break;
        }
    }

    return result;
}

int ParseErrorCollector::count() const {
    return mProblems.size();
}

const QList<ParseErrorCollector::Problem>& ParseErrorCollector::problems() const {
    return mProblems;
}

void ParseErrorCollector::add(Severity severity, const QString& message, qint64 line, qint64 column) {
    Problem problem;
    problem.message = message;
    problem.severity = severity;
    problem.line = line;
    problem.column = column;
    mProblems.append(problem);
}

}  // namespace model
