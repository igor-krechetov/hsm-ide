#ifndef THEMEUTILS_HPP
#define THEMEUTILS_HPP

#include <QtGlobal>
#include <algorithm>
#include <cmath>

#include "view/theme/ThemeManager.hpp"

namespace view {
namespace theme {

// Snaps a size/length value to the nearest multiple of the minor grid step.
// Returns the value unchanged when the configured step is non-positive, and
// otherwise snaps to max(1, round(value / step)) * step so the result is always
// a whole, non-zero multiple of the grid step.
inline qreal snapToGrid(qreal value) {
    const qreal step = ThemeManager::instance().theme().grid.minorLineStep;
    qreal result = value;

    if (step > 0) {
        const qreal multiples = std::max<qreal>(1, std::round(value / step));
        result = multiples * step;
    }

    return result;
}

}  // namespace theme
}  // namespace view

#endif  // THEMEUTILS_HPP
