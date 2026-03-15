#include "HsmHistoryElement.hpp"

#include <QColor>
#include <QPainter>

#include "view/theme/ThemeManager.hpp"

namespace view {

HsmHistoryElement::HsmHistoryElement()
    : HsmRoundElement(HsmElementType::HISTORY) {}

bool HsmHistoryElement::isConnectable() const {
    return true;
}

bool HsmHistoryElement::canBeTopLevel() const {
    return false;
}

void HsmHistoryElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRoundElement::paint(painter, option, widget);

    const qreal r = radius();
    const QPointF c = center();
    // Cache font size calculation for fixed item size
    static int cachedFontSize = 0;
    QFont font = painter->font();

    if (cachedFontSize == 0) {
        const qreal targetHeight = r * 2 * ThemeManager::instance().theme().node.historyFontHeightFactor;
        int fontSize = 1;
        QFontMetrics fm(font);

        while (fm.height() < targetHeight) {
            fontSize++;
            font.setPointSize(fontSize);
            fm = QFontMetrics(font);
        }

        cachedFontSize = fontSize - 1;
    }

    font.setPointSize(cachedFontSize);
    painter->setFont(font);

    // Draw the history type indicator
    const QString historyType = "H";  // Default to shallow history
    // If it's deep history, we would set historyType to "H*"

    // Draw the white filled circle
    painter->setBrush(ThemeManager::instance().theme().node.backgroundBrush);
    painter->drawEllipse(c, r, r);

    // Draw text centered in the ellipse
    painter->drawText(mOuterRect, Qt::AlignCenter, historyType);
}

};  // namespace view
