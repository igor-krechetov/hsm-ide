#include "ModelTypes.hpp"

namespace model {

QString transitionTypeToString(const TransitionType type) {
    switch(type) {
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

};  // namespace model