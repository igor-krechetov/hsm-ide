#include "HsmExitPointElement.hpp"

#include <QColor>
#include <QPainter>

#include "view/theme/ThemeManager.hpp"

namespace view {

// TODO: block outgoing connections

HsmExitPointElement::HsmExitPointElement()
    : HsmRoundElement(HsmElementType::EXIT_POINT) {}

bool HsmExitPointElement::isConnectable() const {
    return false;
}

bool HsmExitPointElement::canBeTopLevel() const {
    return false;
}

void HsmExitPointElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRoundElement::paint(painter, option, widget);

    const qreal r = radius();
    const QPointF c = center();

    // Draw the circle
    painter->setBrush(ThemeManager::instance().theme().node.backgroundBrush);
    painter->drawEllipse(c, r, r);

    // Draw two diameter lines perpendicular to each other, rotated by 45 degrees
    painter->setBrush(ThemeManager::instance().theme().node.mainBrush);

    // Calculate endpoints for the lines
    const qreal d = r * std::sqrt(2);  // diameter of the circle along 45-degree axis
    QPointF p1(c.x() - d / 2, c.y() - d / 2);
    QPointF p2(c.x() + d / 2, c.y() + d / 2);
    QPointF p3(c.x() - d / 2, c.y() + d / 2);
    QPointF p4(c.x() + d / 2, c.y() - d / 2);

    painter->drawLine(p1, p2);
    painter->drawLine(p3, p4);
}

};  // namespace view