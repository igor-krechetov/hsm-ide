#include "DebugRendering.hpp"
#include "view/theme/ThemeManager.hpp"

namespace view {

void debugDrawX(QPainter* painter, const QPointF& pos) {
    constexpr qreal cGripSize = 10.0;

    // draw small X at the center of the grip
    painter->setPen(ThemeManager::instance().theme().grip.debugPen);
    painter->drawLine(QPointF(-cGripSize / 4, -cGripSize / 4), QPointF(cGripSize / 4, cGripSize / 4));
    painter->drawLine(QPointF(-cGripSize / 4, cGripSize / 4), QPointF(cGripSize / 4, -cGripSize / 4));
}

}  // namespace view