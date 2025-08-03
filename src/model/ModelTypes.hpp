#ifndef MODELTYPES_HPP
#define MODELTYPES_HPP

#include <QMetaType>
#include <inttypes.h>

namespace model {

using EntityID_t = uint32_t;

};

Q_DECLARE_METATYPE(model::EntityID_t)

#endif // MODELTYPES_HPP