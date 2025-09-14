#include "HsmStateElement.hpp"

#include <QColor>
#include <QPainter>
#include <QFontMetrics>

namespace view {

HsmStateElement::HsmStateElement()
    : HsmResizableElement(HsmElementType::STATE) {}

bool HsmStateElement::acceptsChildren() const {
    return true;
}

void HsmStateElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setPen(Qt::SolidLine);

    // Set the brush color
    painter->setBrush(QColor("#8bb359"));
    mItemRect = QRectF(mOuterRect);

    // Draw the rounded rectangle with fully rounded corners
    painter->drawRoundedRect(mItemRect, mItemRect.height() / 2, mItemRect.height() / 2);

    if (isHighligted() == true) {
        painter->setPen(mPenHighlightMode);
        painter->drawRoundedRect(mItemRect.adjusted(2, 2, -2, -2), mItemRect.height() / 2, mItemRect.height() / 2);
    }

    // Draw the state name
    QFont nameFont = painter->font();
    nameFont.setBold(true);
    painter->setFont(nameFont);

    // Calculate text position
    QRectF textRect = mItemRect;
    textRect.setHeight(nameFont.pointSize() * 1.5);
    QRectF nameRect = QRectF(
        textRect.x() + 5,
        textRect.y() + 5,
        textRect.width() - 10,
        textRect.height()
    );

    // Draw the state name
    QString stateName = "State Name"; // TODO: Replace with actual state name from model
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignTop, stateName);

    // Draw entry/exit actions and internal transitions if available
    QFont actionFont = painter->font();
    actionFont.setBold(false);
    actionFont.setPointSize(actionFont.pointSize() - 2);
    painter->setFont(actionFont);

    // Example actions and transitions
    QStringList actions = {
        "entry: init()",
        "do: monitor()",
        "exit: cleanup()"
    };

    QStringList transitions = {
        "event [guard] / action",
        "signal [condition] / response"
    };

    // Calculate positions for actions and transitions
    qreal yPos = nameRect.bottom() + 2;
    qreal maxWidth = mItemRect.width() - 10;

    // Draw actions
    for (const QString& action : actions) {
        QRectF actionRect(textRect.x() + 5, yPos, maxWidth, actionFont.pointSize() * 1.5);
        painter->drawText(actionRect, Qt::AlignLeft | Qt::AlignTop, action);
        yPos += actionFont.pointSize() * 1.5;
    }

    // Draw transitions
    for (const QString& transition : transitions) {
        QRectF transitionRect(textRect.x() + 5, yPos, maxWidth, actionFont.pointSize() * 1.5);
        painter->drawText(transitionRect, Qt::AlignLeft | Qt::AlignTop, transition);
        yPos += actionFont.pointSize() * 1.5;
    }

    HsmResizableElement::paint(painter, option, widget);
}

};  // namespace view