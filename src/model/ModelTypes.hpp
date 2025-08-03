#ifndef MODELTYPES_HPP
#define MODELTYPES_HPP

#include <QMetaType>
#include <inttypes.h>

namespace model {

using EntityID_t = uint32_t;
constexpr EntityID_t INVALID_MODEL_ID = 0;

};

Q_DECLARE_METATYPE(model::EntityID_t)

#endif // MODELTYPES_HPP