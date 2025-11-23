#include "ModelTypes.hpp"

namespace model {

QString transitionTypeToString(const TransitionType type) {
    switch (type) {
        case TransitionType::EXTERNAL:
            return "External";
        case TransitionType::INTERNAL:
            return "Internal";
        default:
            break;
    }

    return "";
}

TransitionType transitionTypeFromInt(const int value) {
    TransitionType res = static_cast<TransitionType>(value);

    switch (res) {
        case TransitionType::EXTERNAL:
        case TransitionType::INTERNAL:
            break;
        default:
            res = TransitionType::INVALID;
            break;
    }

    return res;
}

QString historyTypeToString(const HistoryType type) {
    switch (type) {
        case HistoryType::SHALLOW:
            return "Shallow";
        case HistoryType::DEEP:
            return "Deep";
        default:
            break;
    }

    return "";
}

HistoryType historyTypeFromInt(const int value) {
    HistoryType res = static_cast<HistoryType>(value);

    switch (res) {
        case HistoryType::SHALLOW:
        case HistoryType::DEEP:
            break;
        default:
            res = HistoryType::INVALID;
            break;
    }

    return res;
}

};  // namespace model