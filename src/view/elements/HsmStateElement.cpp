#include "HsmStateElement.hpp"

#include <QColor>
#include <QFontMetrics>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QSignalBlocker>
#include <QTextDocument>

#include "model/RegularState.hpp"
#include "private/HsmStateTextItem.hpp"

namespace view {

HsmStateElement::HsmStateElement()
    : HsmRectangularElement(HsmElementType::STATE) {
}

void HsmStateElement::init(const QSharedPointer<model::StateMachineEntity>& modelEntity) {
    HsmRectangularElement::init(modelEntity);

    mTextItem = new HsmStateTextItem(this);
    connect(mTextItem->document(), &QTextDocument::contentsChanged, this, &HsmStateElement::onStateNameChanged);
    connect(mTextItem, &HsmStateTextItem::editingFinished, this, &HsmStateElement::onStateNameEditFinished);

    // update label with current state name
    onModelDataChanged();
}

void HsmStateElement::onStateNameChanged() {
    // Only update header position, do not update model here
    centerHeader();
}

void HsmStateElement::onStateNameEditFinished() {
    auto entityPtr = modelElement<model::RegularState>();

    if (mTextItem && entityPtr) {
        entityPtr->setName(mTextItem->toPlainText());
    }
}

void HsmStateElement::centerHeader() {
    if (mTextItem) {
        mTextItem->setTextWidth(-1);  // Use minimum width based on content
        QRectF textRect = mTextItem->boundingRect();
        qreal x = mOuterRect.x() + (mOuterRect.width() - textRect.width()) / 2;
        qreal y = mOuterRect.y() + 10.0;  // Keep some top padding
        mTextItem->setPos(x, y);
    }
}

bool HsmStateElement::acceptsChildren() const {
    return true;
}

void HsmStateElement::onModelDataChanged() {
    qDebug() << "---- HsmStateElement::onModelDataChanged";
    auto entityPtr = modelElement<model::RegularState>();

    if (mTextItem && entityPtr) {
        QSignalBlocker block(mTextItem->document());

        mTextItem->setPlainText(entityPtr->name());
        centerHeader();
    }

    update();  // trigger repaint
}

void HsmStateElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    HsmRectangularElement::paint(painter, option, widget);

    QFont actionFont = painter->font();
    actionFont.setBold(false);
    actionFont.setPointSize(actionFont.pointSize() - 2);
    painter->setFont(actionFont);

    auto entityPtr = modelElement<model::RegularState>();
    QStringList actions;
    // Calculate positions for actions and transitions
    qreal yPos = mTextItem->y() + mTextItem->boundingRect().height() + 2;
    qreal maxWidth = mOuterRect.width() - 10;

    if (entityPtr) {
        if (!entityPtr->onEnteringCallback().isEmpty()) {
            actions << "onEntry: " + entityPtr->onEnteringCallback();
        }
        if (!entityPtr->onStateChangedCallback().isEmpty()) {
            actions << "do: " + entityPtr->onStateChangedCallback();
        }
        if (!entityPtr->onExitingCallback().isEmpty()) {
            actions << "onExit: " + entityPtr->onExitingCallback();
        }
    }

    // Draw actions
    for (const QString& action : actions) {
        QRectF actionRect(mOuterRect.x() + 5, yPos, maxWidth, actionFont.pointSize() * 1.5);
        painter->drawText(actionRect, Qt::AlignLeft | Qt::AlignTop, action);
        yPos += actionFont.pointSize() * 1.5;
    }
}

void HsmStateElement::updateBoundingRect(const QRectF& newRect) {
    HsmRectangularElement::updateBoundingRect(newRect);
    centerHeader();
}

};  // namespace view