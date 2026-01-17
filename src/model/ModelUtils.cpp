#include "ModelUtils.hpp"

namespace model {

QString sanitiseIdentifier(const QString& input) {
    // remove all characters to make "input" compliant with C++ identifier name rules
    QString out;
    bool firstValidCharFound = false;

    out.reserve(input.length());

    for (const QChar &c : input) {
        if (false == firstValidCharFound) {
            // First valid character must be a letter or underscore
            if (c.isLetter() || c == '_') {
                out.append(c);
                firstValidCharFound = true;
            }
            // Otherwise skip character
        } else {
            // Subsequent characters: letters, digits, underscore
            if (c.isLetterOrNumber() || c == '_') {
                out.append(c);
            }
        }
    }

    return out;
}

};  // namespace model