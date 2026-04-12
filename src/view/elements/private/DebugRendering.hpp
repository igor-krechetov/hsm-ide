#ifdef DEBUG_RENDERING

#ifndef HSMIDE_DEBUGRENDERING_HPP
#define HSMIDE_DEBUGRENDERING_HPP

#include <QPoint>
#include <QPainter>

namespace view {

void debugDrawX(QPainter* painter, const QPointF& pos);

}  // namespace view

#endif  // HSMIDE_DEBUGRENDERING_HPP

#endif  // DEBUG_RENDERING