#ifndef PARSEERRORCOLLECTOR_HPP
#define PARSEERRORCOLLECTOR_HPP

#include <QList>
#include <QString>
#include <QtGlobal>

namespace model {

/**
 * @brief Accumulates problems detected while parsing an SCXML document.
 *
 * A plain value type (no QObject). The serializer owns one collector per
 * deserialization attempt and routes every parse problem into it. Positions use the
 * sentinel -1 when the XML reader cannot supply a location.
 */
class ParseErrorCollector {
public:
    enum class Severity { Warning, Error };

    struct Problem {
        QString message;
        Severity severity = Severity::Error;
        qint64 line = -1;    // -1 when the reader supplies no position
        qint64 column = -1;  // -1 when unavailable
    };

public:
    void addError(const QString& message, qint64 line = -1, qint64 column = -1);
    void addWarning(const QString& message, qint64 line = -1, qint64 column = -1);
    void clear();

    bool isEmpty() const;
    bool hasErrors() const;
    int count() const;
    const QList<Problem>& problems() const;

private:
    void add(Severity severity, const QString& message, qint64 line, qint64 column);

private:
    QList<Problem> mProblems;
};

}  // namespace model

#endif  // PARSEERRORCOLLECTOR_HPP
