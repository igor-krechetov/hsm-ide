#ifndef MODELTYPES_HPP
#define MODELTYPES_HPP

#include <inttypes.h>

#include <QMetaType>

namespace model {

using EntityID_t = uint32_t;
constexpr EntityID_t INVALID_MODEL_ID = 0xFFFFFFFF;

enum class StateType { INVALID, MODEL_ROOT, INITIAL, REGULAR, ENTRYPOINT, EXITPOINT, FINAL, HISTORY, INCLUDE };
enum class TransitionType { INVALID, EXTERNAL, INTERNAL };
enum class HistoryType { INVALID, SHALLOW, DEEP };

QString transitionTypeToString(const TransitionType type);
TransitionType transitionTypeFromInt(const int value);

QString historyTypeToString(const HistoryType type);
HistoryType historyTypeFromInt(const int value);

};  // namespace model

Q_DECLARE_METATYPE(model::EntityID_t)

#endif  // MODELTYPES_HPP