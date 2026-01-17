#ifndef IDENTIFIERSTRING_HPP
#define IDENTIFIERSTRING_HPP

#include <QString>

#include "model/ModelUtils.hpp"

namespace model {
namespace priv {

class IdentifierString {
public:
    IdentifierString() = default;

    IdentifierString(const QString& str)
        : mValue(sanitiseIdentifier(str)) {}

    IdentifierString(const IdentifierString& other)
        : mValue(other.mValue) {}

    IdentifierString& operator=(const IdentifierString& other) {
        if (this != &other) {
            mValue = other.mValue;
        }
        return *this;
    }

    IdentifierString& operator=(const QString& str) {
        mValue = sanitiseIdentifier(str);
        return *this;
    }

    const QString& get() const {
        return mValue;
    }

    void set(const QString& str) {
        mValue = sanitiseIdentifier(str);
    }

private:
    QString mValue;
};

}  // namespace priv
}  // namespace model

#endif  // IDENTIFIERSTRING_HPP
