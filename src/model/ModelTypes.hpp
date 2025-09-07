#ifndef MODELTYPES_HPP
#define MODELTYPES_HPP

#include <inttypes.h>

#include <QMetaType>

namespace model {

using EntityID_t = uint32_t;
constexpr EntityID_t INVALID_MODEL_ID = 0xFFFFFFFF;

};  // namespace model

Q_DECLARE_METATYPE(model::EntityID_t)

#endif  // MODELTYPES_HPP