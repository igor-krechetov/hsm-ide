#include "HsmStateElement.hpp"

#include <QColor>
#include <QPainter>
#include <QFontMetrics>
#include <QGraphicsTextItem>
#include <QTextDocument>

namespace view {

HsmStateElement::HsmStateElement()
    : HsmRectangularElement(HsmElementType::STATE) {
    QFont font;
    font.setBold(true);

    // TODO: FIXME: GraphicView takes over key presses, so we cant type "SPACE"
    mTextItem = new QGraphicsTextItem(this);
    mTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    mTextItem->setPlainText("State Name");
    mTextItem->setFont(font);
    mTextItem->setPos(10, 10); // Adjust position as needed
    mTextItem->setTextWidth(120); // Adjust width as needed
    connect(mTextItem->document(), &QTextDocument::contentsChanged, this, &HsmStateElement::onTextChanged);
}

void HsmStateElement::onTextChanged() {
    centerHeader();
}

void HsmStateElement::centerHeader() {
    if (mTextItem) {
        mTextItem->setTextWidth(-1); // Use minimum width based on content
        QRectF textRect = mTextItem->boundingRect();
        qreal x = mOuterRect.x() + (mOuterRect.width() - textRect.width()) / 2;
        qreal y = mOuterRect.y() + 10.0; // Keep some top padding
        mTextItem->setPos(x, y);
    }
}

bool HsmStateElement::acceptsChildren() const {
    return true;
}

void HsmStateElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRectangularElement::paint(painter, option, widget);

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

    // QStringList transitions = {
    //     "event [guard] / action",
    //     "signal [condition] / response"
    // };

    // Calculate positions for actions and transitions
    qreal yPos = mTextItem->y() + mTextItem->boundingRect().height() + 2;
    qreal maxWidth = mOuterRect.width() - 10;

    // Draw actions
    for (const QString& action : actions) {
        QRectF actionRect(mOuterRect.x() + 5, yPos, maxWidth, actionFont.pointSize() * 1.5);
        painter->drawText(actionRect, Qt::AlignLeft | Qt::AlignTop, action);
        yPos += actionFont.pointSize() * 1.5;
    }

    // // Draw transitions
    // for (const QString& transition : transitions) {
    //     QRectF transitionRect(mOuterRect.x() + 5, yPos, maxWidth, actionFont.pointSize() * 1.5);
    //     painter->drawText(transitionRect, Qt::AlignLeft | Qt::AlignTop, transition);
    //     yPos += actionFont.pointSize() * 1.5;
    // }
}

void HsmStateElement::updateBoundingRect(const QRectF& newRect) {
    HsmRectangularElement::updateBoundingRect(newRect);
    centerHeader();
}

};  // namespace view